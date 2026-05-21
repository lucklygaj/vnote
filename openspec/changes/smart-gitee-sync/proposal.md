## Why

当前 VNote 的 Gitee 同步机制存在以下问题：
1. **触发时机过多**：手动保存、自动保存、窗口关闭、新建文件、配置更新等都会触发同步，容易造成任务重叠和冲突
2. **SHA 缓存过期**：pushFile 后 SHA 未及时更新，导致后续更新失败
3. **用户感知差**：同步任务执行情况不透明，用户无法了解同步状态
4. **持续修改场景效率低**：同一文件持续修改时，队列中的旧任务会造成无效推送

## What Changes

- 新增**智能同步调度器**：支持批量合并、防抖、任务去重
- 新增**文件状态机**：SYNCED/PENDING/SYNCING/FAILED/PAUSED 五种状态
- 新增**同步管理中心 UI**：显示同步统计、文件详情、失败与重试记录
- 新增**状态栏常驻指示器**：实时显示同步状态
- 支持**批量窗口可配置**（默认 3 秒，范围 1000-10000ms）
- 支持**冲突用户选择**：检测到 409 冲突时弹出选择对话框
- 支持**离线自动恢复**：持久化待同步列表，网络恢复后自动重试

## Capabilities

### New Capabilities

- `smart-sync-scheduler`: 智能同步调度器，支持事件路由（Ctrl+S 高优先级、自动保存低优先级）、防抖、批量合并、任务去重
- `smart-sync-status-machine`: 文件同步状态机，管理 SYNCED/PENDING/SYNCING/FAILED/PAUSED 五种状态转换
- `smart-sync-center-ui`: 同步管理中心 UI，显示同步统计、文件详情列表、失败与重试记录
- `smart-sync-status-bar`: 状态栏同步指示器，实时显示同步状态（🟢🟡🔵🔴）
- `smart-sync-config`: 同步配置界面，支持批量窗口、冲突策略、离线恢复配置
- `smart-sync-offline-recovery`: 离线恢复机制，持久化待同步列表到 `~/.vnote/sync_pending.json`

### Modified Capabilities

- `gitee-sync-service`: 优化 SHA 缓存管理，修复 pushFile 后 SHA 未更新的问题

## Impact

- **核心模块**：`src/sync/giteesyncservice.cpp/h`
- **新建模块**：`src/sync/smartsyncscheduler.cpp/h`、`src/sync/syncstatusmanager.cpp/h`
- **UI 模块**：`src/widgets/synccenterview.cpp/h`
- **配置模块**：`src/core/configmgr.cpp/h`（新增 SyncConfig）
- **持久化**：`~/.vnote/sync_pending.json`
