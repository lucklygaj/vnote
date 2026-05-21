## ADDED Requirements

### Requirement: Batch window configuration
The configuration SHALL allow user to set the batch window duration:
- Range: 1000ms to 10000ms
- Default: 3000ms
- UI: Number input with slider

#### Scenario: User changes batch window
- **WHEN** user sets batch window to 5000ms
- **THEN** scheduler uses 5000ms for batch merging
- **AND** configuration is persisted

### Requirement: Conflict resolution strategy configuration
The configuration SHALL allow user to set default conflict resolution:
- **Always ask user** (default)
- **Always use local version**
- **Always use remote version**

#### Scenario: User sets conflict strategy to "always use local"
- **WHEN** conflict occurs and strategy is "always use local"
- **THEN** local version is automatically pushed
- **AND** no dialog is shown

### Requirement: Retry configuration
The configuration SHALL allow user to set retry parameters:
- Max retry count (default: 5)
- Retry interval base (default: 1000ms, exponential backoff)
- Auto-retry on network recovery (default: enabled)

#### Scenario: User sets max retry to 3
- **WHEN** a sync fails
- **THEN** it retries up to 3 times
- **AND** then moves to failed list

### Requirement: Offline recovery configuration
The configuration SHALL allow user to enable/disable offline recovery:
- Enable auto-sync on network recovery (default: enabled)
- Show notification on recovery sync (default: enabled)

#### Scenario: Network recovers with offline recovery enabled
- **WHEN** network becomes available
- **AND** offline recovery is enabled
- **THEN** pending files are synced automatically
- **AND** user sees notification
