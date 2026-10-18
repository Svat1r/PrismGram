// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/plugins/plugin_manager.h"

#include "lang_auto.h"
#include "nexo/nexo_zip.h"
#include "nexo/plugins/plugin_runtime.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QSaveFile>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

namespace Nexo::Plugins {
namespace {

constexpr auto kMaxPackageSize = qint64(5 * 1024 * 1024);
constexpr auto kMaxFileSize = 1024 * 1024;
constexpr auto kMaxFiles = 64;
constexpr auto kMaxManifestSize = 64 * 1024;
constexpr auto kMaxSettingsSize = 256 * 1024;
constexpr auto kMaxLogSize = qint64(256 * 1024);
constexpr auto kMaxFailures = 3;

[[nodiscard]] QString NexoDir() {
	return cWorkingDir() + u"tdata/nexo/"_q;
}

[[nodiscard]] QString DataPath(const QString &id) {
	return NexoDir() + u"plugins-data/"_q + id + u".json"_q;
}

[[nodiscard]] QString EnabledPath() {
	return NexoDir() + u"plugins.json"_q;
}

[[nodiscard]] QByteArray ReadFile(const QString &path, qint64 limit) {
	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly) || file.size() > limit) {
		return QByteArray();
	}
	return file.readAll();
}

bool WriteFile(const QString &path, const QByteArray &content) {
	QDir().mkpath(QFileInfo(path).absolutePath());
	auto file = QSaveFile(path);
	return file.open(QIODevice::WriteOnly)
		&& (file.write(content) == content.size())
		&& file.commit();
}

[[nodiscard]] Zip::Archive ReadPackage(const QString &path) {
	return Zip::Read(path, Zip::Limits{
		.maxArchiveSize = kMaxPackageSize,
		.maxFileSize = kMaxFileSize,
		.maxFiles = kMaxFiles,
	});
}

} // namespace

Manager &Manager::Instance() {
	static auto instance = Manager();
	return instance;
}

Manager::Manager() = default;

Manager::~Manager() {
	for (auto &plugin : _list) {
		stopPlugin(plugin);
	}
}

QString Manager::pluginsPath() const {
	return NexoDir() + u"plugins/"_q;
}

QString Manager::logPath(const QString &id) const {
	return NexoDir() + u"plugins-logs/"_q + id + u".log"_q;
}

QString Manager::readLog(const QString &id) const {
	return QString::fromUtf8(ReadFile(logPath(id), kMaxLogSize * 2));
}

void Manager::log(const QString &id, const QString &text) const {
	const auto path = logPath(id);
	QDir().mkpath(QFileInfo(path).absolutePath());
	auto file = QFile(path);
	if (file.size() > kMaxLogSize && file.open(QIODevice::ReadOnly)) {
		const auto tail = file.readAll().right(kMaxLogSize / 2);
		file.close();
		WriteFile(path, tail);
	}
	if (file.open(QIODevice::Append)) {
		const auto stamp = QDateTime::currentDateTime().toString(
			u"yyyy-MM-dd HH:mm:ss"_q);
		file.write((u"[%1] %2\n"_q.arg(stamp, text)).toUtf8());
	}
}

void Manager::start() {
	if (_started) {
		return;
	}
	_started = true;
	scan();
	for (auto &plugin : _list) {
		startPlugin(plugin);
	}
	_changes.fire({});
}

const std::vector<Plugin> &Manager::list() const {
	return _list;
}

rpl::producer<> Manager::changes() const {
	return _changes.events();
}

Plugin *Manager::find(const QString &id) {
	const auto i = ranges::find(_list, id, [](const Plugin &p) {
		return p.manifest.id;
	});
	return (i != end(_list)) ? &*i : nullptr;
}

