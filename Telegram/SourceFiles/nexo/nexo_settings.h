// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo {

// Client-level interface preferences of Nexo Desk. Stored as JSON in
// <working dir>/tdata/nexo/settings.json, next to the rest of the user
// data, so it survives reinstalls and updates. Holds no secrets.
class Settings final {
public:
	[[nodiscard]] static Settings &Instance();

	[[nodiscard]] bool bottomNavBar() const;
	[[nodiscard]] rpl::producer<bool> bottomNavBarValue() const;
	void setBottomNavBar(bool enabled);

private:
	Settings();

	void load();
	void save() const;

	rpl::variable<bool> _bottomNavBar = true;

};

} // namespace Nexo
