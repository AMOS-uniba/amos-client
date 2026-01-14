# AMOS client
This is the repository of the AMOS client, a GUI application
that manages a single remote AMOS station.

### 0.8.1.6
Final release of the 0.8. branch, the last to support Qt5 and thus Windows 7.
Further updates are discouraged.

## 1.0
Finally reached the required level of operational readiness. Migrated to Qt6.

### 1.4.0
Added the ability to generate test sightings. Probably will be moved to a separate tab or behind a wall.

### 1.4.1
Removed the possibility of deleted sightings when the server detects a duplicate,
instead they are moved to a separate directory `duplicates`.

This is a response to an incident when the server reported duplicates on an
unrelated database integrity error.
