// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/plugins/plugin_runtime.h"

#include "nexo/nexo_version.h"

#include <quickjs.h>

namespace Nexo::Plugins {
namespace {

constexpr auto kMemoryLimit = size_t(32 * 1024 * 1024);
constexpr auto kStackLimit = size_t(1024 * 1024);
constexpr auto kCallTimeout = crl::time(500);
constexpr auto kMaxMenuItems = 8;
constexpr auto kMaxTextLength = 4096;

enum class Function {
	Log,
	Toast,
	Copy,
	SettingGet,
	SettingSet,
	AddMessageItem,
	SetBannerResolver,
};

[[nodiscard]] QString ToQString(JSContext *ctx, JSValueConst value) {
	size_t length = 0;
	const auto data = JS_ToCStringLen(ctx, &length, value);
	if (!data) {
		return QString();
	}
	auto result = QString::fromUtf8(data, int(length));
	JS_FreeCString(ctx, data);
	return result;
}

} // namespace

struct Runtime::Callback {
	JSValue value;
};

struct RuntimeAccess {
	static JSValue Call(
			JSContext *ctx,
			JSValueConst self,
			int argc,
			JSValueConst *argv,
			int magic) {
		const auto runtime = static_cast<Runtime*>(JS_GetContextOpaque(ctx));
		const auto arg = [&](int index) {
			return (index < argc) ? ToQString(ctx, argv[index]) : QString();
		};
		switch (Function(magic)) {
		case Function::Log:
			runtime->_host.log(arg(0).left(kMaxTextLength));
			return JS_UNDEFINED;
		case Function::Toast:
			runtime->_host.toast(arg(0).left(kMaxTextLength));
			return JS_UNDEFINED;
		case Function::Copy:
			runtime->_host.copyText(arg(0));
			return JS_UNDEFINED;
		case Function::SettingGet: {
			const auto json = runtime->_host.settingGet(arg(0));
			if (json.isEmpty()) {
				return (argc > 1) ? JS_DupValue(ctx, argv[1]) : JS_UNDEFINED;
			}
			return JS_ParseJSON(ctx, json.constData(), json.size(), "<setting>");
		}
		case Function::SettingSet: {
			if (argc < 2) {
				return JS_ThrowTypeError(ctx, "settings.set(key, value)");
			}
			const auto json = JS_JSONStringify(
				ctx,
				argv[1],
				JS_UNDEFINED,
				JS_UNDEFINED);
			if (JS_IsException(json)) {
				return json;
			}
			runtime->_host.settingSet(arg(0), ToQString(ctx, json).toUtf8());
			JS_FreeValue(ctx, json);
			return JS_UNDEFINED;
		}
		case Function::AddMessageItem: {
			if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
				return JS_ThrowTypeError(
					ctx,
					"menu.addMessageItem(text, callback)");
			} else if (int(runtime->_menuItems.size()) >= kMaxMenuItems) {
				return JS_ThrowTypeError(ctx, "too many menu items");
			}
			const auto id = int(runtime->_callbacks.size()) + 1;
			runtime->_callbacks.push_back(std::make_unique<Runtime::Callback>(
				Runtime::Callback{ JS_DupValue(ctx, argv[1]) }));
			runtime->_menuItems.push_back({ id, arg(0).left(64) });
			return JS_NewInt32(ctx, id);
		}
		case Function::SetBannerResolver: {
			if (argc < 1 || !JS_IsFunction(ctx, argv[0])) {
				return JS_ThrowTypeError(
					ctx,
					"profile.setBannerResolver(callback)");
			}
			if (runtime->_bannerResolver) {
				JS_FreeValue(ctx, runtime->_bannerResolver->value);
			}
			runtime->_bannerResolver = std::make_unique<Runtime::Callback>(
				Runtime::Callback{ JS_DupValue(ctx, argv[0]) });
			return JS_UNDEFINED;
		}
		}
		return JS_UNDEFINED;
	}
};

Runtime::Runtime(Manifest manifest, Host host)
: _manifest(std::move(manifest))
, _host(std::move(host)) {
	_runtime = JS_NewRuntime();
	JS_SetMemoryLimit(_runtime, kMemoryLimit);
	JS_SetMaxStackSize(_runtime, kStackLimit);
	JS_SetInterruptHandler(_runtime, InterruptHandler, this);
	_context = JS_NewContext(_runtime);
	JS_SetContextOpaque(_context, this);
	installApi();
}

Runtime::~Runtime() {
	for (const auto &callback : _callbacks) {
		JS_FreeValue(_context, callback->value);
	}
	_callbacks.clear();
	if (_bannerResolver) {
		JS_FreeValue(_context, _bannerResolver->value);
		_bannerResolver = nullptr;
	}
	JS_FreeContext(_context);
	JS_FreeRuntime(_runtime);
}

bool Runtime::has(const char *permission) const {
	return _manifest.permissions.contains(QString::fromLatin1(permission));
}

int Runtime::InterruptHandler(JSRuntime *runtime, void *opaque) {
	const auto that = static_cast<Runtime*>(opaque);
	return (that->_deadline && crl::now() > that->_deadline) ? 1 : 0;
}

void Runtime::armDeadline() {
	_deadline = crl::now() + kCallTimeout;
}

