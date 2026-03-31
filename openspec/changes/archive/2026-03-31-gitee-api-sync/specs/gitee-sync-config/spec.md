## ADDED Requirements

### Requirement: Gitee Token 配置
系统 SHALL 提供 Gitee 个人访问令牌配置项。用户可在配置中设置和修改 Gitee token，用于 Gitee API 认证。

#### Scenario: 配置 Gitee token
- **WHEN** 用户在配置中设置 gitee_token 值
- **THEN** 系统保存该值到配置文件
- **AND** GiteeSyncService 使用该 token 进行 API 认证

#### Scenario: 修改 Gitee token
- **WHEN** 用户修改 gitee_token 值
- **THEN** 系统保存新值到配置文件
- **AND** 后续 Gitee API 请求使用新 token

### Requirement: Gitee 仓库信息配置
系统 SHALL 提供 Gitee 仓库信息配置项，包括仓库所有者、仓库名称和分支名称。这些配置项用于定位 Gitee 仓库。

#### Scenario: 配置仓库所有者
- **WHEN** 用户在配置中设置 gitee_owner 值（用户名或组织名）
- **THEN** 系统保存该值到配置文件
- **AND** Gitee API 请求使用该值构造仓库路径

#### Scenario: 配置仓库名称
- **WHEN** 用户在配置中设置 gitee_repo 值
- **THEN** 系统保存该值到配置文件
- **AND** Gitee API 请求使用该值构造仓库路径

#### Scenario: 配置分支名称
- **WHEN** 用户在配置中设置 gitee_branch 值（默认 "master"）
- **THEN** 系统保存该值到配置文件
- **AND** Gitee API 请求使用该值指定目标分支

### Requirement: 同步开关配置
系统 SHALL 提供全局同步开关配置项。用户可通过该配置项启用或禁用 Gitee 同步功能。默认值为 false（关闭）。

#### Scenario: 启用同步功能
- **WHEN** 用户将 gitee_sync_enabled 设置为 true
- **THEN** 系统保存该值到配置文件
- **AND** GiteeSyncService 开始响应文件保存和打开操作
- **AND** 执行拉取和推送操作

#### Scenario: 禁用同步功能
- **WHEN** 用户将 gitee_sync_enabled 设置为 false
- **THEN** 系统保存该值到配置文件
- **AND** GiteeSyncService 停止响应文件操作
- **AND** 不执行拉取和推送操作

#### Scenario: 默认配置为关闭
- **WHEN** 用户首次使用 VNote
- **THEN** gitee_sync_enabled 默认值为 false
- **AND** 不执行任何 Gitee 同步操作

### Requirement: 推送倒计时配置
系统 SHALL 提供推送倒计时秒数配置项。用户可通过该配置项调整合并窗口的倒计时时间，默认值为 5 秒，可配置范围为 1-30 秒。

#### Scenario: 配置推送倒计时秒数
- **WHEN** 用户在配置中设置 gitee_push_delay_seconds 值
- **THEN** 系统保存该值到配置文件
- **AND** PushQueue 使用该值作为倒计时时长
- **AND** 配置变更立即生效

#### Scenario: 使用默认倒计时秒数
- **WHEN** 用户未配置 gitee_push_delay_seconds
- **THEN** 系统使用默认值 5 秒
- **AND** PushQueue 倒计时为 5 秒

#### Scenario: 配置超出范围时限制
- **WHEN** 用户设置 gitee_push_delay_seconds 小于 1 秒或大于 30 秒
- **THEN** 系统将值限制在 1-30 范围内
- **AND** 弹窗提示用户配置已调整

### Requirement: 配置持久化
系统 SHALL 将所有 Gitee 同步配置项持久化存储到配置文件。应用关闭后配置不丢失，下次启动时自动加载。

#### Scenario: 保存配置到文件
- **WHEN** 用户修改任意 Gitee 同步配置项
- **THEN** 系统将所有配置项写入配置文件
- **AND** 文件格式为 JSON

#### Scenario: 应用启动时加载配置
- **WHEN** 用户启动 VNote 应用
- **THEN** 系统从配置文件读取所有 Gitee 同步配置项
- **AND** GiteeSyncService 使用加载的配置初始化

### Requirement: 配置验证
系统 SHALL 在启用 Gitee 同步时验证配置项的完整性和有效性。必要配置项缺失或无效时，系统必须提示用户并禁用同步功能。

#### Scenario: 配置完整验证通过
- **WHEN** 用户启用 Gitee 同步且配置了所有必要配置项（token、owner、repo）
- **THEN** 系统验证通过
- **AND** 启用同步功能

#### Scenario: 配置缺失时提示用户
- **WHEN** 用户启用 Gitee 同步但缺少必要配置项
- **THEN** 系统禁用同步功能
- **AND** 提示用户填写完整配置

#### Scenario: 配置无效时提示用户
- **WHEN** 用户配置的 Gitee token 无效或仓库不存在
- **THEN** 系统记录错误日志
- **AND** 提示用户检查配置
- **AND** 禁用同步功能

### Requirement: 配置默认值
系统 SHALL 为所有 Gitee 同步配置项提供合理的默认值。用户首次使用时可直接启用同步功能（补充必要配置后），无需手动配置所有选项。

#### Scenario: 默认值列表
- **WHEN** 用户首次查看 Gitee 同步配置
- **THEN** 配置项使用以下默认值：
  - gitee_token: 空（用户必须填写）
  - gitee_owner: 空（用户必须填写）
  - gitee_repo: 空（用户必须填写）
  - gitee_branch: "master"
  - gitee_sync_enabled: false
  - gitee_push_delay_seconds: 5

#### Scenario: 用户修改配置项
- **WHEN** 用户修改某个配置项
- **THEN** 其他配置项保持不变
- **AND** 未配置的配置项使用默认值

### Requirement: 配置变更通知
系统 SHALL 在配置项变更时通知 GiteeSyncService，确保服务使用最新配置。配置变更必须立即生效，无需重启应用。

#### Scenario: 启用同步开关立即生效
- **WHEN** 用户在设置界面将 gitee_sync_enabled 从 false 改为 true
- **THEN** GiteeSyncService 立即开始响应文件操作
- **AND** 无需重启应用

#### Scenario: 修改 token 立即生效
- **WHEN** 用户修改 gitee_token 值
- **THEN** GiteeSyncService 立即使用新 token
- **AND** 后续 API 请求使用新 token

#### Scenario: 修改倒计时秒数立即生效
- **WHEN** 用户修改 gitee_push_delay_seconds 值
- **THEN** PushQueue 立即使用新倒计时时长
- **AND** 下次推送使用新值
- **AND** 修改正在进行的倒计时也会生效（重置倒计时）
