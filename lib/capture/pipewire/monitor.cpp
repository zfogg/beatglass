// Provisional private PipeWire monitor; not yet connected to AudioSource.
// The control thread owns this object; one worker may consume its queue.
#include "beatglass/capture/detail/pipewire_monitor.hpp"
#include "beatglass/capture/detail/pipewire_graph.hpp"
#include <pipewire/pipewire.h>
#include <spa/buffer/meta.h>
#include <spa/param/audio/format-utils.h>
#include <spa/pod/builder.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <limits>
#include <string>
#include <thread>
#include "beatglass/core/audio_queue.hpp"

namespace beatglass::pipewire {
static_assert(std::atomic<MonitorState>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<std::int64_t>::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);
struct Monitor::Impl {
  struct Watch {
    Impl* owner{};
    pw_proxy* proxy{};
    spa_hook hook{};
    std::uint32_t id{}, node{}, out_node{}, out_port{}, in_node{}, in_port{};
    const char* channel{};
    bool input{}, monitor{}, initialized{}, owned{};
  };
public:
  explicit Impl(std::uint64_t epoch, SourceAttempt attempt) : ingress_(epoch), epoch_(epoch), attempt_(attempt) {}
  ~Impl() { stop(); }

  // The serial must have come from an explicit sink selection. Epochs are
  // assigned externally and must never be reused while old data is in flight.
  SourceStartResult start(const Sink& sink) {
    const auto& serial = sink.serial;
    if (started_ || state_.load() != MonitorState::stopped) return {{}, SourceError::busy, "Monitor instance already used"};
    if (serial.empty() || sink.name.empty() || !epoch_ || !attempt_.generation ||
        !std::all_of(serial.begin(), serial.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return {{}, SourceError::unsupported, "Invalid monitor request"};
    started_ = true;
    const auto discovered = discover_sinks(std::chrono::seconds(2));
    const auto match = std::count_if(discovered.sinks.begin(), discovered.sinks.end(), [&](const Sink& item) {
      return item.serial == sink.serial && item.name == sink.name;
    });
    const auto same_name = std::count_if(discovered.sinks.begin(), discovered.sinks.end(), [&](const Sink& item) {
      return item.name == sink.name;
    });
    if (!discovered.error.empty() || match != 1 || same_name != 1) {
      state_.store(MonitorState::error, std::memory_order_release);
      return {{}, discovered.error.empty() ? SourceError::not_found : SourceError::backend_error,
              discovered.error.empty() ? "Selected sink is unavailable or ambiguous" : discovered.error};
    }
    pw_init(nullptr, nullptr);
    loop_ = pw_main_loop_new(nullptr);
    if (!loop_) { state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not create PipeWire loop"}; }
    auto* props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music", PW_KEY_NODE_DONT_RECONNECT, "true", "node.dont-fallback", "true", nullptr);
    if (!props) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"}; }
    events_.version = PW_VERSION_STREAM_EVENTS;
    events_.state_changed = &state_changed;
    events_.param_changed = &param_changed;
    events_.process = &process;
    stream_ = pw_stream_new_simple(pw_main_loop_get_loop(loop_), "Beatglass output monitor", props, &events_, this);
    if (!stream_) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"}; }
    format_valid_ = false;
    state_.store(MonitorState::starting, std::memory_order_release);
    uint8_t storage[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    spa_audio_info_raw info{};
    info.format = SPA_AUDIO_FORMAT_F32;
    info.rate = 48000;
    info.channels = 2;
    info.position[0] = SPA_AUDIO_CHANNEL_FL;
    info.position[1] = SPA_AUDIO_CHANNEL_FR;
    const spa_pod* params[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info)};
    const auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_DONT_RECONNECT);
    if (pw_stream_connect(stream_, PW_DIRECTION_INPUT, PW_ID_ANY, flags, params, 1) < 0) {
      stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"};
    }
    selected_sink_ = sink;
    graph_ = std::make_unique<MonitorGraph>(sink.serial, sink.name);
    core_ = pw_stream_get_core(stream_);
    if (!core_) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"}; }
    core_events_.version = PW_VERSION_CORE_EVENTS;
    core_events_.done = &core_done;
    core_events_.error = &core_error;
    pw_core_add_listener(core_, &core_hook_, &core_events_, this);
    core_listening_ = true;
    registry_ = pw_core_get_registry(core_, PW_VERSION_REGISTRY, 0);
    if (!registry_) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"}; }
    registry_events_.version = PW_VERSION_REGISTRY_EVENTS;
    registry_events_.global = &registry_global;
    registry_events_.global_remove = &registry_remove;
    pw_registry_add_listener(registry_, &registry_hook_, &registry_events_, this);
    initial_sync_sequence_ = pw_core_sync(core_, PW_ID_CORE, 0);
    if (initial_sync_sequence_ < 0) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not sync PipeWire registry"}; }
    try { thread_ = std::thread([this] {
      auto* loop = pw_main_loop_get_loop(loop_);
      pw_loop_enter(loop);
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      while (!stop_requested_.load(std::memory_order_acquire)) {
        if (state() == MonitorState::starting && std::chrono::steady_clock::now() >= deadline) {
          diagnostic_.store(12); transition(MonitorState::error); break;
        }
        const int result = pw_loop_iterate(loop, 50);
        if (result < 0 && result != -EINTR) { transition(MonitorState::error); break; }
      }
      pw_loop_leave(loop);
    }); }
    catch (...) { stop(); state_.store(MonitorState::error); return {{}, SourceError::backend_error, "Could not start PipeWire stream"}; }
    return {attempt_, {}, {}};
  }

  // Stop is control-thread-only. The process callback runs on this main loop,
  // never on PipeWire's RT thread; joining the loop quiesces our callbacks.
  void stop() noexcept {
    running_delivered_.store(false, std::memory_order_release);
    worker_epoch_.store(0, std::memory_order_release);
    state_.store(MonitorState::stopped, std::memory_order_release);
    stop_requested_.store(true, std::memory_order_release);
    if (thread_.joinable()) thread_.join();
    for (auto& watch : watches_) {
      if (!watch.proxy) continue;
      spa_hook_remove(&watch.hook);
      if (!watch.owned) pw_proxy_destroy(watch.proxy);
      watch.proxy = nullptr;
    }
    for (auto*& link : owned_links_) if (link) { pw_proxy_destroy(link); link = nullptr; }
    if (registry_) { spa_hook_remove(&registry_hook_); pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry_)); registry_ = nullptr; }
    if (core_listening_) { spa_hook_remove(&core_hook_); core_listening_ = false; }
    if (stream_) { pw_stream_destroy(stream_); stream_ = nullptr; }
    if (loop_) { pw_main_loop_destroy(loop_); loop_ = nullptr; }
    state_.store(MonitorState::stopped, std::memory_order_release);
  }
  MonitorState state() const noexcept { return state_.load(std::memory_order_acquire); }
  // Consumer must invalidate all retained results on any non-streaming state,
  // drain old blocks at stop, and install an epoch before accepting new ones.
  bool install_worker_epoch(std::uint64_t epoch, AudioFormat format) noexcept {
    if (epoch != epoch_ || format.sample_rate != 48000 || format.channels != 2 ||
        format.layout != ChannelLayout::stereo || !running_delivered_.load(std::memory_order_acquire) ||
        state() != MonitorState::streaming) return false;
    worker_epoch_.store(epoch, std::memory_order_release);
    if (state() == MonitorState::streaming) return true;
    worker_epoch_.store(0, std::memory_order_release);
    return false;
  }
  bool pop(MonitorIngress::Block& block) noexcept {
    if (worker_epoch_.load(std::memory_order_acquire) != epoch_ || state() != MonitorState::streaming || !ingress_.pop(block)) return false;
    return state() == MonitorState::streaming && worker_epoch_.load(std::memory_order_acquire) == epoch_ && block.epoch == epoch_;
  }
  std::uint64_t dropped_frames() const noexcept { return ingress_.dropped_frames(); }
  bool pop_event(MonitorEvent& event) noexcept {
    if (event_loss_.exchange(false, std::memory_order_acq_rel)) {
      event_read_.store(event_write_.load(std::memory_order_acquire), std::memory_order_release);
      event = {attempt_, 0, std::nullopt, SourceState::error, std::nullopt, SourceError::backend_error, true}; return true;
    }
    const auto read = event_read_.load(std::memory_order_relaxed);
    if (read == event_write_.load(std::memory_order_acquire)) return false;
    event = events_queue_[read];
    event_read_.store((read + 1) % events_queue_.size(), std::memory_order_release);
    if (event_loss_.exchange(false, std::memory_order_acq_rel)) {
      event_read_.store(event_write_.load(std::memory_order_acquire), std::memory_order_release);
      event = {attempt_, 0, std::nullopt, SourceState::error, std::nullopt, SourceError::backend_error, true};
    }
    if (!event.delivery_lost && event.state == SourceState::running)
      running_delivered_.store(true, std::memory_order_release);
    return true;
  }
  int diagnostic_code() const noexcept { return diagnostic_.load(); }
  std::optional<MonitorGraphClock> graph_clock() const noexcept {
    for (int i = 0; i < 3; ++i) {
      const auto before = clock_seq_.load();
      if (!before || (before & 1) || state() != MonitorState::streaming) continue;
      MonitorGraphClock value{clock_now_.load(), clock_ticks_.load(), clock_rate_num_.load(),
                              clock_rate_denom_.load(), clock_delay_.load(), clock_queued_.load()};
      if (clock_seq_.load() == before && state() == MonitorState::streaming) return value;
    }
    return std::nullopt;
  }

private:
  // Single loop-thread producer, single control-thread consumer. Full delivery
  // fails closed; no allocation, locks or calls to consumer code occur here.
  void transition(MonitorState next) noexcept {
    const auto current = state();
    if (current == MonitorState::stopped || current == MonitorState::error || current == MonitorState::interrupted || current == next) return;
    if (next != MonitorState::streaming) {
      running_delivered_.store(false, std::memory_order_release);
      worker_epoch_.store(0, std::memory_order_release);
    }
    const auto write = event_write_.load(std::memory_order_relaxed);
    const auto after = (write + 1) % events_queue_.size();
    if (after == event_read_.load(std::memory_order_acquire) || event_sequence_ == UINT64_MAX) {
      event_loss_.store(true, std::memory_order_release);
      state_.store(MonitorState::error, std::memory_order_release); return;
    }
    const auto source_state = next == MonitorState::streaming ? SourceState::running :
                              next == MonitorState::interrupted ? SourceState::interrupted : SourceState::error;
    events_queue_[write] = {attempt_, ++event_sequence_, next == MonitorState::streaming ? std::optional{epoch_} : std::nullopt,
                            source_state, std::nullopt, source_state == SourceState::error ? std::optional{SourceError::backend_error} : std::nullopt, false};
    event_write_.store(after, std::memory_order_release);
    state_.store(next, std::memory_order_release);
  }
  static bool equals(const char* a, const char* b) noexcept { return a && b && !std::strcmp(a, b); }
  void watch_failure() noexcept {
    diagnostic_.store(13);
    transition(state() == MonitorState::streaming ? MonitorState::interrupted : MonitorState::error);
  }
  static void node_info(void* data, const pw_node_info* info) {
    auto& watch = *static_cast<Watch*>(data);
    auto& self = *watch.owner;
    if (!info || info->id != watch.id || info->state == PW_NODE_STATE_ERROR) { self.watch_failure(); return; }
    if (watch.initialized && self.state() == MonitorState::streaming && (info->change_mask & PW_NODE_CHANGE_MASK_PROPS)) {
      self.watch_failure(); return;
    }
    if (!watch.initialized || (info->change_mask & PW_NODE_CHANGE_MASK_PROPS)) {
      if (!info->props) { self.watch_failure(); return; }
      const auto* cls = spa_dict_lookup(info->props, PW_KEY_MEDIA_CLASS);
      if (watch.input) {
        if (!equals(cls, "Stream/Input/Audio")) { self.watch_failure(); return; }
      } else {
        if (!self.graph_->matches_node(watch.id, spa_dict_lookup(info->props, PW_KEY_OBJECT_SERIAL),
                                       spa_dict_lookup(info->props, PW_KEY_NODE_NAME), equals(cls, "Audio/Sink"))) {
          self.watch_failure(); return;
        }
      }
    }
    if (!watch.initialized) { watch.initialized = true; ++self.watch_ready_; self.check_graph(); }
  }
  static void port_info(void* data, const pw_port_info* info) {
    auto& watch = *static_cast<Watch*>(data);
    auto& self = *watch.owner;
    if (!info || info->id != watch.id || info->direction != (watch.input ? PW_DIRECTION_INPUT : PW_DIRECTION_OUTPUT)) { self.watch_failure(); return; }
    if (watch.initialized && self.state() == MonitorState::streaming && (info->change_mask & PW_PORT_CHANGE_MASK_PROPS)) {
      self.watch_failure(); return;
    }
    if (!watch.initialized || (info->change_mask & PW_PORT_CHANGE_MASK_PROPS)) {
      if (!info->props) { self.watch_failure(); return; }
      const auto* mon = spa_dict_lookup(info->props, PW_KEY_PORT_MONITOR);
      if (!self.graph_->matches_port(watch.id, number(spa_dict_lookup(info->props, PW_KEY_NODE_ID)),
                                    spa_dict_lookup(info->props, "audio.channel"), equals(mon, "true"), !watch.input)) {
        self.watch_failure(); return;
      }
    }
    if (!watch.initialized) { watch.initialized = true; ++self.watch_ready_; self.check_graph(); }
  }
  static void link_info(void* data, const pw_link_info* info) {
    auto& watch = *static_cast<Watch*>(data);
    auto& self = *watch.owner;
    if (!info || !MonitorGraph::matches_link({watch.out_node, watch.out_port, watch.in_node, watch.in_port},
                                             {info->output_node_id, info->output_port_id, info->input_node_id, info->input_port_id}) ||
        info->state == PW_LINK_STATE_ERROR) {
      self.watch_failure(); return;
    }
    if (watch.initialized && self.state() == MonitorState::streaming && (info->change_mask & PW_LINK_CHANGE_MASK_PROPS)) {
      self.watch_failure(); return;
    }
    if (!watch.initialized) { watch.initialized = true; ++self.watch_ready_; self.check_graph(); }
  }
  static const pw_node_events& node_events() {
    static const auto events = [] { pw_node_events e{}; e.version = PW_VERSION_NODE_EVENTS; e.info = &node_info; return e; }(); return events;
  }
  static const pw_port_events& port_events() {
    static const auto events = [] { pw_port_events e{}; e.version = PW_VERSION_PORT_EVENTS; e.info = &port_info; return e; }(); return events;
  }
  static const pw_link_events& link_events() {
    static const auto events = [] { pw_link_events e{}; e.version = PW_VERSION_LINK_EVENTS; e.info = &link_info; return e; }(); return events;
  }
  bool watch_endpoints(const MonitorGraph::Endpoints& endpoints) {
    for (std::size_t i = 0; i < 6; ++i) {
      auto& watch = watches_[i];
      watch.owner = this;
      if (i < 2) {
        watch.id = i ? endpoints.stream_node : endpoints.target_node; watch.input = i != 0;
        watch.proxy = static_cast<pw_proxy*>(pw_registry_bind(registry_, watch.id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        if (!watch.proxy) return false;
        pw_node_add_listener(reinterpret_cast<pw_node*>(watch.proxy), &watch.hook, &node_events(), &watch);
      } else {
        const auto channel = (i - 2) % 2; watch.input = i >= 4; watch.monitor = !watch.input;
        watch.channel = channel ? "FR" : "FL"; watch.node = watch.input ? endpoints.stream_node : endpoints.target_node;
        watch.id = watch.input ? endpoints.input_ports[channel] : endpoints.source_ports[channel];
        watch.proxy = static_cast<pw_proxy*>(pw_registry_bind(registry_, watch.id, PW_TYPE_INTERFACE_Port, PW_VERSION_PORT, 0));
        if (!watch.proxy) return false;
        pw_port_add_listener(reinterpret_cast<pw_port*>(watch.proxy), &watch.hook, &port_events(), &watch);
      }
    }
    return true;
  }
  static std::uint32_t number(const char* value) {
    if (!value || !*value || *value == '-') return SPA_ID_INVALID;
    char* end{}; errno = 0;
    const auto n = std::strtoul(value, &end, 10);
    if (errno || *end || n >= SPA_ID_INVALID) return SPA_ID_INVALID;
    return static_cast<std::uint32_t>(n);
  }
  void check_graph() noexcept {
    try {
    if (!graph_ || !stream_ || state() == MonitorState::error || state() == MonitorState::interrupted || state() == MonitorState::stopped) return;
    if (!registry_ready_) return;
    if (!links_requested_) {
      MonitorGraph::Endpoints endpoints{};
      const auto resolution = graph_->endpoints(pw_stream_get_node_id(stream_), endpoints);
      if (resolution == MonitorGraph::Result::waiting) return;
      if (resolution == MonitorGraph::Result::invalid) { diagnostic_.store(6); transition(MonitorState::error); return; }
      links_requested_ = true;
      if (!watch_endpoints(endpoints)) { diagnostic_.store(13); transition(MonitorState::error); return; }
      for (std::size_t i = 0; i < owned_links_.size(); ++i) {
        const auto output_node = std::to_string(endpoints.target_node);
        const auto input_node = std::to_string(endpoints.stream_node);
        const auto output_port = std::to_string(endpoints.source_ports[i]);
        const auto input_port = std::to_string(endpoints.input_ports[i]);
        auto* properties = pw_properties_new(PW_KEY_LINK_OUTPUT_NODE, output_node.c_str(),
            PW_KEY_LINK_OUTPUT_PORT, output_port.c_str(), PW_KEY_LINK_INPUT_NODE, input_node.c_str(),
            PW_KEY_LINK_INPUT_PORT, input_port.c_str(), PW_KEY_OBJECT_LINGER, "false", nullptr);
        if (!properties) { diagnostic_.store(10); transition(MonitorState::error); return; }
        owned_links_[i] = static_cast<pw_proxy*>(pw_core_create_object(core_, "link-factory", PW_TYPE_INTERFACE_Link,
            PW_VERSION_LINK, &properties->dict, 0));
        pw_properties_free(properties);
        if (!owned_links_[i]) { diagnostic_.store(10); transition(MonitorState::error); return; }
        auto& watch = watches_[6 + i];
        watch = {};
        watch.owner = this; watch.proxy = owned_links_[i]; watch.owned = true;
        watch.out_node = endpoints.target_node; watch.out_port = endpoints.source_ports[i];
        watch.in_node = endpoints.stream_node; watch.in_port = endpoints.input_ports[i];
        pw_link_add_listener(reinterpret_cast<pw_link*>(watch.proxy), &watch.hook, &link_events(), &watch);
      }
    }
    if (watch_ready_ != watches_.size()) return;
    const auto result = graph_->verify(pw_stream_get_node_id(stream_));
    if (result == MonitorGraph::Result::invalid) {
      diagnostic_.store(6); transition(MonitorState::error);
    } else if (result == MonitorGraph::Result::waiting && graph_verified_) {
      diagnostic_.store(7); transition(state() == MonitorState::starting ? MonitorState::error : MonitorState::interrupted);
    } else if (result == MonitorGraph::Result::verified && !graph_verified_ && sync_sequence_ < 0) {
      sync_sequence_ = pw_core_sync(core_, PW_ID_CORE, 0);
      if (sync_sequence_ < 0) { diagnostic_.store(8); transition(MonitorState::error); }
    } else if (result == MonitorGraph::Result::verified && graph_verified_ && raw_streaming_ && format_valid_) {
      transition(MonitorState::streaming);
    }
    } catch (...) { diagnostic_.store(9); transition(MonitorState::error); }
  }
  static void core_error(void* data, uint32_t, int, int, const char*) {
    auto& self = *static_cast<Impl*>(data);
    self.diagnostic_.store(11);
    self.transition(MonitorState::error);
  }
  static void core_done(void* data, uint32_t id, int sequence) {
    auto& self = *static_cast<Impl*>(data);
    if (id != PW_ID_CORE) return;
    if (sequence == self.initial_sync_sequence_) {
      self.initial_sync_sequence_ = -1;
      self.registry_ready_ = true;
      self.check_graph();
      return;
    }
    if (sequence != self.sync_sequence_) return;
    self.sync_sequence_ = -1;
    if (self.graph_->verify(pw_stream_get_node_id(self.stream_)) == MonitorGraph::Result::verified)
      self.graph_verified_ = true;
    self.check_graph();
  }
  static void registry_global(void* data, uint32_t id, uint32_t, const char* type, uint32_t, const spa_dict* props) {
    auto& self = *static_cast<Impl*>(data);
    if (!type || !props) return;
    try {
    const auto get = [props](const char* key) {
      const auto* value = spa_dict_lookup(props, key);
      if (value && strnlen(value, 1025) > 1024) throw 1;
      return value;
    };
    if (!std::strcmp(type, PW_TYPE_INTERFACE_Node)) {
      const auto serial = get(PW_KEY_OBJECT_SERIAL), name = get(PW_KEY_NODE_NAME), cls = get(PW_KEY_MEDIA_CLASS);
      self.graph_->node(id, serial ? serial : "", name ? name : "", cls && !std::strcmp(cls, "Audio/Sink"));
    } else if (!std::strcmp(type, PW_TYPE_INTERFACE_Port)) {
      const auto node = number(get(PW_KEY_NODE_ID));
      const auto channel = get("audio.channel"), monitor = get(PW_KEY_PORT_MONITOR), direction = get(PW_KEY_PORT_DIRECTION);
      if (node != SPA_ID_INVALID && direction && (!std::strcmp(direction, "out") || !std::strcmp(direction, "in")))
        self.graph_->port(id, {node, channel ? channel : "", monitor && !std::strcmp(monitor, "true"), !std::strcmp(direction, "out")});
    } else if (!std::strcmp(type, PW_TYPE_INTERFACE_Link)) {
      const auto out_node = number(get(PW_KEY_LINK_OUTPUT_NODE)), out_port = number(get(PW_KEY_LINK_OUTPUT_PORT));
      const auto in_node = number(get(PW_KEY_LINK_INPUT_NODE)), in_port = number(get(PW_KEY_LINK_INPUT_PORT));
      if (out_node != SPA_ID_INVALID && out_port != SPA_ID_INVALID && in_node != SPA_ID_INVALID && in_port != SPA_ID_INVALID)
        self.graph_->link(id, {out_node, out_port, in_node, in_port});
    }
    self.check_graph();
    } catch (...) { self.diagnostic_.store(9); self.transition(MonitorState::error); }
  }
  static void registry_remove(void* data, uint32_t id) {
    auto& self = *static_cast<Impl*>(data);
    self.graph_->remove(id);
    self.check_graph();
  }
  static void state_changed(void* data, pw_stream_state, pw_stream_state next, const char*) {
    auto& self = *static_cast<Impl*>(data);
    if (self.state() == MonitorState::stopped || self.state() == MonitorState::error || self.state() == MonitorState::interrupted) return;
    if (next == PW_STREAM_STATE_ERROR || next == PW_STREAM_STATE_UNCONNECTED) {
      self.diagnostic_.store(1);
      self.transition(MonitorState::error);
    } else if (next == PW_STREAM_STATE_STREAMING && self.state() == MonitorState::starting) {
      self.raw_streaming_ = true;
      self.check_graph();
    } else if (next == PW_STREAM_STATE_PAUSED && self.state() == MonitorState::streaming) {
      self.transition(MonitorState::interrupted);
    }
  }
  static void param_changed(void* data, uint32_t id, const spa_pod* param) {
    auto& self = *static_cast<Impl*>(data);
    if (id != SPA_PARAM_Format) return;
    if (!param && !self.format_valid_) return;
    spa_audio_info_raw info{};
    if (!param || spa_format_audio_raw_parse(param, &info) < 0 || info.format != SPA_AUDIO_FORMAT_F32 ||
        info.rate != 48000 || info.channels != 2 || info.position[0] != SPA_AUDIO_CHANNEL_FL || info.position[1] != SPA_AUDIO_CHANNEL_FR) {
      self.format_valid_ = false;
      self.diagnostic_.store(!param ? 20 : info.format != SPA_AUDIO_FORMAT_F32 ? 21 : info.rate != 48000 ? 22 : info.channels != 2 ? 23 : 24);
      self.transition(MonitorState::error);
      return;
    }
    // Renegotiation requires a fresh epoch, even when the format is unchanged.
    if (self.format_valid_) { self.diagnostic_.store(3); self.transition(MonitorState::error); return; }
    self.format_valid_ = true;
    self.check_graph();
  }
  static void process(void* data) {
    auto& self = *static_cast<Impl*>(data);
    auto* buffer = pw_stream_dequeue_buffer(self.stream_);
    if (!buffer) return;
    auto* spa = buffer->buffer;
    if (self.state() == MonitorState::streaming) {
      pw_time observation{};
#if PW_CHECK_VERSION(0, 3, 50)
      const int clock_result = pw_stream_get_time_n(self.stream_, &observation, sizeof(observation));
#else
      const int clock_result = pw_stream_get_time(self.stream_, &observation);
#endif
      if (clock_result >= 0 && observation.now > 0 && observation.rate.num && observation.rate.denom) {
        const auto sequence = self.clock_seq_.load();
        if (sequence <= UINT64_MAX - 2) {
          self.clock_seq_.store(sequence + 1);
          self.clock_now_.store(observation.now); self.clock_ticks_.store(observation.ticks);
          self.clock_rate_num_.store(observation.rate.num); self.clock_rate_denom_.store(observation.rate.denom);
          self.clock_delay_.store(observation.delay); self.clock_queued_.store(observation.queued);
          self.clock_seq_.store(sequence + 2);
        } else self.transition(MonitorState::error);
      }
    }
    if (self.state() == MonitorState::streaming && spa && spa->n_datas == 1 && spa->datas &&
        spa->datas[0].data && spa->datas[0].chunk && self.format_valid_) {
      auto& d = spa->datas[0];
      auto* header = static_cast<spa_meta_header*>(spa_buffer_find_meta_data(spa, SPA_META_Header, sizeof(spa_meta_header)));
      const bool corrupt = (d.chunk->flags & SPA_CHUNK_FLAG_CORRUPTED) || (header && (header->flags & SPA_META_HEADER_FLAG_CORRUPTED));
      const bool gap = header && (header->flags & (SPA_META_HEADER_FLAG_DISCONT | SPA_META_HEADER_FLAG_GAP));
      auto bytes = std::span<const std::byte>(static_cast<const std::byte*>(d.data), d.maxsize);
      if (gap) {
        self.transition(MonitorState::interrupted);
      } else {
        const auto result = self.ingress_.ingest(bytes, d.chunk->offset, d.chunk->size, d.chunk->stride, corrupt, false);
        if (result == IngressResult::invalid) { self.diagnostic_.store(4); self.transition(MonitorState::error); }
        else if (result == IngressResult::dropped) self.transition(MonitorState::interrupted);
      }
    } else if (self.state() == MonitorState::streaming) {
      self.diagnostic_.store(5);
      self.transition(MonitorState::error);
    }
    pw_stream_queue_buffer(self.stream_, buffer);
  }
  pw_main_loop* loop_{};
  pw_stream* stream_{};
  pw_core* core_{};
  spa_hook core_hook_{};
  pw_core_events core_events_{};
  bool core_listening_{};
  int sync_sequence_{-1}, initial_sync_sequence_{-1};
  bool registry_ready_{}, links_requested_{};
  std::array<pw_proxy*, 2> owned_links_{};
  std::array<Watch, 8> watches_{};
  std::size_t watch_ready_{};
  Sink selected_sink_{};
  pw_registry* registry_{};
  spa_hook registry_hook_{};
  pw_registry_events registry_events_{};
  std::unique_ptr<MonitorGraph> graph_;
  bool graph_verified_{}, raw_streaming_{};
  pw_stream_events events_{};
  std::thread thread_;
  std::atomic<MonitorState> state_{MonitorState::stopped};
  std::atomic<int> diagnostic_{0};
  std::atomic<bool> stop_requested_{false};
  MonitorIngress ingress_;
  std::uint64_t epoch_{}, event_sequence_{};
  SourceAttempt attempt_{};
  std::array<MonitorEvent, 16> events_queue_{};
  std::atomic<std::size_t> event_read_{0}, event_write_{0};
  std::atomic<bool> event_loss_{false}, running_delivered_{false};
  std::atomic<std::uint64_t> worker_epoch_{0};
  std::atomic<std::uint64_t> clock_seq_{0}, clock_ticks_{0}, clock_queued_{0};
  std::atomic<std::int64_t> clock_now_{0}, clock_delay_{0};
  std::atomic<std::uint32_t> clock_rate_num_{0}, clock_rate_denom_{0};
  bool started_{};
  bool format_valid_{}; // control loop thread, after start before thread creation
};
EnumerationResult enumerate_monitor_sources(std::chrono::milliseconds timeout) {
  const auto result = discover_sinks(timeout);
  if (!result.error.empty()) return {EnumerationOutcome::error, {}, SourceError::backend_error, result.error};
  EnumerationResult value{EnumerationOutcome::ok, {}, std::nullopt, {}};
  for (const auto& sink : result.sinks) value.sources.push_back({sink.serial, sink.description, true, false});
  return value;
}
Monitor::Monitor(std::uint64_t epoch, SourceAttempt attempt) : impl_(std::make_unique<Impl>(epoch, attempt)) {}
Monitor::~Monitor() = default;
SourceStartResult Monitor::start(const Sink& sink) { return impl_->start(sink); }
void Monitor::stop() noexcept { impl_->stop(); }
MonitorState Monitor::state() const noexcept { return impl_->state(); }
bool Monitor::pop_event(MonitorEvent& event) noexcept { return impl_->pop_event(event); }
bool Monitor::install_worker_epoch(std::uint64_t epoch, AudioFormat format) noexcept { return impl_->install_worker_epoch(epoch, format); }
bool Monitor::pop(MonitorIngress::Block& block) noexcept { return impl_->pop(block); }
std::optional<MonitorGraphClock> Monitor::graph_clock() const noexcept { return impl_->graph_clock(); }
int Monitor::diagnostic_code() const noexcept { return impl_->diagnostic_code(); }
std::uint64_t Monitor::dropped_frames() const noexcept { return impl_->dropped_frames(); }
} // namespace beatglass::pipewire
