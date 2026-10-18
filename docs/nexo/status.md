# Status

Honest state of each part. "Written" means code exists but was not yet
compiled or run. Updated as work goes on.

| Part | State |
|---|---|
| Build environment (VS Build Tools, prepare step) | Done, Windows x64 |
| First build | Done 2026-10-18: Release, no debug info, `out/Release/NexoDesk.exe` |
| Branding: name, AppId, exe name, data folder, icons, rc files | Built and run: title, footer, exe name, data folder checked on the login screen |
| Settings > Connection (bulk import, fastest proxy, remove dead) | Compiles; needs a logged-in session to check |
| Settings > Appearance (bottom bar toggle, links to themes) | Compiles; needs a logged-in session to check |
| Bottom navigation bar | Compiles; needs a logged-in session to check |
| Built-in Russian strings for Nexo | Compiles; not checked in the UI |
| Nexo settings storage (`tdata/nexo/settings.json`) | Compiles; not checked |
| Auto-update | Disabled until own keys and server exist |
| Own update keys and server | Not started |
| Nexo themes | Not started |
| Pill search with proxy chip | Not started |
| Plugin system and manager | Not started (see exteragram-comparison.md) |
| Linux and macOS builds | Not attempted |
| Tests | Launch and login screen (QR code from Telegram servers) checked; nothing else yet |
