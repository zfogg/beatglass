// Private PipeWire discovery implementation. The coordinator must agree the
// shared API before other modules consume this translation unit.
#include "beatglass/capture/detail/pipewire_discovery.hpp"
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <limits>
#include <vector>
#include <algorithm>
#include <set>

namespace beatglass::pipewire {
void RegistrySnapshot::add(std::uint32_t id, const char* type, const char* cls, const char* serial,
                           const char* name, const char* desc) {
  constexpr std::size_t max_identity_length = 4096;
  if (!type || std::strcmp(type, PW_TYPE_INTERFACE_Node) || !cls || std::strcmp(cls, "Audio/Sink") ||
      !serial || !*serial || std::string(serial).find('\0') != std::string::npos ||
      std::string(serial).size() > max_identity_length || std::string(serial).find_first_not_of(" \t\r\n") == std::string::npos ||
      !name || !*name || std::string(name).find('\0') != std::string::npos ||
      std::string(name).size() > max_identity_length || std::string(name).find_first_not_of(" \t\r\n") == std::string::npos) return;
  const std::string description = desc && *desc ? desc : "";
  nodes_[id] = {serial, name,
                description.size() > max_identity_length || description.find('\0') != std::string::npos ||
                        description.find_first_not_of(" \t\r\n") == std::string::npos
                    ? name
                    : description};
}
void RegistrySnapshot::remove(std::uint32_t id) { nodes_.erase(id); }
std::vector<Sink> RegistrySnapshot::sinks() const {
  std::vector<Sink> result;
  std::set<std::string> serials;
  for (const auto& [id, sink] : nodes_) {
    if (serials.insert(sink.serial).second) result.push_back(sink);
  }
  std::sort(result.begin(), result.end(), [](const Sink& a, const Sink& b) {
    return a.serial == b.serial ? a.name < b.name : a.serial < b.serial;
  });
  return result;
}
struct RegistryState {
  Discovery result;
  RegistrySnapshot nodes;
  bool done = false;
  int sequence = -1;
};
static void global(void* data, uint32_t id, uint32_t, const char* type, uint32_t, const spa_dict* props) {
  if (!props) return;
  static_cast<RegistryState*>(data)->nodes.add(id, type, spa_dict_lookup(props, PW_KEY_MEDIA_CLASS),
      spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL), spa_dict_lookup(props, PW_KEY_NODE_NAME),
      spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION));
}
static void global_remove(void* data, uint32_t id) {
  static_cast<RegistryState*>(data)->nodes.remove(id);
}
static void done(void* data, uint32_t id, int seq) {
  auto& state = *static_cast<RegistryState*>(data);
  if (id == PW_ID_CORE && seq == state.sequence) state.done = true;
}
static void error(void* data, uint32_t, int, int res, const char* message) {
  auto& state = *static_cast<RegistryState*>(data);
  state.result.error = message ? message : spa_strerror(res);
}

// Runs on a control thread only. A sync barrier gives a consistent initial
// registry view; timeout and daemon errors never masquerade as an empty list.
Discovery discover_sinks(std::chrono::milliseconds timeout) {
  if (timeout <= std::chrono::milliseconds::zero()) return {{}, "PipeWire discovery timeout must be positive"};
  if (timeout.count() > std::numeric_limits<int>::max()) return {{}, "PipeWire discovery timeout is too large"};
  RegistryState state;
  pw_init(nullptr, nullptr);
  struct PwShutdown { ~PwShutdown() { pw_deinit(); } } pw_shutdown;
  auto* loop = pw_loop_new(nullptr);
  if (!loop) return {{}, "Could not create PipeWire loop"};
  auto* context = pw_context_new(loop, nullptr, 0);
  if (!context) { pw_loop_destroy(loop); return {{}, "Could not create PipeWire context"}; }
  auto* core = pw_context_connect(context, nullptr, 0);
  if (!core) { pw_context_destroy(context); pw_loop_destroy(loop); return {{}, "Could not connect to PipeWire"}; }
  auto* registry = pw_core_get_registry(core, PW_VERSION_REGISTRY, 0);
  if (!registry) { pw_core_disconnect(core); pw_context_destroy(context); pw_loop_destroy(loop); return {{}, "Could not get PipeWire registry"}; }
  spa_hook core_hook{}, registry_hook{};
  pw_core_events core_events{};
  core_events.version = PW_VERSION_CORE_EVENTS;
  core_events.done = done;
  core_events.error = error;
  pw_registry_events registry_events{};
  registry_events.version = PW_VERSION_REGISTRY_EVENTS;
  registry_events.global = global;
  registry_events.global_remove = global_remove;
  pw_core_add_listener(core, &core_hook, &core_events, &state);
  pw_registry_add_listener(registry, &registry_hook, &registry_events, &state);
  state.sequence = pw_core_sync(core, PW_ID_CORE, 0);
  if (state.sequence < 0) state.result.error = "Could not request PipeWire registry sync";
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  pw_loop_enter(loop);
  while (!state.done && state.result.error.empty()) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) { state.result.error = "PipeWire registry discovery timed out"; break; }
    const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
    const int rc = pw_loop_iterate(loop, static_cast<int>(wait > 100 ? 100 : wait + 1));
    if (rc < 0 && rc != -EINTR) state.result.error = spa_strerror(rc);
  }
  pw_loop_leave(loop);
  spa_hook_remove(&registry_hook);
  spa_hook_remove(&core_hook);
  pw_proxy_destroy(reinterpret_cast<pw_proxy*>(registry));
  pw_core_disconnect(core);
  pw_context_destroy(context);
  pw_loop_destroy(loop);
  if (state.result.error.empty())
    state.result.sinks = state.nodes.sinks();
  return state.result;
}
} // namespace beatglass::pipewire
