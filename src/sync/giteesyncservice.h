#ifndef GITEESYNCSERVICE_H
#define GITEESYNCSERVICE_H

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QFile>

#include "giteeapi.h"
#include "shacache.h"

namespace vnotex {

class GiteeSyncConfig;

class GiteeSyncService : public QObject {
  Q_OBJECT
public:
  static GiteeSyncService &getInst();

  void init(GiteeSyncConfig *p_config);

  /**
   * Check if sync is enabled
   */
  bool checkSyncEnabled() const;

  /**
   * Validate configuration
   * @param p_errorMsg Output error message
   * @return true if configuration is valid
   */
  bool validateConfig(QString &p_errorMsg) const;

  /**
   * Pull a file from Gitee
   * @param p_path File path in the repository
   * @param p_content Output file content
   * @return true if successful
   */
  bool pullFile(const QString &p_path, QString &p_content);

  /**
   * Pull a directory from Gitee
   * @param p_path Directory path in the repository
   * @return true if successful
   */
  bool pullDirectory(const QString &p_path);

  /**
   * Push a file to Gitee (blocking, synchronous)
   * @param p_path File path in the repository
   * @param p_rootFolderPath Notebook root folder path (for reading local file)
   * @param p_content File content
   * @param p_commitMessage Commit message
   * @param p_msg Output error message
   * @return true if successful
   */
  bool pushFile(const QString &p_path, const QString &p_rootFolderPath, const QString &p_content, const QString &p_commitMessage, QString &p_msg);

  /**
   * Delete a file from Gitee (blocking, synchronous)
   * @param p_path File path in the repository
   * @param p_commitMessage Commit message
   * @param p_msg Output error message
   * @return true if successful
   */
  bool deleteFile(const QString &p_path, const QString &p_commitMessage, QString &p_msg);

  /**
   * Update configuration
   */
  void updateConfig();

signals:
  void syncFailed(const QString &p_msg);

  void rateLimited(int p_retrySeconds);

  void syncStatusChanged(bool p_syncing);

private:
  GiteeSyncService();

  ~GiteeSyncService();

  void setupConnections();

  void handlePullError(const QString &p_msg);

  void handlePushError(const QString &p_path, const QString &p_msg);

  bool retryPushWithNewSha(const QString &p_path, const QString &p_content, const QString &p_commitMessage);

  QScopedPointer<GiteeApi> m_api;

  QScopedPointer<ShaCache> m_shaCache;

  GiteeSyncConfig *m_config;

  static GiteeSyncService s_inst;
};
} // namespace vnotex

#endif // GITEESYNCSERVICE_H
