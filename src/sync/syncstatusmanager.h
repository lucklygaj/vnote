#ifndef SYNCSTATUSMANAGER_H
#define SYNCSTATUSMANAGER_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

#include "filesyncstate.h"

namespace vnotex {

class FileSyncStateInfo {
public:
  FileSyncState state = FileSyncState::SYNCED;
  QString pendingContent;
  QString syncedContent;
  int retryCount = 0;
  qint64 lastModified = 0;
  qint64 lastSyncTime = 0;
  QString lastError;

  QJsonObject toJson() const;
  void fromJson(const QJsonObject &p_jobj);
};

class SyncStatusManager : public QObject {
  Q_OBJECT
public:
  static SyncStatusManager &getInst();

  void init(const QString &p_stateFilePath);

  FileSyncState getState(const QString &p_path) const;
  void setState(const QString &p_path, FileSyncState p_state);

  void setPendingContent(const QString &p_path, const QString &p_content);
  QString getPendingContent(const QString &p_path) const;

  void setSyncedContent(const QString &p_path, const QString &p_content);
  QString getSyncedContent(const QString &p_path) const;

  int getRetryCount(const QString &p_path) const;
  void incrementRetryCount(const QString &p_path);
  void resetRetryCount(const QString &p_path);

  void setLastError(const QString &p_path, const QString &p_error);
  QString getLastError(const QString &p_path) const;

  void markSynced(const QString &p_path, const QString &p_content);
  void markFailed(const QString &p_path, const QString &p_error);

  // Statistics
  int getSyncedCount() const;
  int getPendingCount() const;
  int getSyncingCount() const;
  int getFailedCount() const;
  int getPausedCount() const;

  // Persistence
  void saveState();
  void loadState();

  // Cleanup
  void removeFile(const QString &p_path);

  // Remove entries for files that no longer exist on disk
  int cleanupStaleEntries();

  // Get all tracked file states (for display in Sync Center)
  const QMap<QString, FileSyncStateInfo> &getAllStates() const { return m_states; }

signals:
  void stateChanged(const QString &p_path, FileSyncState p_state);
  void fileStateChanged(const QString &p_path);
  void statisticsChanged(int p_synced, int p_pending, int p_syncing, int p_failed, int p_paused);

private:
  SyncStatusManager();
  ~SyncStatusManager();

  void notifyStatisticsChanged();

  QMap<QString, FileSyncStateInfo> m_states;
  QString m_stateFilePath;
};

} // namespace vnotex

#endif // SYNCSTATUSMANAGER_H
