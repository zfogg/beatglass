#include "beatglass/capture/detail/pipewire_graph.hpp"
#include <limits>
#include <utility>
namespace beatglass::pipewire {
void MonitorGraph::node(std::uint32_t id, std::string serial, std::string name, bool sink) { if (serial.size() > 1024 || name.size() > 1024 || (!nodes_.contains(id) && nodes_.size() >= 4096)) { overflow_ = true; return; } nodes_.insert_or_assign(id, Node{std::move(serial), std::move(name), sink}); }
void MonitorGraph::port(std::uint32_t id, Port value) { if (value.channel.size() > 64 || (!ports_.contains(id) && ports_.size() >= 4096)) { overflow_ = true; return; } ports_.insert_or_assign(id, std::move(value)); }
void MonitorGraph::link(std::uint32_t id, Link value) { if (!links_.contains(id) && links_.size() >= 4096) { overflow_ = true; return; } links_.insert_or_assign(id, value); }
void MonitorGraph::remove(std::uint32_t id) { nodes_.erase(id); ports_.erase(id); links_.erase(id); }
MonitorGraph::Result MonitorGraph::verify(std::uint32_t stream) const {
  if (overflow_) return Result::invalid;
  if (stream == std::numeric_limits<std::uint32_t>::max()) return Result::waiting;
  std::uint32_t target = std::numeric_limits<std::uint32_t>::max();
  for (const auto& [id, n] : nodes_) if (n.sink && n.serial == serial_ && n.name == name_) {
    if (target != std::numeric_limits<std::uint32_t>::max()) return Result::invalid;
    target = id;
  }
  bool left = false, right = false;
  for (const auto& [id, link] : links_) {
    if (link.in_node != stream) continue;
    if (target == std::numeric_limits<std::uint32_t>::max()) return Result::waiting;
    if (link.out_node != target) return Result::invalid;
    const auto src = ports_.find(link.out_port), dst = ports_.find(link.in_port);
    if (src == ports_.end() || dst == ports_.end()) return Result::waiting;
    if (src->second.node != target || !src->second.monitor || dst->second.node != stream ||
        src->second.channel != dst->second.channel) return Result::invalid;
    if (src->second.channel == "FL" && !left) left = true;
    else if (src->second.channel == "FR" && !right) right = true;
    else return Result::invalid;
  }
  return left && right ? Result::verified : Result::waiting;
}
}
