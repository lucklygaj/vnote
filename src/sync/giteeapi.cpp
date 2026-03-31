#include "giteeapi.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QBuffer>

#include <vtextedit/networkutils.h>

#include <utils/utils.h>
#include <core/logger.h>

using namespace vnotex;

const QString GiteeApi::c_apiUrl = QStringLiteral("https://gitee.com/api/v5");

GiteeApi::GiteeApi(QObject *p_parent) : QObject(p_parent) {
  m_networkMgr = new QNetworkAccessManager(this);
}

GiteeApi::~GiteeApi() {
}

void GiteeApi::setToken(const QString &p_token) {
  m_token = p_token;
  qInfo() << "[GiteeApi] Token set (length:" << p_token.length() << ")";
}

void GiteeApi::setOwner(const QString &p_owner) {
  m_owner = p_owner;
  qInfo() << "[GiteeApi] Owner set:" << p_owner;
}

void GiteeApi::setRepo(const QString &p_repo) {
  m_repo = p_repo;
  qInfo() << "[GiteeApi] Repository set:" << p_repo;
}

void GiteeApi::setBranch(const QString &p_branch) {
  m_branch = p_branch;
  qInfo() << "[GiteeApi] Branch set:" << p_branch;
}

QNetworkReply *GiteeApi::sendRequest(const QString &p_url, const QByteArray &p_method, const QByteArray &p_data) {
  QUrl url(p_url);

  QNetworkRequest request(url);
  request.setRawHeader(QByteArray("Authorization"), QByteArray("Bearer ") + m_token.toUtf8());
  request.setRawHeader(QByteArray("Content-Type"), QByteArray("application/json;charset=UTF-8"));

  qInfo() << "[GiteeApi] Sending" << p_method << "request to:" << p_url;

  return m_networkMgr->sendCustomRequest(request, p_method, p_data);
}

bool GiteeApi::checkReply(QNetworkReply *p_reply, QJsonObject &p_jobj, QString &p_msg) {
  auto error = p_reply->error();
  if (error != QNetworkReply::NoError) {
    int statusCode = p_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = p_reply->readAll();

    qWarning() << "[GiteeApi] Request failed - Status:" << statusCode << "Error:" << p_reply->errorString();
    qWarning() << "[GiteeApi] Response data:" << QString::fromUtf8(data);

    if (statusCode == 401) {
      p_msg = tr("Gitee API authentication failed (401). Please check your token.");
    } else if (statusCode == 403) {
      p_msg = tr("Gitee API access forbidden (403). Please check your permissions.");
    } else if (statusCode == 409) {
      p_msg = tr("Gitee API SHA conflict (409).");
    } else if (statusCode == 429) {
      p_msg = tr("Gitee API rate limit exceeded (429). Please try again later.");
    } else {
      p_msg = tr("Gitee API error (%1): %2").arg(statusCode).arg(QString::fromUtf8(data));
    }

    return false;
  }

  QByteArray data = p_reply->readAll();
  p_jobj = Utils::fromJsonString(data);
  if (p_jobj.isEmpty()) {
    int statusCode = p_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qWarning() << "[GiteeApi] Invalid response - Empty or malformed JSON";
    qWarning() << "[GiteeApi] HTTP Status:" << statusCode;
    qWarning() << "[GiteeApi] Response body:" << QString::fromUtf8(data);
    qWarning() << "[GiteeApi] Raw response (hex):" << data.toHex();
    p_msg = tr("Invalid response from Gitee API");
    return false;
  }

  qInfo() << "[GiteeApi] Request successful - Status:" << p_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  return true;
}

bool GiteeApi::checkReply(QIODevice &p_data, QJsonObject &p_jobj, QString &p_msg) {
  QByteArray data = p_data.readAll();
  p_jobj = Utils::fromJsonString(data);
  if (p_jobj.isEmpty()) {
    qWarning() << "[GiteeApi] Invalid JSON object from data";
    qWarning() << "[GiteeApi] Response body:" << QString::fromUtf8(data);
    qWarning() << "[GiteeApi] Raw response (hex):" << data.toHex();
    p_msg = tr("Invalid JSON data");
    return false;
  }

  return true;
}

