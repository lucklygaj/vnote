#include "synccenterview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QDebug>
#include <QGroupBox>

#include "sync/syncstatusmanager.h"
#include "sync/smartsyncscheduler.h"
#include "sync/filesyncstate.h"

using namespace vnotex;

SyncCenterView::SyncCenterView(QWidget *p_parent)
  : QWidget(p_parent) {
  setupUI();

  // Connect signals
  auto &statusMgr = SyncStatusManager::getInst();
  connect(&statusMgr, &SyncStatusManager::statisticsChanged,
          this, &SyncCenterView::onStatisticsChanged);
  connect(&statusMgr, &SyncStatusManager::fileStateChanged,
          this, &SyncCenterView::onFileStateChanged);

  // Initial refresh
  refreshData();
}

void SyncCenterView::setupUI() {
  QVBoxLayout *mainLayout = new QVBoxLayout(this);

  // Statistics section
  QGroupBox *statsGroup = new QGroupBox(tr("Sync Statistics"), this);
  QGridLayout *statsLayout = new QGridLayout(statsGroup);

  statsLayout->addWidget(new QLabel(tr("🟢 Synced:"), this), 0, 0);
  m_syncedLabel = new QLabel("0", this);
  statsLayout->addWidget(m_syncedLabel, 0, 1);

  statsLayout->addWidget(new QLabel(tr("🟡 Pending:"), this), 0, 2);
  m_pendingLabel = new QLabel("0", this);
  statsLayout->addWidget(m_pendingLabel, 0, 3);

  statsLayout->addWidget(new QLabel(tr("🔵 Syncing:"), this), 1, 0);
  m_syncingLabel = new QLabel("0", this);
  statsLayout->addWidget(m_syncingLabel, 1, 1);

  statsLayout->addWidget(new QLabel(tr("🔴 Failed:"), this), 1, 2);
  m_failedLabel = new QLabel("0", this);
  statsLayout->addWidget(m_failedLabel, 1, 3);

  mainLayout->addWidget(statsGroup);

  // Buttons
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  m_syncNowButton = new QPushButton(tr("Sync Now"), this);
  m_refreshButton = new QPushButton(tr("Refresh"), this);
  buttonLayout->addWidget(m_syncNowButton);
  buttonLayout->addWidget(m_refreshButton);
  buttonLayout->addStretch();
  mainLayout->addLayout(buttonLayout);

  connect(m_syncNowButton, &QPushButton::clicked, this, &SyncCenterView::syncNow);
  connect(m_refreshButton, &QPushButton::clicked, this, &SyncCenterView::refreshData);

  // File list section
  QGroupBox *fileListGroup = new QGroupBox(tr("File Sync Status"), this);
  QVBoxLayout *fileListLayout = new QVBoxLayout(fileListGroup);

  m_fileListTable = new QTableWidget(this);
  m_fileListTable->setColumnCount(5);
  m_fileListTable->setHorizontalHeaderLabels(QStringList() << tr("File") << tr("State") << tr("Last Modified") << tr("Last Sync") << tr("Retry"));
  m_fileListTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_fileListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_fileListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  fileListLayout->addWidget(m_fileListTable);

  mainLayout->addWidget(fileListGroup);

  // Failure list section
  QGroupBox *failureGroup = new QGroupBox(tr("Failed Sync Records (Last 7 Days)"), this);
  QVBoxLayout *failureLayout = new QVBoxLayout(failureGroup);

  m_failureListTable = new QTableWidget(this);
  m_failureListTable->setColumnCount(4);
  m_failureListTable->setHorizontalHeaderLabels(QStringList() << tr("File") << tr("Error") << tr("Retry Count") << tr("Actions"));
  m_failureListTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_failureListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_failureListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  failureLayout->addWidget(m_failureListTable);

  mainLayout->addWidget(failureGroup);

  setLayout(mainLayout);
  setWindowTitle(tr("Gitee Sync Center"));
  resize(800, 600);
}

void SyncCenterView::refreshData() {
  updateStatisticsDisplay();
  updateFileList();
  updateFailureList();
}

void SyncCenterView::onStatisticsChanged(int p_synced, int p_pending,
                                       int p_syncing, int p_failed, int p_paused) {
  Q_UNUSED(p_paused);
  m_syncedLabel->setText(QString::number(p_synced));
  m_pendingLabel->setText(QString::number(p_pending));
  m_syncingLabel->setText(QString::number(p_syncing));
  m_failedLabel->setText(QString::number(p_failed));

  // Update styling for failed count
  if (p_failed > 0) {
    m_failedLabel->setStyleSheet("QLabel { color: #F44336; font-weight: bold; }");
  } else {
    m_failedLabel->setStyleSheet("");
  }
}

void SyncCenterView::onFileStateChanged(const QString &p_path) {
  Q_UNUSED(p_path);
  updateFileList();
}

void SyncCenterView::updateStatisticsDisplay() {
  auto &statusMgr = SyncStatusManager::getInst();
  onStatisticsChanged(
    statusMgr.getSyncedCount(),
    statusMgr.getPendingCount(),
    statusMgr.getSyncingCount(),
    statusMgr.getFailedCount(),
    statusMgr.getPausedCount()
  );
}

void SyncCenterView::updateFileList() {
  auto &statusMgr = SyncStatusManager::getInst();
  m_fileListTable->setRowCount(0);

  // Get all tracked files - iterate through states
  // For now, we'll show the files that have non-SYNCED states
  // This could be enhanced to show all files with sync history

  // Update table based on current state
  // Since we don't have a direct way to iterate all states, 
  // we'll update when the view is shown
}

void SyncCenterView::updateFailureList() {
  auto &statusMgr = SyncStatusManager::getInst();
  m_failureListTable->setRowCount(0);

  // Failure records would be shown here
  // Implementation would iterate through files with FAILED state
}

void SyncCenterView::syncNow() {
  SmartSyncScheduler::getInst().syncAllNow();
}

void SyncCenterView::retryFile(int p_row) {
  QTableWidgetItem *item = m_failureListTable->item(p_row, 0);
  if (item) {
    QString filePath = item->text();
    // Trigger retry for this file
    qInfo() << "[SyncCenterView] Retry requested for:" << filePath;
  }
}

void SyncCenterView::clearFailure(int p_row) {
  QTableWidgetItem *item = m_failureListTable->item(p_row, 0);
  if (item) {
    QString filePath = item->text();
    auto &statusMgr = SyncStatusManager::getInst();
    statusMgr.markSynced(filePath, QString());
    refreshData();
    qInfo() << "[SyncCenterView] Failure cleared for:" << filePath;
  }
}
