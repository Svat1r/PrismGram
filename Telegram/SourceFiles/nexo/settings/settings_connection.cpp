// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Settings > Connection: proxy list, bulk import of tg://proxy links,
// availability checks and switching to the fastest working proxy.
// Proxy storage, checking and auto-switch come from the base client;
// this section only adds a front end and bulk operations on top.
#include "nexo/settings/settings_connection.h"

#include "lang_auto.h"
#include "base/timer.h"
#include "boxes/connection_box.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "core/file_utilities.h"
#include "main/main_account.h"
#include "main/main_session.h"
#include "mainwindow.h"
#include "mtproto/facade.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_main.h"
#include "storage/localstorage.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QFile>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

namespace Settings {
namespace {

using namespace Builder;

constexpr auto kMaxImportFileSize = qint64(1024 * 1024);
constexpr auto kCheckPollInterval = crl::time(300);
constexpr auto kCheckTimeout = crl::time(15000);

using Proxies = std::shared_ptr<ProxiesBoxController>;

// Runs availability checks on a fresh controller (it starts checking
// every proxy in its constructor) and reports once all of them answered
// or the timeout passed. A fresh controller per run keeps it in sync
// with edits made in the full proxy box in between.
class ProxyCheckRunner final {
public:
	explicit ProxyCheckRunner(not_null<Main::Account*> account)
	: _account(account)
	, _timer([=] { poll(); }) {
	}

	[[nodiscard]] rpl::producer<bool> runningValue() const {
		return _running.value();
	}

	// Returns false when there is nothing to check.
	bool start(Fn<void(Proxies)> done) {
		if (_running.current()) {
			return true;
		}
		auto proxies = std::make_shared<ProxiesBoxController>(_account);
		if (!proxies->itemsCount()) {
			return false;
		}
		_proxies = std::move(proxies);
		_done = std::move(done);
		_started = crl::now();
		_running = true;
		_timer.callEach(kCheckPollInterval);
		return true;
	}

private:
	void poll() {
		if (_proxies->checking()
			&& (crl::now() - _started < kCheckTimeout)) {
			return;
		}
		_timer.cancel();
		_running = false;
		const auto done = base::take(_done);
		done(base::take(_proxies));
	}

