// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo {

// Nexo Desk's own version, separate from the base version below.
inline constexpr auto kVersion = "0.1.0";

// The AyuGram Desktop release this build is based on.
inline constexpr auto kBaseName = "AyuGram Desktop";
inline constexpr auto kBaseVersion = "7.2.9";

// Bumped only when the plugin API changes in an incompatible way.
inline constexpr auto kPluginApiVersion = 1;

// Nexo release counter for updates: increase by one for every release.
// Update versions are (base AppVersion << 32) | counter, so a new Nexo
// release on the same AyuGram base is newer, and a new base always is.
inline constexpr auto kUpdateCounter = 1;

// Where the update feed ("current6") and packages are published.
inline constexpr auto kUpdatesPrefix
	= "https://github.com/Svat1r/NexoDesk/releases/latest/download";

} // namespace Nexo
