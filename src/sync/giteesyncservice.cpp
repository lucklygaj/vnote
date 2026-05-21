#include "giteesyncservice.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

#include "sync/giteesyncconfig.h"

#include "core/logger.h"
#include "core/vnotex.h"
#include "utils/pathutils.h"
#include "widgets/messageboxhelper.h"
#include "widgets/mainwindow.h"

using namespace vnotex;

GiteeSyncService &GiteeSyncService::getInst() {
  static GiteeSyncService inst;
  return inst;
}

GiteeSyncService::GiteeSyncService()
  : m_api(new GiteeApi(this)), m_shaCache(new ShaCache(this)),
    m_config(nullptr), m_lastSyncTime(0) {
  setupConnections();
}

GiteeSyncService::~GiteeSyncService() {
}

void GiteeSyncService::init(GiteeSyncConfig *p_config) {
  m_config = p_config;
  qInfo() << "[GiteeSyncService] Initializing with config";
  updateConfig();
}

void GiteeSyncService::updateConfig() {
  if (!m_config) {
    qWarning() << "[GiteeSyncService] Config is null, cannot update";
    return;
  }

  qInfo() << "[GiteeSyncService] Updating config - Sync enabled:" << m_config->isSyncEnabled();
  m_api->setToken(m_config->getToken());
  m_api->setOwner(m_config->getOwner());
  m_api->setRepo(m_config->getRepo());
  m_api->setBranch(m_config->getBranch());
}

bool GiteeSyncService::checkSyncEnabled() const {
  bool enabled = m_config && m_config->isSyncEnabled();
  qInfo() << "[GiteeSyncService] Sync enabled check:" << enabled;
  return enabled;
}

bool GiteeSyncService::validateConfig(QString &p_errorMsg) const {
  if (!m_config) {
    p_errorMsg = tr("GiteeSyncConfig is not initialized");
    qCritical() << "[GiteeSyncService] Config is null";
    return false;
  }
  bool valid = m_config->validateConfig(p_errorMsg);
  if (!valid) {
    qCritical() << "[GiteeSyncService] Config validation failed:" << p_errorMsg;
  } else {
    qInfo() << "[GiteeSyncService] Config validation passed";
  }
  return valid;
}

void GiteeSyncService::setupConnections() {
  connect(m_api.data(), &GiteeApi::requestFinished, this, [this]() {
    emit syncStatusChanged(false);
  });
}

bool GiteeSyncService::pullFile(const QString &p_path, QString &p_content) {
  qInfo() << "[GiteeSyncService::pullFile] START - Path:" << p_path;

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService::pullFile] Sync is disabled, skipping pull";
    return false;
  }

  if (m_newlyCreatedFiles.contains(p_path)) {
    qInfo() << "[GiteeSyncService::pullFile] File is newly created, skipping pull:" << p_path;
    QStringList list;
    for (const auto &item : m_newlyCreatedFiles) {
      list << item;
    }
    qInfo() << "[GiteeSyncService::pullFile] Newly created files set:" << list;
    m_newlyCreatedFiles.remove(p_path);
    qInfo() << "[GiteeSyncService::pullFile] Removed from newly created set, returning FALSE (keep local content)";
    return false;
  }

  QString errorMsg;
  QString sha;
  qInfo() << "[GiteeSyncService::pullFile] Fetching file from Gitee API";
  bool success = m_api->getFileContent(p_path, p_content, sha, errorMsg);
  qInfo() << "[GiteeSyncService::pullFile] API result:" << success << "SHA:" << (sha.isEmpty() ? "(empty)" : sha.left(8) + "...") << "Remote content length:" << p_content.length();

  if (!success) {
    if (errorMsg.isEmpty()) {
      // File doesn't exist on Gitee (404), ask user whether to create it
      qInfo() << "[GiteeSyncService::pullFile] File not found on Gitee (404), asking user whether to create it:" << p_path;
      QString questionText = tr("File '%1' does not exist in Gitee repository. Do you want to create it?").arg(p_path);
      int ret = MessageBoxHelper::questionYesNo(MessageBoxHelper::Question, questionText, tr("Create file on Gitee"),
                                                 QString(), VNoteX::getInst().getMainWindow());
      if (ret == QMessageBox::Yes) {
        // Read local file content
        QFile file(p_path);
        if (!file.open(QIODevice::ReadOnly)) {
          QString msg = tr("Failed to open local file: %1").arg(p_path);
          qCritical() << "[GiteeSyncService::pullFile]" << msg;
          emit syncFailed(msg);
          return false;
        }
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Create file on Gitee
        QString createErrorMsg;
        QString sha = m_api->createFile(p_path, content, tr("Create file from local"), createErrorMsg);
        if (sha.isEmpty()) {
          qCritical() << "[GiteeSyncService::pullFile] Failed to create file on Gitee:" << createErrorMsg;
          emit syncFailed(createErrorMsg);
          return false;
        }
        m_shaCache->updateSha(p_path, sha);
        qInfo() << "[GiteeSyncService::pullFile] File created on Gitee:" << p_path << "SHA:" << sha.left(8) + "...";
      }
      qInfo() << "[GiteeSyncService::pullFile] Returning FALSE (404 case - keep local content)";
      return false;
    }
    qCritical() << "[GiteeSyncService::pullFile] Pull failed:" << errorMsg;
    handlePullError(errorMsg);
    return false;
  }

  // Update SHA cache
  m_shaCache->updateSha(p_path, sha);
  qInfo() << "[GiteeSyncService::pullFile] File pulled successfully, returning TRUE (WILL replace content)";
  qInfo() << "[GiteeSyncService::pullFile] END - Path:" << p_path << "Content length:" << p_content.length();

  return true;
}

