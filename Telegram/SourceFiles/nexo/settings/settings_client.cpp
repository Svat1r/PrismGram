// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Settings > PrismGram: versions, updates and settings backup.
#include "nexo/settings/settings_client.h"

#include "lang_auto.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "core/version.h"
#include "mainwindow.h"
#include "nexo/backup/settings_backup.h"
#include "nexo/hidden/nexo_hidden.h"
#include "lang/lang_instance.h"
#include "main/main_session.h"
#include "nexo/plugins/plugin_manager.h"
#include "nexo/updates/nexo_updates.h"
#include "settings/sections/settings_advanced.h"
#include "ui/widgets/labels.h"
#include "nexo/nexo_version.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_main.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QDate>
#include <QtCore/QStandardPaths>

namespace Settings {
namespace {

using namespace Builder;

void BuildVersions(SectionBuilder &builder) {
	builder.addSkip();
	builder.addButton({
		.id = u"nexo/client/version"_q,
		.title = tr::nexo_client_version(),
		.icon = { &st::menuIconInfo },
		.label = rpl::single(QString::fromLatin1(Nexo::kVersion)),
		.keywords = { u"version"_q, u"about"_q },
	});
	builder.addButton({
		.id = u"nexo/client/base"_q,
		.title = tr::nexo_client_base(),
		.icon = { &st::menuIconInfo },
		.label = rpl::single(QString::fromLatin1(Nexo::kBaseName)
			+ ' '
			+ QString::fromLatin1(AppVersionStr)),
		.keywords = { u"ayugram"_q, u"base"_q },
	});
	builder.addButton({
		.id = u"nexo/client/api"_q,
		.title = tr::nexo_client_plugin_api(),
		.icon = { &st::menuIconInfo },
		.label = rpl::single(QString::number(Nexo::kPluginApiVersion)),
		.keywords = { u"plugin"_q, u"api"_q },
	});
	builder.addSkip();
}

void ShowReleaseNotes(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	controller->showToast(tr::nexo_updates_notes_loading(tr::now));
	Nexo::Updates::FetchReleaseNotes([=](Nexo::Updates::ReleaseNotes notes) {
		const auto strong = weak.get();
		if (!strong) {
			return;
		} else if (!notes.error.isEmpty()) {
			strong->showToast(tr::nexo_updates_notes_failed(
				tr::now,
				lt_error,
				notes.error));
			return;
		}
		strong->show(Ui::MakeConfirmBox({
			.text = notes.body.isEmpty()
				? tr::nexo_updates_notes_empty(tr::now)
				: notes.body,
			.title = rpl::single(notes.title.isEmpty()
				? notes.tag
				: notes.title),
			.inform = true,
		}));
	});
}

[[nodiscard]] bool AnyPluginEnabled() {
	for (const auto &plugin : Nexo::Plugins::Manager::Instance().list()) {
		if (plugin.enabled) {
			return true;
		}
	}
	return false;
}

void BuildUpdates(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/client/updates"_q,
		.title = tr::nexo_updates_title(),
		.keywords = { u"update"_q, u"version"_q, u"changelog"_q },
	});
	if (HasUpdate()) {
		// The base client's block: check now, auto-check, download
		// progress and restart.
		builder.add([](const WidgetContext &ctx) {
			auto wrap = object_ptr<Ui::VerticalLayout>(ctx.container);
			SetupUpdate(wrap.data());
			return SectionBuilder::WidgetToAdd{ .widget = std::move(wrap) };
		});
	}
	builder.addButton({
		.id = u"nexo/client/notes"_q,
		.title = tr::nexo_updates_notes(),
		.icon = { &st::menuIconInfo },
		.onClick = [=] { ShowReleaseNotes(controller); },
		.keywords = { u"changelog"_q, u"notes"_q, u"whats new"_q },
	});
	if (Nexo::Updates::HasPreviousVersion()) {
		builder.addButton({
			.id = u"nexo/client/rollback"_q,
			.title = tr::nexo_updates_rollback(),
			.icon = { &st::menuIconRestore },
			.onClick = [=] {
				controller->show(Ui::MakeConfirmBox({
					.text = tr::nexo_updates_rollback_sure(tr::now),
					.confirmed = [=](Fn<void()> &&close) {
						auto error = QString();
						if (Nexo::Updates::StartRollback(&error)) {
							close();
							Core::Quit();
						} else {
							controller->showToast(error);
							close();
						}
					},
					.confirmText = tr::nexo_updates_rollback_button(),
				}));
			},
			.keywords = { u"rollback"_q, u"previous"_q, u"downgrade"_q },
		});
	}
	// Warn before installing an update that changes the plugin API.
	builder.scope([&] {
		builder.addSkip();
		builder.addDividerText(Nexo::Updates::FeedPluginApiValue(
		) | rpl::map([](int api) {
			return tr::nexo_updates_plugins_warning(
				tr::now,
				lt_version,
				QString::number(api));
		}));
	}, Nexo::Updates::FeedPluginApiValue() | rpl::map([](int api) {
		return api
			&& (api != Nexo::kPluginApiVersion)
			&& AnyPluginEnabled();
	}));
	builder.addSkip();
	builder.addDividerText(tr::nexo_updates_about());
}

