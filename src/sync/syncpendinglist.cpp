#include "syncpendinglist.h"

#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>

#include "core/configmgr.h"

using namespace vnotex;

SyncPendingList &SyncPendingList::getInst() {
  static SyncPendingList inst;
  return inst;
}

SyncPendingList::SyncPendingList()
  : m_filePath(ConfigMgr::getInst().getUserFolder() + "/sync_pending.json") {
}

SyncPendingList::~SyncPendingList() {
  save();
}

void SyncPendingList::init() {
  load();
}

void SyncPendingList::addPendingItem(const PendingSyncItem &p_item) {
  m_items[p_item.filePath] = p_item;
  save();
  emit pendingListChanged();
}

void SyncPendingList::removePendingItem(const QString &p_filePath) {
  if (m_items.remove(p_filePath) > 0) {
    save();
    emit pendingListChanged();
  }
}

PendingSyncItem SyncPendingList::getPendingItem(const QString &p_filePath) const {
  return m_items.value(p_filePath);
}

QList<PendingSyncItem> SyncPendingList::getAllPendingItems() const {
  return m_items.values();
}

void SyncPendingList::incrementRetryCount(const QString &p_filePath) {
  auto it = m_items.find(p_filePath);
  if (it != m_items.end()) {
    it.value().retryCount++;
    save();
  }
}

void SyncPendingList::resetRetryCount(const QString &p_filePath) {
  auto it = m_items.find(p_filePath);
  if (it != m_items.end()) {
    it.value().retryCount = 0;
    save();
  }
}

bool SyncPendingList::hasPendingItems() const {
  return !m_items.isEmpty();
}

int SyncPendingList::pendingCount() const {
  return m_items.size();
}

void SyncPendingList::cleanupStaleItems(int p_maxAgeDays) {
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  qint64 maxAgeMs = p_maxAgeDays * 24 * 60 * 60 * 1000LL;
  bool changed = false;

  auto it = m_items.begin();
  while (it != m_items.end()) {
    if (now - it.value().timestamp > maxAgeMs) {
      qInfo() << "[SyncPendingList] Removing stale item:" << it.key();
      it = m_items.erase(it);
      changed = true;
    } else {
      ++it;
    }
  }

  if (changed) {
    save();
    emit pendingListChanged();
  }
}

void SyncPendingList::save() {
  QJsonObject rootObj;
  QJsonArray itemsArray;
  for (const auto &item : m_items) {
    itemsArray.append(item.toJson());
  }
  rootObj["items"] = itemsArray;
  rootObj["lastUpdated"] = QDateTime::currentMSecsSinceEpoch();

  QJsonDocument doc(rootObj);
  QFile file(m_filePath);
  QDir().mkpath(QFileInfo(m_filePath).absolutePath());
  if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson());
    file.close();
    qInfo() << "[SyncPendingList] Saved" << m_items.size() << "pending items";
  }
}

void SyncPendingList::load() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly)) {
    qInfo() << "[SyncPendingList] No pending file found, starting fresh";
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject()) {
    return;
  }

  QJsonObject rootObj = doc.object();
  QJsonArray itemsArray = rootObj["items"].toArray();
  for (const QJsonValue &val : itemsArray) {
    PendingSyncItem item;
    item.fromJson(val.toObject());
    m_items[item.filePath] = item;
  }

  qInfo() << "[SyncPendingList] Loaded" << m_items.size() << "pending items";

  // Cleanup stale items on load
  cleanupStaleItems();
}

QJsonObject PendingSyncItem::toJson() const {
  QJsonObject obj;
  obj["filePath"] = filePath;
  obj["rootFolderPath"] = rootFolderPath;
  obj["content"] = content;
  obj["sha"] = sha;
  obj["timestamp"] = timestamp;
  obj["retryCount"] = retryCount;
  return obj;
}

void PendingSyncItem::fromJson(const QJsonObject &p_jobj) {
  filePath = p_jobj["filePath"].toString();
  rootFolderPath = p_jobj["rootFolderPath"].toString();
  content = p_jobj["content"].toString();
  sha = p_jobj["sha"].toString();
  timestamp = p_jobj["timestamp"].toInteger();
  retryCount = p_jobj["retryCount"].toInt();
}
