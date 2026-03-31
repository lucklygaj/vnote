#ifndef SHACACHE_H
#define SHACACHE_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace vnotex {

class ShaCache : public QObject {
  Q_OBJECT
public:
  explicit ShaCache(QObject *p_parent = nullptr);

  ~ShaCache();

  /**
   * Get SHA for a file path
   * @param p_path File path
   * @return SHA if cached, empty string otherwise
   */
  QString getSha(const QString &p_path) const;

  /**
   * Update SHA for a file path
   * @param p_path File path
   * @param p_sha SHA value
   */
  void updateSha(const QString &p_path, const QString &p_sha);

  /**
   * Batch update SHA cache
   * @param p_shas JSON array of {name, sha} objects
   * @param p_prefix Path prefix for the cached items
   */
  void batchUpdateSha(const QJsonArray &p_shas, const QString &p_prefix);

  /**
   * Clear all cached SHAs
   */
  void clear();

  /**
   * Check if a file path is cached
   * @param p_path File path
   * @return true if cached
   */
  bool hasSha(const QString &p_path) const;

  /**
   * Get count of cached items
   */
  int count() const;

  /**
   * Remove SHA for a file path
   * @param p_path File path
   */
  void removeSha(const QString &p_path);

private:
  QMap<QString, QString> m_cache;
};
} // namespace vnotex

#endif // SHACACHE_H