bool GiteeSyncService::pullDirectory(const QString &p_path) {
  qInfo() << "[GiteeSyncService] Pulling directory:" << p_path;

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService] Sync is disabled, skipping directory pull";
    return true;
  }

  QString errorMsg;
  QStringList files;
  QStringList dirs;
  QJsonArray shas;

  bool success = m_api->getDirectoryContents(p_path, files, dirs, shas, errorMsg);

  if (!success) {
    if (errorMsg.isEmpty()) {
      // Directory doesn't exist on Gitee (404), ask user whether to create it
      qInfo() << "[GiteeSyncService] Directory not found on Gitee, asking user whether to create it:" << p_path;
      QString questionText = tr("Directory '%1' does not exist in Gitee repository. Do you want to create it?").arg(p_path);
      int ret = MessageBoxHelper::questionYesNo(MessageBoxHelper::Question, questionText, tr("Create directory on Gitee"),
                                                 QString(), VNoteX::getInst().getMainWindow());
      if (ret == QMessageBox::Yes) {
        // Create directory by creating a .gitkeep file
        QString gitkeepPath = p_path.endsWith('/') ? p_path + ".gitkeep" : p_path + "/.gitkeep";
        QString createErrorMsg;
        QString sha = m_api->createFile(gitkeepPath, "", tr("Create directory from local"), createErrorMsg);
        if (sha.isEmpty()) {
          qCritical() << "[GiteeSyncService] Failed to create directory on Gitee:" << createErrorMsg;
          emit syncFailed(createErrorMsg);
          return false;
        }
        m_shaCache->updateSha(gitkeepPath, sha);
        qInfo() << "[GiteeSyncService] Directory created on Gitee:" << p_path << "via .gitkeep";
      }
      return true;
    }
    qCritical() << "[GiteeSyncService] Directory pull failed:" << errorMsg;
    handlePullError(errorMsg);
    return false;
  }

  // Batch update SHA cache
  m_shaCache->batchUpdateSha(shas, p_path);
  qInfo() << "[GiteeSyncService] Directory pulled successfully - Files:" << files.size() << "Dirs:" << dirs.size();

  return true;
}