void Manager::scan() {
	auto enabled = QStringList();
	const auto saved = QJsonDocument::fromJson(
		ReadFile(EnabledPath(), kMaxManifestSize));
	for (const auto &value : saved.object().value(u"enabled"_q).toArray()) {
		enabled.push_back(value.toString());
	}

	_list.clear();
	const auto folders = QDir(pluginsPath()).entryList(
		QDir::Dirs | QDir::NoDotAndDotDot,
		QDir::Name);
	for (const auto &folder : folders) {
		if (folder.startsWith('.')) {
			continue; // Unfinished install.
		}
		auto plugin = Plugin{ .folder = folder };
		const auto parsed = ParseManifest(ReadFile(
			pluginsPath() + folder + u"/manifest.json"_q,
			kMaxManifestSize));
		plugin.manifest = parsed.manifest;
		if (plugin.manifest.id.isEmpty()) {
			plugin.manifest.id = folder;
		}
		if (plugin.manifest.name.isEmpty()) {
			plugin.manifest.name = folder;
		}
		plugin.enabled = enabled.contains(plugin.manifest.id);
		if (!parsed.error.isEmpty()) {
			plugin.state = State::Broken;
			plugin.error = parsed.error;
		} else if (parsed.manifest.id != folder) {
			plugin.state = State::Broken;
			plugin.error = u"Folder name must match the plugin id."_q;
		} else if (const auto reason = CheckCompatibility(parsed.manifest)
			; !reason.isEmpty()) {
			plugin.state = State::Incompatible;
			plugin.error = reason;
		}
		_list.push_back(std::move(plugin));
	}
}

void Manager::startPlugin(Plugin &plugin) {
	if (!plugin.enabled
		|| plugin.state == State::Broken
		|| plugin.state == State::Incompatible
		|| _runtimes.contains(plugin.manifest.id)) {
		return;
	}
	const auto id = plugin.manifest.id;
	const auto code = ReadFile(
		pluginsPath() + plugin.folder + '/' + plugin.manifest.entry,
		kMaxFileSize);
	if (code.isEmpty()) {
		plugin.state = State::Error;
		plugin.error = u"Could not read %1."_q.arg(plugin.manifest.entry);
		log(id, plugin.error);
		return;
	}
	auto host = Runtime::Host{
		.log = [=](const QString &text) { log(id, text); },
		.toast = [=](const QString &text) {
			if (_toast) {
				_toast(text);
			} else {
				log(id, u"toast (no window): "_q + text);
			}
		},
		.copyText = [=](const QString &text) {
			// Only right after the user clicked the plugin's item.
			if (_toast) {
				QGuiApplication::clipboard()->setText(text);
			} else {
				log(id, u"copyText ignored outside a user action"_q);
			}
		},
		.settingGet = [=](const QString &key) {
			const auto object = QJsonDocument::fromJson(
				ReadFile(DataPath(id), kMaxSettingsSize)).object();
			const auto value = object.value(key);
			return value.isUndefined()
				? QByteArray()
				: QJsonDocument(QJsonArray{ value }).toJson(
					QJsonDocument::Compact).mid(1).chopped(1);
		},
		.settingSet = [=](const QString &key, const QByteArray &json) {
			auto object = QJsonDocument::fromJson(
				ReadFile(DataPath(id), kMaxSettingsSize)).object();
			const auto wrapped = QJsonDocument::fromJson(
				'[' + json + ']').array();
			if (wrapped.isEmpty()) {
				object.remove(key);
			} else {
				object.insert(key, wrapped.first());
			}
			const auto content = QJsonDocument(object).toJson();
			if (content.size() > kMaxSettingsSize) {
				log(id, u"settings.set ignored: settings over 256 KB"_q);
				return;
			}
			WriteFile(DataPath(id), content);
		},
	};
	auto runtime = std::make_unique<Runtime>(plugin.manifest, host);
	auto error = QString();
	if (!runtime->start(code, &error)) {
		plugin.state = State::Error;
		plugin.error = error;
		log(id, u"Start failed: "_q + error);
		runtime->stop();
		return;
	}
	plugin.state = State::Running;
	plugin.error = QString();
	_failures.remove(id);
	_runtimes.emplace(id, std::move(runtime));
	log(id, u"Started, version "_q + plugin.manifest.version);
}

