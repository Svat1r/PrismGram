// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/backup/settings_backup.h"

#include "nexo/nexo_version.h"
#include "nexo/nexo_zip.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>

namespace Nexo::Backup {
namespace {

constexpr auto kFormat = 1;
constexpr auto kMaxFileSize = 5 * 1024 * 1024;
constexpr auto kMaxArchiveSize = qint64(50 * 1024 * 1024);
constexpr auto kMaxFiles = 2000;

// Paths relative to tdata/. Only these are ever exported or imported.
const auto kFiles = {
	"nexo/settings.json",
	"nexo/plugins.json",
	"ayu_settings.json",
	"experimental_options.json",
};
const auto kFolders = {
	"nexo/plugins/",
	"nexo/plugins-data/",
};

[[nodiscard]] QString TData() {
	return cWorkingDir() + u"tdata/"_q;
}

[[nodiscard]] bool Allowed(const QString &relative) {
	if (!Zip::IsSafeRelativePath(relative)) {
		return false;
	}
	for (const auto file : kFiles) {
		if (relative == QString::fromLatin1(file)) {
			return true;
		}
	}
	for (const auto folder : kFolders) {
		const auto prefix = QString::fromLatin1(folder);
		if (relative.startsWith(prefix) && relative.size() > prefix.size()) {
			// Skip unfinished plugin installs and backups.
			return !relative.mid(prefix.size()).startsWith('.');
		}
	}
	return false;
}

} // namespace

bool Export(const QString &path, QString *error) {
	auto files = std::vector<std::pair<QString, QByteArray>>();
	auto list = QJsonArray();
	const auto add = [&](const QString &relative) {
		if (!Allowed(relative)) {
			return;
		}
		auto file = QFile(TData() + relative);
		if (file.size() > kMaxFileSize || !file.open(QIODevice::ReadOnly)) {
			return;
		}
		files.emplace_back(u"tdata/"_q + relative, file.readAll());
		list.push_back(relative);
	};
	for (const auto file : kFiles) {
		add(QString::fromLatin1(file));
	}
	for (const auto folder : kFolders) {
		const auto root = TData() + QString::fromLatin1(folder);
		auto it = QDirIterator(
			root,
			QDir::Files,
			QDirIterator::Subdirectories);
		while (it.hasNext() && int(files.size()) < kMaxFiles) {
			const auto absolute = it.next();
			add(QString::fromLatin1(folder)
				+ QDir(root).relativeFilePath(absolute));
		}
	}
	const auto meta = QJsonObject{
		{ u"format"_q, kFormat },
		{ u"app"_q, u"PrismGram"_q },
		{ u"version"_q, QString::fromLatin1(kVersion) },
		{ u"created"_q, QDateTime::currentDateTimeUtc().toString(Qt::ISODate) },
		{ u"files"_q, list },
	};
	files.insert(
		files.begin(),
		{ u"backup.json"_q, QJsonDocument(meta).toJson() });
	if (!Zip::Write(path, files)) {
		if (error) {
			*error = u"Could not write the backup file."_q;
		}
		return false;
	}
	return true;
}

Summary Inspect(const QString &path) {
	auto result = Summary();
	const auto archive = Zip::Read(path, Zip::Limits{
		.maxArchiveSize = kMaxArchiveSize,
		.maxFileSize = kMaxFileSize,
		.maxFiles = kMaxFiles + 1,
	});
	if (!archive.error.isEmpty()) {
		result.error = archive.error;
		return result;
	}
	const auto meta = Zip::Find(archive, u"backup.json"_q);
	const auto object = meta
		? QJsonDocument::fromJson(*meta).object()
		: QJsonObject();
	const auto app = object.value(u"app"_q).toString();
	if (app != u"PrismGram"_q && app != u"Nexo Desk"_q) {
		result.error = u"This is not a PrismGram backup."_q;
		return result;
	} else if (object.value(u"format"_q).toInt() != kFormat) {
		result.error = u"Unsupported backup format."_q;
		return result;
	}
	result.version = object.value(u"version"_q).toString();
	result.created = object.value(u"created"_q).toString();
	for (const auto &[name, content] : archive.files) {
		if (name == u"backup.json"_q) {
			continue;
		} else if (!name.startsWith(u"tdata/"_q)
			|| !Allowed(name.mid(6))) {
			result.error = u"Unexpected file in the backup: %1"_q.arg(name);
			return result;
		}
		++result.files;
		if (name.startsWith(u"tdata/nexo/plugins/"_q)) {
			result.plugins = true;
		}
	}
	return result;
}

bool Import(const QString &path, QString *error) {
	const auto fail = [&](const QString &text) {
		if (error) {
			*error = text;
		}
		return false;
	};
	const auto summary = Inspect(path);
	if (!summary.error.isEmpty()) {
		return fail(summary.error);
	}
	const auto archive = Zip::Read(path, Zip::Limits{
		.maxArchiveSize = kMaxArchiveSize,
		.maxFileSize = kMaxFileSize,
		.maxFiles = kMaxFiles + 1,
	});
	for (const auto &[name, content] : archive.files) {
		if (name == u"backup.json"_q) {
			continue;
		}
		const auto relative = name.mid(6);
		if (!Allowed(relative)) {
			return fail(u"Unexpected file in the backup: %1"_q.arg(name));
		}
		const auto target = TData() + relative;
		QDir().mkpath(QFileInfo(target).absolutePath());
		auto file = QSaveFile(target);
		if (!file.open(QIODevice::WriteOnly)
			|| file.write(content) != content.size()
			|| !file.commit()) {
			return fail(u"Could not write %1."_q.arg(relative));
		}
	}
	return true;
}

} // namespace Nexo::Backup
