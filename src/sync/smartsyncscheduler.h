#ifndef SMARTSYNCSCHEDULER_H
#define SMARTSYNCSCHEDULER_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <QQueue>

#include "filesyncstate.h"

namespace vnotex {

class SyncStatusManager;
class GiteeSyncService;

enum class SyncPriority {
  High,    // Ctrl+S - immediate execution, debounce=0
  Normal,  // Auto-save - batched execution
  Low      // Background operations
};

struct SyncTask {
  QString filePath;
  QString rootFolderPath;
  QString content;
  SyncPriority priority;
  qint64 timestamp;
};

class SmartSyncScheduler : public QObject {
  Q_OBJECT
public:
  static SmartSyncScheduler &getInst();

  void init();

  void initStatusManager(const QString &p_stateFilePath);

  /**
   * Request sync for a file
   * @param p_filePath File path in the repository
   * @param p_rootFolderPath Notebook root folder path
   * @param p_content File content
   * @param p_priority Sync priority
   */
  void requestSync(const QString &p_filePath,
                   const QString &p_rootFolderPath,
                   const QString &p_content,
                   SyncPriority p_priority = SyncPriority::Normal);

  /**
   * Request immediate sync (Ctrl+S)
   */
  void requestImmediateSync(const QString &p_filePath,
                            const QString &p_rootFolderPath,
                            const QString &p_content);

  /**
   * Cancel pending sync for a file
   */
  void cancelSync(const QString &p_filePath);

  /**
   * Force sync all pending files immediately
   */
  void syncAllNow();

  /**
   * Pause all sync operations
   */
  void pause();

  /**
   * Resume all sync operations
   */
  void resume();

  bool isPaused() const;

  // Configuration
  void setBatchWindowMs(int p_ms);
  int getBatchWindowMs() const;

signals:
  void syncStarted(const QString &p_filePath);
  void syncCompleted(const QString &p_filePath, bool p_success, const QString &p_error);
  void queueChanged(int p_pendingCount);
  void taskFinished(const QString &p_filePath, bool p_success, const QString &p_error);

public slots:
  void onNetworkStatusChanged(bool p_online);

private slots:
  void onDebounceTimeout(const QString &p_filePath);
  void onBatchTimeout();

private:
  SmartSyncScheduler();
  ~SmartSyncScheduler();

  void processHighPriorityQueue();
  void addToBatchQueue(const SyncTask &p_task);
  void executeSync(const SyncTask &p_task);
  bool deduplicateTask(const QString &p_filePath, const QString &p_content);

  static QString getFilePathFromKey(const QString &p_key);

  QMap<QString, SyncTask> m_pendingTasks;
  QMap<QString, QTimer*> m_debounceTimers;
  QQueue<SyncTask> m_batchQueue;
  QTimer m_batchTimer;

  QMutex m_mutex;
  bool m_paused = false;
  int m_batchWindowMs = 3000;
  bool m_networkOnline = true;

  SyncStatusManager *m_statusManager;
  GiteeSyncService *m_syncService;
};

} // namespace vnotex

#endif // SMARTSYNCSCHEDULER_H
