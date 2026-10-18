# PrismGram plugins

Plugin API version: **1**. Engine: QuickJS-ng 0.17.0 (MIT), submodule
`Telegram/ThirdParty/quickjs`, built by `Telegram/cmake/lib_quickjs.cmake`.

## What plugins are and are not

- A plugin is JavaScript run by PrismGram inside its own process, with its
  own QuickJS runtime: 32 MB memory limit, 1 MB stack, 500 ms limit for
  every call into plugin code.
- Plugins only see the `nexo` object below. The engine is built without
  `quickjs-libc`, so there is no file, OS, process or network access from
  JavaScript itself.
- This is **not** an OS-level sandbox. A bug in the engine or in the client
  could still be exploited by a malicious plugin. Install only trusted
  plugins; the client says so before every install.
- exteraGram and AyuGram for Android plugins (`.plugin`, Python) do not run
  in PrismGram: they use Android/Java APIs. AyuGram's info box for such
  files is kept.

## Package

A `.nexoplugin` file is a zip:

```
manifest.json
main.js
```

Limits: 5 MB per package, 1 MB per file, 64 files, plain relative paths
only (no `..`, no absolute paths). Files may sit in one top-level folder.
See `plugin-example/` and `plugin-example/pack.py`.

## Manifest

```json
{
  "id": "nexo.example.uppercase",
  "name": "Uppercase copy",
  "version": "1.0.0",
  "author": "PrismGram",
  "description": "What it does.",
  "source": "https://example.com/plugin",
  "api_version": 1,
  "min_client": "0.1.0",
  "max_client": "0.x",
  "entry": "main.js",
  "permissions": ["menu.message", "messages.read", "clipboard.write", "settings", "profile.banner"]
}
```

| Field | Rule |
|---|---|
| `id` | Required, `^[a-z][a-z0-9_.-]{1,63}$` |
| `name`, `author` | Required |
| `version` | Required, `1`, `1.2` or `1.2.3` |
| `entry` | Required, a `.js` file in the package root |
| `api_version` | Required, must be `1` |
| `min_client`, `max_client` | Optional PrismGram versions, `x` allowed in `max_client` |
| `source` | Optional, `https://` only; shown before install |
| `permissions` | Only the names below; an unknown name makes the plugin incompatible |

## API v1

Always available:

| Call | Does |
|---|---|
| `nexo.version` | PrismGram version string |
| `nexo.apiVersion` | `1` |
| `nexo.log(text)` | Appends to the plugin's log |
| `nexo.toast(text)` | Shows a toast; outside a user action it only goes to the log |

With permissions:

| Permission | Adds |
|---|---|
| `menu.message` | `nexo.menu.addMessageItem(text, callback)`: up to 8 items in the message context menu. `callback(message)` gets `{ messageId, chatId, chatTitle, outgoing, date }` |
| `messages.read` | `message.text` in that callback |
| `clipboard.write` | `nexo.copyText(text)`, only works right after the user clicked the plugin's item |
| `settings` | `nexo.settings.get(key, default)`, `nexo.settings.set(key, value)`; JSON values, 256 KB in total |
| `profile.banner` | `nexo.profile.setBannerResolver(fn)`: `fn(user)` gets `{ id, username, firstName, bio }` and returns an `https://` image link or `null`. The client downloads it (5 MB max, no local or IP hosts) only if that exact link is in the user's bio, and draws it as the profile header |

Lifecycle: the entry file runs once, then `onLoad()` if defined. When the
plugin is turned off, removed or the app quits, `onUnload()` if defined.

## Where things are stored

All under the user data folder (`%APPDATA%\PrismGram\tdata\nexo\`):

| Path | What |
|---|---|
| `plugins/<id>/` | Installed plugin files |
| `plugins.json` | Which plugins are turned on |
| `plugins-data/<id>.json` | Plugin settings; kept when a plugin is removed, updated or incompatible |
| `plugins-logs/<id>.log` | Plugin log, trimmed at 256 KB |

## Install, update, errors

- Settings > Plugins > Import plugin shows name, version, author, source
  (or "not specified, author not verified"), permissions and compatibility,
  and installs only after confirmation. A new plugin starts turned off.
- Installing a plugin with an id that is already installed updates it and
  keeps its on/off state and settings. Files go to a temporary folder first
  and the old version is put back if the swap fails.
- A failing menu action shows a toast and is logged; after 3 failures in a
  row the plugin is stopped and marked "Error" until turned on again.
- On start every plugin is checked against `api_version`, `min_client` and
  `max_client`. Incompatible ones do not run and show the reason; their
  files and settings stay.

## Versioning

- PrismGram version, base AyuGram version and plugin API version are
  separate (`nexo/nexo_version.h`).
- New calls and permissions can be added within API v1. Removing or
  changing anything bumps the API version.

## Not in v1

Network access, sending messages, chat/profile menus, plugin settings
pages in the UI, update checks for plugins. They need their own
permissions and review before being added.

## Example: profile banners

`plugin-example/profile-banners` shows banners in profiles. You set yours by
adding `banner: https://i.imgur.com/XXXX.jpg` (or any direct image link) to
your bio; everyone with the plugin sees it on your profile. The image host
sees the IP address of people who open your profile, as with any image link.
