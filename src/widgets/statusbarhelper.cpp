#include "statusbarhelper.h"

#include <QStatusBar>
#include <QLabel>
#include <QDateTime>
#include <QTimer>

#include "mainwindow.h"
#include "sync/giteesyncservice.h"

using namespace vnotex;

void StatusBarHelper::setupStatusBar(MainWindow *p_mainWindow) {
  auto bar = new QStatusBar(p_mainWindow);

  // Create Gitee sync status label
  auto syncStatusLabel = new QLabel(bar);
  syncStatusLabel->setObjectName("GiteeSyncStatusLabel");

  // Update function for sync status
  auto updateSyncStatus = [syncStatusLabel]() {
    qint64 lastSync = GiteeSyncService::getInst().getLastSyncTime();
    if (lastSync == 0) {
      syncStatusLabel->setText("Last sync: Never");
    } else {
      QDateTime syncTime = QDateTime::fromMSecsSinceEpoch(lastSync);
      syncStatusLabel->setText("Last sync: " + syncTime.toString("yyyy-MM-dd HH:mm:ss"));
    }
  };

  // Initial update
  updateSyncStatus();

  // Set up timer to update sync status every 10 seconds
  auto syncStatusTimer = new QTimer(bar);
  QObject::connect(syncStatusTimer, &QTimer::timeout, updateSyncStatus);
  syncStatusTimer->start(10000);

  // Add permanent widget to the right side of status bar
  bar->addPermanentWidget(syncStatusLabel);

  p_mainWindow->setStatusBar(bar);
}
