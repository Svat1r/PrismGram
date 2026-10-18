// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "nexo/plugins/plugin_manifest.h"

namespace Nexo::Plugins {

class Runtime;

enum class State {
	Disabled,
	Running,
	Incompatible,
	Broken,     // Bad package or manifest.
	Error,      // Failed while running.
};

struct Plugin {
	QString folder; // Folder name inside the plugins directory.
	Manifest manifest;
	State state = State::Disabled;
	QString error;
	bool enabled = false;
};

struct PackageInfo {
	Manifest manifest;
	QString error;          // Why it can't be installed, empty if it can.
	QString compatibility;  // Why it won't run, empty if it will.
	bool replaces = false;  // A plugin with this id is installed.
};

struct MessageMenuItem {
	QString pluginId;
	QString pluginName;
	int itemId = 0;
	QString text;
};

// Owns installed plugins and their runtimes. Lives for the whole app.
// Everything stays in <working dir>/tdata/nexo/: plugins/<id>/ for code,
// plugins-data/<id>.json for settings, plugins-logs/<id>.log for logs,
// plugins.json for which plugins are enabled.
class Manager final {
public:
	[[nodiscard]] static Manager &Instance();

	void start();

	[[nodiscard]] const std::vector<Plugin> &list() const;
	[[nodiscard]] rpl::producer<> changes() const;

	void setEnabled(const QString &id, bool enabled);
	bool remove(const QString &id, QString *error);

	// Reads a .zip / .nexoplugin package without installing it.
	[[nodiscard]] PackageInfo inspect(const QString &path) const;
	bool install(const QString &path, QString *error);

	[[nodiscard]] QString pluginsPath() const;
	[[nodiscard]] QString logPath(const QString &id) const;
	[[nodiscard]] QString readLog(const QString &id) const;

	[[nodiscard]] std::vector<MessageMenuItem> messageMenuItems() const;
	[[nodiscard]] bool hasPermission(
		const QString &id,
		const char *permission) const;
	// First non-empty banner link from running "profile.banner" plugins.
	[[nodiscard]] QString resolveBanner(const QByteArray &user);

	void activate(
		const MessageMenuItem &item,
		const QByteArray &context,
		Fn<void(const QString &)> toast);

	~Manager();

private:
	Manager();

	void scan();
	void startPlugin(Plugin &plugin);
	void stopPlugin(Plugin &plugin);
	void saveEnabled() const;
	void log(const QString &id, const QString &text) const;
	[[nodiscard]] Plugin *find(const QString &id);

	std::vector<Plugin> _list;
	base::flat_map<QString, std::unique_ptr<Runtime>> _runtimes;
	base::flat_map<QString, int> _failures;
	Fn<void(const QString &)> _toast;
	rpl::event_stream<> _changes;
	bool _started = false;

};

[[nodiscard]] QString StateText(const Plugin &plugin);

} // namespace Nexo::Plugins
