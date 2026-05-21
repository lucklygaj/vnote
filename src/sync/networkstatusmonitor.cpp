#include "networkstatusmonitor.h"

#include <QDebug>

using namespace vnotex;

NetworkStatusMonitor &NetworkStatusMonitor::getInst() {
  static NetworkStatusMonitor inst;
  return inst;
}

NetworkStatusMonitor::NetworkStatusMonitor()
  : m_lastOnlineStatus(false) {
}

NetworkStatusMonitor::~NetworkStatusMonitor() {
}

void NetworkStatusMonitor::init() {
  m_networkInfo = QNetworkInformation::instance();
  if (m_networkInfo) {
    m_lastOnlineStatus = m_networkInfo->reachability() != QNetworkInformation::Reachability::Disconnected;
    connect(m_networkInfo, &QNetworkInformation::reachabilityChanged,
            this, &NetworkStatusMonitor::onReachabilityChanged);
  }

  qInfo() << "[NetworkStatusMonitor] Initial status:" << (m_lastOnlineStatus ? "online" : "offline");
}

bool NetworkStatusMonitor::isOnline() const {
  if (m_networkInfo) {
    return m_networkInfo->reachability() != QNetworkInformation::Reachability::Disconnected;
  }
  return true; // Default to online if no network info available
}

void NetworkStatusMonitor::onReachabilityChanged(QNetworkInformation::Reachability p_reachability) {
  bool currentStatus = (p_reachability != QNetworkInformation::Reachability::Disconnected);
  if (currentStatus != m_lastOnlineStatus) {
    qInfo() << "[NetworkStatusMonitor] Status changed via reachability:" << (currentStatus ? "online" : "offline");
    m_lastOnlineStatus = currentStatus;
    emit onlineStatusChanged(currentStatus);
  }
}