void Manager::stopPlugin(Plugin &plugin) {
	const auto i = _runtimes.find(plugin.manifest.id);
	if (i == end(_runtimes)) {
		return;
	}
	i->second->stop();
	_runtimes.erase(i);
	if (plugin.state == State::Running) {
		plugin.state = State::Disabled;
	}
}

void Manager::saveEnabled() const {
	auto array = QJsonArray();
	for (const auto &plugin : _list) {
		if (plugin.enabled) {
			array.push_back(plugin.manifest.id);
		}
	}
	WriteFile(
		EnabledPath(),
		QJsonDocument(QJsonObject{ { u"enabled"_q, array } }).toJson());
}

void Manager::setEnabled(const QString &id, bool enabled) {
	const auto plugin = find(id);
	if (!plugin) {
		return;
	}
	plugin->enabled = enabled;
	saveEnabled();
	if (enabled) {
		if (plugin->state == State::Error) {
			plugin->state = State::Disabled;
		}
		startPlugin(*plugin);
	} else {
		stopPlugin(*plugin);
		if (plugin->state == State::Error) {
			plugin->state = State::Disabled;
			plugin->error = QString();
		}
	}
	_changes.fire({});
}

bool Manager::remove(const QString &id, QString *error) {
	const auto plugin = find(id);
	if (!plugin) {
		return false;
	}
	stopPlugin(*plugin);
	if (!QDir(pluginsPath() + plugin->folder).removeRecursively()) {
		if (error) {
			*error = u"Could not delete the plugin folder."_q;
		}
		return false;
	}
	// Settings in plugins-data are kept, in case the plugin comes back.
	_list.erase(_list.begin() + (plugin - _list.data()));
	saveEnabled();
	_changes.fire({});
	return true;
}

PackageInfo Manager::inspect(const QString &path) const {
	auto result = PackageInfo();
	const auto package = ReadPackage(path);
	if (!package.error.isEmpty()) {
		result.error = package.error;
		return result;
	}
	const auto manifest = Zip::Find(package, u"manifest.json"_q);
	if (!manifest) {
		result.error = u"No manifest.json in the package."_q;
		return result;
	}
	const auto parsed = ParseManifest(*manifest);
	result.manifest = parsed.manifest;
	if (!parsed.error.isEmpty()) {
		result.error = parsed.error;
		return result;
	} else if (!Zip::Find(package, parsed.manifest.entry)) {
		result.error = u"No %1 in the package."_q.arg(parsed.manifest.entry);
		return result;
	}
	result.compatibility = CheckCompatibility(parsed.manifest);
	result.replaces = ranges::any_of(_list, [&](const Plugin &p) {
		return p.manifest.id == parsed.manifest.id;
	});
	return result;
}

bool Manager::install(const QString &path, QString *error) {
	const auto fail = [&](const QString &text) {
		if (error) {
			*error = text;
		}
		return false;
	};
	const auto info = inspect(path);
	if (!info.error.isEmpty()) {
		return fail(info.error);
	}
	const auto package = ReadPackage(path);
	const auto id = info.manifest.id;

	// Write everything to a temporary folder first, then swap folders,
	// so a failed install never leaves a half-written plugin behind.
	const auto temp = pluginsPath() + u".install-"_q + id + '/';
	QDir(temp).removeRecursively();
	for (const auto &[name, content] : package.files) {
		if (!WriteFile(temp + name, content)) {
			QDir(temp).removeRecursively();
			return fail(u"Could not write %1."_q.arg(name));
		}
	}

	auto wasEnabled = false;
	if (const auto existing = find(id)) {
		wasEnabled = existing->enabled;
		stopPlugin(*existing);
	}
	const auto target = pluginsPath() + id;
	const auto backup = pluginsPath() + u".backup-"_q + id;
	QDir(backup).removeRecursively();
	if (QDir(target).exists() && !QDir().rename(target, backup)) {
		QDir(temp).removeRecursively();
		return fail(u"Could not replace the installed version."_q);
	}
	if (!QDir().rename(temp, target)) {
		QDir().rename(backup, target); // Put the old version back.
		QDir(temp).removeRecursively();
		return fail(u"Could not move the plugin into place."_q);
	}
	QDir(backup).removeRecursively();
	log(id, u"Installed version "_q + info.manifest.version);

	scan();
	if (const auto plugin = find(id)) {
		// A new plugin starts disabled; an update keeps its old state.
		plugin->enabled = wasEnabled;
	}
	saveEnabled();
	for (auto &plugin : _list) {
		startPlugin(plugin);
	}
	_changes.fire({});
	return true;
}

