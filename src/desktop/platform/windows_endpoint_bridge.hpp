#pragma once

#include "windows_endpoints.hpp"
#include <QObject>
#include <QVariantList>
#include <functional>

namespace beatglass::windows {

/** GUI-thread endpoint snapshot bridge. Discovery and selection use owned snapshots; no audio callback or worker state is touched. */
class EndpointBridge final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList endpoints READ endpoints NOTIFY changed)
  Q_PROPERTY(QString selectedEndpoint READ selectedEndpoint NOTIFY changed)
  Q_PROPERTY(bool selectionLost READ selectionLost NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)
public:
  /** Construct on the GUI thread. The optional discovery function is a synchronous test seam. */
  explicit EndpointBridge(QObject* parent = nullptr, std::function<EndpointResult()> discovery = {});
  /** Refresh the owned endpoint snapshot on the GUI/control thread. */
  Q_INVOKABLE void refresh();
  /** Select an ID from the current snapshot; stale or failed snapshots are rejected. */
  Q_INVOKABLE bool select(const QString& id);
  QVariantList endpoints() const { return endpoints_; }
  QString selectedEndpoint() const { return selected_; }
  bool selectionLost() const { return selection_lost_; }
  QString error() const { return error_; }
signals:
  void changed();
private:
  std::function<EndpointResult()> discovery_;
  EndpointResult snapshot_;
  QVariantList endpoints_;
  QString selected_;
  bool selection_lost_{};
  QString error_;
};
}
