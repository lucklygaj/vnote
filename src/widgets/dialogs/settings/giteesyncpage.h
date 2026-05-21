#ifndef GITEESYNCPAGE_H
#define GITEESYNCPAGE_H

#include "settingspage.h"

class QLineEdit;
class QCheckBox;
class QComboBox;
class QSpinBox;

namespace vnotex {
class GiteeSyncPage : public SettingsPage {
  Q_OBJECT
public:
  explicit GiteeSyncPage(QWidget *p_parent = nullptr);

  QString title() const Q_DECL_OVERRIDE;

protected:
  void loadInternal() Q_DECL_OVERRIDE;

  bool saveInternal() Q_DECL_OVERRIDE;

private:
  void setupUI();

  QLineEdit *m_tokenLineEdit = nullptr;

  QLineEdit *m_ownerLineEdit = nullptr;

  QLineEdit *m_repoLineEdit = nullptr;

  QLineEdit *m_branchLineEdit = nullptr;

  QCheckBox *m_syncEnabledCheckBox = nullptr;

  // Smart sync settings
  QSpinBox *m_batchWindowSpinBox = nullptr;
  QComboBox *m_conflictStrategyComboBox = nullptr;
  QSpinBox *m_maxRetrySpinBox = nullptr;
  QSpinBox *m_retryIntervalSpinBox = nullptr;
  QCheckBox *m_offlineRecoveryCheckBox = nullptr;
};
} // namespace vnotex

#endif // GITEESYNCPAGE_H
