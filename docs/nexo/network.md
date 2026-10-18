# Network requests

Every host the client contacts besides Telegram's own servers.

## Added by Nexo Desk

| Host | Code | Purpose |
|---|---|---|
| `github.com/Svat1r/NexoDesk/releases/latest/download/` (redirects to GitHub's file storage) | `storage/localstorage.cpp`, `core/update_checker.cpp` | Update feed `current6` and signed update packages |
| `api.github.com/repos/Svat1r/NexoDesk/releases/latest` | `nexo/updates/nexo_updates.cpp` | Release notes, only when "What's new" is clicked |

No account, session or message data is sent; these are plain downloads.

## Inherited from AyuGram Desktop

Found in AyuGram 7.2.9 sources. They still point to AyuGram's servers.

| Host | Code | Purpose | Nexo status |
|---|---|---|---|
| `update.ayugram.one` | `storage/localstorage.cpp` (update prefix) | Auto-update | Replaced by the Nexo GitHub feed |
| `update.ayugram.one/rc/...` | `ayu/utils/rc_manager.cpp` | AyuGram remote config | Still active, to be reviewed |
| `api.exteragram.app` | `ayu/utils/rc_manager.cpp` | Fallback for remote config / profile badges | Still active, to be reviewed |
| `translate.ayugram.one` | `ayu/ui/settings/settings_main.cpp` (link) | Translation project link | Link only |
| `docs.ayugram.one`, `ayugram.one` | settings and main menu links | Documentation links | Link only |

Nexo accepts only v2 packages signed through its own manifest; AyuGram's
packages (v1 RSA or v2 with AyuGram keys) are rejected. See releasing.md.
