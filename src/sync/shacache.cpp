#include "shacache.h"

using namespace vnotex;

ShaCache::ShaCache(QObject *p_parent) : QObject(p_parent) {
}

ShaCache::~ShaCache() {
  clear();
}

QString ShaCache::getSha(const QString &p_path) const {
  return m_cache.value(p_path);
}

void ShaCache::updateSha(const QString &p_path, const QString &p_sha) {
  if (p_path.isEmpty()) {
    return;
  }
  m_cache[p_path] = p_sha;
}

void ShaCache::batchUpdateSha(const QJsonArray &p_shas, const QString &p_prefix) {
  for (const auto &item : p_shas) {
    QJsonObject itemObj = item.toObject();
    QString name = itemObj.value(QStringLiteral("name")).toString();
    QString sha = itemObj.value(QStringLiteral("sha")).toString();

    if (!name.isEmpty() && !sha.isEmpty()) {
      QString fullPath = p_prefix.isEmpty() ? name : (p_prefix + "/" + name);
      m_cache[fullPath] = sha;
    }
  }
}

void ShaCache::clear() {
  m_cache.clear();
}

bool ShaCache::hasSha(const QString &p_path) const {
  return m_cache.contains(p_path);
}

int ShaCache::count() const {
  return m_cache.count();
}

void ShaCache::removeSha(const QString &p_path) {
  m_cache.remove(p_path);
}
