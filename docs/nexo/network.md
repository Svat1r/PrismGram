# Network requests

Every host the client contacts besides Telegram's own servers.

## Added by Nexo Desk

None.

## Inherited from AyuGram Desktop

Found in AyuGram 7.2.9 sources. They still point to AyuGram's servers.

| Host | Code | Purpose | Nexo status |
|---|---|---|---|
| `update.ayugram.one` | `storage/localstorage.cpp` (update prefix) | Auto-update | Disabled: builds use `DESKTOP_APP_DISABLE_AUTOUPDATE=ON` until Nexo has its own signed update server |
| `update.ayugram.one/rc/...` | `ayu/utils/rc_manager.cpp` | AyuGram remote config | Still active, to be reviewed |
| `api.exteragram.app` | `ayu/utils/rc_manager.cpp` | Fallback for remote config / profile badges | Still active, to be reviewed |
| `translate.ayugram.one` | `ayu/ui/settings/settings_main.cpp` (link) | Translation project link | Link only |
| `docs.ayugram.one`, `ayugram.one` | settings and main menu links | Documentation links | Link only |

Why auto-update is off: AyuGram's update packages are signed with AyuGram's
key, which the client trusts. A Nexo build pointed at AyuGram's server would
accept an AyuGram update and turn itself back into AyuGram. Nexo needs its
own key pair and update server first.
