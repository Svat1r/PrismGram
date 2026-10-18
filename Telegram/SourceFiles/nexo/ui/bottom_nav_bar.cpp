// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/ui/bottom_nav_bar.h"

#include "lang_auto.h"
#include "boxes/peer_list_controllers.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "dialogs/dialogs_entry.h"
#include "dialogs/dialogs_main_list.h"
#include "info/info_controller.h"
#include "info/info_section_widget.h"
#include "info/info_wrap_widget.h"
#include "mainwidget.h"
#include "main/main_session.h"
#include "ui/painter.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_nexo_ui.h"

namespace Nexo::Ui {
namespace {

// Background of the highlighted (current) item, over the accent color.
constexpr auto kActiveAlpha = 0.14;
constexpr auto kChatsIndex = 0;
constexpr auto kSettingsIndex = 2;
constexpr auto kProfileIndex = 3;

[[nodiscard]] QString BadgeText(int count) {
	return (count > 999) ? u"999+"_q : QString::number(count);
}

} // namespace

BottomNavBar::BottomNavBar(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	Fn<void()> chatsClicked)
: RpWidget(parent)
, _controller(controller) {
	_items = {
		{ &st::menuIconChats, std::move(chatsClicked) },
		{ &st::menuIconUserShow, [=] {
			_controller->show(PrepareContactsBox(_controller));
		} },
		{ &st::menuIconSettings, [=] {
			_controller->showSettings();
		} },
		{ &st::menuIconProfile, [=] {
			_controller->showPeerInfo(_controller->session().user());
		} },
	};
	auto labels = std::vector<rpl::producer<QString>>();
	labels.push_back(tr::nexo_nav_chats());
	labels.push_back(tr::nexo_nav_contacts());
	labels.push_back(tr::nexo_nav_settings());
	labels.push_back(tr::nexo_nav_profile());
	for (auto i = 0; i != int(labels.size()); ++i) {
		std::move(labels[i]) | rpl::on_next([=](QString text) {
			_items[i].text = std::move(text);
			update();
		}, lifetime());
	}

	_controller->session().data().unreadBadgeChanges(
	) | rpl::on_next([=] {
		update();
	}, lifetime());

	// MainWidget may still be constructing when the chat list creates us.
	crl::on_main(this, [=] {
		_controller->content()->mainSectionChanges(
		) | rpl::on_next([=] {
			update();
		}, lifetime());
	});

	resize(
		width(),
		st::nexoNavBarMargin.top()
			+ st::nexoNavBarHeight
			+ st::nexoNavBarMargin.bottom());
	setMouseTracking(true);
}

QRect BottomNavBar::pillRect() const {
	return rect().marginsRemoved(st::nexoNavBarMargin);
}

QRect BottomNavBar::itemRect(int index) const {
	const auto inner = pillRect().marginsRemoved(st::nexoNavBarInner);
	const auto count = int(_items.size());
	const auto left = inner.x() + (inner.width() * index) / count;
	const auto right = inner.x() + (inner.width() * (index + 1)) / count;
	return QRect(left, inner.y(), right - left, inner.height());
}

int BottomNavBar::itemAt(QPoint point) const {
	for (auto i = 0; i != int(_items.size()); ++i) {
		if (itemRect(i).contains(point)) {
			return i;
		}
	}
	return -1;
}

void BottomNavBar::setOver(int index) {
	if (_over == index) {
		return;
	}
	_over = index;
	setCursor((index >= 0) ? style::cur_pointer : style::cur_default);
	update();
}

int BottomNavBar::activeIndex() const {
	const auto section = _controller->content()->mainSection();
	const auto wrap = section
		? section->findChild<Info::WrapWidget*>()
		: nullptr;
	if (!wrap) {
		return kChatsIndex;
	}
	const auto key = wrap->key();
	if (key.settingsSelf()) {
		return kSettingsIndex;
	} else if (key.peer() == _controller->session().user().get()) {
		return kProfileIndex;
	}
	return kChatsIndex;
}

void BottomNavBar::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);

	// Same background as the chat list, so the bar looks like it floats.
	p.fillRect(e->rect(), st::dialogsBg);

	const auto pill = pillRect();
	p.setPen(QPen(st::shadowFg, st::lineWidth));
	p.setBrush(st::windowBgOver);
	p.drawRoundedRect(pill, st::nexoNavBarRadius, st::nexoNavBarRadius);

	const auto accent = st::windowActiveTextFg->c;
	auto activeBg = accent;
	activeBg.setAlphaF(kActiveAlpha);

	const auto activeIndex = this->activeIndex();
	for (auto i = 0; i != int(_items.size()); ++i) {
		const auto &item = _items[i];
		const auto rect = itemRect(i);
		const auto active = (i == activeIndex);
		if (active || i == _over) {
			p.setPen(Qt::NoPen);
			p.setBrush(active ? QBrush(activeBg) : st::windowBgRipple->b);
			p.drawRoundedRect(
				rect,
				st::nexoNavItemRadius,
				st::nexoNavItemRadius);
		}
		const auto fg = active ? accent : st::windowSubTextFg->c;
		const auto icon = item.icon;
		icon->paint(
			p,
			rect.x() + (rect.width() - icon->width()) / 2,
			rect.y() + st::nexoNavIconTop,
			width(),
			fg);

		const auto &font = st::nexoNavLabelFont;
		const auto text = font->elided(item.text, rect.width());
		p.setFont(font);
		p.setPen(fg);
		p.drawText(
			rect.x() + (rect.width() - font->width(text)) / 2,
			rect.y() + rect.height() - st::nexoNavLabelBottom - font->descent,
			text);

		if (i == kChatsIndex) {
			paintBadge(p, rect);
		}
	}
}

