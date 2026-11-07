#pragma once
#include "beatglass/core/results.hpp"
#include <memory>
#include <string>
namespace beatglass {
// Immutable shared snapshots may be handed to a presentation thread; the
// publisher bounds timeline lengths before publication. GUI types stay local.
/** Shared immutable snapshot; the publisher is responsible for bounding timeline lengths before publication. */
using PublishedSnapshot = std::shared_ptr<const Snapshot>;
/** Application-facing labels and stale flag. */
struct PresentationState { std::string status, tempo, key, chord; bool stale{}; };
/** Provisional presentation interface; GUI types remain local to the application. */
class PresentationAdapter {
public:
  virtual ~PresentationAdapter() = default;
  /** Convert a borrowed snapshot into presentation labels and stale status. Threading and error behavior depend on the implementation. */
  virtual PresentationState present(const Snapshot&) const = 0;
};
}