	const not_null<Main::Account*> _account;
	base::Timer _timer;
	rpl::variable<bool> _running = false;
	Proxies _proxies;
	Fn<void(Proxies)> _done;
	crl::time _started = 0;

};

[[nodiscard]] QString ImportResultText(
		const ProxiesBoxController::ImportResult &result) {
	if (!result.added && !result.existing && !result.invalid) {
		return tr::nexo_connection_import_none(tr::now);
	}
	return tr::nexo_connection_import_result(
		tr::now,
		lt_added,
		QString::number(result.added),
		lt_existing,
		QString::number(result.existing),
		lt_invalid,
		QString::number(result.invalid));
}

void ImportFromText(
		not_null<Window::SessionController*> controller,
		const QString &text) {
	auto proxies = ProxiesBoxController(&controller->session().account());
	controller->showToast(ImportResultText(proxies.importLinks(text)));
}

void ImportFromFile(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetOpenPath(
		controller->widget().get(),
		tr::nexo_connection_import_file_title(tr::now),
		u"Text files (*.txt);;All files (*.*)"_q,
		[=](FileDialog::OpenResult &&result) {
			const auto strong = weak.get();
			if (!strong || result.paths.isEmpty()) {
				return;
			}
			auto file = QFile(result.paths.front());
			if (file.size() > kMaxImportFileSize) {
				strong->showToast(
					tr::nexo_connection_import_too_big(tr::now));
				return;
			} else if (!file.open(QIODevice::ReadOnly)) {
				strong->showToast(
					tr::nexo_connection_import_read_error(tr::now));
				return;
			}
			ImportFromText(strong, QString::fromUtf8(file.readAll()));
		});
}

[[nodiscard]] QString ConnectionTypeText(not_null<Main::Account*> account) {
	const auto transport = account->mtp().dctransport();
	if (!Core::App().settings().proxy().isEnabled()) {
		return transport.isEmpty()
			? tr::lng_connection_auto_connecting(tr::now)
			: tr::lng_connection_auto(tr::now, lt_transport, transport);
	}
	return transport.isEmpty()
		? tr::lng_connection_proxy_connecting(tr::now)
		: tr::lng_connection_proxy(tr::now, lt_transport, transport);
}

void BuildStatus(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto account = &builder.session()->account();

	builder.addSkip();
	builder.addButton({
		.id = u"nexo/connection/type"_q,
		.title = tr::lng_settings_connection_type(),
		.icon = { &st::menuIconNetwork },
		.label = rpl::merge(
			Core::App().settings().proxy().connectionTypeChanges(),
			tr::lng_connection_auto_connecting() | rpl::to_empty
		) | rpl::map([=] { return ConnectionTypeText(account); }),
		.onClick = [=] {
			controller->show(ProxiesBoxController::CreateOwningBox(account));
		},
		.keywords = { u"proxy"_q, u"connection"_q, u"network"_q },
	});
	builder.addButton({
		.id = u"nexo/connection/list"_q,
		.title = tr::nexo_connection_proxy_list(),
		.icon = { &st::menuIconSettings },
		.onClick = [=] {
			controller->show(ProxiesBoxController::CreateOwningBox(account));
		},
		.keywords = { u"proxy"_q, u"socks"_q, u"mtproto"_q },
	});
	builder.addSkip();
}

void BuildImport(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/connection/import"_q,
		.title = tr::nexo_connection_import_title(),
		.keywords = { u"proxy"_q, u"import"_q, u"tg://proxy"_q },
	});
	builder.addButton({
		.id = u"nexo/connection/import_clipboard"_q,
		.title = tr::nexo_connection_import_clipboard(),
		.icon = { &st::menuIconCopy },
		.onClick = [=] {
			ImportFromText(
				controller,
				QGuiApplication::clipboard()->text());
		},
		.keywords = { u"paste"_q, u"clipboard"_q, u"link"_q },
	});
	builder.addButton({
		.id = u"nexo/connection/import_file"_q,
		.title = tr::nexo_connection_import_file(),
		.icon = { &st::menuIconFile },
		.onClick = [=] { ImportFromFile(controller); },
		.keywords = { u"file"_q, u"txt"_q },
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_connection_import_about());
}

void BuildChecks(SectionBuilder &builder) {
	const auto controller = builder.controller();
	const auto container = builder.container();
	const auto account = &builder.session()->account();
	const auto runner = container
		? container->lifetime().make_state<ProxyCheckRunner>(account)
		: nullptr;
	auto checkingLabel = [=]() -> rpl::producer<QString> {
		if (!runner) {
			return rpl::single(QString());
		}
		return runner->runningValue() | rpl::map([](bool running) {
			return running
				? tr::nexo_connection_checking(tr::now)
				: QString();
		});
	};

	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/connection/checks"_q,
		.title = tr::nexo_connection_checks_title(),
		.keywords = { u"ping"_q, u"check"_q },
	});
	builder.addButton({
		.id = u"nexo/connection/fastest"_q,
		.title = tr::nexo_connection_pick_fastest(),
		.icon = { &st::menuIconBoosts },
		.label = checkingLabel(),
		.onClick = [=] {
			const auto started = runner->start([=](Proxies proxies) {
				if (!proxies->applyFastest()) {
					controller->showToast(
						tr::nexo_connection_none_available(tr::now));
					return;
				}
				controller->showToast(tr::nexo_connection_picked(
					tr::now,
					lt_server,
					proxies->selectedData().host,
					lt_ping,
					QString::number(proxies->selectedPing())));
			});
			if (!started) {
				controller->showToast(tr::nexo_connection_empty(tr::now));
			}
		},
		.keywords = { u"fastest"_q, u"ping"_q, u"best"_q },
	});
	builder.addButton({
		.id = u"nexo/connection/remove_dead"_q,
		.title = tr::nexo_connection_remove_dead(),
		.icon = { &st::menuIconDelete },
		.label = checkingLabel(),
		.onClick = [=] {
			const auto started = runner->start([=](Proxies proxies) {
				const auto count = proxies->unavailableCount();
				if (!count) {
					controller->showToast(
						tr::nexo_connection_remove_dead_none(tr::now));
					return;
				}
				controller->show(Ui::MakeConfirmBox({
					.text = tr::nexo_connection_remove_dead_confirm(
						lt_count,
						rpl::single(count) | tr::to_count()),
					.confirmed = [=](Fn<void()> &&close) {
						const auto removed = proxies->deleteUnavailable();
						controller->showToast(tr::nexo_connection_removed(
							tr::now,
							lt_count,
							removed));
						close();
					},
					.confirmText = tr::lng_box_delete(),
					.confirmStyle = &st::attentionBoxButton,
				}));
			});
			if (!started) {
				controller->showToast(tr::nexo_connection_empty(tr::now));
			}
		},
		.keywords = { u"delete"_q, u"unavailable"_q, u"dead"_q },
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_connection_checks_about());
}

void BuildAutoSwitch(SectionBuilder &builder) {
	auto &proxy = Core::App().settings().proxy();

	builder.addSkip();
	const auto button = builder.addButton({
		.id = u"nexo/connection/auto_switch"_q,
		.title = tr::lng_proxy_auto_switch(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(proxy.proxyRotationEnabled()),
		.keywords = { u"rotation"_q, u"auto"_q, u"switch"_q },
	});
	if (button) {
		button->toggledChanges(
		) | rpl::filter([=, &proxy](bool enabled) {
			return enabled != proxy.proxyRotationEnabled();
		}) | rpl::on_next([=, &proxy](bool enabled) {
			proxy.setProxyRotationEnabled(enabled);
			Core::App().proxyRotationSettingsChanged();
			Local::writeSettings();
		}, button->lifetime());
	}
	builder.addSkip();
	builder.addDividerText(tr::lng_proxy_auto_switch_about());
}

const auto kMeta = BuildHelper({
	.id = NexoConnection::Id(),
	.parentId = MainId(),
	.title = &tr::nexo_connection_title,
	.icon = &st::menuIconNetwork,
}, [](SectionBuilder &builder) {
	BuildStatus(builder);
	BuildImport(builder);
	BuildChecks(builder);
	BuildAutoSwitch(builder);
});

} // namespace

NexoConnection::NexoConnection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NexoConnection::title() {
	return tr::nexo_connection_title();
}

void NexoConnection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type NexoConnectionId() {
	return NexoConnection::Id();
}

} // namespace Settings
