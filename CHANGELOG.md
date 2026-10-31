# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-10-30

First release.

### Added

- Clip Inbox dock in the OBS Docks menu. Saved replays, stopped recordings, and split recording files show up newest first, and the dock title counts them, like "Clip Inbox (4)".
- Clips only appear once OBS has finished writing them. With "Automatically remux to mp4" on, the inbox shows the remuxed file.
- Each clip shows its thumbnail, length, file size, and when it was recorded. Thumbnails and lengths come from Windows, the same way File Explorer gets them.
- Type a name and press Enter or click Rename to rename the clip to `<name> - <date>.<ext>` in the same folder.
- Renames never overwrite: a taken name gets ` (2)`, ` (3)`, and so on. Characters Windows doesn't allow are cleaned out, and empty names are refused.
- When another app has a clip open, the rename waits and finishes on its own once the file is free.
- Skip takes a clip out of the inbox with its original name.
- Delete sends a clip to the Recycle Bin after a confirmation. Files Windows can't recycle are left alone instead of being deleted for good.
- Settings window, opened from the gear button, with a date format choice (or no date) and quick tags.
- Quick tags show up as buttons on every clip. Clicking one adds it to the name.
- The inbox survives an OBS restart, including clips still finishing when OBS closed. Clips whose files were moved or deleted outside OBS drop out on startup.
- Windows installer with an uninstaller, plus a .zip for manual installs.
- Installs in both OBS 33's plugin layout and the older one, so it loads in OBS 31.1.1 and every version since.
- A readable name and description in OBS's Plugin Manager.

[Unreleased]: https://github.com/Vultify/clip-inbox/compare/1.0.0...HEAD
[1.0.0]: https://github.com/Vultify/clip-inbox/releases/tag/1.0.0
