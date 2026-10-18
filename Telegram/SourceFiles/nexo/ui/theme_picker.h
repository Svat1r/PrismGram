// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "ui/rp_widget.h"

namespace Nexo::Ui {

// Row of cards with the built-in Nexo themes; a click applies one.
class ThemePicker final : public ::Ui::RpWidget {
public:
	explicit ThemePicker(QWidget *parent);

protected:
	int resizeGetHeight(int newWidth) override;
	void paintEvent(QPaintEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void leaveEventHook(QEvent *e) override;

private:
	[[nodiscard]] QRect cardRect(int index) const;
	[[nodiscard]] int cardAt(QPoint point) const;

	QString _current;
	int _over = -1;

};

} // namespace Nexo::Ui
