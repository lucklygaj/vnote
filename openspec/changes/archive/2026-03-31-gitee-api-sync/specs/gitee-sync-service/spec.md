## ADDED Requirements

### Requirement: Gitee API 文件上传
系统 SHALL 提供通过 Gitee API v5 Contents 接口上传文件到远程仓库的能力。上传时必须提供文件路径、内容、提交消息和远端当前 SHA（更新文件时）。

#### Scenario: 首次上传新文件
- **WHEN** 用户保存一个新文件到已启用 Gitee 同步的笔记本
- **THEN** 系统通过 Gitee API 创建文件，返回新的 SHA
- **AND** 新文件的 SHA 被缓存到 ShaCache

#### Scenario: 更新已存在的文件
- **WHEN** 用户修改并保存一个已存在的文件
- **THEN** 系统从 ShaCache 获取该文件的远端当前 SHA
- **AND** 通过 Gitee API 更新文件，返回新的 SHA
- **AND** 更新后的 SHA 被缓存到 ShaCache

### Requirement: Gitee API 文件下载
系统 SHALL 提供通过 Gitee API v5 Contents 接口从远程仓库下载文件内容的能力。下载成功后必须更新本地文件内容。

#### Scenario: 打开文件时下载
- **WHEN** 用户打开一个已启用 Gitee 同步的笔记本中的文件
- **THEN** 系统在 VXNodeFile::read() 前通过 Gitee API 获取该文件内容
- **AND** 远端内容被写入本地文件系统
- **AND** 文件的 SHA 被缓存到 ShaCache

#### Scenario: 下载不存在的文件
- **WHEN** 用户尝试下载一个在远程仓库中不存在的文件
- **THEN** 系统记录警告日志
- **AND** 不阻塞本地文件读取操作

### Requirement: Gitee API 目录内容获取
系统 SHALL 提供通过 Gitee API v5 Contents 接口获取目录内容的能力，包括子目录和文件列表。

#### Scenario: 打开笔记本时获取根目录
- **WHEN** 用户打开一个已启用 Gitee 同步的笔记本
- **THEN** 系统在 LocalNotebookBackend::loadRootNode() 前通过 Gitee API 获取根目录内容
- **AND** 远端目录结构被同步到本地文件系统

#### Scenario: 展开目录时获取子目录内容
- **WHEN** 用户在笔记树中展开一个目录节点
- **THEN** 系统在 ConfigMgr::loadNode() 前通过 Gitee API 获取该目录的内容
- **AND** 远端子目录和文件被同步到本地文件系统

### Requirement: SHA 缓存管理
系统 SHALL 提供内存缓存机制，存储文件路径到远端 SHA 的映射关系。缓存必须在应用启动时初始化，关闭时清空。

#### Scenario: 首次上传文件时缓存 SHA
- **WHEN** 用户首次上传一个新文件到 Gitee
- **THEN** Gitee API 返回文件的 SHA
- **AND** SHA 被缓存到 ShaCache（文件路径 → SHA）

#### Scenario: 后续上传使用缓存 SHA
- **WHEN** 用户再次上传同一个文件到 Gitee
- **THEN** 系统从 ShaCache 获取该文件的远端当前 SHA
- **AND** 不再调用 Gitee API 获取 SHA

#### Scenario: 拉取文件后更新 SHA 缓存
- **WHEN** 系统从 Gitee 拉取文件成功
- **THEN** 返回内容的 SHA 被写入 ShaCache
- **AND** 下次推送该文件时使用缓存中的最新 SHA

#### Scenario: 获取目录时批量预缓存 SHA
- **WHEN** 系统从 Gitee 获取目录内容成功
- **THEN** 所有子文件和子目录的 SHA 被批量写入 ShaCache
- **AND** 避免后续推送时逐个获取 SHA

#### Scenario: SHA 冲突时自动刷新重试
- **WHEN** 推送文件时遇到 SHA 冲突（409 错误）
- **THEN** 系统重新调用 API 获取该文件的最新 SHA
- **AND** 用新 SHA 更新缓存
- **AND** 重试推送一次
- **AND** 仍失败则记录日志并弹窗提示用户

#### Scenario: 应用关闭时清空缓存
- **WHEN** 用户关闭 VNote 应用
- **THEN** ShaCache 中的所有缓存被清空
- **AND** 下次应用启动时缓存为空

### Requirement: 合并窗口推送机制
系统 SHALL 提供合并窗口机制，将短时间内多次文件保存合并为一次推送。用户保存文件后，启动可配置的倒计时定时器（默认 5 秒），倒计时结束前有新保存操作则重置倒计时。

#### Scenario: 单次保存立即入队
- **WHEN** 用户保存文件到已启用 Gitee 同步的笔记本
- **THEN** 推送任务被加入 PushQueue
- **AND** 启动倒计时定时器（默认 5 秒）

#### Scenario: 短时间内多次保存合并为一次推送
- **WHEN** 用户在 5 秒内连续保存同一个文件多次
- **THEN** 每次保存都重置倒计时定时器
- **AND** 只推送最后一次保存的内容
- **AND** 队列中只保留该文件的一个推送任务

#### Scenario: 倒计时结束后执行推送
- **WHEN** 倒计时定时器触发（5 秒内无新保存操作）
- **THEN** 系统从 PushQueue 取出推送任务
- **AND** 通过 Gitee API 上传文件内容
- **AND** 更新 ShaCache 中的 SHA

