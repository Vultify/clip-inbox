# Changelog

All notable changes to this project are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Project setup from the official OBS plugin template.
- Clip Inbox dock in the OBS Docks menu, with an empty-inbox message.
- Saved replays, stopped recordings, and split recording files appear in the inbox, newest first.
- Clips only appear once OBS has finished writing them, and after an auto-remux the inbox shows the remuxed file.
- Automated tests for remux detection and finished-file checks.
