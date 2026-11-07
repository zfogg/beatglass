#include <catch2/catch_test_macros.hpp>
#include "beatglass/capture/detail/pipewire_graph.hpp"
using namespace beatglass::pipewire;
TEST_CASE("graph evidence requires both selected sink monitor channels") {
 MonitorGraph graph("serial", "sink");
 graph.node(3, "serial", "sink", true);
 graph.port(10, {3,"FL",true,true}); graph.port(11,{3,"FR",true,true});
 graph.port(20, {5,"FL",false});
 MonitorGraph::Endpoints endpoints{};
 CHECK(graph.endpoints(5, endpoints) == MonitorGraph::Result::waiting);
 graph.port(21,{5,"FR",false});
 REQUIRE(graph.endpoints(5, endpoints) == MonitorGraph::Result::verified);
 CHECK(endpoints.source_ports[0] == 10);
 CHECK(endpoints.source_ports[1] == 11);
 CHECK(endpoints.input_ports[0] == 20);
 CHECK(endpoints.input_ports[1] == 21);
 graph.link(30,{3,10,5,20});
 CHECK(graph.verify(5) == MonitorGraph::Result::waiting);
 graph.link(31,{3,11,5,21});
 CHECK(graph.verify(5) == MonitorGraph::Result::verified);
 graph.remove(31);
 CHECK(graph.verify(5) == MonitorGraph::Result::waiting);
}
TEST_CASE("graph evidence rejects another node or non-monitor source") {
 MonitorGraph graph("serial", "sink");
 graph.node(3,"serial","sink",true); graph.node(4,"other","other",true);
 graph.port(10,{4,"FL",true,true}); graph.port(20,{5,"FL",false});
 graph.link(30,{4,10,5,20});
 CHECK(graph.verify(5) == MonitorGraph::Result::invalid);
 graph.remove(30); graph.port(10,{3,"FL",false,true}); graph.link(30,{3,10,5,20});
 CHECK(graph.verify(5) == MonitorGraph::Result::invalid);
}
TEST_CASE("graph evidence fails closed on bounded storage exhaustion") {
 MonitorGraph graph("serial", "sink");
 for (std::uint32_t n = 0; n < 4097; ++n) graph.node(n,"x","y",true);
 CHECK(graph.verify(1) == MonitorGraph::Result::invalid);
}

TEST_CASE("graph rejects ambiguous explicit link ports") {
 MonitorGraph graph("serial", "sink");
 graph.node(3,"serial","sink",true);
 graph.port(10,{3,"FL",true,true}); graph.port(11,{3,"FR",true,true});
 graph.port(20,{5,"FL",false,false}); graph.port(21,{5,"FR",false,false});
 MonitorGraph::Endpoints endpoints{};
 REQUIRE(graph.endpoints(5, endpoints) == MonitorGraph::Result::verified);
 graph.port(22,{5,"FL",false,false});
 CHECK(graph.endpoints(5, endpoints) == MonitorGraph::Result::invalid);
}
