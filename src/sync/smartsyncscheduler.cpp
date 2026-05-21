#include "smartsyncscheduler.h"

#include <QTimer>
#include <QThread>
#include <QMutexLocker>
#include <QDebug>

#include "syncstatusmanager.h"
#include "giteesyncservice.h"
#include "giteesyncconfig.h"

using namespace vnotex;

SmartSyncScheduler &SmartSyncScheduler::getInst() {
  static SmartSyncScheduler inst;
  return inst;
}

SmartSyncScheduler::SmartSyncScheduler()
  : m_batchTimer(this) {
  m_batchTimer.setSingleShot(true);
  connect(&m_batchTimer, &QTimer::timeout, this, &SmartSyncScheduler::onBatchTimeout);
}

SmartSyncScheduler::~SmartSyncScheduler() {
  for (auto timer : m_debounceTimers) {
    delete timer;
  }
}

void SmartSyncScheduler::init() {
  m_statusManager = &SyncStatusManager::getInst();
  m_syncService = &GiteeSyncService::getInst();
}

void SmartSyncScheduler::initStatusManager(const QString &p_stateFilePath) {
  m_statusManager->init(p_stateFilePath);
}

void SmartSyncScheduler::requestSync(const QString &p_filePath,
                                     const QString &p_rootFolderPath,
                                     const QString &p_content,
                                     SyncPriority p_priority) {
  QMutexLocker locker(&m_mutex);

  if (m_paused) {
    qInfo() << "[SmartSyncScheduler] Scheduler is paused, ignoring sync request for:" << p_filePath;
    return;
  }

  if (!m_networkOnline) {
    qInfo() << "[SmartSyncScheduler] Network is offline, queueing sync for later:" << p_filePath;
    m_statusManager->setState(p_filePath, FileSyncState::PAUSED);
    m_statusManager->setPendingContent(p_filePath, p_content);
    return;
  }

  // Deduplicate: update pending content if already queued
  if (deduplicateTask(p_filePath, p_content)) {
    qInfo() << "[SmartSyncScheduler] Updated pending content for:" << p_filePath;
    return;
  }

  SyncTask task;
  task.filePath = p_filePath;
  task.rootFolderPath = p_rootFolderPath;
  task.content = p_content;
  task.priority = p_priority;
  task.timestamp = QDateTime::currentMSecsSinceEpoch();

  if (p_priority == SyncPriority::High) {
    // Immediate execution for high priority
    qInfo() << "[SmartSyncScheduler] High priority sync for:" << p_filePath;
    m_statusManager->setState(p_filePath, FileSyncState::SYNCING);
    QTimer::singleShot(0, this, [this, task]() {
      executeSync(task);
    });
  } else {
    // Debounced execution for normal/low priority
    qInfo() << "[SmartSyncScheduler] Queuing debounced sync for:" << p_filePath;
    m_pendingTasks[p_filePath] = task;
    m_statusManager->setState(p_filePath, FileSyncState::PENDING);
    m_statusManager->setPendingContent(p_filePath, p_content);

    // Start or reset debounce timer
    QTimer *timer = m_debounceTimers.value(p_filePath);
    if (!timer) {
      timer = new QTimer(this);
      timer->setSingleShot(true);
      m_debounceTimers[p_filePath] = timer;
      connect(timer, &QTimer::timeout, this, [this, p_filePath]() {
        onDebounceTimeout(p_filePath);
      });
    }
    timer->start(m_batchWindowMs);

    emit queueChanged(m_pendingTasks.size());
  }
}

void SmartSyncScheduler::requestImmediateSync(const QString &p_filePath,
                                               const QString &p_rootFolderPath,
                                               const QString &p_content) {
  // Cancel any pending debounce for this file
  QMutexLocker locker(&m_mutex);
  if (m_debounceTimers.contains(p_filePath)) {
    m_debounceTimers[p_filePath]->stop();
  }
  m_pendingTasks.remove(p_filePath);

  requestSync(p_filePath, p_rootFolderPath, p_content, SyncPriority::High);
}

void SmartSyncScheduler::cancelSync(const QString &p_filePath) {
  QMutexLocker locker(&m_mutex);

  if (m_debounceTimers.contains(p_filePath)) {
    m_debounceTimers[p_filePath]->stop();
    delete m_debounceTimers.take(p_filePath);
  }
  m_pendingTasks.remove(p_filePath);

  m_statusManager->setState(p_filePath, FileSyncState::SYNCED);
  emit queueChanged(m_pendingTasks.size());
}

void SmartSyncScheduler::syncAllNow() {
  QMutexLocker locker(&m_mutex);

  qInfo() << "[SmartSyncScheduler] Syncing all now, pending:" << m_pendingTasks.size();

  // Stop all debounce timers and execute immediately
  for (auto timer : m_debounceTimers) {
    timer->stop();
  }

  // Execute all pending tasks
  for (auto it = m_pendingTasks.begin(); it != m_pendingTasks.end(); ++it) {
    const SyncTask &task = it.value();
    m_statusManager->setState(task.filePath, FileSyncState::SYNCING);
    QTimer::singleShot(0, this, [this, task]() {
      executeSync(task);
    });
  }

  m_pendingTasks.clear();
  m_batchTimer.stop();
  emit queueChanged(0);
}