std::vector<MessageMenuItem> Manager::messageMenuItems() const {
	auto result = std::vector<MessageMenuItem>();
	for (const auto &plugin : _list) {
		const auto i = _runtimes.find(plugin.manifest.id);
		if (i == end(_runtimes)) {
			continue;
		}
		for (const auto &item : i->second->menuItems()) {
			result.push_back({
				.pluginId = plugin.manifest.id,
				.pluginName = plugin.manifest.name,
				.itemId = item.id,
				.text = item.text,
			});
		}
	}
	return result;
}

bool Manager::hasPermission(
		const QString &id,
		const char *permission) const {
	const auto i = ranges::find(_list, id, [](const Plugin &p) {
		return p.manifest.id;
	});
	return (i != end(_list))
		&& i->manifest.permissions.contains(QString::fromLatin1(permission));
}

QString Manager::resolveBanner(const QByteArray &user) {
	for (const auto &plugin : _list) {
		const auto i = _runtimes.find(plugin.manifest.id);
		if (i == end(_runtimes) || !i->second->hasBannerResolver()) {
			continue;
		}
		auto error = QString();
		const auto link = i->second->resolveBanner(user, &error);
		if (!error.isEmpty()) {
			log(plugin.manifest.id, u"Banner resolver failed: "_q + error);
		} else if (!link.isEmpty()) {
			return link;
		}
	}
	return QString();
}

void Manager::activate(
		const MessageMenuItem &item,
		const QByteArray &context,
		Fn<void(const QString &)> toast) {
	const auto i = _runtimes.find(item.pluginId);
	if (i == end(_runtimes)) {
		return;
	}
	_toast = toast;
	auto error = QString();
	const auto ok = i->second->callMenuItem(item.itemId, context, &error);
	_toast = nullptr;
	if (ok) {
		_failures.remove(item.pluginId);
		return;
	}
	log(item.pluginId, u"Menu item failed: "_q + error);
	if (++_failures[item.pluginId] < kMaxFailures) {
		toast(tr::nexo_plugins_action_failed(
			tr::now,
			lt_name,
			item.pluginName));
		return;
	}
	if (const auto plugin = find(item.pluginId)) {
		stopPlugin(*plugin);
		plugin->state = State::Error;
		plugin->error = error;
		log(item.pluginId, u"Stopped after repeated errors."_q);
	}
	toast(tr::nexo_plugins_stopped(tr::now, lt_name, item.pluginName));
	_changes.fire({});
}

QString StateText(const Plugin &plugin) {
	switch (plugin.state) {
	case State::Running: return tr::nexo_plugins_state_running(tr::now);
	case State::Disabled: return tr::nexo_plugins_state_disabled(tr::now);
	case State::Incompatible:
		return tr::nexo_plugins_state_incompatible(tr::now);
	case State::Broken: return tr::nexo_plugins_state_broken(tr::now);
	case State::Error: return tr::nexo_plugins_state_error(tr::now);
	}
	return QString();
}

} // namespace Nexo::Plugins
