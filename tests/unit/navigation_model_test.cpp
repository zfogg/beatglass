#include "beatglass/cli/navigation.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("CLI navigation adapts layout and bounds command focus") {
  beatglass::cli::NavigationModel model;
  model.resize(80);
  CHECK(model.layout() == beatglass::cli::Layout::compact);
  model.resize(120);
  CHECK(model.layout() == beatglass::cli::Layout::two_column);
  model.toggle_palette();
  model.move_command(100);
  CHECK(model.command_index() == 4);
  model.move_command(-100);
  CHECK(model.command_index() == 0);
}

TEST_CASE("CLI command palette filters ids and labels") {
  beatglass::cli::NavigationModel model;
  model.set_query("NOW");
  const auto commands = model.visible_commands();
  REQUIRE(commands.size() == 1);
  CHECK(commands.front().id == "now-playing");
  model.toggle_palette();
  CHECK(model.modal() == beatglass::cli::Modal::command_palette);
  model.dismiss_modal();
  CHECK(model.modal() == beatglass::cli::Modal::none);
}
