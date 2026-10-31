# Clip Inbox Roadmap

Clip Inbox is an OBS Studio dock where every new recording and replay waits to be named, kept, or deleted. Days later, every file in the recordings folder has a name that says what's in it.

## Decisions

| Topic | Decision |
| --- | --- |
| Name | Clip Inbox (`clip-inbox` for the folder, repo, and plugin ID) |
| Type | Native OBS Studio plugin, C++ with Qt, built from [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate) |
| Why C++ and Qt | It's what OBS itself is built with, so the dock looks and behaves like the built-in Scenes and Sources docks |
| Platform | Windows first. macOS and Linux later through the template's GitHub Actions builds |
| Minimum OBS | 31.1.1 (the template's pinned version). Tested on 32.2.2 |
| Price and license | Free, GPL-2.0 (OBS plugins have to match OBS's license) |
| Preview | None. The thumbnail and length are how you recognize a clip |
| Thumbnails and length | Read from Windows, the same way File Explorer does. No FFmpeg |
| Version | 0.x during development, 1.0.0 at first public release |

## Build order

Each stage gets tested in OBS before moving on.

- [x] **0. Setup.** Build tools installed, template copied in, project docs written.
- [ ] **1. Hello dock.** Plugin builds on Windows, installs into OBS, and shows an empty Clip Inbox dock under the Docks menu.
- [ ] **2. Clips appear.** Saved replays, stopped recordings, and split recording files show up as rows (filename only), but only after OBS has finished writing and remuxing them.
- [ ] **3. Rename and persistence.** Type a name, press Enter, and the file is renamed safely. The list survives an OBS restart. Naming rules are covered by automated tests.
- [ ] **4. Delete, skip, thumbnail, and length.** Recycle Bin delete with a confirm, a skip button, and a thumbnail plus clip length on each row.
- [ ] **5. Polish.** File size and time recorded on each row, counter in the dock title, settings window, and a check under every OBS theme.
- [ ] **6. Release.** Windows installer, README with screenshots, GitHub release, and a listing in the OBS forum Resources section.

## Rules

The plugin must never lose, overwrite, or permanently delete a recording.

- **Final name:** `<your text> - <date>.<ext>`, for example `clutch 1v3 ridge - 2026-10-30.mp4`. The date format is a setting.
- **Clean names:** strip `\ / : * ? " < > |`, trim spaces, refuse empty names.
- **Taken names:** add ` (2)`, ` (3)`, and so on. Never overwrite.
- **Same folder:** rename in place. Never move a file.
- **Delete:** confirm first, then send to the Recycle Bin. Never delete permanently.
- **Busy files:** never touch a file OBS is still writing. If another app has the file open, keep the clip in the inbox and retry instead of failing.
- **No popups during gameplay.** Nothing steals focus from the game.
- **Off the UI thread:** renames, deletes, and thumbnails run in the background so OBS never stutters.

## Edge cases

| Situation | Expected behavior |
| --- | --- |
| File is open in another app when renaming | Keep the clip in the inbox, show "waiting for the file to close", and finish the rename when it's free |
| File moved or deleted outside OBS | Drop it from the inbox on the next refresh or startup |
| Auto-remux to MP4 is on | Track the final `.mp4`, not the `.mkv` |
| File splitting is on | Each split file is its own row |
| OBS crashes mid-recording | Unfinished files are never added, and nothing already in the inbox is lost |
| Recording folder changes in OBS settings | Existing rows keep their own full paths |
| Replay saved while recording | Both appear as separate rows |

## File structure

Proposed. To be confirmed before stage 1.

```
src/
  plugin-main.cpp           Starts and stops the plugin and connects the parts
  clip.hpp                  What the inbox knows about one clip
  clip-events.cpp/.hpp      Hears OBS finish a recording, replay, or split file
  file-ready.cpp/.hpp       Waits until OBS is done writing (and remuxing) a file
  clip-store.cpp/.hpp       The inbox list, saved to inbox.json
  clip-naming.cpp/.hpp      Builds safe file names (cleans text, adds date, adds (2))
  file-actions.cpp/.hpp     Renames files and sends them to the Recycle Bin
  file-info.cpp/.hpp        Gets thumbnails and lengths from Windows
  inbox-dock.cpp/.hpp       The dock panel
  clip-row.cpp/.hpp         One row: thumbnail, details, name box, buttons
  settings-dialog.cpp/.hpp  Settings window (stage 5)
tests/
  clip-naming-tests.cpp     Proves the naming rules never produce a bad or duplicate name
data/locale/en-US.ini       All on-screen text
```

## Notes from the OBS source (31.1.1)

- `OBS_FRONTEND_EVENT_RECORDING_STOPPED` and `OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED` both fire **before** auto-remux starts, so the plugin has to wait for the remuxed file itself.
- Auto-remux writes `<same name>.mp4` (`.mov` for ProRes, `<name>.remuxed.<ext>` for fragmented formats) and keeps the original file.
- Hybrid MP4 recordings don't need a remux.
- File splitting has no frontend event. It's a `file_changed(string next_file)` signal on the recording output, so the plugin reconnects to it every time a recording starts (OBS can rebuild its outputs when settings change).
- Paths come from `obs_frontend_get_last_recording()` and `obs_frontend_get_last_replay()`. The dock is added with `obs_frontend_add_dock_by_id()`.

## Known limitations

- Windows can only make thumbnails for codecs it has an extension for. HEVC and AV1 recordings show a placeholder thumbnail unless the free AV1 or paid HEVC extension from the Microsoft Store is installed. H.264 always works.
