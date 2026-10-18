// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "ui/rp_widget.h"

namespace Window {
class SessionController;
} // namespace Window

namespace Nexo::Ui {

// Floating bottom navigation under the chat list:
// Chats / Contacts / Settings / Profile.
class BottomNavBar final : public ::Ui::RpWidget {
public:
	BottomNavBar(
		QWidget *parent,
		not_null<Window::SessionController*> controller,
		Fn<void()> chatsClicked);

protected:
	void paintEvent(QPaintEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void leaveEventHook(QEvent *e) override;

private:
	struct Item {
		const style::icon *icon = nullptr;
		Fn<void()> handler;
		QString text;
	};

	[[nodiscard]] int activeIndex() const;
	[[nodiscard]] QRect pillRect() const;
	[[nodiscard]] QRect itemRect(int index) const;
	[[nodiscard]] int itemAt(QPoint point) const;
	void setOver(int index);
	void paintBadge(QPainter &p, const QRect &item);

	const not_null<Window::SessionController*> _controller;
	std::vector<Item> _items;
	int _over = -1;
	int _pressed = -1;

};

} // namespace Nexo::Ui
