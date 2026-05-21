#ifndef FILESYNCSTATE_H
#define FILESYNCSTATE_H

namespace vnotex {

enum class FileSyncState {
  SYNCED,     // File content matches Gitee repository
  PENDING,    // File has pending changes, waiting for debounce
  SYNCING,    // Sync operation in progress
  FAILED,     // Sync failed, waiting for retry
  PAUSED      // Sync paused (e.g., offline mode)
};

} // namespace vnotex

#endif // FILESYNCSTATE_H
