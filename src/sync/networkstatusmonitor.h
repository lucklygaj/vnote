#ifndef NETWORKSTATUSMONITOR_H
#define NETWORKSTATUSMONITOR_H

#include <QObject>
#include <QNetworkInformation>

namespace vnotex {

class NetworkStatusMonitor : public QObject {
  Q_OBJECT
public:
  static NetworkStatusMonitor &getInst();

  void init();

  bool isOnline() const;

signals:
  void onlineStatusChanged(bool p_online);

private:
  NetworkStatusMonitor();
  ~NetworkStatusMonitor();

  void onReachabilityChanged(QNetworkInformation::Reachability p_reachability);

  QNetworkInformation *m_networkInfo = nullptr;
  bool m_lastOnlineStatus;
};

} // namespace vnotex

#endif // NETWORKSTATUSMONITOR_H
