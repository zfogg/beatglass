#include "windows_endpoint_bridge.hpp"

#include <QString>

namespace beatglass::windows {
EndpointBridge::EndpointBridge(QObject* parent, std::function<EndpointResult()> discovery)
    : QObject(parent), discovery_(discovery ? std::move(discovery) : enumerate_render_endpoints) {}

void EndpointBridge::refresh() {
  const auto previous = selected_;
  snapshot_ = discovery_();
  endpoints_.clear();
  error_ = QString::fromStdString(snapshot_.error);
  if (snapshot_.error.empty()) {
    for (const auto& endpoint : snapshot_.endpoints) {
      endpoints_.append(QVariantMap{{QStringLiteral("id"), QString::fromStdString(endpoint.id)},
                                    {QStringLiteral("name"), QString::fromStdString(endpoint.name)},
                                    {QStringLiteral("default"), endpoint.is_default}});
    }
    selection_lost_ = !previous.isEmpty() && !contains_render_endpoint(snapshot_, previous.toStdString());
    if (selection_lost_) selected_.clear();
    // Establish a usable initial capture source without overriding an
    // explicit user selection.  The native endpoint inventory marks the
    // system render default; retaining it here keeps the GUI selection and
    // the control-thread capture request in sync after the first refresh.
    if (previous.isEmpty() && selected_.isEmpty()) {
      for (const auto& endpoint : snapshot_.endpoints) {
        if (endpoint.is_default && select_render_endpoint(snapshot_, endpoint.id) == SelectionResult::accepted) {
          selected_ = QString::fromStdString(endpoint.id);
          break;
        }
      }
    }
  } else {
    selected_.clear();
    selection_lost_ = !previous.isEmpty();
  }
  emit changed();
}

bool EndpointBridge::select(const QString& id) {
  if (id.isEmpty() || select_render_endpoint(snapshot_, id.toStdString()) != SelectionResult::accepted) return false;
  if (selected_ == id) return true;
  selected_ = id;
  selection_lost_ = false;
  emit changed();
  return true;
}
}
