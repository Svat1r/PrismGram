// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo::Plugins {

struct Manifest {
	QString id;
	QString name;
	QString version;
	QString author;
	QString description;
	QString source;
	QString entry;
	QString minClient;
	QString maxClient;
	int apiVersion = 0;
	QStringList permissions;
};

struct ManifestResult {
	Manifest manifest;
	QString error; // Empty when the manifest is valid.
};

// Permission names of plugin API v1.
inline constexpr auto kPermissionMenuMessage = "menu.message";
inline constexpr auto kPermissionSettings = "settings";
inline constexpr auto kPermissionMessagesRead = "messages.read";
inline constexpr auto kPermissionClipboard = "clipboard.write";

[[nodiscard]] const QStringList &KnownPermissions();
[[nodiscard]] ManifestResult ParseManifest(const QByteArray &json);

// Empty if the plugin can run in this client, otherwise the reason.
[[nodiscard]] QString CheckCompatibility(const Manifest &manifest);

// Compares dotted versions; 'x' in b matches anything ("0.x").
// Returns <0, 0 or >0.
[[nodiscard]] int CompareVersions(const QString &a, const QString &b);

} // namespace Nexo::Plugins
