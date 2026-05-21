#include "syncstatusindicator.h"

#include <QMouseEvent>
#include <QAction>
#include <QActionGroup>
#include <QDateTime>

#include "sync/syncstatusmanager.h"
#include "sync/smartsyncscheduler.h"
#include "sync/giteesyncservice.h"

using namespace vnotex;

SyncStatusIndicator::SyncStatusIndicator(QWidget *p_parent)
  : QLabel(p_parent) {
  setCursor(Qt::PointingHandCursor);
  setTextFormat(Qt::PlainText);

  setupMenu();

  // Connect to GiteeSyncService for last sync time updates FIRST (must be before initial display)
  auto &giteeService = GiteeSyncService::getInst();
  m_lastSyncTime = giteeService.getLastSyncTime();

  // Connect to SyncStatusManager
  auto &statusMgr = SyncStatusManager::getInst();
  connect(&statusMgr, &SyncStatusManager::statisticsChanged,
          this, &SyncStatusIndicator::onStatisticsChanged);
  connect(&statusMgr, &SyncStatusManager::fileStateChanged,
          this, &SyncStatusIndicator::onStateChanged);

  // Initial update (AFTER m_lastSyncTime is set so time appears)
  onStatisticsChanged(
    statusMgr.getSyncedCount(),
    statusMgr.getPendingCount(),
    statusMgr.getSyncingCount(),
    statusMgr.getFailedCount(),
    statusMgr.getPausedCount()
  );

  // Poll last sync time every 5 seconds to keep display current
  QTimer *timePollTimer = new QTimer(this);
  timePollTimer->setInterval(5000);
  connect(timePollTimer, &QTimer::timeout, this, [this]() {
    qint64 newTime = GiteeSyncService::getInst().getLastSyncTime();
    if (newTime != m_lastSyncTime) {
      m_lastSyncTime = newTime;
      updateDisplay();
    }
  });
  timePollTimer->start(); // AI-Generated
}

void SyncStatusIndicator::setupMenu() {
  m_menu = new QMenu(this);

  QAction *syncNowAction = new QAction(tr("Sync Now"), m_menu);
  connect(syncNowAction, &QAction::triggered, this, []() {
    SmartSyncScheduler::getInst().syncAllNow();
  });
  m_menu->addAction(syncNowAction);

  QAction *pauseAction = new QAction(tr("Pause Sync"), m_menu);
  connect(pauseAction, &QAction::triggered, this, []() {
    SmartSyncScheduler::getInst().pause();
  });
  m_menu->addAction(pauseAction);

  m_menu->addSeparator();

  QAction *viewCenterAction = new QAction(tr("Open Sync Center"), m_menu);
  connect(viewCenterAction, &QAction::triggered, this, [this]() {
    emit syncCenterRequested();
  });
  m_menu->addAction(viewCenterAction);
}

void SyncStatusIndicator::updateDisplay() {
  QString text;
  QString style;

  // Format last sync time
  QString timeStr;
  if (m_lastSyncTime > 0) {
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_lastSyncTime);
    timeStr = dt.toString("HH:mm:ss");
  }

  if (m_syncingCount > 0) {
    text = QString("🔵 Syncing... (%1)").arg(m_syncingCount);
    style = "QLabel { color: #2196F3; }";
  } else if (m_failedCount > 0) {
    text = QString("🔴 %1 failed").arg(m_failedCount);
    style = "QLabel { color: #F44336; }";
  } else if (m_pendingCount > 0) {
    text = QString("🟡 %1 pending").arg(m_pendingCount);
    style = "QLabel { color: #FFC107; }";
  } else {
    text = "🟢 Synced";
    style = "QLabel { color: #4CAF50; }";
  }

  // Append last sync time
  if (!timeStr.isEmpty()) {
    text += QString(" @ %1").arg(timeStr);
  }

  setText(text);
  setStyleSheet(style);

  // Update tooltip
  QString lastSyncDetail;
  if (m_lastSyncTime > 0) {
    lastSyncDetail = QDateTime::fromMSecsSinceEpoch(m_lastSyncTime).toString("yyyy-MM-dd HH:mm:ss");
  } else {
    lastSyncDetail = tr("Never");
  }
  QString tooltip = QString("Gitee Sync Status\n"
                           "━━━━━━━━━━━━━━━━━\n"
                           "🟢 Synced: %1\n"
                           "🟡 Pending: %2\n"
                           "🔵 Syncing: %3\n"
                           "🔴 Failed: %4\n"
                           "⚪ Paused: %5\n"
                           "🕐 Last Push: %6")
    .arg(m_syncedCount)
    .arg(m_pendingCount)
    .arg(m_syncingCount)
    .arg(m_failedCount)
    .arg(m_pausedCount)
    .arg(lastSyncDetail);
  setToolTip(tooltip);
} // AI-Generated

void SyncStatusIndicator::onStatisticsChanged(int p_synced, int p_pending,
                                               int p_syncing, int p_failed, int p_paused) {
  m_syncedCount = p_synced;
  m_pendingCount = p_pending;
  m_syncingCount = p_syncing;
  m_failedCount = p_failed;
  m_pausedCount = p_paused;
  updateDisplay();
}

void SyncStatusIndicator::onStateChanged(const QString &p_path) {
  Q_UNUSED(p_path);
  auto &statusMgr = SyncStatusManager::getInst();
  onStatisticsChanged(
    statusMgr.getSyncedCount(),
    statusMgr.getPendingCount(),
    statusMgr.getSyncingCount(),
    statusMgr.getFailedCount(),
    statusMgr.getPausedCount()
  );
}

void SyncStatusIndicator::mousePressEvent(QMouseEvent *p_event) {
  if (p_event->button() == Qt::LeftButton) {
    if (m_menu) {
      m_menu->exec(mapToGlobal(p_event->pos()));
    }
  }
  QLabel::mousePressEvent(p_event);
}
