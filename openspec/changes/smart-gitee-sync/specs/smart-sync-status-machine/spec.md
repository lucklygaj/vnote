## ADDED Requirements

### Requirement: Five-state file sync model
The sync status manager SHALL maintain one of five states for each tracked file:
- **SYNCED**: File content matches Gitee repository
- **PENDING**: File has pending changes, waiting for debounce
- **SYNCING**: Sync operation in progress
- **FAILED**: Sync failed, waiting for retry
- **PAUSED**: Sync paused (e.g., offline mode)

#### Scenario: File transitions to SYNCED after successful sync
- **WHEN** pushFile completes successfully
- **THEN** file state changes to SYNCED
- **AND** timestamp is updated

#### Scenario: File transitions to PENDING on local change
- **WHEN** user modifies a file
- **THEN** file state changes to PENDING
- **AND** pending content is stored

#### Scenario: File transitions to SYNCING when executing
- **WHEN** sync task starts executing
- **THEN** file state changes to SYNCING

#### Scenario: File transitions to FAILED on sync error
- **WHEN** pushFile fails with error
- **THEN** file state changes to FAILED
- **AND** retry count is incremented

#### Scenario: File transitions to PAUSED on offline
- **WHEN** network becomes unavailable
- **THEN** all pending/syncing files transition to PAUSED

### Requirement: State persistence
The manager SHALL persist sync states to survive application restart.

#### Scenario: States restored on app restart
- **WHEN** application restarts
- **THEN** sync states are loaded from persistence
- **AND** pending files resume syncing when network available

### Requirement: State transition events
The manager SHALL emit signals for state changes to update UI components.

#### Scenario: UI updates on state change
- **WHEN** file state changes
- **THEN** stateChanged signal is emitted
- **AND** status bar indicator updates