bool GiteeSyncService::pushFile(const QString &p_path, const QString &p_rootFolderPath, const QString &p_content, const QString &p_commitMessage, QString &p_msg) {
  qInfo() << "[GiteeSyncService::pushFile] START - Path:" << p_path << "Root:" << p_rootFolderPath << "Message:" << p_commitMessage;
  qInfo() << "[GiteeSyncService::pushFile] Local content length:" << p_content.length() << "First 100 chars:" << p_content.left(100);

  // Check if this file is already being pushed (prevent concurrent pushes)
  {
    QMutexLocker locker(&m_pushMutex);
    if (m_pushingFiles.contains(p_path)) {
      qWarning() << "[GiteeSyncService::pushFile] Push already in progress for:" << p_path << "- skipping duplicate request";
      return false;
    }
    m_pushingFiles.insert(p_path);
  }

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService::pushFile] Sync is disabled, skipping push";
    QMutexLocker locker(&m_pushMutex);
    m_pushingFiles.remove(p_path);
    return true;
  }

  // Get current SHA from cache
  QString sha = m_shaCache->getSha(p_path);
  qInfo() << "[GiteeSyncService::pushFile] Cached SHA for" << p_path << ":" << (sha.isEmpty() ? "(empty)" : sha.left(8) + "...");
  qInfo() << "[GiteeSyncService::pushFile] Full cached SHA:" << sha;

  if (sha.isEmpty()) {
    qInfo() << "[GiteeSyncService::pushFile] No cached SHA, fetching from Gitee API";
    // Try to get SHA from Gitee
    QString errorMsg;
    if (!m_api->getFileSha(p_path, sha, errorMsg)) {
      qInfo() << "[GiteeSyncService::pushFile] getFileSha failed, errorMsg:" << errorMsg;
      if (errorMsg.contains("404") || errorMsg.isEmpty()) {
        // File doesn't exist on Gitee, create it
        qInfo() << "[GiteeSyncService::pushFile] File doesn't exist on Gitee (404), creating file";
        QString newSha = m_api->createFile(p_path, p_content, p_commitMessage, p_msg);
        qInfo() << "[GiteeSyncService::pushFile] createFile result - newSha:" << (newSha.isEmpty() ? "(empty)" : newSha.left(8) + "...") << "errorMsg:" << p_msg;
        if (newSha.isEmpty()) {
          qCritical() << "[GiteeSyncService::pushFile] Failed to create file on Gitee:" << p_msg;
          QMutexLocker locker(&m_pushMutex);
          m_pushingFiles.remove(p_path);
          return false;
        }
        m_shaCache->updateSha(p_path, newSha);
        qInfo() << "[GiteeSyncService::pushFile] File created on Gitee:" << p_path << "SHA:" << newSha.left(8) + "...";
        m_lastSyncTime = QDateTime::currentMSecsSinceEpoch();
        QMutexLocker locker(&m_pushMutex);
        m_pushingFiles.remove(p_path);
        return true;
      } else {
        qCritical() << "[GiteeSyncService::pushFile] Failed to get file SHA:" << errorMsg;
        p_msg = errorMsg;
        QMutexLocker locker(&m_pushMutex);
        m_pushingFiles.remove(p_path);
        return false;
      }
    } else {
      m_shaCache->updateSha(p_path, sha);
      qInfo() << "[GiteeSyncService::pushFile] SHA fetched from Gitee:" << sha.left(8) + "...";
      qInfo() << "[GiteeSyncService::pushFile] Full fetched SHA:" << sha;
    }
  }

  // Update file
  qInfo() << "[GiteeSyncService::pushFile] Calling updateFile with SHA:" << sha.left(8) + "...";
  QString newSha = m_api->updateFile(p_path, p_content, p_commitMessage, sha, p_msg);
  qInfo() << "[GiteeSyncService::pushFile] updateFile result - newSha:" << (newSha.isEmpty() ? "(empty)" : newSha.left(8) + "...") << "errorMsg:" << p_msg;
  qInfo() << "[GiteeSyncService::pushFile] Full newSha:" << newSha;

  if (newSha.isEmpty()) {
    qInfo() << "[GiteeSyncService::pushFile] Push failed, checking error type";
    if (p_msg.contains("409")) {
      // SHA conflict, retry with new SHA
      qWarning() << "[GiteeSyncService::pushFile] SHA conflict (409) detected, retrying with new SHA";
      bool retryResult = retryPushWithNewSha(p_path, p_content, p_commitMessage);
      QMutexLocker locker(&m_pushMutex);
      m_pushingFiles.remove(p_path);
      return retryResult;
    } else if (p_msg.contains("429")) {
      // Rate limited
      qWarning() << "[GiteeSyncService::pushFile] Rate limited (429) when pushing:" << p_path;
      emit rateLimited(60);
      QMutexLocker locker(&m_pushMutex);
      m_pushingFiles.remove(p_path);
      return false;
    } else if (p_msg.contains("400") && p_msg.contains("Blob SHA does not match")) {
      // 400 error with SHA mismatch
      qCritical() << "[GiteeSyncService::pushFile] 400 Blob SHA does not match - Cached SHA:" << sha;
      qCritical() << "[GiteeSyncService::pushFile] This indicates a cache-Gitee mismatch. Consider clearing cache for this file.";
      QMutexLocker locker(&m_pushMutex);
      m_pushingFiles.remove(p_path);
      return false;
    } else {
      qCritical() << "[GiteeSyncService::pushFile] Push failed with unknown error:" << p_msg;
      QMutexLocker locker(&m_pushMutex);
      m_pushingFiles.remove(p_path);
      return false;
    }
  }

  m_shaCache->updateSha(p_path, newSha);
  qInfo() << "[GiteeSyncService::pushFile] File pushed to Gitee successfully:" << p_path << "New SHA:" << newSha.left(8) + "...";
  qInfo() << "[GiteeSyncService::pushFile] END - Success";
  m_lastSyncTime = QDateTime::currentMSecsSinceEpoch();

  // Remove from pushing set
  {
    QMutexLocker locker(&m_pushMutex);
    m_pushingFiles.remove(p_path);
  }

  return true;
}