QString GiteeApi::createFile(const QString &p_path, const QString &p_content, const QString &p_message, QString &p_msg) {
  qInfo() << "[GiteeApi] Creating file:" << p_path << "Commit message:" << p_message;

  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    qCritical() << "[GiteeApi] Configuration incomplete - Token:" << m_token.isEmpty() << "Owner:" << m_owner.isEmpty() << "Repo:" << m_repo.isEmpty();
    p_msg = tr("Gitee API configuration incomplete");
    return QString();
  }

  QJsonObject requestDataObj;
  requestDataObj[QStringLiteral("message")] = p_message;
  requestDataObj[QStringLiteral("content")] = QString::fromUtf8(p_content.toUtf8().toBase64());
  requestDataObj[QStringLiteral("branch")] = m_branch;

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);

  QNetworkReply *reply = sendRequest(urlStr, "POST", Utils::toJsonString(requestDataObj));

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QJsonObject replyObj;
  if (!checkReply(reply, replyObj, p_msg)) {
    reply->deleteLater();
    qCritical() << "[GiteeApi] Failed to create file:" << p_path << "Error:" << p_msg;
    return QString();
  }

  reply->deleteLater();

  // Gitee API returns sha in content object
  QString sha = replyObj.value(QStringLiteral("content")).toObject().value(QStringLiteral("sha")).toString();

  if (sha.isEmpty()) {
    qCritical() << "[GiteeApi] Failed to get SHA from create response";
    p_msg = tr("Failed to get SHA from create response");
    return QString();
  }

  qInfo() << "[GiteeApi] File created successfully:" << p_path << "SHA:" << sha;
  emit requestFinished();
  return sha;
}

QString GiteeApi::updateFile(const QString &p_path, const QString &p_content, const QString &p_message, const QString &p_sha, QString &p_msg) {
  qInfo() << "[GiteeApi] Updating file:" << p_path << "SHA:" << p_sha << "Commit message:" << p_message;

  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    qCritical() << "[GiteeApi] Configuration incomplete - Token:" << m_token.isEmpty() << "Owner:" << m_owner.isEmpty() << "Repo:" << m_repo.isEmpty();
    p_msg = tr("Gitee API configuration incomplete");
    return QString();
  }

  QJsonObject requestDataObj;
  requestDataObj[QStringLiteral("message")] = p_message;
  requestDataObj[QStringLiteral("content")] = QString::fromUtf8(p_content.toUtf8().toBase64());
  requestDataObj[QStringLiteral("sha")] = p_sha;
  requestDataObj[QStringLiteral("branch")] = m_branch;

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);

  QNetworkReply *reply = sendRequest(urlStr, "PUT", Utils::toJsonString(requestDataObj));

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QJsonObject replyObj;
  if (!checkReply(reply, replyObj, p_msg)) {
    reply->deleteLater();
    qCritical() << "[GiteeApi] Failed to update file:" << p_path << "Error:" << p_msg;
    return QString();
  }

  reply->deleteLater();

  // Gitee API returns sha in content object
  QString sha = replyObj.value(QStringLiteral("content")).toObject().value(QStringLiteral("sha")).toString();

  if (sha.isEmpty()) {
    qCritical() << "[GiteeApi] Failed to get SHA from update response";
    p_msg = tr("Failed to get SHA from update response");
    return QString();
  }

  qInfo() << "[GiteeApi] File updated successfully:" << p_path << "New SHA:" << sha;
  emit requestFinished();
  return sha;
}

