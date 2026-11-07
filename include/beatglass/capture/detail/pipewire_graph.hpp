#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <array>
#include <string>
#include <utility>
namespace beatglass::pipewire {
/** Provisional graph evidence; it must be mutated only on the PipeWire control loop. */
class MonitorGraph {
public:
  /** Verification outcome for one registry snapshot. */
  enum class Result { waiting, verified, invalid };
  /** Port identity and monitor/channel properties. */
  struct Port { std::uint32_t node; std::string channel; bool monitor; bool output; };
  /** Port IDs selected for left and right explicit links. */
  struct Endpoints { std::uint32_t target_node, stream_node; std::array<std::uint32_t, 2> source_ports, input_ports; };
  /** Directed PipeWire link between node and port identities. */
  struct Link { std::uint32_t out_node, out_port, in_node, in_port; };
  /** Remember the selected server-session object serial and name. */
  MonitorGraph(std::string serial, std::string name) : serial_(std::move(serial)), name_(std::move(name)) {}
  /** Record a registry node; only Audio/Sink nodes can match the selection. */
  void node(std::uint32_t id, std::string serial, std::string name, bool sink);
  /** Record a port. */
  void port(std::uint32_t id, Port value);
  /** Record a link. */
  void link(std::uint32_t id, Link value);
  /** Remove a registry global. */
  void remove(std::uint32_t id);
  /** Find unique matching monitor outputs and capture inputs. Incomplete is waiting; ambiguous evidence is invalid. */
  Result endpoints(std::uint32_t stream_id, Endpoints& result) const;
  /** Validate both channels and reject any incoming link from a different node or non-monitor port. */
  Result verify(std::uint32_t stream_id) const;
private:
  struct Node { std::string serial, name; bool sink; };
  std::string serial_, name_;
  bool overflow_{};
  std::map<std::uint32_t, Node> nodes_;
  std::map<std::uint32_t, Port> ports_;
  std::map<std::uint32_t, Link> links_;
};
}
