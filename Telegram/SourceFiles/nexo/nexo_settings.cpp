// This file is part of Nexo Desk, based on AyuGram Desktop and
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

Settings &Settings::Instance() {
	static auto instance = Settings();
	return instance;
}

Settings::Settings() {
	load();
}

bool Settings::bottomNavBar() const {
	return _bottomNavBar.current();
}

rpl::producer<bool> Settings::bottomNavBarValue() const {
	return _bottomNavBar.value();
}

void Settings::setBottomNavBar(bool enabled) {
	if (_bottomNavBar.current() == enabled) {
		return;
	}
	_bottomNavBar = enabled;
	save();
}

bool Settings::proxyChip() const {
	return _proxyChip.current();
}

rpl::producer<bool> Settings::proxyChipValue() const {
	return _proxyChip.value();
}

void Settings::setProxyChip(bool enabled) {
	if (_proxyChip.current() == enabled) {
		return;
	}
	_proxyChip = enabled;
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
	const auto bottomNavBar = object.value(u"bottom_nav_bar"_q);
	if (bottomNavBar.isBool()) {
		_bottomNavBar = bottomNavBar.toBool();
	}
	const auto proxyChip = object.value(u"proxy_chip"_q);
	if (proxyChip.isBool()) {
		_proxyChip = proxyChip.toBool();
	}
}

void Settings::save() const {
	QDir().mkpath(SettingsDir());
	auto object = QJsonObject();
	object.insert(u"version"_q, 1);
	object.insert(u"bottom_nav_bar"_q, _bottomNavBar.current());
	object.insert(u"proxy_chip"_q, _proxyChip.current());

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
