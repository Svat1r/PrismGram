// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "ui/abstract_button.h"

namespace Nexo::Ui {

// "Proxy" chip shown inside the chat list search field. Shows whether a
// proxy is in use; a click opens Settings > Connection.
class ProxyChip final : public ::Ui::AbstractButton {
public:
	explicit ProxyChip(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *e) override;
	void onStateChanged(State was, StateChangeSource source) override;

private:
	void refresh();

	QString _text;
	bool _enabled = false;

};

} // namespace Nexo::Ui