bool GiteeSyncService::deleteFile(const QString &p_path, const QString &p_commitMessage, QString &p_msg) {
  qInfo() << "[GiteeSyncService] Deleting file (blocking) - File:" << p_path << "Message:" << p_commitMessage;

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService] Sync is disabled, skipping delete";
    return true;
  }

  // Get current SHA
  QString sha = m_shaCache->getSha(p_path);
  if (sha.isEmpty()) {
    qInfo() << "[GiteeSyncService] No cached SHA, fetching from Gitee";
    QString errorMsg;
    if (!m_api->getFileSha(p_path, sha, errorMsg)) {
      if (errorMsg.contains("404") || errorMsg.isEmpty()) {
        // File doesn't exist on Gitee, nothing to delete
        qInfo() << "[GiteeSyncService] File doesn't exist on Gitee (404), skipping delete";
        return true;
      } else {
        qCritical() << "[GiteeSyncService] Failed to get file SHA for delete:" << errorMsg;
        p_msg = errorMsg;
        return false;
      }
    } else {
      m_shaCache->updateSha(p_path, sha);
    }
  }

  // Delete file
  qInfo() << "[GiteeSyncService] Deleting file on Gitee - SHA:" << sha.left(8) + "...";
  bool success = m_api->deleteFile(p_path, p_commitMessage, sha, p_msg);
  if (success) {
    m_shaCache->removeSha(p_path);
    qInfo() << "[GiteeSyncService] File deleted from Gitee successfully:" << p_path;
  } else {
    qCritical() << "[GiteeSyncService] Failed to delete file:" << p_msg;
  }

  return success;
}

void GiteeSyncService::markFileAsNewlyCreated(const QString &p_path) {
  qInfo() << "[GiteeSyncService::markFileAsNewlyCreated] START - Path:" << p_path;
  QStringList list;
  for (const auto &item : m_newlyCreatedFiles) {
    list << item;
  }
  qInfo() << "[GiteeSyncService::markFileAsNewlyCreated] Current newly created files set:" << list;
  m_newlyCreatedFiles.insert(p_path);
  list.clear();
  for (const auto &item : m_newlyCreatedFiles) {
    list << item;
  }
  qInfo() << "[GiteeSyncService::markFileAsNewlyCreated] After insertion, newly created files set:" << list;
  qInfo() << "[GiteeSyncService::markFileAsNewlyCreated] Total count:" << m_newlyCreatedFiles.count();
}

qint64 GiteeSyncService::getLastSyncTime() const {
  return m_lastSyncTime;
}

void GiteeSyncService::handlePullError(const QString &p_msg) {
  qCritical() << "[GiteeSyncService] Pull error:" << p_msg;
  emit syncFailed(p_msg);
}

void GiteeSyncService::handlePushError(const QString &p_path, const QString &p_msg) {
  qCritical() << "[GiteeSyncService] Push error for" << p_path << ":" << p_msg;
  emit syncFailed(tr("Failed to push file: %1").arg(p_path));
}

bool GiteeSyncService::retryPushWithNewSha(const QString &p_path, const QString &p_content, const QString &p_commitMessage) {
  qInfo() << "[GiteeSyncService] Retrying push with new SHA for:" << p_path;

  QString errorMsg;
  QString newSha;

  // Get the latest SHA from Gitee
  if (!m_api->getFileSha(p_path, newSha, errorMsg)) {
    qCritical() << "[GiteeSyncService] Failed to get new SHA for retry:" << errorMsg;
    return false;
  }

  qInfo() << "[GiteeSyncService] New SHA fetched:" << newSha.left(8) + "...";

  // Retry push with new SHA
  QString resultSha = m_api->updateFile(p_path, p_content, p_commitMessage, newSha, errorMsg);
  if (resultSha.isEmpty()) {
    qCritical() << "[GiteeSyncService] Retry push failed:" << errorMsg;
    return false;
  }

  m_shaCache->updateSha(p_path, resultSha);
  qInfo() << "[GiteeSyncService] Retry successful - New SHA:" << resultSha.left(8) + "...";
  return true;
}