### Requirement: 推送队列管理
系统 SHALL 提供推送队列，管理待推送到 Gitee 的文件任务。队列必须支持任务添加、任务取出、任务取消和队列状态查询。队列无上限，保证数据完整性。应用关闭前检查队列状态，如有未推送任务则弹窗提示用户。

#### Scenario: 添加推送任务
- **WHEN** 用户保存文件到已启用 Gitee 同步的笔记本
- **THEN** 推送任务被添加到 PushQueue
- **AND** 任务包含文件路径（内容从磁盘读取）
- **AND** 队列无上限，始终成功添加

#### Scenario: 取消推送任务
- **WHEN** 用户在倒计时结束前删除或移动文件
- **THEN** 该文件的推送任务从 PushQueue 中移除
- **AND** 不执行推送操作

#### Scenario: 应用关闭前检查队列
- **WHEN** 用户尝试关闭 VNote 应用
- **THEN** 系统检查 PushQueue 中是否有未推送任务
- **AND** 如果有未推送任务，弹窗提示"有 N 个文件未同步，是否等待同步完成？"
- **AND** 用户选择"等待同步"时，应用暂停关闭，等待队列处理完成
- **AND** 用户选择"退出"时，应用直接关闭，未推送任务丢失
- **AND** 如果队列已空，应用直接关闭

### Requirement: Gitee API 认证
系统 SHALL 通过 Gitee 个人访问令牌（Personal Access Token）进行 Gitee API 认证。每次 API 请求必须在 Authorization header 中携带 token。

#### Scenario: 使用有效 token 调用 API
- **WHEN** 用户配置了有效的 Gitee token
- **THEN** 所有 Gitee API 请求携带 Authorization header
- **AND** API 调用成功执行

#### Scenario: 使用无效 token 调用 API
- **WHEN** 用户配置的 Gitee token 无效或已过期
- **THEN** Gitee API 返回 401 或 403 错误
- **AND** 系统记录错误日志
- **AND** 提示用户检查 token 配置

### Requirement: 同步开关控制
系统 SHALL 提供全局同步开关，控制是否启用 Gitee 同步功能。同步开关关闭时，所有拉取和推送操作必须被跳过。

#### Scenario: 同步开关关闭时不执行同步
- **WHEN** 用户关闭 Gitee 同步开关
- **THEN** 保存文件时不触发推送
- **AND** 打开文件时不触发拉取
- **AND** 展开/折叠目录时不触发拉取

#### Scenario: 同步开关打开时执行同步
- **WHEN** 用户打开 Gitee 同步开关
- **THEN** 保存文件时触发推送
- **AND** 打开文件时触发拉取
- **AND** 展开/折叠目录时触发拉取

### Requirement: 错误处理和重试
系统 SHALL 提供非阻塞的错误处理机制。网络错误和 API 错误必须记录日志并弹窗提示，不阻塞主流程。遇到速率限制时，队列任务必须指数退避重试。

#### Scenario: 网络错误时记录日志并弹窗提示
- **WHEN** 系统调用 Gitee API 时遇到网络错误（超时、断网）
- **THEN** 系统记录错误日志
- **AND** 弹窗提示用户网络错误（仅首次出现时）
- **AND** 不阻塞本地文件保存操作
- **AND** 任务留在队列中等待网络恢复后重试

#### Scenario: 速率限制时指数退避并弹窗提示
- **WHEN** Gitee API 返回速率限制错误（429）
- **THEN** 任务等待后重试（首次 1 秒，后续指数退避）
- **AND** 记录日志提示用户减少编辑频率
- **AND** 弹窗提示用户速率限制并显示重试倒计时

#### Scenario: API 认证错误时弹窗提示
- **WHEN** Gitee API 返回认证错误（401/403）
- **THEN** 系统记录错误日志
- **AND** 弹窗提示用户检查 token 配置
- **AND** 不重试该任务

#### Scenario: SHA 冲突时自动重试
- **WHEN** Gitee API 返回 SHA 冲突错误（409）
- **THEN** 系统自动获取最新 SHA 并重试一次推送
- **AND** 仍失败则记录日志并弹窗提示用户

#### Scenario: 同步成功时不提示
- **WHEN** 文件推送或拉取成功
- **THEN** 系统静默成功，不弹窗提示
- **AND** 仅在状态栏显示同步成功图标（可选）

### Requirement: vx.json 同步
系统 MUST 将 vx.json 节点配置文件同步到 Gitee 仓库。拉取和推送操作必须包含 vx.json 文件，确保多设备间节点元数据一致。

#### Scenario: 加载节点配置时拉取 vx.json
- **WHEN** ConfigMgr::loadNode() 加载节点配置
- **THEN** 系统先通过 Gitee API 拉取该节点的 vx.json
- **AND** 用远端内容更新本地 vx.json
- **AND** 再从本地 vx.json 加载节点配置

#### Scenario: 保存节点配置时推送 vx.json
- **WHEN** ConfigMgr::saveNode() 保存节点配置
- **THEN** 系统先保存 vx.json 到本地
- **AND** 再将 vx.json 加入 PushQueue 推送到 Gitee

#### Scenario: vx.json 不存在时跳过拉取
- **WHEN** 远端仓库中不存在该节点的 vx.json
- **THEN** 系统记录警告日志
- **AND** 继续加载本地 vx.json（如果存在）
