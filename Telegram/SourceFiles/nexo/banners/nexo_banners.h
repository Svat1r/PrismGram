// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Local banners, ported from the exteraGram plugin "Combined Custom Banner"
// (@KUNSESHBOY, @chestertech): a custom picture or the avatar as the
// background of profile headers. Everything stays on this computer, only
// you see these banners.
#pragma once

#include "ui/widgets/menu/menu_add_action_callback.h"

class PeerData;

namespace Window {
class SessionController;
} // namespace Window

namespace Nexo::Banners {

enum class Crop {
	Top,
	Center,
	Bottom,
};

struct Options {
	int blur = 20;      // 0..100
	int darken = 45;    // 0..100
	Crop crop = Crop::Center;
	bool profileAvatars = false; // Avatar as banner for every profile.

	// Chat header (top bar of an open chat).
	bool chatHeader = true;      // Show per-chat banners in the header.
	bool chatAvatars = false;    // Avatar as banner for every chat header.
	bool chatEverywhere = false; // The common picture in every chat.

	// Chat list header.
	bool listBanner = false;
	bool listWide = false;       // Also under the folder tabs.
	int foldersTransparency = 60; // 0 = opaque tabs, 100 = see-through.
};

enum class Common {
	Chats, // Picture for "chatEverywhere".
	List,  // Picture of the chat list header.
};

[[nodiscard]] Options CurrentOptions();
void SetOptions(const Options &options);
[[nodiscard]] rpl::producer<> Changes();

// 0 = top, 0.5 = center, 1 = bottom.
[[nodiscard]] float64 CropAlign();

// Banner for the profile header, effects applied: the custom picture or
// avatar chosen for this peer, else the avatar if "profileAvatars" is on,
// else a plugin banner (profile.banner). Null image when there is none.
[[nodiscard]] rpl::producer<QImage> ProfileValue(not_null<PeerData*> peer);

// Banner for the header of an open chat, effects applied, or null.
[[nodiscard]] rpl::producer<QImage> ChatValue(not_null<PeerData*> peer);

// Banner for the chat list header, effects applied, or null.
[[nodiscard]] rpl::producer<QImage> ListValue();

[[nodiscard]] bool HasCommon(Common which);
void ChooseCommon(
	Common which,
	not_null<Window::SessionController*> controller);
void RemoveCommon(Common which);

// Draws image as a center/top/bottom crop filling rect.
void PaintCover(QPainter &p, const QImage &image, QRect rect);

// "Banner" submenu in the ⋮ menu of a profile.
void AddMenuActions(
	not_null<PeerData*> peer,
	not_null<Window::SessionController*> controller,
	const ::Ui::Menu::MenuCallback &addAction);

} // namespace Nexo::Banners
