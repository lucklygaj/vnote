#ifndef GITEEAPI_H
#define GITEEAPI_H

#include <QObject>
#include <QString>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonArray>

namespace vnotex {

class GiteeApi : public QObject {
  Q_OBJECT
public:
  explicit GiteeApi(QObject *p_parent = nullptr);

  ~GiteeApi();

  void setToken(const QString &p_token);

  void setOwner(const QString &p_owner);

  void setRepo(const QString &p_repo);

  void setBranch(const QString &p_branch);

  /**
   * Create a file in the repository
   * @param p_path File path in the repository
   * @param p_content File content
   * @param p_message Commit message
   * @return SHA of the created file
   */
  QString createFile(const QString &p_path, const QString &p_content, const QString &p_message, QString &p_msg);

  /**
   * Update an existing file in the repository
   * @param p_path File path in the repository
   * @param p_content File content
   * @param p_message Commit message
   * @param p_sha Current SHA of the file
   * @return SHA of the updated file
   */
  QString updateFile(const QString &p_path, const QString &p_content, const QString &p_message, const QString &p_sha, QString &p_msg);

  /**
   * Get file content from the repository
   * @param p_path File path in the repository
   * @param p_content Output file content
   * @param p_sha Output file SHA
   * @return true if successful
   */
  bool getFileContent(const QString &p_path, QString &p_content, QString &p_sha, QString &p_msg);

  /**
   * Get directory contents from the repository
   * @param p_path Directory path in the repository
   * @param p_files Output list of files
   * @param p_dirs Output list of directories
   * @return true if successful
   */
  bool getDirectoryContents(const QString &p_path, QStringList &p_files, QStringList &p_dirs, QJsonArray &p_shas, QString &p_msg);

  /**
   * Get SHA of a file from the repository
   * @param p_path File path in the repository
   * @param p_sha Output file SHA
   * @return true if successful
   */
  bool getFileSha(const QString &p_path, QString &p_sha, QString &p_msg);

  /**
   * Delete a file from the repository
   * @param p_path File path in the repository
   * @param p_message Commit message
   * @param p_sha File SHA
   * @return true if successful
   */
  bool deleteFile(const QString &p_path, const QString &p_message, const QString &p_sha, QString &p_msg);

signals:
  void requestFinished();

private:
  QNetworkReply *sendRequest(const QString &p_url, const QByteArray &p_method, const QByteArray &p_data = QByteArray());

  bool checkReply(QNetworkReply *p_reply, QJsonObject &p_jobj, QString &p_msg);
  bool checkReply(QIODevice &p_data, QJsonObject &p_jobj, QString &p_msg);

  static const QString c_apiUrl;

  QString m_token;

  QString m_owner;

  QString m_repo;

  QString m_branch;

  QNetworkAccessManager *m_networkMgr = nullptr;
};
} // namespace vnotex

#endif // GITEEAPI_H
