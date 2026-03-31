## Why

当前 VNote 使用本地文件系统 + git 命令的方式保存数据，用户需要手动执行 git 操作才能实现多设备同步。这种方式操作繁琐且容易忘记提交/推送，导致多设备间数据不同步。用户期望在编辑的同时，通过 Gitee API 自动将数据推送到云端，实现跨设备无缝同步。

## What Changes

- **新增 Gitee API 同步功能**
  - 添加 GiteeSyncService 单例组件，提供文件上传/下载能力
  - 实现拉取策略：打开笔记本时同步根目录，展开目录时同步子目录，打开文件时同步文件内容
  - 实现推送策略：保存文件时通过合并窗口机制延迟推送到 Gitee（可配置倒计时秒数）
  - 添加 SHA 缓存机制，避免重复获取远端文件 SHA
  - 保留 vx.json 同步到 Gitee（路径 A 方案）
  - 在 ConfigMgr 中添加 Gitee 同步配置项（token、仓库信息、倒计时秒数等）
  - 参考现有 GiteeImageHost 实现 Gitee API 认证和调用

## Capabilities

### New Capabilities
- `gitee-sync-service`: Gitee API 同步服务核心功能，包括文件上传、下载、SHA 缓存、合并窗口推送机制
- `gitee-sync-config`: Gitee 同步配置管理，存储 token、仓库信息、同步策略参数等

### Modified Capabilities
无（当前无现有 specs，纯新增功能）

## Impact

- **新增代码模块**
  - `src/sync/giteesyncservice.h/cpp` - Gitee 同步服务核心类
  - `src/sync/shacache.h/cpp` - SHA 缓存管理
  - `src/sync/pushqueue.h/cpp` - 推送队列和合并窗口
  - `src/sync/giteeapi.h/cpp` - Gitee API 封装
  - ConfigMgr 中新增 Gitee 同步配置项

- **修改现有代码**
  - `src/core/notebook/vxnodefile.cpp` - 在 read() 前触发拉取，write() 后触发推送
  - `src/core/notebookbackend/localnotebookbackend.cpp` - 在 loadRootNode() 前触发拉取
  - `src/core/notebookconfigmgr/vxnodeconfig.cpp` - 在加载节点配置前触发拉取

- **依赖**
  - 复用 `src/imagehost/giteeimagehost.h/cpp` 的 Gitee API 认证和调用逻辑
  - 复用 `libs/vtextedit/src/include/vtextedit/networkutils.h` 网络访问工具

- **配置影响**
  - 用户需要配置 Gitee token 和仓库信息才能启用同步功能
  - 新增配置选项不影响现有默认行为（功能默认关闭）
