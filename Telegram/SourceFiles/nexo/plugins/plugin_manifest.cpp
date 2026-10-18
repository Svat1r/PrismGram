// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/plugins/plugin_manifest.h"

#include "nexo/nexo_version.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>

namespace Nexo::Plugins {
namespace {

constexpr auto kMaxFieldLength = 200;
constexpr auto kMaxDescriptionLength = 4000;

[[nodiscard]] bool IsVersion(const QString &value, bool allowX) {
	static const auto plain = QRegularExpression(
		u"^\d{1,5}(\.\d{1,5}){0,2}$"_q);
	static const auto withX = QRegularExpression(
		u"^\d{1,5}(\.(\d{1,5}|x)){0,2}$"_q);
	return (allowX ? withX : plain).match(value).hasMatch();
}

} // namespace

const QStringList &KnownPermissions() {
	static const auto result = QStringList{
		QString::fromLatin1(kPermissionMenuMessage),
		QString::fromLatin1(kPermissionSettings),
		QString::fromLatin1(kPermissionMessagesRead),
		QString::fromLatin1(kPermissionClipboard),
	};
	return result;
}

int CompareVersions(const QString &a, const QString &b) {
	const auto left = a.split('.');
	const auto right = b.split('.');
	for (auto i = 0; i != 3; ++i) {
		const auto r = (i < right.size()) ? right[i] : u"0"_q;
		if (r == u"x"_q) {
			return 0;
		}
		const auto l = (i < left.size()) ? left[i].toInt() : 0;
		const auto diff = l - r.toInt();
		if (diff) {
			return diff;
		}
	}
	return 0;
}

ManifestResult ParseManifest(const QByteArray &json) {
	auto error = QJsonParseError();
	const auto document = QJsonDocument::fromJson(json, &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) {
		return { .error = u"manifest.json is not a JSON object"_q };
	}
	const auto object = document.object();
	const auto string = [&](const char *key) {
		return object.value(QString::fromLatin1(key)).toString().trimmed();
	};
	auto manifest = Manifest{
		.id = string("id"),
		.name = string("name").left(kMaxFieldLength),
		.version = string("version"),
		.author = string("author").left(kMaxFieldLength),
		.description = string("description").left(kMaxDescriptionLength),
		.source = string("source"),
		.entry = string("entry"),
		.minClient = string("min_client"),
		.maxClient = string("max_client"),
		.apiVersion = object.value(u"api_version"_q).toInt(),
	};
	for (const auto &value : object.value(u"permissions"_q).toArray()) {
		manifest.permissions.push_back(value.toString());
	}
	manifest.permissions.removeDuplicates();

	static const auto idRegex = QRegularExpression(
		u"^[a-z][a-z0-9_.-]{1,63}$"_q);
	static const auto entryRegex = QRegularExpression(
		u"^[A-Za-z0-9_-]{1,64}\.js$"_q);
	const auto fail = [&](QString text) {
		return ManifestResult{ .manifest = manifest, .error = text };
	};
	if (!idRegex.match(manifest.id).hasMatch()) {
		return fail(u"\"id\" must match ^[a-z][a-z0-9_.-]{1,63}$"_q);
	} else if (manifest.name.isEmpty() || manifest.author.isEmpty()) {
		return fail(u"\"name\" and \"author\" are required"_q);
	} else if (!IsVersion(manifest.version, false)) {
		return fail(u"\"version\" must look like 1.2.3"_q);
	} else if (!entryRegex.match(manifest.entry).hasMatch()) {
		return fail(u"\"entry\" must be a .js file name in the plugin root"_q);
	} else if (manifest.apiVersion <= 0) {
		return fail(u"\"api_version\" is required"_q);
	} else if (!manifest.minClient.isEmpty()
		&& !IsVersion(manifest.minClient, false)) {
		return fail(u"\"min_client\" must look like 0.1.0"_q);
	} else if (!manifest.maxClient.isEmpty()
		&& !IsVersion(manifest.maxClient, true)) {
		return fail(u"\"max_client\" must look like 0.x or 1.2.0"_q);
	} else if (!manifest.source.isEmpty()
		&& !manifest.source.startsWith(u"https://"_q)) {
		return fail(u"\"source\" must be an https:// link"_q);
	}
	return { .manifest = manifest };
}

QString CheckCompatibility(const Manifest &manifest) {
	const auto client = QString::fromLatin1(kVersion);
	if (manifest.apiVersion != kPluginApiVersion) {
		return u"Needs plugin API %1, this client has %2."_q
			.arg(manifest.apiVersion)
			.arg(kPluginApiVersion);
	} else if (!manifest.minClient.isEmpty()
		&& CompareVersions(client, manifest.minClient) < 0) {
		return u"Needs Nexo Desk %1 or newer."_q.arg(manifest.minClient);
	} else if (!manifest.maxClient.isEmpty()
		&& CompareVersions(client, manifest.maxClient) > 0) {
		return u"Supports Nexo Desk up to %1."_q.arg(manifest.maxClient);
	}
	for (const auto &permission : manifest.permissions) {
		if (!KnownPermissions().contains(permission)) {
			return u"Unknown permission \"%1\"."_q.arg(permission);
		}
	}
	return QString();
}

} // namespace Nexo::Plugins