void SmartSyncScheduler::pause() {
  QMutexLocker locker(&m_mutex);
  m_paused = true;

  // Pause all pending tasks
  for (auto it = m_pendingTasks.begin(); it != m_pendingTasks.end(); ++it) {
    m_statusManager->setState(it.key(), FileSyncState::PAUSED);
  }

  // Stop all timers
  for (auto timer : m_debounceTimers) {
    timer->stop();
  }
  m_batchTimer.stop();

  qInfo() << "[SmartSyncScheduler] Scheduler paused";
}

void SmartSyncScheduler::resume() {
  QMutexLocker locker(&m_mutex);
  m_paused = false;
  m_networkOnline = true;

  // Resume all paused files
  for (auto it = m_pendingTasks.begin(); it != m_pendingTasks.end(); ++it) {
    m_statusManager->setState(it.key(), FileSyncState::PENDING);
    // Restart debounce timers
    QTimer *timer = m_debounceTimers.value(it.key());
    if (timer) {
      timer->start(m_batchWindowMs);
    }
  }

  qInfo() << "[SmartSyncScheduler] Scheduler resumed";
}

bool SmartSyncScheduler::isPaused() const {
  return m_paused;
}

void SmartSyncScheduler::setBatchWindowMs(int p_ms) {
  QMutexLocker locker(&m_mutex);
  m_batchWindowMs = qBound(1000, p_ms, 10000);
  qInfo() << "[SmartSyncScheduler] Batch window set to:" << m_batchWindowMs << "ms";
}

int SmartSyncScheduler::getBatchWindowMs() const {
  return m_batchWindowMs;
}

void SmartSyncScheduler::onNetworkStatusChanged(bool p_online) {
  QMutexLocker locker(&m_mutex);
  m_networkOnline = p_online;

  if (p_online) {
    qInfo() << "[SmartSyncScheduler] Network recovered, resuming sync";
    resume();
  } else {
    qInfo() << "[SmartSyncScheduler] Network lost, pausing sync";
    pause();
  }
}

void SmartSyncScheduler::onDebounceTimeout(const QString &p_filePath) {
  QMutexLocker locker(&m_mutex);

  auto it = m_pendingTasks.find(p_filePath);
  if (it == m_pendingTasks.end()) {
    return;
  }

  SyncTask task = it.value();
  m_pendingTasks.remove(p_filePath);
  delete m_debounceTimers.take(p_filePath);

  qInfo() << "[SmartSyncScheduler] Debounce timeout for:" << p_filePath;

  // Add to batch queue for batch execution
  addToBatchQueue(task);
}

void SmartSyncScheduler::onBatchTimeout() {
  QMutexLocker locker(&m_mutex);

  qInfo() << "[SmartSyncScheduler] Batch timeout, executing" << m_batchQueue.size() << "tasks";

  while (!m_batchQueue.isEmpty()) {
    SyncTask task = m_batchQueue.dequeue();
    m_statusManager->setState(task.filePath, FileSyncState::SYNCING);
    QTimer::singleShot(0, this, [this, task]() {
      executeSync(task);
    });
  }
}

bool SmartSyncScheduler::deduplicateTask(const QString &p_filePath, const QString &p_content) {
  auto it = m_pendingTasks.find(p_filePath);
  if (it != m_pendingTasks.end()) {
    // Update content and reset timer
    it.value().content = p_content;
    it.value().timestamp = QDateTime::currentMSecsSinceEpoch();
    m_statusManager->setPendingContent(p_filePath, p_content);
    return true;
  }
  return false;
}

void SmartSyncScheduler::addToBatchQueue(const SyncTask &p_task) {
  m_batchQueue.enqueue(p_task);

  // Start batch timer if not running
  if (!m_batchTimer.isActive()) {
    m_batchTimer.start(m_batchWindowMs);
  }
}

void SmartSyncScheduler::executeSync(const SyncTask &p_task) {
  QString errorMsg;
  bool success = false;

  qInfo() << "[SmartSyncScheduler] Executing sync for:" << p_task.filePath;

  emit syncStarted(p_task.filePath);

  // Execute the actual sync
  success = m_syncService->pushFile(
    p_task.filePath,
    p_task.rootFolderPath,
    p_task.content,
    QStringLiteral("Update file"),
    errorMsg
  );

  // Update status based on result
  if (success) {
    m_statusManager->markSynced(p_task.filePath, p_task.content);
    qInfo() << "[SmartSyncScheduler] Sync completed for:" << p_task.filePath;
  } else {
    m_statusManager->markFailed(p_task.filePath, errorMsg);
    m_statusManager->incrementRetryCount(p_task.filePath);
    qInfo() << "[SmartSyncScheduler] Sync failed for:" << p_task.filePath << "error:" << errorMsg;
  }

  emit syncCompleted(p_task.filePath, success, errorMsg);
  emit taskFinished(p_task.filePath, success, errorMsg);
}
