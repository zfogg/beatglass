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
#include <vector>

namespace beatglass::pipewire {
void RegistrySnapshot::add(std::uint32_t id, const char* type, const char* cls, const char* serial,
                           const char* name, const char* desc) {
  if (!type || std::strcmp(type, PW_TYPE_INTERFACE_Node) || !cls || std::strcmp(cls, "Audio/Sink") ||
      !serial || !*serial || !name || !*name) return;
  nodes_[id] = {serial, name, desc && *desc ? desc : name};
}
void RegistrySnapshot::remove(std::uint32_t id) { nodes_.erase(id); }
std::vector<Sink> RegistrySnapshot::sinks() const {
  std::vector<Sink> result;
  for (const auto& [id, sink] : nodes_) result.push_back(sink);
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
    state.result.sinks = state.nodes.sinks();
  return state.result;
}
} // namespace beatglass::pipewire
