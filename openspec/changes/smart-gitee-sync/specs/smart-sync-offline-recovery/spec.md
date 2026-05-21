## ADDED Requirements

### Requirement: Persist pending sync list
The system SHALL persist pending sync files to disk for recovery:
- Location: `~/.vnote/sync_pending.json`
- Content: file path, content, SHA, timestamp, retry count

#### Scenario: File saved during offline
- **WHEN** user saves a file while offline
- **THEN** file is added to pending list
- **AND** persisted to sync_pending.json

#### Scenario: Pending list survives app crash
- **WHEN** application crashes
- **AND** pending list exists
- **THEN** pending list is loaded on restart
- **AND** files resume syncing

### Requirement: Automatic sync on network recovery
The system SHALL automatically retry pending syncs when network recovers.

#### Scenario: Network recovers
- **WHEN** network becomes available
- **AND** pending files exist
- **THEN** pending files are synced automatically
- **AND** pending list is updated on success

### Requirement: Cleanup old pending records
The system SHALL periodically cleanup stale pending records.

#### Scenario: Pending record older than 7 days
- **WHEN** a pending record exists for more than 7 days
- **THEN** it is removed from pending list
- **AND** user is notified

### Requirement: SHA cache synchronization
The system SHALL update SHA cache after successful pullFile operations.

#### Scenario: SHA updated after pull
- **WHEN** pullFile completes successfully
- **THEN** the file's SHA is updated in cache
- **AND** subsequent pushFile uses updated SHA
