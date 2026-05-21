#include "syncstatusmanager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDir>
#include <QDateTime>

#include "utils/pathutils.h"

using namespace vnotex;

SyncStatusManager &SyncStatusManager::getInst() {
  static SyncStatusManager inst;
  return inst;
}

SyncStatusManager::SyncStatusManager()
  : m_stateFilePath() {
}

SyncStatusManager::~SyncStatusManager() {
  saveState();
}

void SyncStatusManager::init(const QString &p_stateFilePath) {
  m_stateFilePath = p_stateFilePath;
  loadState();
}

FileSyncState SyncStatusManager::getState(const QString &p_path) const {
  auto it = m_states.find(p_path);
  if (it == m_states.end()) {
    return FileSyncState::SYNCED;
  }
  return it.value().state;
}

void SyncStatusManager::setState(const QString &p_path, FileSyncState p_state) {
  FileSyncStateInfo &info = m_states[p_path];
  if (info.state != p_state) {
    info.state = p_state;
    emit stateChanged(p_path, p_state);
    emit fileStateChanged(p_path);
    notifyStatisticsChanged();
  }
}

void SyncStatusManager::setPendingContent(const QString &p_path, const QString &p_content) {
  FileSyncStateInfo &info = m_states[p_path];
  info.pendingContent = p_content;
  info.lastModified = QDateTime::currentMSecsSinceEpoch();
}

QString SyncStatusManager::getPendingContent(const QString &p_path) const {
  auto it = m_states.find(p_path);
  if (it == m_states.end()) {
    return QString();
  }
  return it.value().pendingContent;
}

void SyncStatusManager::setSyncedContent(const QString &p_path, const QString &p_content) {
  FileSyncStateInfo &info = m_states[p_path];
  info.syncedContent = p_content;
}

QString SyncStatusManager::getSyncedContent(const QString &p_path) const {
  auto it = m_states.find(p_path);
  if (it == m_states.end()) {
    return QString();
  }
  return it.value().syncedContent;
}

int SyncStatusManager::getRetryCount(const QString &p_path) const {
  auto it = m_states.find(p_path);
  if (it == m_states.end()) {
    return 0;
  }
  return it.value().retryCount;
}

void SyncStatusManager::incrementRetryCount(const QString &p_path) {
  FileSyncStateInfo &info = m_states[p_path];
  info.retryCount++;
}

void SyncStatusManager::resetRetryCount(const QString &p_path) {
  auto it = m_states.find(p_path);
  if (it != m_states.end()) {
    it.value().retryCount = 0;
  }
}

void SyncStatusManager::setLastError(const QString &p_path, const QString &p_error) {
  FileSyncStateInfo &info = m_states[p_path];
  info.lastError = p_error;
}

QString SyncStatusManager::getLastError(const QString &p_path) const {
  auto it = m_states.find(p_path);
  if (it == m_states.end()) {
    return QString();
  }
  return it.value().lastError;
}

void SyncStatusManager::markSynced(const QString &p_path, const QString &p_content) {
  FileSyncStateInfo &info = m_states[p_path];
  info.state = FileSyncState::SYNCED;
  info.syncedContent = p_content;
  info.pendingContent.clear();
  info.retryCount = 0;
  info.lastSyncTime = QDateTime::currentMSecsSinceEpoch();
  info.lastError.clear();
  emit stateChanged(p_path, FileSyncState::SYNCED);
  emit fileStateChanged(p_path);
  notifyStatisticsChanged();
}

void SyncStatusManager::markFailed(const QString &p_path, const QString &p_error) {
  FileSyncStateInfo &info = m_states[p_path];
  info.state = FileSyncState::FAILED;
  info.lastError = p_error;
  emit stateChanged(p_path, FileSyncState::FAILED);
  emit fileStateChanged(p_path);
  notifyStatisticsChanged();
}

int SyncStatusManager::getSyncedCount() const {
  int count = 0;
  for (const auto &info : m_states) {
    if (info.state == FileSyncState::SYNCED) {
      count++;
    }
  }
  return count;
}

