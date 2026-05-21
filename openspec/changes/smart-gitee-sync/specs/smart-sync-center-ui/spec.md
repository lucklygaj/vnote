## ADDED Requirements

### Requirement: Sync statistics display
The sync center view SHALL display overall sync statistics:
- Total synced files count
- Pending sync files count
- Currently syncing files count
- Failed sync files count

#### Scenario: Statistics update in real-time
- **WHEN** any file sync state changes
- **THEN** statistics section updates immediately
- **AND** user sees current sync status

### Requirement: File sync details list
The sync center view SHALL display a scrollable list of files with sync details:
- File name and path
- Current sync state (color-coded)
- Last modified time
- Last sync time
- Retry count (if failed)

#### Scenario: List shows all tracked files
- **WHEN** user opens sync center
- **THEN** all files with sync activity are listed
- **AND** sorted by last modified time (newest first)

#### Scenario: Failed files highlighted
- **WHEN** any file has FAILED state
- **THEN** it is visually highlighted in red
- **AND** retry action is available

### Requirement: Failure and retry records
The sync center view SHALL display failure history:
- Last 7 days of failure records
- Failure reason
- Number of retry attempts
- Manual retry button per file

#### Scenario: User retries failed sync
- **WHEN** user clicks retry button for a failed file
- **THEN** sync task is re-queued
- **AND** state changes to SYNCING

#### Scenario: User clears failure record
- **WHEN** user clicks clear button
- **THEN** failure record is removed from list
- **AND** file state resets to PENDING

### Requirement: Manual sync trigger
The sync center view SHALL allow user to manually trigger sync.

#### Scenario: User forces immediate sync
- **WHEN** user clicks "Sync Now" button
- **THEN** all pending files are synced immediately
- **AND** batch window is bypassed
