// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Settings > Plugins: list, details, enable/disable, import, remove.
#include "nexo/settings/settings_plugins.h"

#include "lang_auto.h"
#include "core/file_utilities.h"
#include "mainwindow.h"
#include "nexo/plugins/plugin_manager.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "settings/sections/settings_main.h"
#include "ui/boxes/confirm_box.h"
#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/vertical_list.h"
#include "window/window_session_controller.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

#include <QtCore/QDir>

namespace Settings {
namespace {

using namespace Builder;
using namespace Nexo::Plugins;

[[nodiscard]] QString PermissionText(const QString &permission) {
	if (permission == QString::fromLatin1(kPermissionMenuMessage)) {
		return tr::nexo_plugins_perm_menu_message(tr::now);
	} else if (permission == QString::fromLatin1(kPermissionSettings)) {
		return tr::nexo_plugins_perm_settings(tr::now);
	} else if (permission == QString::fromLatin1(kPermissionMessagesRead)) {
		return tr::nexo_plugins_perm_messages_read(tr::now);
	} else if (permission == QString::fromLatin1(kPermissionClipboard)) {
		return tr::nexo_plugins_perm_clipboard(tr::now);
	}
	return permission;
}

[[nodiscard]] QString DescribeManifest(const Manifest &manifest) {
	auto lines = QStringList();
	lines.push_back(tr::nexo_plugins_field_version(tr::now)
		+ u": "_q + manifest.version);
	lines.push_back(tr::nexo_plugins_field_author(tr::now)
		+ u": "_q + manifest.author);
	lines.push_back(tr::nexo_plugins_field_source(tr::now) + u": "_q
		+ (manifest.source.isEmpty()
			? tr::nexo_plugins_source_none(tr::now)
			: manifest.source));
	if (!manifest.description.isEmpty()) {
		lines.push_back(QString());
		lines.push_back(manifest.description);
	}
	lines.push_back(QString());
	lines.push_back(tr::nexo_plugins_field_permissions(tr::now) + ':');
	if (manifest.permissions.isEmpty()) {
		lines.push_back(tr::nexo_plugins_permissions_none(tr::now));
	}
	for (const auto &permission : manifest.permissions) {
		lines.push_back(u"• "_q + PermissionText(permission));
	}
	return lines.join('\n');
}

[[nodiscard]] const Plugin *FindPlugin(const QString &id) {
	for (const auto &plugin : Manager::Instance().list()) {
		if (plugin.manifest.id == id) {
			return &plugin;
		}
	}
	return nullptr;
}

void ShowPluginBox(
		not_null<Window::SessionController*> controller,
		const QString &id) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		const auto plugin = FindPlugin(id);
		if (!plugin) {
			box->closeBox();
			return;
		}
		auto text = DescribeManifest(plugin->manifest);
		text += u"\n\n"_q + tr::nexo_plugins_field_state(tr::now)
			+ u": "_q + StateText(*plugin);
		if (!plugin->error.isEmpty()) {
			text += '\n' + plugin->error;
		}
		box->setTitle(rpl::single(plugin->manifest.name));
		box->addRow(object_ptr<Ui::FlatLabel>(box, text, st::boxLabel));

