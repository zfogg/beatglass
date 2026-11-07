#include <catch2/catch_test_macros.hpp>
#include "../../lib/capture/pipewire/discovery.cpp"

TEST_CASE("registry discovery selects only addressable output sinks") {
  using namespace beatglass::pipewire;
  std::vector<Sink> sinks;
  spa_dict_item items[] = {{PW_KEY_MEDIA_CLASS, "Audio/Sink"}, {PW_KEY_OBJECT_SERIAL, "123"},
                           {PW_KEY_NODE_NAME, "output"}, {PW_KEY_NODE_DESCRIPTION, "Speakers"}};
  spa_dict props = SPA_DICT_INIT(items, 4);
  add_sink(sinks, PW_TYPE_INTERFACE_Node, &props);
  REQUIRE(sinks.size() == 1);
  CHECK(sinks[0].serial == "123");
  CHECK(sinks[0].description == "Speakers");
  items[0].value = "Audio/Source";
  add_sink(sinks, PW_TYPE_INTERFACE_Node, &props);
  items[0].value = "Audio/Sink";
  items[1].value = "";
  add_sink(sinks, PW_TYPE_INTERFACE_Node, &props);
  items[1].value = "123";
  add_sink(sinks, "PipeWire:Interface:Device", &props);
  CHECK(sinks.size() == 1);
}

TEST_CASE("sink names provide fallback when description is absent") {
  std::vector<beatglass::pipewire::Sink> sinks;
  spa_dict_item items[] = {{PW_KEY_MEDIA_CLASS, "Audio/Sink"}, {PW_KEY_OBJECT_SERIAL, "5"}, {PW_KEY_NODE_NAME, "sink"}};
  spa_dict props = SPA_DICT_INIT(items, 3);
  beatglass::pipewire::add_sink(sinks, PW_TYPE_INTERFACE_Node, &props);
  REQUIRE(sinks.size() == 1);
  CHECK(sinks[0].description == "sink");
}

TEST_CASE("removed registry nodes are not returned") {
  beatglass::pipewire::RegistryState state;
  spa_dict_item items[] = {{PW_KEY_MEDIA_CLASS, "Audio/Sink"}, {PW_KEY_OBJECT_SERIAL, "5"}, {PW_KEY_NODE_NAME, "sink"}};
  spa_dict props = SPA_DICT_INIT(items, 3);
  beatglass::pipewire::global(&state, 7, 0, PW_TYPE_INTERFACE_Node, 0, &props);
  REQUIRE(state.nodes.size() == 1);
  beatglass::pipewire::global_remove(&state, 7);
  CHECK(state.nodes.empty());
}
