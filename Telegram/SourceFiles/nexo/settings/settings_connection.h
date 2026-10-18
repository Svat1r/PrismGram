// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

#include "settings/settings_common.h"
#include "settings/settings_common_session.h"

namespace Window {
class SessionController;
} // namespace Window

namespace Settings {

class NexoConnection : public Section<NexoConnection> {
public:
	NexoConnection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

[[nodiscard]] Type NexoConnectionId();

} // namespace Settings
