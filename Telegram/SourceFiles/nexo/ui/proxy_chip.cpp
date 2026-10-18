// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/ui/proxy_chip.h"

#include "lang_auto.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "ui/painter.h"
#include "styles/style_nexo_ui.h"

namespace Nexo::Ui {

ProxyChip::ProxyChip(QWidget *parent)
: AbstractButton(parent) {
	tr::nexo_proxy_chip() | rpl::on_next([=](QString text) {
		_text = std::move(text);
		refresh();
	}, lifetime());

	Core::App().settings().proxy().connectionTypeValue(
	) | rpl::on_next([=] {
		refresh();
	}, lifetime());
}

void ProxyChip::refresh() {
	_enabled = Core::App().settings().proxy().isEnabled();
	const auto &padding = st::nexoProxyChipPadding;
	resize(
		padding.left()
			+ st::nexoProxyChipIconOn.width()
			+ st::nexoProxyChipSkip
			+ st::nexoProxyChipFont->width(_text)
			+ padding.right(),
		st::nexoProxyChipHeight);
	update();
}

void ProxyChip::onStateChanged(State was, StateChangeSource source) {
	update();
}

void ProxyChip::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);

	const auto radius = height() / 2.;
	p.setPen(Qt::NoPen);
	p.setBrush(isOver() ? st::windowBgOver : st::windowBg);
	p.drawRoundedRect(rect(), radius, radius);

	const auto &icon = _enabled
		? st::nexoProxyChipIconOn
		: st::nexoProxyChipIconOff;
	const auto iconLeft = st::nexoProxyChipPadding.left();
	icon.paint(
		p,
		iconLeft,
		(height() - icon.height()) / 2,
		width());

	const auto &font = st::nexoProxyChipFont;
	p.setFont(font);
	p.setPen(_enabled ? st::windowActiveTextFg : st::windowSubTextFg);
	p.drawText(
		iconLeft + icon.width() + st::nexoProxyChipSkip,
		(height() - font->height) / 2 + font->ascent,
		_text);
}

} // namespace Nexo::Ui
