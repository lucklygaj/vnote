#include "synccenterview.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QDebug>
#include <QGroupBox>
#include <QFile>

#include "sync/syncstatusmanager.h"
#include "sync/smartsyncscheduler.h"
#include "sync/filesyncstate.h"

using namespace vnotex;

SyncCenterView::SyncCenterView(QWidget *p_parent)
  : QWidget(p_parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowTitleHint) { // AI-Generated
  setAttribute(Qt::WA_DeleteOnClose);
  setupUI(); // AI-Generated

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

void SyncCenterView::updateFileList() { // AI-Generated
  auto &statusMgr = SyncStatusManager::getInst();
  const auto &allStates = statusMgr.getAllStates();

  m_fileListTable->setRowCount(0);

  if (allStates.isEmpty()) {
    m_fileListTable->insertRow(0);
    QTableWidgetItem *emptyItem = new QTableWidgetItem(tr("No sync records yet. Files will appear here after first sync."));
    emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEditable);
    m_fileListTable->setItem(0, 0, emptyItem);
    m_fileListTable->setSpan(0, 0, 1, 5);
    return;
  }

  int row = 0;
  for (auto it = allStates.constBegin(); it != allStates.constEnd(); ++it) {
    const QString &path = it.key();
    const FileSyncStateInfo &info = it.value();

    m_fileListTable->insertRow(row);

    QTableWidgetItem *fileItem = new QTableWidgetItem(path);
    fileItem->setData(Qt::UserRole, path);
    m_fileListTable->setItem(row, 0, fileItem);

    QString stateStr;
    switch (info.state) {
      case FileSyncState::SYNCED:  stateStr = tr("Synced");   break;
      case FileSyncState::PENDING: stateStr = tr("Pending");  break;
      case FileSyncState::SYNCING: stateStr = tr("Syncing");  break;
      case FileSyncState::FAILED:  stateStr = tr("Failed");   break;
      case FileSyncState::PAUSED:  stateStr = tr("Paused");   break;
      default:                    stateStr = tr("Unknown"); break;
    }
    QTableWidgetItem *stateItem = new QTableWidgetItem(stateStr);
    if (info.state == FileSyncState::PENDING) stateItem->setBackground(QBrush(QColor("#FFC107")));
    else if (info.state == FileSyncState::SYNCING) stateItem->setBackground(QBrush(QColor("#2196F3")));
    else if (info.state == FileSyncState::FAILED) stateItem->setBackground(QBrush(QColor("#F44336")));
    else if (info.state == FileSyncState::PAUSED) stateItem->setBackground(QBrush(QColor("#9E9E9E")));
    m_fileListTable->setItem(row, 1, stateItem);

    if (info.lastModified > 0)
      m_fileListTable->setItem(row, 2,
        new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(info.lastModified).toString("yyyy-MM-dd HH:mm:ss")));

    if (info.lastSyncTime > 0)
      m_fileListTable->setItem(row, 3,
        new QTableWidgetItem(QDateTime::fromMSecsSinceEpoch(info.lastSyncTime).toString("yyyy-MM-dd HH:mm:ss")));

    if (info.state != FileSyncState::SYNCED) {
      QPushButton *retryBtn = new QPushButton(tr("Retry"), this);
      connect(retryBtn, &QPushButton::clicked, this, [this, row]() { retryFile(row); });
      m_fileListTable->setCellWidget(row, 4, retryBtn);
    }

    ++row;
  }
} // AI-Generated

void SyncCenterView::updateFailureList() { // AI-Generated
  auto &statusMgr = SyncStatusManager::getInst();
  const auto &allStates = statusMgr.getAllStates();

  m_failureListTable->setRowCount(0);

  qint64 sevenDaysAgo = QDateTime::currentDateTime().addDays(-7).toMSecsSinceEpoch();
  int row = 0;

  for (auto it = allStates.constBegin(); it != allStates.constEnd(); ++it) {
    const QString &path = it.key();
    const FileSyncStateInfo &info = it.value();

    bool isFailed = (info.state == FileSyncState::FAILED);
    bool hasError = !info.lastError.isEmpty();
    if (!isFailed && !hasError) continue;
    if (!isFailed && info.lastModified > 0 && info.lastModified < sevenDaysAgo) continue;

    m_failureListTable->insertRow(row);

    QTableWidgetItem *fileItem = new QTableWidgetItem(path);
    fileItem->setData(Qt::UserRole, path);
    m_failureListTable->setItem(row, 0, fileItem);
    m_failureListTable->setItem(row, 1, new QTableWidgetItem(info.lastError.isEmpty() ? "-" : info.lastError));
    m_failureListTable->setItem(row, 2, new QTableWidgetItem(QString::number(info.retryCount)));

    QWidget *actionWidget = new QWidget(this);
    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(4, 2, 4, 2);
    actionLayout->setSpacing(4);

    QPushButton *retryBtn = new QPushButton(tr("Retry"), this);
    connect(retryBtn, &QPushButton::clicked, this, [this, row]() { retryFile(row); });
    actionLayout->addWidget(retryBtn);

    QPushButton *clearBtn = new QPushButton(tr("Clear"), this);
    connect(clearBtn, &QPushButton::clicked, this, [this, row]() { clearFailure(row); });
    actionLayout->addWidget(clearBtn);

    actionLayout->addStretch();
    m_failureListTable->setCellWidget(row, 3, actionWidget);

    ++row;
  }

  if (row == 0) {
    m_failureListTable->insertRow(0);
    QTableWidgetItem *emptyItem = new QTableWidgetItem(tr("No failure records. Great job!"));
    emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEditable);
    m_failureListTable->setItem(0, 0, emptyItem);
    m_failureListTable->setSpan(0, 0, 1, 4);
  }
} // AI-Generated

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