void BottomNavBar::paintBadge(QPainter &p, const QRect &item) {
	// Same rule as the folder counters: unread chats, muted ones only
	// when "count muted chats" is on.
	const auto badges = Dialogs::BadgesForUnread(
		_controller->session().data().chatsList()->unreadState(),
		Dialogs::CountInBadge::Chats);
	const auto count = badges.unreadCounter;
	if (count <= 0) {
		return;
	}
	const auto text = BadgeText(count);
	const auto &font = st::nexoNavBadgeFont;
	const auto height = st::nexoNavBadgeHeight;
	const auto width = std::max(
		height,
		font->width(text) + 2 * st::nexoNavBadgePadding);
	const auto left = item.x()
		+ item.width() / 2
		+ st::nexoNavBadgeShift.x();
	const auto top = item.y() + st::nexoNavBadgeShift.y();
	const auto badge = QRect(left, top, width, height);

	p.setPen(Qt::NoPen);
	p.setBrush(badges.unreadMuted
		? st::dialogsUnreadBgMuted
		: st::dialogsUnreadBg);
	p.drawRoundedRect(badge, height / 2., height / 2.);
	p.setFont(font);
	p.setPen(st::dialogsUnreadFg);
	p.drawText(badge, Qt::AlignCenter, text);
}

void BottomNavBar::mouseMoveEvent(QMouseEvent *e) {
	setOver(itemAt(e->pos()));
}

void BottomNavBar::mousePressEvent(QMouseEvent *e) {
	if (e->button() == Qt::LeftButton) {
		_pressed = itemAt(e->pos());
	}
}

void BottomNavBar::mouseReleaseEvent(QMouseEvent *e) {
	const auto pressed = std::exchange(_pressed, -1);
	if (e->button() != Qt::LeftButton
		|| pressed < 0
		|| pressed != itemAt(e->pos())) {
		return;
	}
	if (const auto handler = _items[pressed].handler) {
		handler();
	}
}

void BottomNavBar::leaveEventHook(QEvent *e) {
	setOver(-1);
}

} // namespace Nexo::Ui
