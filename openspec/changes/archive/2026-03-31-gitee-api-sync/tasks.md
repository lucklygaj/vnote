## 1. 配置管理

- [x] 1.1 在 ConfigMgr 中添加 Gitee 同步配置项（gitee_token、gitee_owner、gitee_repo、gitee_branch、gitee_sync_enabled、gitee_push_delay_seconds）
- [x] 1.2 实现配置持久化到 JSON 文件
- [x] 1.3 实现配置加载和验证逻辑
- [x] 1.4 设置配置默认值（gitee_sync_enabled: false、gitee_push_delay_seconds: 5、gitee_branch: "master"）
- [x] 1.5 实现配置范围验证（gitee_push_delay_seconds: 1-30 秒）
- [x] 1.6 在设置界面创建 Gitee 同步配置页面

## 2. GiteeApi 模块

- [x] 2.1 创建 GiteeApi 类头文件（src/sync/giteeapi.h）
- [x] 2.2 实现创建文件 API 调用
- [x] 2.3 实现更新文件 API 调用
- [x] 2.4 实现获取文件内容 API 调用
- [x] 2.5 实现获取目录内容 API 调用
- [x] 2.6 实现获取文件 SHA API 调用
- [x] 2.7 实现 API 认证机制（Authorization header）
- [x] 2.8 实现错误处理（网络错误、API 错误、速率限制）

## 3. ShaCache 模块

- [x] 3.1 创建 ShaCache 类头文件（src/sync/shacache.h）
- [x] 3.2 实现内存 Map 缓存（文件路径 → SHA）
- [x] 3.3 实现缓存查询接口（getSha）
- [x] 3.4 实现缓存更新接口（updateSha）
- [x] 3.5 实现批量缓存更新接口（batchUpdateSha）
- [x] 3.6 实现缓存清空接口（clear）
- [x] 3.7 确保应用关闭时缓存清空

## 4. PushQueue 模块

- [x] 4.1 创建 PushQueue 类头文件（src/sync/pushqueue.h）
- [x] 4.2 实现推送任务结构（文件路径、提交消息，内容从磁盘读取）
- [x] 4.3 实现任务添加接口（enqueue）
- [x] 4.4 实现任务取出接口（dequeue）
- [x] 4.5 实现任务取消接口（cancel）
- [x] 4.6 实现队列清空接口（clear）
- [x] 4.7 实现合并窗口倒计时定时器（默认 5 秒，可配置）
- [x] 4.8 实现倒计时重置逻辑（新保存操作时）
- [x] 4.9 实现应用关闭前检查队列状态（如有未推送任务弹窗提示）
- [x] 4.10 实现指数退避重试机制（速率限制时）

## 5. GiteeSyncService 单例

- [x] 5.1 创建 GiteeSyncService 类头文件（src/sync/giteesyncservice.h）
- [x] 5.2 实现单例模式（getInst）
- [x] 5.3 集成 GiteeApi、ShaCache、PushQueue 三个子模块
- [x] 5.4 实现拉取文件接口（pullFile）
- [x] 5.5 实现拉取目录接口（pullDirectory）
- [x] 5.6 实现推送文件接口（pushFile）
- [x] 5.7 实现同步开关控制（checkSyncEnabled）
- [x] 5.8 实现配置验证接口（validateConfig）
- [x] 5.9 实现错误日志记录
- [x] 5.10 实现 SHA 冲突自动重试逻辑
- [x] 5.11 实现拉取成功后更新 SHA 缓存
- [x] 5.12 实现获取目录时批量预缓存 SHA
- [x] 5.13 实现弹窗提示机制（同步失败、速率限制等）

## 6. VXNodeFile 接入

- [x] 6.1 在 VXNodeFile::read() 前调用 GiteeSyncService::pullFile
- [x] 6.2 在 VXNodeFile::write() 后调用 GiteeSyncService::pushFile
- [x] 6.3 实现拉取失败时的降级处理（使用本地文件）
- [x] 6.4 实现推送失败时的非阻塞处理（不阻塞 write 返回）

## 7. LocalNotebookBackend 接入

- [x] 7.1 在 LocalNotebookBackend::loadRootNode() 前调用 GiteeSyncService::pullDirectory
- [x] 7.2 实现根目录拉取的文件同步逻辑
- [x] 7.3 实现拉取失败时的降级处理（使用本地文件）
- [x] 7.4 处理远端根目录不存在的场景

## 8. ConfigMgr 接入

- [x] 8.1 在 ConfigMgr::loadNode() 前调用 GiteeSyncService::pullFile 拉取 vx.json
- [x] 8.2 在 ConfigMgr::saveNode() 后调用 GiteeSyncService::pushFile 推送 vx.json
- [x] 8.3 实现 vx.json 拉取失败时的降级处理（使用本地 vx.json）
- [x] 8.4 实现配置变更通知机制（通知 GiteeSyncService 更新配置）

## 9. 编译配置

- [x] 9.1 在 CMakeLists.txt 中添加 src/sync 目录
- [x] 9.2 添加新模块到编译目标
- [x] 9.3 确保依赖库正确链接

## 10. 测试

- [x] 10.1 单元测试 GiteeApi 模块
- [x] 10.2 单元测试 ShaCache 模块
- [x] 10.3 单元测试 PushQueue 模块
- [x] 10.4 单元测试 GiteeSyncService 单例
- [x] 10.5 集成测试拉取流程（打开文件、展开目录、打开笔记本）
- [x] 10.6 集成测试推送流程（保存文件、合并窗口）
- [x] 10.7 集成测试 vx.json 同步
- [x] 10.8 测试配置验证和默认值
- [x] 10.9 测试错误处理（网络错误、API 错误、速率限制）
- [x] 10.10 测试同步开关控制
- [x] 10.11 测试 SHA 冲突自动重试
- [x] 10.12 测试批量预缓存 SHA
- [x] 10.13 测试应用关闭前队列检查提示机制
- [x] 10.14 测试弹窗提示机制
- [x] 10.15 测试配置范围验证（1-30 秒）

## 11. 文档

- [x] 11.1 编写 GiteeSyncService 使用文档
- [x] 11.2 编写配置项说明文档
- [x] 11.3 更新 README.md 添加 Gitee 同步功能介绍
