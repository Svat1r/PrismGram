// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/ui/theme_picker.h"

#include "nexo/themes/nexo_themes.h"
#include "ui/painter.h"
#include "window/themes/window_theme.h"
#include "styles/style_nexo_ui.h"

namespace Nexo::Ui {

ThemePicker::ThemePicker(QWidget *parent)
: RpWidget(parent)
, _current(Themes::CurrentId()) {
	Window::Theme::Background()->updates(
	) | rpl::on_next([=] {
		_current = Themes::CurrentId();
		update();
	}, lifetime());
	setMouseTracking(true);
}

int ThemePicker::resizeGetHeight(int newWidth) {
	const auto &padding = st::nexoThemeCardPadding;
	return padding.top()
		+ st::nexoThemeCardSize.height()
		+ st::nexoThemeNameTop
		+ st::nexoThemeNameFont->height
		+ padding.bottom();
}

QRect ThemePicker::cardRect(int index) const {
	const auto size = st::nexoThemeCardSize;
	const auto count = int(Themes::List().size());
	const auto &padding = st::nexoThemeCardPadding;
	const auto available = width() - padding.left() - padding.right();
	// Shrink the gaps (not the cards) when the column is narrow.
	const auto skip = (count > 1)
		? std::clamp(
			(available - count * size.width()) / (count - 1),
			0,
			int(st::nexoThemeCardSkip))
		: 0;
	return QRect(
		QPoint(padding.left() + index * (size.width() + skip), padding.top()),
		size);
}

int ThemePicker::cardAt(QPoint point) const {
	for (auto i = 0; i != int(Themes::List().size()); ++i) {
		if (cardRect(i).contains(point)) {
			return i;
		}
	}
	return -1;
}

void ThemePicker::paintEvent(QPaintEvent *e) {
	auto p = QPainter(this);
	auto hq = PainterHighQualityEnabler(p);

	const auto &themes = Themes::List();
	const auto radius = st::nexoThemeCardRadius;
	for (auto i = 0; i != int(themes.size()); ++i) {
		const auto &theme = themes[i];
		const auto card = cardRect(i);
		const auto selected = (theme.id == _current);

		p.setPen(selected
			? QPen(theme.accent, st::nexoThemeCardBorder)
			: QPen((i == _over) ? st::windowSubTextFg : st::shadowFg,
				st::lineWidth));
		p.setBrush(theme.background);
		const auto half = st::nexoThemeCardBorder / 2.;
		p.drawRoundedRect(
			QRectF(card).marginsRemoved({ half, half, half, half }),
			radius,
			radius);

		// Mini chat: an outgoing bubble in the accent color above an
		// incoming bubble.
		const auto inner = card.marginsRemoved({ 8, 10, 8, 10 });
		const auto bubble = inner.height() / 5;
		p.setPen(Qt::NoPen);
		p.setBrush(theme.accent);
		p.drawRoundedRect(
			QRect(inner.x(), inner.y(), inner.width(), bubble),
			bubble / 2.,
			bubble / 2.);
		auto incoming = theme.background.lighter(260);
		if (theme.background.lightness() < 8) {
			incoming = QColor(0x2A, 0x2F, 0x36);
		}
		p.setBrush(incoming);
		p.drawRoundedRect(
			QRect(
				inner.x(),
				inner.y() + bubble + bubble / 2,
				inner.width() * 2 / 3,
				bubble),
			bubble / 2.,
			bubble / 2.);

		const auto &font = st::nexoThemeNameFont;
		const auto name = font->elided(
			theme.name,
			card.width() + st::nexoThemeCardSkip);
		p.setFont(font);
		p.setPen(selected ? st::windowActiveTextFg : st::windowSubTextFg);
		p.drawText(
			card.x() + (card.width() - font->width(name)) / 2,
			card.y() + card.height() + st::nexoThemeNameTop + font->ascent,
			name);
	}
}

void ThemePicker::mouseMoveEvent(QMouseEvent *e) {
	const auto over = cardAt(e->pos());
	if (_over != over) {
		_over = over;
		setCursor((over >= 0) ? style::cur_pointer : style::cur_default);
		update();
	}
}

void ThemePicker::mouseReleaseEvent(QMouseEvent *e) {
	const auto index = cardAt(e->pos());
	if (e->button() != Qt::LeftButton || index < 0) {
		return;
	}
	const auto &theme = Themes::List()[index];
	if (theme.id != _current) {
		Themes::Apply(theme.id);
	}
}

void ThemePicker::leaveEventHook(QEvent *e) {
	if (_over >= 0) {
		_over = -1;
		setCursor(style::cur_default);
		update();
	}
}

} // namespace Nexo::Ui
