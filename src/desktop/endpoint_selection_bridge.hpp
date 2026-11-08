#pragma once
#include <QObject>
#include <QStringList>
/** GUI-thread bridge for the current output endpoint snapshot. */
class EndpointSelectionBridge final : public QObject { Q_OBJECT Q_PROPERTY(QStringList endpoints READ endpoints NOTIFY changed) Q_PROPERTY(QString status READ status NOTIFY changed) public: using QObject::QObject; Q_INVOKABLE void refresh(); QStringList endpoints() const{return endpoints_;} QString status()const{return status_;} signals:void changed(); private:QStringList endpoints_;QString status_{QStringLiteral("Unavailable on this platform")};};
