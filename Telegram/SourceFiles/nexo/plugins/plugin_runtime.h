// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "nexo/plugins/plugin_manifest.h"

struct JSRuntime;
struct JSContext;

namespace Nexo::Plugins {

// One QuickJS runtime per plugin. Runs on the main thread, every call into
// plugin code has a time limit and the runtime has a memory limit. This is
// not an OS-level sandbox: plugin code runs inside the client process; it
// only gets the functions the API below exposes for its permissions.
class Runtime final {
public:
	struct Host {
		Fn<void(const QString &)> log;
		Fn<void(const QString &)> toast;
		Fn<void(const QString &)> copyText;
		Fn<QByteArray(const QString &)> settingGet;   // JSON or empty
		Fn<void(const QString &, const QByteArray &)> settingSet;
	};
	struct MenuItem {
		int id = 0;
		QString text;
	};

	Runtime(Manifest manifest, Host host);
	~Runtime();

	// Evaluates the plugin and calls its onLoad(). False on error.
	bool start(const QByteArray &code, QString *error);
	// Calls onUnload() if the plugin defines it.
	void stop();

	[[nodiscard]] const std::vector<MenuItem> &menuItems() const;
	// context is a JSON object passed to the item callback.
	bool callMenuItem(int id, const QByteArray &context, QString *error);

	// "profile.banner": asks the plugin's resolver for a banner link.
	// user is a JSON object; returns an empty string when there is none.
	[[nodiscard]] bool hasBannerResolver() const;
	QString resolveBanner(const QByteArray &user, QString *error);

private:
	struct Callback;

	[[nodiscard]] bool has(const char *permission) const;
	void installApi();
	bool takeException(QString *error);
	void armDeadline();

	static int InterruptHandler(JSRuntime *runtime, void *opaque);

	const Manifest _manifest;
	const Host _host;
	JSRuntime *_runtime = nullptr;
	JSContext *_context = nullptr;
	crl::time _deadline = 0;
	std::vector<MenuItem> _menuItems;
	std::vector<std::unique_ptr<Callback>> _callbacks;
	std::unique_ptr<Callback> _bannerResolver;
	bool _started = false;

	friend struct RuntimeAccess;

};

} // namespace Nexo::Plugins
