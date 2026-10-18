// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo {

// On/off options of Nexo Desk features.
enum class Flag {
	BottomNavBar,
	ProxyChip,
	NoNumberRounding,
	CommaAfterMention,
	CenteredTitle,
	SpeedBoost,
};

// Client-level preferences of Nexo Desk. Stored as JSON in
// <working dir>/tdata/nexo/settings.json, next to the rest of the user
// data, so it survives reinstalls and updates. Holds no secrets.
class Settings final {
public:
	[[nodiscard]] static Settings &Instance();

	[[nodiscard]] bool get(Flag flag) const;
	[[nodiscard]] rpl::producer<bool> value(Flag flag) const;
	void set(Flag flag, bool enabled);

	// Shortcuts kept for existing callers.
	[[nodiscard]] bool bottomNavBar() const {
		return get(Flag::BottomNavBar);
	}
	[[nodiscard]] rpl::producer<bool> bottomNavBarValue() const {
		return value(Flag::BottomNavBar);
	}
	void setBottomNavBar(bool enabled) {
		set(Flag::BottomNavBar, enabled);
	}
	[[nodiscard]] bool proxyChip() const {
		return get(Flag::ProxyChip);
	}
	[[nodiscard]] rpl::producer<bool> proxyChipValue() const {
		return value(Flag::ProxyChip);
	}
	void setProxyChip(bool enabled) {
		set(Flag::ProxyChip, enabled);
	}

private:
	struct Entry {
		Flag flag;
		const char *key;
		bool defaultValue;
	};

	Settings();

	void load();
	void save() const;
	[[nodiscard]] static const std::vector<Entry> &Entries();

	base::flat_map<Flag, std::unique_ptr<rpl::variable<bool>>> _values;

};

} // namespace Nexo
