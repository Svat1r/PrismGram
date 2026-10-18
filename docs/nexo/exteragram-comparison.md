# PrismGram: exteraGram vs AyuGram Desktop

Status: draft for approval, 2026-10-18.

## Sources

What was actually checked:

- exteraGram source code: https://github.com/exteraSquad/exteraGram — Android
  (Java/Kotlin, fork of Telegram for Android), GPL-2.0. The repository is
  archived, last push 2025-01-03. The list of settings below comes from its
  `ExteraConfig.java` at that commit.
- exteraGram plugin docs: https://plugins.exteragram.app/docs (Setup, Plugin
  Class, Xposed Method Hooking pages). Plugins need exteraGram 12.5.1+, which
  is newer than the archived source.
- exteraGram site: https://exteragram.app (feature overview only).
- Screenshots of the current exteraGram app sent by the user (bottom
  navigation bar, pill-shaped search with a proxy chip, settings layout).
- AyuGram Desktop 7.2.9, commit d4b7546, `ayu/ayu_settings.h` and the
  Telegram Desktop settings sections.

Not checked: features of current closed builds that are not in the archived
source, the docs or the screenshots. Nothing below is claimed for them.

## Architecture

exteraGram is an Android app (Java, Android SDK). AyuGram Desktop is C++/Qt.
They share no UI or platform code, so code is not merged or copied. PrismGram
takes ideas and UX from exteraGram and implements them on top of AyuGram,
in `Telegram/SourceFiles/nexo/`, with small hooks into upstream files.
Copying exteraGram code would also need a license check (GPL-2.0 vs the
GPLv3 of Telegram Desktop), which is one more reason to reimplement.

## Comparison

Legend: "AyuGram" column says what already exists in AyuGram Desktop or
Telegram Desktop. Conflict risk is about future AyuGram merges.

| Feature | In exteraGram | In AyuGram Desktop | Compatibility | Effort | Conflict risk | Priority |
|---|---|---|---|---|---|---|
| Python plugins | `.py`/`.plugin` files, `BasePlugin` class, Python runtime on Android | Only shows info for a `.plugin` file sent in chat, never runs it | Plugins call Android/Java APIs and Xposed hooks, impossible on desktop as-is | Very high | Low (separate module) | P1 own plugin API, P2 study a subset |
| Plugin settings and menu items | `create_settings`, `add_menu_item` (message, drawer, chat, profile menus) | None | Same concepts can exist in the Nexo plugin API | Medium | Low | P1 |
| Request/update hooks | `pre_request_hook`, `post_request_hook`, `on_update_hook`, `on_send_message_hook` by TL name | None | Idea maps to the desktop MTProto layer; security sensitive | High | Medium (mtproto code) | P2 |
| Java method hooking | Xposed-style `hook_method` on Telegram Android classes | None | No equivalent; will not be supported | n/a | n/a | Not planned |
| Bottom navigation bar | In current app (user screenshots), not in archived source | None | Own implementation | Medium | Low | Done in code, not built yet |
| Pill search with proxy chip | In current app (user screenshots) | Search field; proxy box in Advanced settings | Own implementation | Low | Low | P1 |
| Hide "All chats" folder | `hideAllChats` | `hideAllChatsFolder` | Already there | — | — | Keep |
| Folder tab counters, icons, style | `tabCounter`, `tabIcons`, `tabStyle` | Folder rail with icons and counters (Telegram Desktop) | Visual restyle only | Low | Low | P1 (pill style) |
| Avatar corners | `avatarCorners` | `avatarCorners`, `singleCornerRadius` | Already there | — | — | Keep |
| New switch style | `newSwitchStyle` | `materialSwitches` | Already there | — | — | Keep |
| System fonts | `useSystemFonts` | Font customization in AyuGram | Already there | — | — | Keep |
| Centered title, custom title text | `centerTitle`, `titleText` | None | Own implementation | Low | Low | P2 |
| Icon pack | `useSolarIcons` | None | Needs a full own icon set | High | Medium (style files) | P2 |
| Blur, snow, no dividers | `forceBlur`, `forceSnow`, `disableDividers` | None | Own implementation | Low to medium | Low | P2 |
| Seconds in message time | `formatTimeWithSeconds` | `showMessageSeconds` | Already there | — | — | Keep |
| No number rounding | `disableNumberRounding` | None found | Own implementation | Low | Low | P2 |
| Show ID and DC, hide phone | `showIdAndDc`, `hidePhoneNumber` | Streamer mode hides data; ID display to be checked in the running app | Partly | Low | Low | P2 |
| Extra message menu buttons | copy photo, save, details, history, report, clear | AyuGram context menu has history and details; to be checked | Partly | Low | Medium (history menu code) | P2 |
| Double tap action | `doubleTapAction` | Telegram Desktop quick action on double click; to be checked | Partly | Low | Low | P2 |
| Sticker size and shape, hide sticker time | `stickerSize`, `stickerShape`, `hideStickerTime` | None found | Own implementation | Medium | Medium (history view code) | P2 |
| Unlimited recent stickers | `unlimitedRecentStickers` | `unlimitedRecentStickers` | Already there | — | — | Keep |
| Admin shortcuts in profile | permissions, admins, members, recent actions | `quickAdminShortcuts` | Already there | — | — | Keep |
| Comma after mention | `addCommaAfterMention` | None found | Own implementation | Low | Low | P2 |
| Photo send quality | `sendPhotosQuality` | HD photo sending in Telegram Desktop | Already there | — | — | Keep |
| Download/upload speed boost | `downloadSpeedBoost`, `uploadSpeedBoost`, marked experimental by exteraGram | None | Possible via more parallel parts; risk of server limits | Medium | Medium (storage/file loader) | P2 experimental, off by default |
| Translator | `targetLanguage` | AyuGram translator | Already there | — | — | Keep |
| Dynamic (Material You) theme | Android 12+ only | "Use system accent color" in Telegram Desktop | Desktop analog exists | — | — | Keep |
| Update check | `checkUpdatesOnLaunch`, own channel | Signed updater (Telegram Desktop v2 format) | Needs own keys and server | Medium | Medium | P0 |
| Camera, archive on pull, keyboard, tablet mode | Android-only | n/a | Not applicable to desktop | — | — | Not planned |
| Settings search | Not in archived config | Settings search in Telegram Desktop; Nexo sections register keywords | Already there | — | — | Keep |
| Custom keyboard shortcuts | Not found | Shortcuts settings section in Telegram Desktop | Already there | — | — | Keep |
| Animation settings | Not found | Power saving / animation settings in Telegram Desktop | Already there | — | — | Keep |

## Features system (18.3)

Each Nexo feature lives in its own folder under `nexo/`, has its own entry
in `Nexo::Settings` with a default, and is switched off cleanly when its
setting is off. Experimental features get an "Experimental" group in
settings and default to off. A future step is a registry where every
feature declares id, title, description, settings and an "experimental"
flag, which the plugin API can extend.

## Proposed order

1. P0: build, rebrand, own updater keys, keep user data and AyuGram features.
2. P1: Connection section (done in code), Appearance section with Nexo
   themes, bottom bar (done in code), pill search with proxy chip, folder
   tab restyle, plugin manager with manifests, plugin API docs.
3. P2: the rows marked P2 above, one small change at a time.