bool GiteeApi::getFileContent(const QString &p_path, QString &p_content, QString &p_sha, QString &p_msg) {
  qInfo() << "[GiteeApi] Getting file content:" << p_path << "Branch:" << m_branch;

  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    qCritical() << "[GiteeApi] Configuration incomplete - Token:" << m_token.isEmpty() << "Owner:" << m_owner.isEmpty() << "Repo:" << m_repo.isEmpty();
    p_msg = tr("Gitee API configuration incomplete");
    return false;
  }

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);
  urlStr += QStringLiteral("?ref=") + m_branch;

  QNetworkReply *reply = sendRequest(urlStr, "GET");

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QJsonObject replyObj;
  if (!checkReply(reply, replyObj, p_msg)) {
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 404) {
      qInfo() << "[GiteeApi] File not found (404):" << p_path;
      p_msg.clear();
      reply->deleteLater();
      return false;
    }
    reply->deleteLater();
    return false;
  }

  reply->deleteLater();

  qInfo() << "[GiteeApi] Response keys:" << replyObj.keys();
  qInfo() << "[GiteeApi] Full response JSON:" << Utils::toJsonString(replyObj);

  // Gitee API returns the file info directly in the response object
  // Not wrapped in a "content" field like GitHub API
  p_sha = replyObj.value(QStringLiteral("sha")).toString();
  qInfo() << "[GiteeApi] SHA from response:" << p_sha;

  QString contentBase64 = replyObj.value(QStringLiteral("content")).toString();
  qInfo() << "[GiteeApi] Content base64 (first 100 chars):" << contentBase64.left(100);

  if (contentBase64.isEmpty()) {
    qInfo() << "[GiteeApi] File content is empty (possibly empty file):" << p_path;
    p_content.clear();
  } else {
    p_content = QString::fromUtf8(QByteArray::fromBase64(contentBase64.toUtf8()));
    qInfo() << "[GiteeApi] File content retrieved successfully:" << p_path << "SHA:" << p_sha << "Size:" << p_content.length();
  }

  emit requestFinished();
  return true;
}

bool GiteeApi::getDirectoryContents(const QString &p_path, QStringList &p_files, QStringList &p_dirs, QJsonArray &p_shas, QString &p_msg) {
  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    p_msg = tr("Gitee API configuration incomplete");
    return false;
  }

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);
  urlStr += QStringLiteral("?ref=") + m_branch;

  QNetworkReply *reply = sendRequest(urlStr, "GET");

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QJsonArray replyArr;
  {
    QJsonObject replyObj;
    if (!checkReply(reply, replyObj, p_msg)) {
      int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      if (statusCode == 404) {
        p_msg.clear();
        reply->deleteLater();
        return false;
      }
      reply->deleteLater();
      return false;
    }

    qInfo() << "[GiteeApi] Directory response keys:" << replyObj.keys();

    // Gitee API returns array directly for directory contents
    if (replyObj.value(QStringLiteral("type")).toString() == QStringLiteral("file")) {
      // This is actually a file, not a directory
      qWarning() << "[GiteeApi] Expected directory but got file:" << p_path;
      reply->deleteLater();
      return false;
    }

    // Try to get array from response (Gitee API may return array directly or in content field)
    replyArr = replyObj.value(QStringLiteral("content")).toArray();
    if (replyArr.isEmpty()) {
      // Try to parse the response as array directly
      QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
      if (doc.isArray()) {
        replyArr = doc.array();
      }
    }
  }

  reply->deleteLater();

  p_files.clear();
  p_dirs.clear();
  p_shas = QJsonArray();

  for (const auto &item : replyArr) {
    QJsonObject itemObj = item.toObject();
    QString type = itemObj.value(QStringLiteral("type")).toString();
    QString name = itemObj.value(QStringLiteral("name")).toString();
    QString sha = itemObj.value(QStringLiteral("sha")).toString();

    if (type == QStringLiteral("file")) {
      p_files.append(name);
    } else if (type == QStringLiteral("dir")) {
      p_dirs.append(name);
    }

    QJsonObject shaObj;
    shaObj[QStringLiteral("name")] = name;
    shaObj[QStringLiteral("sha")] = sha;
    p_shas.append(shaObj);
  }

  emit requestFinished();
  return true;
}