int SyncStatusManager::getPendingCount() const {
  int count = 0;
  for (const auto &info : m_states) {
    if (info.state == FileSyncState::PENDING) {
      count++;
    }
  }
  return count;
}

int SyncStatusManager::getSyncingCount() const {
  int count = 0;
  for (const auto &info : m_states) {
    if (info.state == FileSyncState::SYNCING) {
      count++;
    }
  }
  return count;
}

int SyncStatusManager::getFailedCount() const {
  int count = 0;
  for (const auto &info : m_states) {
    if (info.state == FileSyncState::FAILED) {
      count++;
    }
  }
  return count;
}

int SyncStatusManager::getPausedCount() const {
  int count = 0;
  for (const auto &info : m_states) {
    if (info.state == FileSyncState::PAUSED) {
      count++;
    }
  }
  return count;
}

void SyncStatusManager::saveState() {
  QJsonObject rootObj;
  QJsonArray filesArray;
  for (auto it = m_states.constBegin(); it != m_states.constEnd(); ++it) {
    QJsonObject fileObj;
    fileObj["path"] = it.key();
    fileObj["state"] = static_cast<int>(it.value().state);
    fileObj["info"] = it.value().toJson();
    filesArray.append(fileObj);
  }
  rootObj["files"] = filesArray;
  rootObj["lastUpdated"] = QDateTime::currentMSecsSinceEpoch();

  QJsonDocument doc(rootObj);
  QFile file(m_stateFilePath);
  QDir().mkpath(QFileInfo(m_stateFilePath).absolutePath());
  if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson());
    file.close();
  }
}

void SyncStatusManager::loadState() {
  QFile file(m_stateFilePath);
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject()) {
    return;
  }

  QJsonObject rootObj = doc.object();
  QJsonArray filesArray = rootObj["files"].toArray();
  for (const QJsonValue &val : filesArray) {
    QJsonObject fileObj = val.toObject();
    QString path = fileObj["path"].toString();
    FileSyncStateInfo info;
    info.fromJson(fileObj["info"].toObject());
    m_states[path] = info;
  }
}

void SyncStatusManager::removeFile(const QString &p_path) {
  if (m_states.remove(p_path) > 0) {
    emit fileStateChanged(p_path);
    notifyStatisticsChanged();
  }
}

int SyncStatusManager::cleanupStaleEntries() {
  QStringList stalePaths;
  for (auto it = m_states.constBegin(); it != m_states.constEnd(); ++it) {
    if (!QFile::exists(it.key())) {
      stalePaths << it.key();
    }
  }
  for (const QString &path : stalePaths) {
    m_states.remove(path);
  }
  if (!stalePaths.isEmpty()) {
    notifyStatisticsChanged();
  }
  return stalePaths.size();
}

void SyncStatusManager::notifyStatisticsChanged() {
  emit statisticsChanged(getSyncedCount(), getPendingCount(), getSyncingCount(), getFailedCount(), getPausedCount());
}

QJsonObject FileSyncStateInfo::toJson() const {
  QJsonObject obj;
  obj["state"] = static_cast<int>(state);
  obj["syncedContent"] = syncedContent;
  obj["retryCount"] = retryCount;
  obj["lastModified"] = lastModified;
  obj["lastSyncTime"] = lastSyncTime;
  obj["lastError"] = lastError;
  return obj;
}

void FileSyncStateInfo::fromJson(const QJsonObject &p_jobj) {
  state = static_cast<FileSyncState>(p_jobj["state"].toInt(static_cast<int>(FileSyncState::SYNCED)));
  syncedContent = p_jobj["syncedContent"].toString();
  retryCount = p_jobj["retryCount"].toInt();
  lastModified = p_jobj["lastModified"].toInteger();
  lastSyncTime = p_jobj["lastSyncTime"].toInteger();
  lastError = p_jobj["lastError"].toString();
}
