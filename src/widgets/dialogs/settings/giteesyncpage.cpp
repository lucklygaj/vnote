#include "giteesyncpage.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QFormLayout>

#include <core/configmgr.h>
#include <sync/giteesyncconfig.h>
#include <sync/giteesyncservice.h>
#include <sync/smartsyncscheduler.h>
#include <core/logger.h>
#include <widgets/widgetsfactory.h>

using namespace vnotex;

GiteeSyncPage::GiteeSyncPage(QWidget *p_parent) : SettingsPage(p_parent) { setupUI(); }

void GiteeSyncPage::setupUI() {
  auto *mainLayout = WidgetsFactory::createFormLayout(this);

  // Add help information at the top
  {
    auto *helpLabel = new QLabel(
      tr("Configure Gitee Sync to automatically backup your notes to Gitee repository.\n\n"
         "Configuration steps:\n"
         "1. Create a Gitee repository for your notes\n"
         "2. Generate a Personal Access Token: Gitee -> Settings -> Personal Access Tokens\n"
         "   (Select 'repo' permission scope)\n"
         "3. Fill in the configuration fields below"),
      this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");
    mainLayout->addRow(QString(), helpLabel);
  }

  {
    const QString label(tr("Enable Gitee Sync"));
    m_syncEnabledCheckBox = WidgetsFactory::createCheckBox(label, this);
    m_syncEnabledCheckBox->setToolTip(tr("Enable automatic sync to Gitee repository"));
    mainLayout->addRow(m_syncEnabledCheckBox);
    addSearchItem(label, m_syncEnabledCheckBox->toolTip(), m_syncEnabledCheckBox);
    connect(m_syncEnabledCheckBox, &QCheckBox::stateChanged, this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_tokenLineEdit = WidgetsFactory::createLineEdit(this);
    m_tokenLineEdit->setToolTip(tr("Personal access token from Gitee. Generate at: Gitee -> Settings -> Personal Access Tokens (requires 'repo' permission)"));
    m_tokenLineEdit->setEchoMode(QLineEdit::Password);
    m_tokenLineEdit->setPlaceholderText(tr("a1b2c3d4e5f6g7h8i9j0"));

    const QString label(tr("Token:"));
    mainLayout->addRow(label, m_tokenLineEdit);
    addSearchItem(label, m_tokenLineEdit->toolTip(), m_tokenLineEdit);
    connect(m_tokenLineEdit, &QLineEdit::textChanged, this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_ownerLineEdit = WidgetsFactory::createLineEdit(this);
    m_ownerLineEdit->setToolTip(tr("Your Gitee username or organization name"));
    m_ownerLineEdit->setPlaceholderText(tr("e.g., john_doe or my-organization"));

    const QString label(tr("Owner:"));
    mainLayout->addRow(label, m_ownerLineEdit);
    addSearchItem(label, m_ownerLineEdit->toolTip(), m_ownerLineEdit);
    connect(m_ownerLineEdit, &QLineEdit::textChanged, this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_repoLineEdit = WidgetsFactory::createLineEdit(this);
    m_repoLineEdit->setToolTip(tr("Gitee repository name (without owner prefix)"));
    m_repoLineEdit->setPlaceholderText(tr("e.g., vnote-backup or my-notes"));

    const QString label(tr("Repository:"));
    mainLayout->addRow(label, m_repoLineEdit);
    addSearchItem(label, m_repoLineEdit->toolTip(), m_repoLineEdit);
    connect(m_repoLineEdit, &QLineEdit::textChanged, this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_branchLineEdit = WidgetsFactory::createLineEdit(this);
    m_branchLineEdit->setToolTip(tr("Target branch name in the repository"));
    m_branchLineEdit->setPlaceholderText(tr("e.g., master or main"));

    const QString label(tr("Branch:"));
    mainLayout->addRow(label, m_branchLineEdit);
    addSearchItem(label, m_branchLineEdit->toolTip(), m_branchLineEdit);
    connect(m_branchLineEdit, &QLineEdit::textChanged, this, &GiteeSyncPage::pageIsChanged);
  }

  // Smart Sync Settings section
  {
    auto *separator = new QLabel(tr("--- Smart Sync Settings ---"), this);
    separator->setStyleSheet("QLabel { font-weight: bold; margin-top: 10px; }");
    mainLayout->addRow(QString(), separator);
  }

  {
    m_batchWindowSpinBox = WidgetsFactory::createSpinBox(this);
    m_batchWindowSpinBox->setToolTip(tr("Maximum time (ms) to wait for more changes before syncing (1000-10000ms)"));
    m_batchWindowSpinBox->setRange(1000, 10000);
    m_batchWindowSpinBox->setSingleStep(500);
    m_batchWindowSpinBox->setSuffix(" ms");

    const QString label(tr("Batch window:"));
    mainLayout->addRow(label, m_batchWindowSpinBox);
    addSearchItem(label, m_batchWindowSpinBox->toolTip(), m_batchWindowSpinBox);
    connect(m_batchWindowSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_conflictStrategyComboBox = WidgetsFactory::createComboBox(this);
    m_conflictStrategyComboBox->setToolTip(tr("Strategy to use when a file conflict is detected during sync"));
    m_conflictStrategyComboBox->addItem(tr("Ask user"), static_cast<int>(GiteeSyncConfig::ConflictStrategy::AskUser));
    m_conflictStrategyComboBox->addItem(tr("Use local version"), static_cast<int>(GiteeSyncConfig::ConflictStrategy::UseLocal));
    m_conflictStrategyComboBox->addItem(tr("Use remote version"), static_cast<int>(GiteeSyncConfig::ConflictStrategy::UseRemote));

    const QString label(tr("Conflict strategy:"));
    mainLayout->addRow(label, m_conflictStrategyComboBox);
    addSearchItem(label, m_conflictStrategyComboBox->toolTip(), m_conflictStrategyComboBox);
    connect(m_conflictStrategyComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_maxRetrySpinBox = WidgetsFactory::createSpinBox(this);
    m_maxRetrySpinBox->setToolTip(tr("Maximum number of retry attempts for failed sync operations"));
    m_maxRetrySpinBox->setRange(0, 20);

    const QString label(tr("Max retries:"));
    mainLayout->addRow(label, m_maxRetrySpinBox);
    addSearchItem(label, m_maxRetrySpinBox->toolTip(), m_maxRetrySpinBox);
    connect(m_maxRetrySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GiteeSyncPage::pageIsChanged);
  }

  {
    m_retryIntervalSpinBox = WidgetsFactory::createSpinBox(this);
    m_retryIntervalSpinBox->setToolTip(tr("Base interval (ms) between retry attempts (doubles each retry)"));
    m_retryIntervalSpinBox->setRange(100, 60000);
    m_retryIntervalSpinBox->setSingleStep(500);
    m_retryIntervalSpinBox->setSuffix(" ms");

    const QString label(tr("Retry interval:"));
    mainLayout->addRow(label, m_retryIntervalSpinBox);
    addSearchItem(label, m_retryIntervalSpinBox->toolTip(), m_retryIntervalSpinBox);
    connect(m_retryIntervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &GiteeSyncPage::pageIsChanged);
  }

  {
    const QString label(tr("Offline recovery"));
    m_offlineRecoveryCheckBox = WidgetsFactory::createCheckBox(label, this);
    m_offlineRecoveryCheckBox->setToolTip(tr("Automatically save pending syncs when offline and retry when network recovers"));

    mainLayout->addRow(m_offlineRecoveryCheckBox);
    addSearchItem(label, m_offlineRecoveryCheckBox->toolTip(), m_offlineRecoveryCheckBox);
    connect(m_offlineRecoveryCheckBox, &QCheckBox::stateChanged, this, &GiteeSyncPage::pageIsChanged);
  }
}

void GiteeSyncPage::loadInternal() {
  const auto &config = ConfigMgr::getInst().getGiteeSyncConfig();

  m_syncEnabledCheckBox->setChecked(config.isSyncEnabled());
  m_tokenLineEdit->setText(config.getToken());
  m_ownerLineEdit->setText(config.getOwner());
  m_repoLineEdit->setText(config.getRepo());
  m_branchLineEdit->setText(config.getBranch());

  // Smart sync settings
  m_batchWindowSpinBox->setValue(config.getBatchWindowMs());
  int strategyIdx = m_conflictStrategyComboBox->findData(static_cast<int>(config.getConflictStrategy()));
  m_conflictStrategyComboBox->setCurrentIndex(strategyIdx >= 0 ? strategyIdx : 0);
  m_maxRetrySpinBox->setValue(config.getMaxRetryCount());
  m_retryIntervalSpinBox->setValue(config.getRetryIntervalBase());
  m_offlineRecoveryCheckBox->setChecked(config.isOfflineRecoveryEnabled());
}

bool GiteeSyncPage::saveInternal() {
  qInfo() << "[GiteeSyncPage] Saving Gitee sync configuration";

  auto &config = ConfigMgr::getInst().getGiteeSyncConfig();

  config.setSyncEnabled(m_syncEnabledCheckBox->isChecked());
  config.setToken(m_tokenLineEdit->text());
  config.setOwner(m_ownerLineEdit->text());
  config.setRepo(m_repoLineEdit->text());
  config.setBranch(m_branchLineEdit->text());

  // Smart sync settings
  config.setBatchWindowMs(m_batchWindowSpinBox->value());
  int strategyData = m_conflictStrategyComboBox->currentData().toInt();
  config.setConflictStrategy(static_cast<GiteeSyncConfig::ConflictStrategy>(strategyData));
  config.setMaxRetryCount(m_maxRetrySpinBox->value());
  config.setRetryIntervalBase(m_retryIntervalSpinBox->value());
  config.setOfflineRecoveryEnabled(m_offlineRecoveryCheckBox->isChecked());

  qInfo() << "[GiteeSyncPage] Config saved - Enabled:" << m_syncEnabledCheckBox->isChecked()
          << "Owner:" << m_ownerLineEdit->text() << "Repo:" << m_repoLineEdit->text()
          << "Branch:" << m_branchLineEdit->text()
          << "BatchWindow:" << m_batchWindowSpinBox->value()
          << "MaxRetry:" << m_maxRetrySpinBox->value();

  // Update GiteeSyncService runtime config after saving
  GiteeSyncService::getInst().updateConfig();

  // Update SmartSyncScheduler batch window
  SmartSyncScheduler::getInst().setBatchWindowMs(m_batchWindowSpinBox->value());

  QString errorMsg;
  if (!config.validateConfig(errorMsg)) {
    qCritical() << "[GiteeSyncPage] Config validation failed:" << errorMsg;
    setError(errorMsg);
    return false;
  }

  qInfo() << "[GiteeSyncPage] Configuration saved and validated successfully";
  return true;
}

QString GiteeSyncPage::title() const { return tr("Gitee Sync"); }
