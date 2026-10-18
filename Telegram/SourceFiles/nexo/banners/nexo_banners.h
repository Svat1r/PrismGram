// This file is part of Nexo Desk, based on AyuGram Desktop and
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

// "Banner" submenu in the ⋮ menu of a profile.
void AddMenuActions(
	not_null<PeerData*> peer,
	not_null<Window::SessionController*> controller,
	const ::Ui::Menu::MenuCallback &addAction);

} // namespace Nexo::Banners
