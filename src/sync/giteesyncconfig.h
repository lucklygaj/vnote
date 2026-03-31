#ifndef GITEESYNCCONFIG_H
#define GITEESYNCCONFIG_H

#include "iconfig.h"

#include <QString>
#include <QtGlobal>

class QJsonObject;

namespace vnotex {
class ConfigMgr;

class GiteeSyncConfig : public IConfig {
public:
  GiteeSyncConfig(ConfigMgr *p_mgr, IConfig *p_topConfig);

  void init(const QJsonObject &p_app, const QJsonObject &p_user) Q_DECL_OVERRIDE;

  QJsonObject toJson() const Q_DECL_OVERRIDE;

  const QString &getToken() const;
  void setToken(const QString &p_token);

  const QString &getOwner() const;
  void setOwner(const QString &p_owner);

  const QString &getRepo() const;
  void setRepo(const QString &p_repo);

  const QString &getBranch() const;
  void setBranch(const QString &p_branch);

  bool isSyncEnabled() const;
  void setSyncEnabled(bool p_enabled);

  bool validateConfig(QString &p_errorMsg) const;

private:
  friend class MainConfig;

  void loadGiteeSync(const QJsonObject &p_app, const QJsonObject &p_user);

  QJsonObject saveGiteeSync() const;

  // Gitee personal access token
  QString m_token;

  // Repository owner (username or organization)
  QString m_owner;

  // Repository name
  QString m_repo;

  // Branch name (default: "master")
  QString m_branch;

  // Whether Gitee sync is enabled (default: false)
  bool m_syncEnabled = false;
};
} // namespace vnotex

#endif // GITEESYNCCONFIG_H
