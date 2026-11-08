#include <catch2/catch_test_macros.hpp>
#include "beatglass/library/analysis_operation.hpp"
TEST_CASE("analysis operation retains source identity and bounded callback contract") { beatglass::library::AnalysisOperation op; op.source.size_bytes=12; op.identity.mode="offline"; op.serialize=[] { return std::string("result"); }; REQUIRE(op.serialize()=="result"); REQUIRE(op.identity.mode=="offline"); }
