#ifndef SYNCPENDINGLIST_H
#define SYNCPENDINGLIST_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace vnotex {

struct PendingSyncItem {
  QString filePath;
  QString rootFolderPath;
  QString content;
  QString sha;
  qint64 timestamp;
  int retryCount;

  QJsonObject toJson() const;
  void fromJson(const QJsonObject &p_jobj);
};

class SyncPendingList : public QObject {
  Q_OBJECT
public:
  static SyncPendingList &getInst();

  void init();

  void addPendingItem(const PendingSyncItem &p_item);
  void removePendingItem(const QString &p_filePath);
  PendingSyncItem getPendingItem(const QString &p_filePath) const;
  QList<PendingSyncItem> getAllPendingItems() const;

  void incrementRetryCount(const QString &p_filePath);
  void resetRetryCount(const QString &p_filePath);

  bool hasPendingItems() const;
  int pendingCount() const;

  // Cleanup
  void cleanupStaleItems(int p_maxAgeDays = 7);

  // Persistence
  void save();
  void load();

signals:
  void pendingListChanged();

private:
  SyncPendingList();
  ~SyncPendingList();

  QString m_filePath;
  QMap<QString, PendingSyncItem> m_items;
};

} // namespace vnotex

#endif // SYNCPENDINGLIST_H
