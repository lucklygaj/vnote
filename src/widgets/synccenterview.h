#ifndef SYNCCENTERVIEW_H
#define SYNCCENTERVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>

namespace vnotex {

class SyncCenterView : public QWidget {
  Q_OBJECT
public:
  explicit SyncCenterView(QWidget *p_parent = nullptr);

signals:
  void closeRequested();

private slots:
  void refreshData();
  void onStatisticsChanged(int p_synced, int p_pending, int p_syncing, int p_failed, int p_paused);
  void onFileStateChanged(const QString &p_path);
  void retryFile(int p_row);
  void clearFailure(int p_row);
  void syncNow();

private:
  void setupUI();
  void updateStatisticsDisplay();
  void updateFileList();
  void updateFailureList();

  QLabel *m_syncedLabel = nullptr;
  QLabel *m_pendingLabel = nullptr;
  QLabel *m_syncingLabel = nullptr;
  QLabel *m_failedLabel = nullptr;

  QTableWidget *m_fileListTable = nullptr;
  QTableWidget *m_failureListTable = nullptr;

  QPushButton *m_syncNowButton = nullptr;
  QPushButton *m_refreshButton = nullptr;
};

} // namespace vnotex

#endif // SYNCCENTERVIEW_H