void ExportBackup(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	const auto name = u"PrismGram-backup-%1.nexobackup"_q.arg(
		QDate::currentDate().toString(Qt::ISODate));
	FileDialog::GetWritePath(
		controller->widget().get(),
		tr::nexo_backup_export(tr::now),
		u"PrismGram backup (*.nexobackup)"_q,
		QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
			+ '/' + name,
		[=](QString &&path) {
			const auto strong = weak.get();
			if (!strong || path.isEmpty()) {
				return;
			}
			auto error = QString();
			strong->showToast(Nexo::Backup::Export(path, &error)
				? tr::nexo_backup_exported(tr::now)
				: tr::nexo_backup_failed(tr::now, lt_error, error));
		});
}

void ImportBackup(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetOpenPath(
		controller->widget().get(),
		tr::nexo_backup_import(tr::now),
		u"PrismGram backup (*.nexobackup)"_q,
		[=](FileDialog::OpenResult &&result) {
			const auto strong = weak.get();
			if (!strong || result.paths.isEmpty()) {
				return;
			}
			const auto path = result.paths.front();
			const auto summary = Nexo::Backup::Inspect(path);
			if (!summary.error.isEmpty()) {
				strong->show(Ui::MakeInformBox(tr::nexo_backup_failed(
					tr::now,
					lt_error,
					summary.error)));
				return;
			}
			strong->show(Ui::MakeConfirmBox({
				.text = tr::nexo_backup_import_sure(
					tr::now,
					lt_version,
					summary.version,
					lt_date,
					summary.created.left(10)),
				.confirmed = [=](Fn<void()> &&close) {
					auto error = QString();
					if (Nexo::Backup::Import(path, &error)) {
						close();
						// Settings are read on start, so restart now.
						Core::Restart();
					} else {
						strong->showToast(tr::nexo_backup_failed(
							tr::now,
							lt_error,
							error));
						close();
					}
				},
				.confirmText = tr::nexo_backup_import_restart(),
				.title = tr::nexo_backup_import(),
			}));
		});
}

void BuildBackup(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/client/backup"_q,
		.title = tr::nexo_backup_title(),
		.keywords = { u"backup"_q, u"export"_q, u"import"_q },
	});
	builder.addButton({
		.id = u"nexo/client/backup_export"_q,
		.title = tr::nexo_backup_export(),
		.icon = { &st::menuIconDownload },
		.onClick = [=] { ExportBackup(controller); },
		.keywords = { u"backup"_q, u"export"_q, u"save"_q },
	});
	builder.addButton({
		.id = u"nexo/client/backup_import"_q,
		.title = tr::nexo_backup_import(),
		.icon = { &st::menuIconRestore },
		.onClick = [=] { ImportBackup(controller); },
		.keywords = { u"backup"_q, u"import"_q, u"restore"_q },
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_backup_about());
}

[[nodiscard]] rpl::producer<QString> HiddenText(const char *en, const char *ru) {
	return rpl::single(Lang::GetInstance().id().startsWith(u"ru"_q)
		? QString::fromUtf8(ru)
		: QString::fromUtf8(en));
}

void BuildHidden(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addDivider();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/client/hidden"_q,
		.title = HiddenText("Hidden chats", "Скрытые чаты"),
		.keywords = { u"hidden"_q, u"pin"_q, u"hide"_q },
	});
	builder.addButton({
		.id = u"nexo/client/hidden_show"_q,
		.title = HiddenText("Show hidden chats (PIN)", "Показать скрытые чаты (PIN)"),
		.icon = { &st::menuIconUnarchive },
		.onClick = [=] { Nexo::Hidden::Unlock(controller, nullptr); },
		.keywords = { u"hidden"_q, u"pin"_q },
	});
	builder.addButton({
		.id = u"nexo/client/hidden_lock"_q,
		.title = HiddenText("Hide them again", "Скрыть их снова"),
		.icon = { &st::menuIconLock },
		.onClick = [] { Nexo::Hidden::Lock(); },
		.keywords = { u"hidden"_q, u"lock"_q },
	});
	builder.addButton({
		.id = u"nexo/client/hidden_pin"_q,
		.title = HiddenText("Change PIN", "Изменить PIN"),
		.icon = { &st::menuIconEdit },
		.onClick = [=] { Nexo::Hidden::ChangePin(controller); },
		.keywords = { u"hidden"_q, u"pin"_q },
	});
	builder.addSkip();
	builder.addDividerText(HiddenText(
		"To hide a chat, right-click it or open its menu, then Hide chat (PIN). Hidden chats leave the chat list and the search, and hide again a minute after PrismGram goes to the background. This hides chats on screen; it is not encryption.",
		"Чтобы скрыть чат, нажмите на него правой кнопкой или откройте его меню → «Скрыть чат (PIN)». Скрытые чаты пропадают из списка и поиска и снова прячутся через минуту после того, как PrismGram уходит в фон. Это прячет чаты на экране, а не шифрует их."));
}

const auto kMeta = BuildHelper({
	.id = NexoClient::Id(),
	.parentId = MainId(),
	.title = &tr::nexo_client_title,
	.icon = &st::menuIconInfo,
}, [](SectionBuilder &builder) {
	BuildVersions(builder);
	BuildUpdates(builder);
	BuildBackup(builder);
	BuildHidden(builder);
});

} // namespace

NexoClient::NexoClient(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NexoClient::title() {
	return tr::nexo_client_title();
}

void NexoClient::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type NexoClientId() {
	return NexoClient::Id();
}

} // namespace Settings
