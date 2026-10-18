# Nexo Desk plugins: proposed design

Status: proposal, nothing of this is implemented yet.

## What exists today

- AyuGram Desktop 7.2.9 has no plugin runtime. It only shows an info box
  for `.plugin` files (exteraGram / AyuGram for Android plugins) received
  in a chat: `ayu/ui/boxes/plugin_info_box.cpp`. Nexo keeps that box.
- exteraGram plugins are Python files that call Android and Java APIs and
  hook Java methods (see `exteragram-comparison.md`). They cannot run in a
  C++/Qt desktop client. Nexo will not claim to run them.
- The Qt 5.15 build used by Nexo has no QML/JavaScript engine, so a script
  engine is a new dependency in any case.

## Engine choice (needs a decision)

| Option | Size | Isolation | Notes |
|---|---|---|---|
| Lua 5.4 | ~300 KB of C, MIT | Good: only the libraries we register exist, no file or OS access unless given | Simple, widely embedded |
| QuickJS | ~1 MB of C, MIT | Same model as Lua | JavaScript, familiar to more people |
| Python | Large runtime | Hard to restrict | Closest to exteraGram, still not compatible with its plugins |

Recommendation: Lua 5.4 or QuickJS. Either way plugins run inside the
client process. This is not a sandbox in the OS sense: a bug in the engine
could still affect the client. The permission system below limits what
the plugin API offers, nothing more.

## Plugin package

A plugin is a folder or a `.nexoplugin` zip with this layout:

```
my-plugin/
  manifest.json
  main.lua        (or main.js, per the engine)
  icon.png        (optional)
```

Installed plugins live in `<data>/tdata/nexo/plugins/<id>/`, apart from the
install folder. Plugin settings live in `<data>/tdata/nexo/plugins-data/<id>.json`
and are kept when a plugin is disabled, updated or found incompatible.

## Manifest

```json
{
  "id": "com.example.hello",
  "name": "Hello",
  "version": "1.2.0",
  "author": "Example",
  "description": "Adds a greeting to the message menu.",
  "source": "https://example.com/hello",
  "api_version": 1,
  "min_client": "0.1.0",
  "max_client": "0.x",
  "entry": "main.lua",
  "permissions": ["menu.message", "settings"]
}
```

| Field | Rule |
|---|---|
| `id` | Required, `^[a-z][a-z0-9_.-]{1,63}$`, unique |
| `name`, `author`, `version`, `entry` | Required |
| `version` | Semantic version `MAJOR.MINOR.PATCH` |
| `api_version` | Required integer; must equal the client's plugin API version |
| `min_client`, `max_client` | Nexo Desk versions; `x` allowed in `max_client` |
| `permissions` | Only known names; unknown ones make the plugin incompatible |
| `source` | Optional, shown to the user; https only |

## Permissions (API v1 draft)

| Permission | Gives |
|---|---|
| `menu.message` | Add items to the message context menu |
| `menu.chat` | Add items to the chat and profile menus |
| `settings` | Own settings page in Settings > Plugins |
| `messages.read` | Read text of the message the user acted on |
| `messages.send` | Send text into the current chat after a user action |
| `network` | HTTPS requests to hosts listed in the manifest |

Never available: session keys, auth data, local database, file system
outside the plugin's own folder, other plugins' data.

## Versioning and compatibility

- Client version (`Nexo Desk 0.1.0`), base version (`AyuGram 7.2.9`) and
  plugin API version (`1`) are separate numbers, all shown in About.
- API v1 additions keep old plugins working; removals or changes bump the
  API version.
- On start and after every client update the manager checks each plugin.
  An incompatible plugin is disabled with the reason shown; its files and
  data are kept.

## Plugin manager (Settings > Plugins)

List of plugins with name, author, version, source, permissions and state
(enabled, disabled, incompatible with reason, error with log). Actions:
enable, disable, remove, import from file, open plugins folder, view log.
Installing always shows the permissions and the source and needs a
confirmation; nothing is installed automatically. Unknown sources are
marked as such.

## Errors

A plugin error is logged to `<data>/tdata/nexo/plugins-logs/<id>.log`, shown
in the manager, and disables only that plugin after repeated failures.
