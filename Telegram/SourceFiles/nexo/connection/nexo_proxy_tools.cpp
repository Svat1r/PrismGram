// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Bulk proxy tools for ProxiesBoxController, used by the
// Connection settings section.
#include "boxes/connection_box.h"

#include "core/local_url_handlers.h"

#include <QtCore/QRegularExpression>

namespace {

using ProxyData = MTP::ProxyData;

[[nodiscard]] std::vector<QString> ExtractProxyLinks(const QString &text) {
	// Same link shapes ProxiesBox accepts from the clipboard.
	static const auto regex = QRegularExpression(
		R"((?:https?:\/\/[^\s]+|tg:\/\/[^\s]+|(?:www\.)?(?:t\.me|telegram\.me|telegram\.dog)\/[^\s]+))",
		QRegularExpression::CaseInsensitiveOption);
	auto result = std::vector<QString>();
	auto it = regex.globalMatch(text);
	while (it.hasNext()) {
		const auto trimmed = it.next().captured(0).trimmed();
		const auto converted = Core::TryConvertUrlToLocal(trimmed);
		const auto local = converted.isEmpty() ? trimmed : converted;
		const auto isProxy = local.startsWith(
				u"tg://proxy"_q,
				Qt::CaseInsensitive)
			|| local.startsWith(u"tg://socks"_q, Qt::CaseInsensitive)
			|| local.startsWith(u"tg://webproxy"_q, Qt::CaseInsensitive);
		if (isProxy) {
			result.push_back(local);
		}
	}
	return result;
}

} // namespace

auto ProxiesBoxController::importLinks(const QString &text)
-> ImportResult {
	auto result = ImportResult();
	for (const auto &link : ExtractProxyLinks(text)) {
		const auto proxy = ProxyFromLink(link);
		if (!proxy) {
			++result.invalid;
		} else if (contains(proxy)) {
			++result.existing;
		} else {
			// Unlike addNewItem() this does not switch to the new proxy,
			// so importing a long list keeps the current connection.
			_settings.addToList(proxy);
			_list.push_back({ ++_idCounter, proxy });
			refreshChecker(_list.back());
			++result.added;
		}
	}
	if (result.added > 0) {
		saveDelayed();
	}
	return result;
}

int ProxiesBoxController::itemsCount() const {
	return int(ranges::count(_list, false, &Item::deleted));
}

bool ProxiesBoxController::checking() const {
	return ranges::any_of(_list, [](const Item &item) {
		return !item.deleted && (item.state == ItemState::Checking);
	});
}

int ProxiesBoxController::unavailableCount() const {
	return int(ranges::count_if(_list, [&](const Item &item) {
		return !item.deleted
			&& (item.state == ItemState::Unavailable)
			&& (item.data != _settings.selected());
	}));
}

bool ProxiesBoxController::applyFastest() {
	auto best = end(_list);
	for (auto i = begin(_list); i != end(_list); ++i) {
		if (i->deleted
			|| i->data.type == Type::Web
			|| i->state != ItemState::Available) {
			continue;
		} else if (best == end(_list) || i->ping < best->ping) {
			best = i;
		}
	}
	if (best == end(_list)) {
		return false;
	}
	applyItem(best->id);
	return true;
}

int ProxiesBoxController::deleteUnavailable() {
	auto ids = std::vector<int>();
	for (const auto &item : _list) {
		// Never drop the proxy the app is using right now.
		if (!item.deleted
			&& (item.state == ItemState::Unavailable)
			&& (item.data != _settings.selected())) {
			ids.push_back(item.id);
		}
	}
	for (const auto id : ids) {
		deleteItem(id);
	}
	if (!ids.empty()) {
		saveDelayed();
	}
	return int(ids.size());
}

auto ProxiesBoxController::selectedData() const -> ProxyData {
	return _settings.isEnabled() ? _settings.selected() : ProxyData();
}

int ProxiesBoxController::selectedPing() const {
	const auto selected = selectedData();
	const auto i = ranges::find(_list, selected, &Item::data);
	return (i != end(_list) && !i->deleted) ? i->ping : 0;
}