		const auto canRun = (plugin->state != State::Broken)
			&& (plugin->state != State::Incompatible);
		if (canRun) {
			const auto enabled = plugin->enabled;
			box->addButton(
				enabled ? tr::nexo_plugins_disable() : tr::nexo_plugins_enable(),
				[=] {
					Manager::Instance().setEnabled(id, !enabled);
					box->closeBox();
				});
		}
		box->addButton(tr::nexo_plugins_show_log(), [=] {
			const auto path = Manager::Instance().logPath(id);
			if (QFile::exists(path)) {
				File::ShowInFolder(path);
			} else {
				controller->showToast(tr::nexo_plugins_log_empty(tr::now));
			}
		});
		box->addLeftButton(tr::nexo_plugins_remove(), [=] {
			const auto name = plugin->manifest.name;
			controller->show(Ui::MakeConfirmBox({
				.text = tr::nexo_plugins_remove_sure(
					tr::now,
					lt_name,
					name),
				.confirmed = [=](Fn<void()> &&close) {
					auto error = QString();
					if (!Manager::Instance().remove(id, &error)) {
						controller->showToast(error);
					}
					close();
					box->closeBox();
				},
				.confirmText = tr::nexo_plugins_remove(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		});
	}));
}

void ImportPlugin(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetOpenPath(
		controller->widget().get(),
		tr::nexo_plugins_import(tr::now),
		u"PrismGram plugins (*.nexoplugin *.zip)"_q,
		[=](FileDialog::OpenResult &&result) {
			const auto strong = weak.get();
			if (!strong || result.paths.isEmpty()) {
				return;
			}
			const auto path = result.paths.front();
			const auto info = Manager::Instance().inspect(path);
			if (!info.error.isEmpty()) {
				strong->show(Ui::MakeInformBox(
					tr::nexo_plugins_install_failed(
						tr::now,
						lt_error,
						info.error)));
				return;
			}
			// Always ask: show who made it, where it's from and what it
			// will be allowed to do.
			auto text = info.manifest.name + u"\n\n"_q
				+ DescribeManifest(info.manifest);
			if (info.manifest.source.isEmpty()) {
				text += u"\n\n"_q + tr::nexo_plugins_install_unverified(tr::now);
			}
			if (!info.compatibility.isEmpty()) {
				text += u"\n\n"_q + tr::nexo_plugins_incompatible_warning(
					tr::now,
					lt_reason,
					info.compatibility);
			}
			text += u"\n\n"_q + tr::nexo_plugins_install_warning(tr::now);
			const auto name = info.manifest.name;
			strong->show(Ui::MakeConfirmBox({
				.text = text,
				.confirmed = [=](Fn<void()> &&close) {
					auto error = QString();
					if (Manager::Instance().install(path, &error)) {
						strong->showToast(tr::nexo_plugins_installed_toast(
							tr::now,
							lt_name,
							name));
					} else {
						strong->showToast(tr::nexo_plugins_install_failed(
							tr::now,
							lt_error,
							error));
					}
					close();
				},
				.confirmText = info.replaces
					? tr::nexo_plugins_update()
					: tr::nexo_plugins_install(),
				.title = tr::nexo_plugins_install_title(),
			}));
		});
}

void BuildActions(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addSkip();
	builder.addButton({
		.id = u"nexo/plugins/import"_q,
		.title = tr::nexo_plugins_import(),
		.icon = { &st::menuIconDownload },
		.onClick = [=] { ImportPlugin(controller); },
		.keywords = { u"plugin"_q, u"install"_q, u"import"_q },
	});
	builder.addButton({
		.id = u"nexo/plugins/folder"_q,
		.title = tr::nexo_plugins_open_folder(),
		.icon = { &st::menuIconShowInFolder },
		.onClick = [=] {
			const auto path = Manager::Instance().pluginsPath();
			QDir().mkpath(path);
			File::ShowInFolder(path);
		},
		.keywords = { u"plugin"_q, u"folder"_q },
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_plugins_about());
}

void BuildList(SectionBuilder &builder) {
	const auto controller = builder.controller();
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/plugins/installed"_q,
		.title = tr::nexo_plugins_installed(),
		.keywords = { u"plugins"_q, u"extensions"_q },
	});
	builder.add([=](const WidgetContext &ctx) {
		auto result = object_ptr<Ui::VerticalLayout>(ctx.container);
		const auto list = result.data();
		const auto rebuild = [=] {
			list->clear();
			const auto &plugins = Manager::Instance().list();
			if (plugins.empty()) {
				list->add(
					object_ptr<Ui::FlatLabel>(
						list,
						tr::nexo_plugins_empty(),
						st::boxDividerLabel),
					st::boxRowPadding);
			}
			for (const auto &plugin : plugins) {
				const auto id = plugin.manifest.id;
				const auto button = AddButtonWithLabel(
					list,
					rpl::single(plugin.manifest.name
						+ u" "_q
						+ plugin.manifest.version),
					rpl::single(StateText(plugin)),
					st::settingsButton,
					{ &st::menuIconCustomize });
				button->addClickHandler([=] {
					ShowPluginBox(controller, id);
				});
			}
			list->resizeToWidth(list->width());
		};
		rebuild();
		Manager::Instance().changes(
		) | rpl::on_next(rebuild, list->lifetime());
		return SectionBuilder::WidgetToAdd{ .widget = std::move(result) };
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = NexoPlugins::Id(),
	.parentId = MainId(),
	.title = &tr::nexo_plugins_title,
	.icon = &st::menuIconCustomize,
}, [](SectionBuilder &builder) {
	BuildActions(builder);
	BuildList(builder);
});

} // namespace

NexoPlugins::NexoPlugins(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NexoPlugins::title() {
	return tr::nexo_plugins_title();
}

void NexoPlugins::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type NexoPluginsId() {
	return NexoPlugins::Id();
}

} // namespace Settings
