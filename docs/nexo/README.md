# Nexo Desk

Nexo Desk is a Telegram Desktop client based on AyuGram Desktop, which is
itself based on Telegram Desktop. It keeps all AyuGram features and adds its
own branding, interface and settings.

Base versions: AyuGram Desktop 7.2.9 (commit d4b7546), Telegram Desktop 7.2.9.
License: GPLv3 with the OpenSSL exception, same as the base (see `LEGAL`).
AyuGram asks to credit its authors: Nexo Desk is built on their work.

Status of everything listed here is tracked in [status.md](status.md).

## Repository layout

| Path | What |
|---|---|
| `Telegram/SourceFiles/nexo/` | All Nexo Desk code |
| `Telegram/SourceFiles/nexo/nexo_settings.*` | Nexo settings, stored in `tdata/nexo/settings.json` |
| `Telegram/SourceFiles/nexo/nexo_lang.*` | Built-in translations of `nexo_*` strings |
| `Telegram/SourceFiles/nexo/settings/` | Settings sections: Connection, Appearance |
| `Telegram/SourceFiles/nexo/connection/` | Bulk proxy tools |
| `Telegram/SourceFiles/nexo/ui/` | Bottom navigation bar, proxy chip, theme picker, `nexo_ui.style` |
| `Telegram/SourceFiles/nexo/themes/` | Built-in Nexo themes |
| `Telegram/Resources/nexo/themes/` | Theme files and `make_themes.py` |
| `Telegram/Resources/qrc/nexo/nexo.qrc` | Nexo resources |
| `Telegram/Resources/art/nexo/` | Logo source and `make_icons.py` |
| `docs/nexo/` | This documentation |

## Changes outside `nexo/`

Kept small on purpose, so AyuGram updates merge with few conflicts.

| File | Change |
|---|---|
| `Telegram/CMakeLists.txt` | Nexo sources, exe name `NexoDesk`, bundle id |
| `Telegram/cmake/td_ui.cmake` | `nexo_ui.style` |
| `Telegram/build/prepare/prepare.py` | MSYS2 ucrt64 packages (same fix as upstream tdesktop) |
| `Telegram/Resources/langs/lang.strings` | English `nexo_*` strings |
| `Telegram/Resources/winrc/*.rc` | Product and company name |
| `Telegram/Resources/art/*` | App icons replaced by the Nexo logo |
| `SourceFiles/core/version.h` | `AppId` (new GUID), `AppName`, `AppFile` |
| `SourceFiles/core/application.cpp` | `Nexo::Lang::Init()`, short app name |
| `SourceFiles/core/launcher.cpp` | Qt application name |
| `SourceFiles/boxes/connection_box.h` | Declarations of bulk proxy tools |
| `SourceFiles/dialogs/dialogs_widget.*` | Bottom navigation bar, proxy chip |
| `SourceFiles/window/main_window.cpp` | Window title |
| `SourceFiles/settings/sections/settings_main.cpp` | Appearance and Connection entries |
| `SourceFiles/ayu/utils/windows_utils.cpp` | Touch only Nexo shortcuts, never AyuGram's |
| `about_box.cpp`, `intro_widget.cpp`, `crash_report*.cpp`, `history_item_helpers.cpp` | Visible name |

## User data

User data lives in `%APPDATA%\Nexo Desk\` on Windows (from `AppName`), apart
from the install folder and apart from AyuGram's and Telegram's data. Nexo
never reads the AyuGram or Telegram data folders. Updates and reinstalls do
not touch it.

## Network

Nexo Desk adds no network requests of its own so far. Proxy checks use the
base client's MTProto connection checks. Requests inherited from AyuGram are
listed in [network.md](network.md).

## Git branches

| Branch | Purpose |
|---|---|
| `main` | Nexo Desk |
| `upstream-sync` | Merging new AyuGram releases before they reach `main` |
| `experimental` | Features not ready for `main` |

Remote `upstream` is https://github.com/AyuGram/AyuGramDesktop.

See [building-win.md](building-win.md) and [updating-from-ayugram.md](updating-from-ayugram.md).
