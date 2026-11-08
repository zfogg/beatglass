#include "beatglass/library/analysis_operation.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "analysis operation smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

beatglass::library::AnalysisOperation valid_operation() {
  beatglass::library::AnalysisOperation operation;
  operation.source.path = "track.wav";
  operation.source.content_hash = "sha256:abc";
  operation.identity.content_hash = operation.source.content_hash;
  operation.identity.engine_version = "engine-v1";
  operation.identity.configuration_id = "default";
  operation.identity.mode = "offline";
  operation.serialize = [] { return std::string("result"); };
  operation.verify = [](std::stop_token) {
    return beatglass::FileVerificationResult{
        beatglass::FileVerificationOutcome::match, {}};
  };
  return operation;
}
}

int main() {
  using beatglass::library::AnalysisOperation;
  auto operation = valid_operation();
  require(operation.valid(), "complete operation was rejected");

  operation.identity.mode = "unsupported";
  require(!operation.valid(), "unsupported mode was accepted");
  operation = valid_operation();
  operation.identity.content_hash = "sha256:different";
  require(!operation.valid(), "identity/source mismatch was accepted");
  operation = valid_operation();
  operation.serialize = {};
  require(!operation.valid(), "missing serializer was accepted");
  operation = valid_operation();
  operation.identity.configuration_id.assign(4097, 'x');
  require(!operation.valid(), "overlong configuration was accepted");
  return EXIT_SUCCESS;
}
