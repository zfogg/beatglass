#include "windows_endpoint_bridge.hpp"

#include <QString>

namespace beatglass::windows {
EndpointBridge::EndpointBridge(QObject* parent, std::function<EndpointResult()> discovery)
    : QObject(parent), discovery_(discovery ? std::move(discovery) : enumerate_render_endpoints) {}

void EndpointBridge::refresh() {
  snapshot_ = discovery_();
  endpoints_.clear();
  error_ = QString::fromStdString(snapshot_.error);
  if (snapshot_.error.empty()) {
    for (const auto& endpoint : snapshot_.endpoints) {
      endpoints_.append(QVariantMap{{QStringLiteral("id"), QString::fromStdString(endpoint.id)},
                                    {QStringLiteral("name"), QString::fromStdString(endpoint.name)},
                                    {QStringLiteral("default"), endpoint.is_default}});
    }
    if (!selected_.isEmpty() && !contains_render_endpoint(snapshot_, selected_.toStdString())) selected_.clear();
  } else {
    selected_.clear();
  }
  emit changed();
}

bool EndpointBridge::select(const QString& id) {
  if (id.isEmpty() || select_render_endpoint(snapshot_, id.toStdString()) != SelectionResult::accepted) return false;
  if (selected_ == id) return true;
  selected_ = id;
  emit changed();
  return true;
}
}
