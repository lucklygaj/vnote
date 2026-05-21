## ADDED Requirements

### Requirement: Persistent status bar indicator
The status bar SHALL display a persistent sync status indicator:
- 🟢 All files synced
- 🟡 X files pending (X > 0)
- 🔵 Syncing... (during active sync)
- 🔴 X files failed (X > 0)

#### Scenario: Indicator shows pending count
- **WHEN** 3 files have PENDING state
- **THEN** status bar shows "🟡 3 pending"

#### Scenario: Indicator shows syncing state
- **WHEN** any file is in SYNCING state
- **THEN** status bar shows "🔵 Syncing..."

#### Scenario: Indicator shows failed count
- **WHEN** 2 files have FAILED state
- **THEN** status bar shows "🔴 2 failed"

### Requirement: Click to open sync center
The status bar indicator SHALL be clickable to open sync center view.

#### Scenario: User clicks indicator
- **WHEN** user clicks the sync status indicator
- **THEN** sync center view opens
- **AND** shows detailed sync information

### Requirement: Tooltip with quick info
The indicator SHALL show a tooltip with quick sync information on hover.

#### Scenario: Tooltip shows sync summary
- **WHEN** user hovers over the indicator
- **THEN** tooltip shows: "X synced, Y pending, Z failed"
