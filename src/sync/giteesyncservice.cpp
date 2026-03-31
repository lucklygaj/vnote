#include "giteesyncservice.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "sync/giteesyncconfig.h"

#include "core/logger.h"
#include "core/vnotex.h"
#include "utils/pathutils.h"
#include "widgets/messageboxhelper.h"
#include "widgets/mainwindow.h"

using namespace vnotex;

GiteeSyncService GiteeSyncService::s_inst;

GiteeSyncService &GiteeSyncService::getInst() {
  return s_inst;
}

GiteeSyncService::GiteeSyncService()
  : m_api(new GiteeApi(this)), m_shaCache(new ShaCache(this)),
    m_config(nullptr) {
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
  qInfo() << "[GiteeSyncService] Pulling file:" << p_path;

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService] Sync is disabled, skipping pull";
    return true;
  }

  QString errorMsg;
  QString sha;
  bool success = m_api->getFileContent(p_path, p_content, sha, errorMsg);

  if (!success) {
    if (errorMsg.isEmpty()) {
      // File doesn't exist on Gitee (404), ask user whether to create it
      qInfo() << "[GiteeSyncService] File not found on Gitee, asking user whether to create it:" << p_path;
      QString questionText = tr("File '%1' does not exist in Gitee repository. Do you want to create it?").arg(p_path);
      int ret = MessageBoxHelper::questionYesNo(MessageBoxHelper::Question, questionText, tr("Create file on Gitee"),
                                                 QString(), VNoteX::getInst().getMainWindow());
      if (ret == QMessageBox::Yes) {
        // Read local file content
        QFile file(p_path);
        if (!file.open(QIODevice::ReadOnly)) {
          QString msg = tr("Failed to open local file: %1").arg(p_path);
          qCritical() << "[GiteeSyncService]" << msg;
          emit syncFailed(msg);
          return false;
        }
        QString content = QString::fromUtf8(file.readAll());
        file.close();

        // Create file on Gitee
        QString createErrorMsg;
        QString sha = m_api->createFile(p_path, content, tr("Create file from local"), createErrorMsg);
        if (sha.isEmpty()) {
          qCritical() << "[GiteeSyncService] Failed to create file on Gitee:" << createErrorMsg;
          emit syncFailed(createErrorMsg);
          return false;
        }
        m_shaCache->updateSha(p_path, sha);
        qInfo() << "[GiteeSyncService] File created on Gitee:" << p_path << "SHA:" << sha.left(8) + "...";
      }
      return true;
    }
    qCritical() << "[GiteeSyncService] Pull failed:" << errorMsg;
    handlePullError(errorMsg);
    return false;
  }

  // Update SHA cache
  m_shaCache->updateSha(p_path, sha);
  qInfo() << "[GiteeSyncService] File pulled successfully:" << p_path << "SHA:" << sha.left(8) + "...";

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
  qInfo() << "[GiteeSyncService] Pushing file (blocking) - File:" << p_path << "Root:" << p_rootFolderPath << "Message:" << p_commitMessage;

  if (!checkSyncEnabled()) {
    qInfo() << "[GiteeSyncService] Sync is disabled, skipping push";
    return true;
  }

  // Get current SHA from cache
  QString sha = m_shaCache->getSha(p_path);
  qInfo() << "[GiteeSyncService] Cached SHA:" << (sha.isEmpty() ? "(empty)" : sha.left(8) + "...");

  if (sha.isEmpty()) {
    qInfo() << "[GiteeSyncService] No cached SHA, fetching from Gitee";
    // Try to get SHA from Gitee
    QString errorMsg;
    if (!m_api->getFileSha(p_path, sha, errorMsg)) {
      if (errorMsg.contains("404") || errorMsg.isEmpty()) {
        // File doesn't exist on Gitee, create it
        qInfo() << "[GiteeSyncService] File doesn't exist on Gitee (404), creating file";
        QString newSha = m_api->createFile(p_path, p_content, p_commitMessage, p_msg);
        if (newSha.isEmpty()) {
          qCritical() << "[GiteeSyncService] Failed to create file on Gitee:" << p_msg;
          return false;
        }
        m_shaCache->updateSha(p_path, newSha);
        qInfo() << "[GiteeSyncService] File created on Gitee:" << p_path << "SHA:" << newSha.left(8) + "...";
        return true;
      } else {
        qCritical() << "[GiteeSyncService] Failed to get file SHA:" << errorMsg;
        p_msg = errorMsg;
        return false;
      }
    } else {
      m_shaCache->updateSha(p_path, sha);
      qInfo() << "[GiteeSyncService] SHA fetched from Gitee:" << sha.left(8) + "...";
    }
  }

  // Update file
  qInfo() << "[GiteeSyncService] Updating file on Gitee";
  QString newSha = m_api->updateFile(p_path, p_content, p_commitMessage, sha, p_msg);
  if (newSha.isEmpty()) {
    if (p_msg.contains("409")) {
      // SHA conflict, retry with new SHA
      qWarning() << "[GiteeSyncService] SHA conflict detected, retrying with new SHA";
      return retryPushWithNewSha(p_path, p_content, p_commitMessage);
    } else if (p_msg.contains("429")) {
      // Rate limited
      qWarning() << "[GiteeSyncService] Rate limited when pushing:" << p_path;
      emit rateLimited(60);
      return false;
    } else {
      qCritical() << "[GiteeSyncService] Push failed:" << p_msg;
      return false;
    }
  }

  m_shaCache->updateSha(p_path, newSha);
  qInfo() << "[GiteeSyncService] File pushed to Gitee successfully:" << p_path << "New SHA:" << newSha.left(8) + "...";
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