void Runtime::installApi() {
	const auto ctx = _context;
	const auto add = [&](JSValue object, const char *name, Function f, int length) {
		JS_SetPropertyStr(
			ctx,
			object,
			name,
			JS_NewCFunctionMagic(
				ctx,
				&RuntimeAccess::Call,
				name,
				length,
				JS_CFUNC_generic_magic,
				int(f)));
	};
	const auto global = JS_GetGlobalObject(ctx);
	const auto nexo = JS_NewObject(ctx);
	JS_SetPropertyStr(ctx, nexo, "version", JS_NewString(ctx, kVersion));
	JS_SetPropertyStr(ctx, nexo, "apiVersion", JS_NewInt32(ctx, kPluginApiVersion));
	add(nexo, "log", Function::Log, 1);
	add(nexo, "toast", Function::Toast, 1);
	if (has(kPermissionClipboard)) {
		add(nexo, "copyText", Function::Copy, 1);
	}
	if (has(kPermissionSettings)) {
		const auto settings = JS_NewObject(ctx);
		add(settings, "get", Function::SettingGet, 2);
		add(settings, "set", Function::SettingSet, 2);
		JS_SetPropertyStr(ctx, nexo, "settings", settings);
	}
	if (has(kPermissionMenuMessage)) {
		const auto menu = JS_NewObject(ctx);
		add(menu, "addMessageItem", Function::AddMessageItem, 2);
		JS_SetPropertyStr(ctx, nexo, "menu", menu);
	}
	if (has(kPermissionProfileBanner)) {
		const auto profile = JS_NewObject(ctx);
		add(profile, "setBannerResolver", Function::SetBannerResolver, 1);
		JS_SetPropertyStr(ctx, nexo, "profile", profile);
	}
	JS_SetPropertyStr(ctx, global, "nexo", nexo);
	JS_FreeValue(ctx, global);
}

bool Runtime::takeException(QString *error) {
	const auto exception = JS_GetException(_context);
	auto text = ToQString(_context, exception);
	if (JS_IsError(exception)) {
		const auto stack = JS_GetPropertyStr(_context, exception, "stack");
		if (!JS_IsUndefined(stack)) {
			text += '\n' + ToQString(_context, stack);
		}
		JS_FreeValue(_context, stack);
	}
	JS_FreeValue(_context, exception);
	if (error) {
		*error = text.left(kMaxTextLength);
	}
	return false;
}

bool Runtime::start(const QByteArray &code, QString *error) {
	armDeadline();
	const auto file = _manifest.entry.toUtf8();
	const auto result = JS_Eval(
		_context,
		code.constData(),
		code.size(),
		file.constData(),
		JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_STRICT);
	_deadline = 0;
	if (JS_IsException(result)) {
		return takeException(error);
	}
	JS_FreeValue(_context, result);

	const auto global = JS_GetGlobalObject(_context);
	const auto onLoad = JS_GetPropertyStr(_context, global, "onLoad");
	auto ok = true;
	if (JS_IsFunction(_context, onLoad)) {
		armDeadline();
		const auto called = JS_Call(_context, onLoad, global, 0, nullptr);
		_deadline = 0;
		if (JS_IsException(called)) {
			ok = takeException(error);
		} else {
			JS_FreeValue(_context, called);
		}
	}
	JS_FreeValue(_context, onLoad);
	JS_FreeValue(_context, global);
	_started = ok;
	return ok;
}

void Runtime::stop() {
	if (!_started) {
		return;
	}
	_started = false;
	const auto global = JS_GetGlobalObject(_context);
	const auto onUnload = JS_GetPropertyStr(_context, global, "onUnload");
	if (JS_IsFunction(_context, onUnload)) {
		armDeadline();
		const auto called = JS_Call(_context, onUnload, global, 0, nullptr);
		_deadline = 0;
		if (JS_IsException(called)) {
			auto error = QString();
			takeException(&error);
			_host.log(u"onUnload failed: "_q + error);
		} else {
			JS_FreeValue(_context, called);
		}
	}
	JS_FreeValue(_context, onUnload);
	JS_FreeValue(_context, global);
}

const std::vector<Runtime::MenuItem> &Runtime::menuItems() const {
	return _menuItems;
}

bool Runtime::callMenuItem(
		int id,
		const QByteArray &context,
		QString *error) {
	if (!_started || id <= 0 || id > int(_callbacks.size())) {
		return false;
	}
	const auto argument = JS_ParseJSON(
		_context,
		context.constData(),
		context.size(),
		"<context>");
	if (JS_IsException(argument)) {
		return takeException(error);
	}
	armDeadline();
	auto args = argument;
	const auto called = JS_Call(
		_context,
		_callbacks[id - 1]->value,
		JS_UNDEFINED,
		1,
		&args);
	_deadline = 0;
	JS_FreeValue(_context, argument);
	if (JS_IsException(called)) {
		return takeException(error);
	}
	JS_FreeValue(_context, called);
	return true;
}

bool Runtime::hasBannerResolver() const {
	return _started && (_bannerResolver != nullptr);
}

QString Runtime::resolveBanner(const QByteArray &user, QString *error) {
	if (!hasBannerResolver()) {
		return QString();
	}
	const auto argument = JS_ParseJSON(
		_context,
		user.constData(),
		user.size(),
		"<user>");
	if (JS_IsException(argument)) {
		takeException(error);
		return QString();
	}
	armDeadline();
	auto args = argument;
	const auto result = JS_Call(
		_context,
		_bannerResolver->value,
		JS_UNDEFINED,
		1,
		&args);
	_deadline = 0;
	JS_FreeValue(_context, argument);
	if (JS_IsException(result)) {
		takeException(error);
		return QString();
	}
	const auto link = JS_IsString(result)
		? ToQString(_context, result).left(2048)
		: QString();
	JS_FreeValue(_context, result);
	return link;
}

} // namespace Nexo::Plugins
