#include "beatglass/library/store.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "store contract smoke failure: " << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}

int main() {
  using namespace beatglass;

  CommitDecision cancelled;
  require(!cancelled.begin_commit(true), "cancellation entered commit");
  require(cancelled.cancelled(), "cancellation state was not retained");
  require(!cancelled.finish(true).has_value(), "cancelled decision finished");

  CommitDecision committed;
  require(committed.begin_commit(false), "commit decision was rejected");
  require(committed.finish(true) == PublishOutcome::committed,
          "successful commit was not reported");
  require(!committed.finish(false).has_value(), "commit was resolved twice");

  require(verification_failure(FileVerificationOutcome::match) == std::nullopt,
          "matching evidence blocked publication");
  require(verification_failure(FileVerificationOutcome::changed) == PublishOutcome::stale_input,
          "changed evidence was not stale");
  require(verification_failure(FileVerificationOutcome::cancelled) == PublishOutcome::cancelled,
          "cancelled evidence was not cancelled");
  require(verification_failure(FileVerificationOutcome::error) == PublishOutcome::error,
          "verification error was not propagated");
  return EXIT_SUCCESS;
}
