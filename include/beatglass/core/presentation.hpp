#pragma once
#include "beatglass/core/results.hpp"
#include <memory>
#include <string>
namespace beatglass {
// Immutable shared snapshots may be handed to a presentation thread; the
// publisher bounds timeline lengths before publication. GUI types stay local.
using PublishedSnapshot = std::shared_ptr<const Snapshot>;
struct PresentationState { std::string status, tempo, key, chord; bool stale{}; };
class PresentationAdapter {
public:
  virtual ~PresentationAdapter() = default;
  virtual PresentationState present(const Snapshot&) const = 0;
};
}
