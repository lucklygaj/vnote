#ifndef SYNCSTATUSINDICATOR_H
#define SYNCSTATUSINDICATOR_H

#include <QLabel>
#include <QMenu>

namespace vnotex {

class SyncStatusIndicator : public QLabel {
  Q_OBJECT
public:
  explicit SyncStatusIndicator(QWidget *p_parent = nullptr);

signals:
  void syncCenterRequested();

protected:
  void mousePressEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;

private slots:
  void updateDisplay();
  void onStatisticsChanged(int p_synced, int p_pending, int p_syncing, int p_failed, int p_paused);
  void onStateChanged(const QString &p_path);

private:
  void setupMenu();

  int m_pendingCount = 0;
  int m_syncingCount = 0;
  int m_failedCount = 0;
  int m_syncedCount = 0;
  int m_pausedCount = 0;

  qint64 m_lastSyncTime = 0;

  QMenu *m_menu = nullptr;
};

} // namespace vnotex

#endif // SYNCSTATUSINDICATOR_H
