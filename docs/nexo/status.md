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
| Auto-update | On. Signed v2 packages from GitHub; download, verify, install and rollback tested locally |
| Own update keys | Created in C:\TBuild\nexo-keys (not in git), see releasing.md |
| Nexo themes (Night, AMOLED, Ocean, Violet, Forest) in Appearance | Compiles, app starts; needs a logged-in session to check |
| Proxy chip in the search field (field was already pill-shaped) | Compiles, app starts; needs a logged-in session to check |
| Plugins (QuickJS, API v1, manager) | Compiles, app starts; manager and example need a logged-in check |
| Linux and macOS builds | Not attempted |
| Tests | Launch and login screen (QR code from Telegram servers) checked; nothing else yet |
| Settings backup export/import | Compiles; needs a logged-in check |
| About, versions, Nexo Desk section | Compiles; needs a logged-in check |
| Exact numbers, mention comma, centered title, sticker size, speed boost | Compiles; needs a logged-in check |
| Bottom bar: section highlight, unread chats badge | Compiles; badge and bar seen working by the user before this change |
| Data folder apart from install folder | Built; portable copies with tdata next to the exe keep using it |
| Windows installer | Built: out/installer/NexoDesk-Setup-x64-0.1.0.exe; install not tested |
| GitHub repo and first release | Not done, needs approval |
