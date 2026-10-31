# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Project setup from the official OBS plugin template.
- Clip Inbox dock in the OBS Docks menu, with an empty-inbox message.
- Saved replays, stopped recordings, and split recording files appear in the inbox, newest first.
- Clips only appear once OBS has finished writing them, and after an auto-remux the inbox shows the remuxed file.
- Type a name and press Enter to rename a clip to `<name> - <date>.<ext>` in the same folder.
- Renames never overwrite: a taken name gets ` (2)`, ` (3)`, and so on.
- Names are cleaned of characters Windows doesn't allow, and empty names are refused.
- When another app has a clip open, the rename waits and finishes on its own once the file is free.
- The inbox is saved and survives an OBS restart, including clips still finishing when OBS closed.
- Clips whose files were moved or deleted outside OBS drop out of the inbox on startup.
- Rename button next to the name box (Enter still works too).
- Skip button takes a clip out of the inbox with its original name.
- Delete button sends a clip to the Recycle Bin after a confirmation. Files Windows can't recycle are left alone instead of being deleted for good.
- Each row shows the clip's thumbnail and length, read from Windows the same way File Explorer does.
- Each row also shows the file size and when the clip was recorded.
- The dock title counts waiting clips, like "Clip Inbox (4)".
- Settings window, opened from the gear button, with a date format choice (or no date) and quick tags.
- Quick tags show up as buttons on every clip. Clicking one adds it to the name.
- Automated tests for remux detection, finished-file checks, naming rules, safe renames, saving, delete safety, clip lengths, and settings.
