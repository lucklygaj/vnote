## ADDED Requirements

### Requirement: Event routing with priority queue
The smart sync scheduler SHALL route sync events to appropriate queues based on priority:
- Ctrl+S events → high priority queue (immediate execution, debounce=0)
- Auto-save events → low priority queue (batched execution, debounce=batchWindowMs)
- New file events → highest priority queue (immediate execution)

#### Scenario: Ctrl+S triggers immediate sync
- **WHEN** user presses Ctrl+S while editing a file
- **THEN** the file is added to the high priority queue
- **AND** the sync is executed immediately without waiting

#### Scenario: Auto-save triggers batched sync
- **WHEN** auto-save triggers for a file within the batch window
- **THEN** the file is added to the low priority queue
- **AND** sync executes after batch window expires

### Requirement: Debounce mechanism
The scheduler SHALL debounce sync events to prevent redundant API calls.

#### Scenario: Multiple saves within debounce window
- **WHEN** user saves a file multiple times within 3 seconds
- **THEN** only one sync task is executed
- **AND** the latest content is synced

#### Scenario: Debounce resets on new save
- **WHEN** a sync is pending in debounce state
- **AND** user saves again
- **THEN** the debounce timer resets
- **AND** sync waits for the full debounce period

### Requirement: Task deduplication
The scheduler SHALL deduplicate pending tasks for the same file.

#### Scenario: Same file queued multiple times
- **WHEN** file A is already in pending queue
- **AND** a new sync request for file A arrives
- **THEN** the new request updates the pending content
- **AND** only one sync task is executed

### Requirement: Batch merge within time window
The scheduler SHALL merge multiple file changes within the batch window into a single sync operation.

#### Scenario: Multiple files changed within batch window
- **WHEN** files A, B, C are changed within 3 seconds
- **THEN** all changes are batched together
- **AND** synced in a single batch operation
