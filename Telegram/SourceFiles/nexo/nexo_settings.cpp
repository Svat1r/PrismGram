// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/nexo_settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

namespace Nexo {
namespace {

constexpr auto kMaxFileSize = qint64(64 * 1024);

[[nodiscard]] QString SettingsDir() {
	return cWorkingDir() + u"tdata/nexo/"_q;
}

[[nodiscard]] QString SettingsPath() {
	return SettingsDir() + u"settings.json"_q;
}

} // namespace

const std::vector<Settings::Entry> &Settings::Entries() {
	// Keys are stored in settings.json; never rename them.
	static const auto result = std::vector<Entry>{
		{ Flag::BottomNavBar, "bottom_nav_bar", true },
		{ Flag::ProxyChip, "proxy_chip", true },
		{ Flag::NoNumberRounding, "no_number_rounding", false },
		{ Flag::CommaAfterMention, "comma_after_mention", false },
		{ Flag::CenteredTitle, "centered_title", false },
		{ Flag::SpeedBoost, "speed_boost", false },
	};
	return result;
}

Settings &Settings::Instance() {
	static auto instance = Settings();
	return instance;
}

Settings::Settings() {
	for (const auto &entry : Entries()) {
		_values.emplace(
			entry.flag,
			std::make_unique<rpl::variable<bool>>(entry.defaultValue));
	}
	load();
}

bool Settings::get(Flag flag) const {
	return _values.find(flag)->second->current();
}

rpl::producer<bool> Settings::value(Flag flag) const {
	return _values.find(flag)->second->value();
}

void Settings::set(Flag flag, bool enabled) {
	auto &variable = *_values.find(flag)->second;
	if (variable.current() == enabled) {
		return;
	}
	variable = enabled;
	save();
}

void Settings::load() {
	auto file = QFile(SettingsPath());
	if (!file.exists()) {
		return;
	} else if (file.size() > kMaxFileSize
		|| !file.open(QIODevice::ReadOnly)) {
		LOG(("Nexo Settings: could not read %1, using defaults."
			).arg(file.fileName()));
		return;
	}
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		LOG(("Nexo Settings: bad JSON in %1 (%2), using defaults."
			).arg(file.fileName(), error.errorString()));
		return;
	}
	// Unknown keys are ignored and missing keys keep their defaults,
	// so files from older and newer versions both load.
	const auto object = document.object();
	for (const auto &entry : Entries()) {
		const auto value = object.value(QString::fromLatin1(entry.key));
		if (value.isBool()) {
			*_values.find(entry.flag)->second = value.toBool();
		}
	}
}

void Settings::save() const {
	QDir().mkpath(SettingsDir());
	auto object = QJsonObject();
	object.insert(u"version"_q, 1);
	for (const auto &entry : Entries()) {
		object.insert(
			QString::fromLatin1(entry.key),
			_values.find(entry.flag)->second->current());
	}

	// QSaveFile writes to a temp file and renames it, so a crash while
	// saving never leaves a half-written settings file.
	auto file = QSaveFile(SettingsPath());
	if (!file.open(QIODevice::WriteOnly)
		|| file.write(QJsonDocument(object).toJson()) < 0
		|| !file.commit()) {
		LOG(("Nexo Settings: could not write %1.").arg(SettingsPath()));
	}
}

} // namespace Nexo
