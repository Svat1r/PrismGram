// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo::Backup {

// A backup is a zip (.nexobackup) with backup.json and copies of an
// explicit list of settings files from tdata. Account data (keys,
// sessions, message cache) is never written or read.
struct Summary {
	QString version;   // Nexo Desk version that made it.
	QString created;   // ISO date.
	int files = 0;
	bool plugins = false;
	QString error;     // Empty if the backup can be imported.
};

bool Export(const QString &path, QString *error);
[[nodiscard]] Summary Inspect(const QString &path);
// Writes the files; the app must restart to load them.
bool Import(const QString &path, QString *error);

} // namespace Nexo::Backup
