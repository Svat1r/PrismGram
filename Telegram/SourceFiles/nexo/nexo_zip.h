// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo::Zip {

struct Limits {
	qint64 maxArchiveSize = 5 * 1024 * 1024;
	int maxFileSize = 1024 * 1024;
	int maxFiles = 64;
};

struct Archive {
	std::vector<std::pair<QString, QByteArray>> files;
	QString error; // Empty on success.
};

// Reads a zip with plain relative paths only (no "..", no absolute or
// backslash paths). If every file sits in one top-level folder, that
// folder is stripped.
[[nodiscard]] Archive Read(const QString &path, const Limits &limits);

[[nodiscard]] const QByteArray *Find(
	const Archive &archive,
	const QString &name);

// Writes the files into a zip at path. False on error.
bool Write(
	const QString &path,
	const std::vector<std::pair<QString, QByteArray>> &files);

[[nodiscard]] bool IsSafeRelativePath(const QString &path);

} // namespace Nexo::Zip
