// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

class PeerData;

namespace Main {
class Session;
} // namespace Main

namespace Window {
class SessionController;
} // namespace Window

namespace Nexo::Hidden {

// Hidden chats: left out of the chat list and the search until the PIN
// is entered; hidden again a minute after PrismGram leaves the
// foreground. Stored in tdata/nexo/hidden.json, the PIN only as a salted
// SHA-256 hash. Hides chats on screen, it is not encryption.

[[nodiscard]] bool IsHidden(not_null<PeerData*> peer);
[[nodiscard]] bool ShouldHide(not_null<PeerData*> peer);
[[nodiscard]] bool Active(not_null<Main::Session*> session);
[[nodiscard]] bool Locked();
[[nodiscard]] int Count(not_null<Main::Session*> session);
[[nodiscard]] rpl::producer<> Changes();

void Lock();
void Unlock(not_null<Window::SessionController*> controller, Fn<void()> done);
void Toggle(not_null<Window::SessionController*> controller, not_null<PeerData*> peer);
void ChangePin(not_null<Window::SessionController*> controller);

} // namespace Nexo::Hidden
