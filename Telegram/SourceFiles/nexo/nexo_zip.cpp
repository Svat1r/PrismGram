// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/nexo_zip.h"

#include "base/zlib_help.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>

namespace Nexo::Zip {

bool IsSafeRelativePath(const QString &path) {
	static const auto regex = QRegularExpression(
		u"^[A-Za-z0-9_.-]+(/[A-Za-z0-9_.-]+)*$"_q);
	if (!regex.match(path).hasMatch()) {
		return false;
	}
	for (const auto &part : path.split('/')) {
		if (part == u"."_q || part == u".."_q) {
			return false;
		}
	}
	return true;
}

Archive Read(const QString &path, const Limits &limits) {
	auto result = Archive();
	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly)) {
		result.error = u"Could not open the file."_q;
		return result;
	} else if (file.size() > limits.maxArchiveSize) {
		result.error = u"The file is too large."_q;
		return result;
	}
	auto zip = zlib::FileToRead(file.readAll());
	if (zip.goToFirstFile() != UNZ_OK) {
		result.error = u"Not a zip archive."_q;
		return result;
	}
	do {
		const auto name = zip.getCurrentFileName();
		if (name.isEmpty()) {
			result.error = u"Bad file name in the archive."_q;
			return result;
		} else if (name.endsWith('/')) {
			continue; // Folder entry.
		} else if (!IsSafeRelativePath(name)) {
			result.error = u"Unsafe file path in the archive: %1"_q.arg(name);
			return result;
		} else if (int(result.files.size()) >= limits.maxFiles) {
			result.error = u"Too many files in the archive."_q;
			return result;
		}
		auto content = zip.readCurrentFileContent(limits.maxFileSize);
		if (zip.error() != UNZ_OK) {
			result.error = u"Could not read %1 (too large?)"_q.arg(name);
			return result;
		}
		result.files.emplace_back(name, std::move(content));
	} while (zip.goToNextFile() == UNZ_OK);

	if (!result.files.empty()) {
		const auto first = result.files.front().first;
		const auto slash = first.indexOf('/');
		if (slash > 0) {
			const auto prefix = first.left(slash + 1);
			const auto all = ranges::all_of(result.files, [&](const auto &f) {
				return f.first.startsWith(prefix);
			});
			if (all) {
				for (auto &[name, content] : result.files) {
					name = name.mid(prefix.size());
				}
			}
		}
	}
	return result;
}

const QByteArray *Find(const Archive &archive, const QString &name) {
	for (const auto &[path, content] : archive.files) {
		if (path == name) {
			return &content;
		}
	}
	return nullptr;
}

bool Write(
		const QString &path,
		const std::vector<std::pair<QString, QByteArray>> &files) {
	auto zip = zlib::FileToWrite();
	for (const auto &[name, content] : files) {
		const auto utf8 = name.toUtf8();
		zip_fileinfo info = { { 0, 0, 0, 0, 0, 0 }, 0, 0, 0 };
		zip.openNewFile(
			utf8.constData(),
			&info,
			nullptr,
			0,
			nullptr,
			0,
			nullptr,
			Z_DEFLATED,
			Z_DEFAULT_COMPRESSION);
		zip.writeInFile(content.constData(), unsigned(content.size()));
		zip.closeFile();
	}
	zip.close();
	if (zip.error() != ZIP_OK) {
		return false;
	}
	auto out = QSaveFile(path);
	const auto bytes = zip.result();
	return out.open(QIODevice::WriteOnly)
		&& (out.write(bytes) == bytes.size())
		&& out.commit();
}

} // namespace Nexo::Zip
