## 1. Core Infrastructure

- [x] 1.1 Add SmartSyncScheduler class to src/sync/
- [x] 1.2 Add SyncStatusManager class to src/sync/
- [x] 1.3 Create FileSyncState enum (SYNCED/PENDING/SYNCING/FAILED/PAUSED)
- [x] 1.4 Implement event routing with priority queues
- [x] 1.5 Implement debounce mechanism with configurable window
- [x] 1.6 Implement task deduplication logic

## 2. SHA Cache Fix

- [x] 2.1 Fix GiteeSyncService::pullFile() to update SHA cache
- [x] 2.2 Add m_fileShaMap update after successful pushFile

## 3. Sync Status Management

- [x] 3.1 Implement state transition logic (SYNCED ↔ PENDING ↔ SYNCING ↔ FAILED)
- [x] 3.2 Implement state transition signals/slots
- [x] 3.3 Add state persistence to JSON file (~/.vnote/sync_state.json)

## 4. Offline Recovery

- [x] 4.1 Create SyncPendingList class for persistence
- [x] 4.2 Implement save/load sync_pending.json
- [x] 4.3 Add network status listener
- [ ] 4.4 Implement auto-retry on network recovery
- [x] 4.5 Add cleanup of stale pending records (>7 days)

## 5. Sync Center UI

- [x] 5.1 Create SyncCenterView widget
- [x] 5.2 Add sync statistics section (synced/pending/syncing/failed counts)
- [x] 5.3 Add file list view with state indicators
- [x] 5.4 Add failure records list with retry/clear buttons
- [x] 5.5 Add "Sync Now" manual trigger button

## 6. Status Bar Indicator

- [x] 6.1 Add SyncStatusIndicator to main window status bar
- [x] 6.2 Implement color-coded state display (🟢🟡🔵🔴)
- [x] 6.3 Add click handler to open Sync Center
- [x] 6.4 Add tooltip with sync summary

## 7. Configuration UI

- [x] 7.1 Add SyncConfig class to ConfigMgr
- [x] 7.2 Add batch window configuration (slider + input, 1000-10000ms)
- [x] 7.3 Add conflict resolution strategy dropdown
- [x] 7.4 Add retry configuration (max count, interval base)
- [x] 7.5 Add offline recovery toggle

## 8. Integration

- [x] 8.1 Replace direct GiteeSyncService calls with SmartSyncScheduler
- [x] 8.2 Wire up Ctrl+S to high-priority queue
- [x] 8.3 Wire up auto-save to low-priority queue
- [ ] 8.4 Test end-to-end sync workflow