bool GiteeApi::getFileSha(const QString &p_path, QString &p_sha, QString &p_msg) {
  qInfo() << "[GiteeApi] Getting file SHA:" << p_path << "Branch:" << m_branch;

  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    qCritical() << "[GiteeApi] Configuration incomplete - Token:" << m_token.isEmpty() << "Owner:" << m_owner.isEmpty() << "Repo:" << m_repo.isEmpty();
    p_msg = tr("Gitee API configuration incomplete");
    return false;
  }

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);
  urlStr += QStringLiteral("?ref=") + m_branch;

  QNetworkReply *reply = sendRequest(urlStr, "GET");

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  // Check if response is an array (file doesn't exist or path is a directory)
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isArray()) {
    QJsonArray arr = doc.array();
    qInfo() << "[GiteeApi] File not found (API returned array with" << arr.size() << "items):" << p_path;
    p_msg.clear();
    reply->deleteLater();
    return false;
  }

  QBuffer buffer(&data);
  buffer.open(QIODevice::ReadOnly);
  QJsonObject replyObj;
  if (!checkReply(buffer, replyObj, p_msg)) {
    // Since we already checked for 404 when calling checkReply with QNetworkReply,
    // here we handle HTTP errors separately
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200) {
      if (statusCode == 404) {
        qInfo() << "[GiteeApi] File not found (404) when getting SHA:" << p_path;
        p_msg.clear();
        reply->deleteLater();
        return false;
      }
      qWarning() << "[GiteeApi] HTTP error:" << statusCode;
      reply->deleteLater();
      return false;
    }
    reply->deleteLater();
    return false;
  }

  reply->deleteLater();

  qInfo() << "[GiteeApi] Response keys:" << replyObj.keys();

  // Gitee API returns the file info directly in the response object
  p_sha = replyObj.value(QStringLiteral("sha")).toString();

  if (p_sha.isEmpty()) {
    qCritical() << "[GiteeApi] Failed to get file SHA from response";
    p_msg = tr("Failed to get file SHA from Gitee API");
    return false;
  }

  qInfo() << "[GiteeApi] File SHA retrieved successfully:" << p_path << "SHA:" << p_sha;
  emit requestFinished();
  return true;
}

bool GiteeApi::deleteFile(const QString &p_path, const QString &p_message, const QString &p_sha, QString &p_msg) {
  qInfo() << "[GiteeApi] Deleting file:" << p_path << "SHA:" << p_sha << "Commit message:" << p_message;

  if (m_token.isEmpty() || m_owner.isEmpty() || m_repo.isEmpty()) {
    qCritical() << "[GiteeApi] Configuration incomplete - Token:" << m_token.isEmpty() << "Owner:" << m_owner.isEmpty() << "Repo:" << m_repo.isEmpty();
    p_msg = tr("Gitee API configuration incomplete");
    return false;
  }

  QJsonObject requestDataObj;
  requestDataObj[QStringLiteral("message")] = p_message;
  requestDataObj[QStringLiteral("sha")] = p_sha;
  requestDataObj[QStringLiteral("branch")] = m_branch;

  QString encodedPath = QUrl::toPercentEncoding(p_path, "/");
  QString urlStr = QStringLiteral("%1/repos/%2/%3/contents/%4").arg(c_apiUrl, m_owner, m_repo, encodedPath);

  QNetworkReply *reply = sendRequest(urlStr, "DELETE", Utils::toJsonString(requestDataObj));

  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  QJsonObject replyObj;
  if (!checkReply(reply, replyObj, p_msg)) {
    reply->deleteLater();
    qCritical() << "[GiteeApi] Failed to delete file:" << p_path << "Error:" << p_msg;
    return false;
  }

  reply->deleteLater();

  qInfo() << "[GiteeApi] File deleted successfully:" << p_path;
  emit requestFinished();
  return true;
}
