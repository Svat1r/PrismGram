// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Settings > Nexo Desk: versions, updates and settings backup.
#include "nexo/settings/settings_client.h"

#include "lang_auto.h"
#include "core/application.h"
#include "core/file_utilities.h"
#include "core/version.h"
#include "mainwindow.h"
#include "nexo/backup/settings_backup.h"
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
	const auto name = u"NexoDesk-backup-%1.nexobackup"_q.arg(
		QDate::currentDate().toString(Qt::ISODate));
	FileDialog::GetWritePath(
		controller->widget().get(),
		tr::nexo_backup_export(tr::now),
		u"Nexo Desk backup (*.nexobackup)"_q,
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
		u"Nexo Desk backup (*.nexobackup)"_q,
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

const auto kMeta = BuildHelper({
	.id = NexoClient::Id(),
	.parentId = MainId(),
	.title = &tr::nexo_client_title,
	.icon = &st::menuIconInfo,
}, [](SectionBuilder &builder) {
	BuildVersions(builder);
	BuildUpdates(builder);
	BuildBackup(builder);
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
