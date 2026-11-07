// Private PipeWire discovery implementation. The coordinator must agree the
// shared API before other modules consume this translation unit.
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <map>
#include <vector>

namespace beatglass::pipewire {
struct Sink { std::string serial, name, description; };
struct Discovery { std::vector<Sink> sinks; std::string error; };

// Registry properties are untrusted; accept only addressable output sinks.
static void add_sink(std::vector<Sink>& sinks, const char* type, const spa_dict* props) {
  if (!type || std::strcmp(type, PW_TYPE_INTERFACE_Node) || !props) return;
  const auto* cls = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
  const auto* serial = spa_dict_lookup(props, PW_KEY_OBJECT_SERIAL);
  const auto* name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
  if (!cls || std::strcmp(cls, "Audio/Sink") || !serial || !*serial || !name || !*name) return;
  const auto* desc = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
  sinks.push_back({serial, name, desc && *desc ? desc : name});
}

struct RegistryState {
  Discovery result;
  std::map<uint32_t, Sink> nodes;
  bool done = false;
  int sequence = -1;
};
static void global(void* data, uint32_t id, uint32_t, const char* type, uint32_t, const spa_dict* props) {
  auto& state = *static_cast<RegistryState*>(data);
  std::vector<Sink> found;
  add_sink(found, type, props);
  if (!found.empty()) state.nodes[id] = std::move(found.front());
}
static void global_remove(void* data, uint32_t id) {
  static_cast<RegistryState*>(data)->nodes.erase(id);
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
  RegistryState state;
  pw_init(nullptr, nullptr);
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
    for (auto& [id, sink] : state.nodes) state.result.sinks.push_back(std::move(sink));
  return state.result;
}
} // namespace beatglass::pipewire
