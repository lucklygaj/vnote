#include "giteesyncconfig.h"

#include <QJsonObject>

#include "core/configmgr.h"

using namespace vnotex;

GiteeSyncConfig::GiteeSyncConfig(ConfigMgr *p_mgr, IConfig *p_topConfig)
  : IConfig(p_mgr, p_topConfig) {
  m_sessionName = QStringLiteral("gitee_sync");
}

void GiteeSyncConfig::init(const QJsonObject &p_app, const QJsonObject &p_user) {
  loadGiteeSync(p_app, p_user);
}

QJsonObject GiteeSyncConfig::toJson() const {
  return saveGiteeSync();
}

const QString &GiteeSyncConfig::getToken() const {
  return m_token;
}

void GiteeSyncConfig::setToken(const QString &p_token) {
  updateConfig(m_token, p_token, this);
}

const QString &GiteeSyncConfig::getOwner() const {
  return m_owner;
}

void GiteeSyncConfig::setOwner(const QString &p_owner) {
  updateConfig(m_owner, p_owner, this);
}

const QString &GiteeSyncConfig::getRepo() const {
  return m_repo;
}

void GiteeSyncConfig::setRepo(const QString &p_repo) {
  updateConfig(m_repo, p_repo, this);
}

const QString &GiteeSyncConfig::getBranch() const {
  return m_branch;
}

void GiteeSyncConfig::setBranch(const QString &p_branch) {
  updateConfig(m_branch, p_branch, this);
}

bool GiteeSyncConfig::isSyncEnabled() const {
  return m_syncEnabled;
}

void GiteeSyncConfig::setSyncEnabled(bool p_enabled) {
  updateConfig(m_syncEnabled, p_enabled, this);
}

bool GiteeSyncConfig::validateConfig(QString &p_errorMsg) const {
  if (m_syncEnabled) {
    if (m_token.isEmpty()) {
      p_errorMsg = QObject::tr("Gitee token is empty");
      return false;
    }
    if (m_owner.isEmpty()) {
      p_errorMsg = QObject::tr("Gitee owner is empty");
      return false;
    }
    if (m_repo.isEmpty()) {
      p_errorMsg = QObject::tr("Gitee repository name is empty");
      return false;
    }
    if (m_branch.isEmpty()) {
      p_errorMsg = QObject::tr("Gitee branch name is empty");
      return false;
    }
  }

  return true;
}

void GiteeSyncConfig::loadGiteeSync(const QJsonObject &p_app, const QJsonObject &p_user) {
  const auto appGitee = p_app.value(QStringLiteral("gitee_sync")).toObject();
  const auto userGitee = p_user.value(QStringLiteral("gitee_sync")).toObject();

  // Load with user config taking precedence
  m_token = readString(appGitee, userGitee, QStringLiteral("token"));
  m_owner = readString(appGitee, userGitee, QStringLiteral("owner"));
  m_repo = readString(appGitee, userGitee, QStringLiteral("repo"));
  m_branch = readString(appGitee, userGitee, QStringLiteral("branch"));
  if (m_branch.isEmpty()) {
    m_branch = QStringLiteral("master");
  }

  m_syncEnabled = readBool(appGitee, userGitee, QStringLiteral("sync_enabled"));
}

QJsonObject GiteeSyncConfig::saveGiteeSync() const {
  QJsonObject obj;
  writeString(obj, QStringLiteral("token"), m_token);
  writeString(obj, QStringLiteral("owner"), m_owner);
  writeString(obj, QStringLiteral("repo"), m_repo);
  writeString(obj, QStringLiteral("branch"), m_branch);
  writeBool(obj, QStringLiteral("sync_enabled"), m_syncEnabled);
  return obj;
}
