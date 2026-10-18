// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/plugins/plugin_menu.h"

#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "main/main_session.h"
#include "nexo/plugins/plugin_manager.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Nexo::Plugins {

void AddMessageMenuItems(
		not_null<Ui::PopupMenu*> menu,
		not_null<HistoryItem*> item) {
	auto &manager = Manager::Instance();
	const auto items = manager.messageMenuItems();
	if (items.empty()) {
		return;
	}
	const auto history = item->history();
	const auto peer = history->peer;
	const auto session = &history->session();
	const auto messageId = item->fullId();
	for (const auto &entry : items) {
		menu->addAction(entry.text, [=] {
			const auto item = session->data().message(messageId);
			if (!item) {
				return;
			}
			// Only what the plugin's permissions allow.
			auto context = QJsonObject{
				{ u"messageId"_q, double(item->id.bare) },
				{ u"chatId"_q, QString::number(peer->id.value) },
				{ u"chatTitle"_q, peer->name() },
				{ u"outgoing"_q, item->out() },
				{ u"date"_q, double(item->date()) },
			};
			auto &manager = Manager::Instance();
			if (manager.hasPermission(entry.pluginId, kPermissionMessagesRead)) {
				context.insert(u"text"_q, item->originalText().text);
			}
			const auto window = session->tryResolveWindow();
			manager.activate(
				entry,
				QJsonDocument(context).toJson(QJsonDocument::Compact),
				[=](const QString &text) {
					if (window) {
						window->showToast(text);
					}
				});
		}, &st::menuIconCustomize);
	}
}

} // namespace Nexo::Plugins
