// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Settings > Appearance. For now it holds the bottom navigation toggle
// and links to the theme settings that already exist in the base
// client; Nexo themes will be added here.
#include "nexo/settings/settings_appearance.h"

#include "lang_auto.h"
#include "ayu/ui/settings/settings_appearance.h"
#include "nexo/nexo_settings.h"
#include "settings/settings_builder.h"
#include "settings/sections/settings_chat.h"
#include "settings/sections/settings_main.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

void BuildNavigation(SectionBuilder &builder) {
	auto &settings = Nexo::Settings::Instance();

	builder.addSkip();
	const auto button = builder.addButton({
		.id = u"nexo/appearance/bottom_nav"_q,
		.title = tr::nexo_appearance_bottom_nav(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings.bottomNavBar()),
		.keywords = { u"bottom"_q, u"navigation"_q, u"panel"_q, u"bar"_q },
	});
	if (button) {
		button->toggledChanges(
		) | rpl::on_next([=, &settings](bool enabled) {
			settings.setBottomNavBar(enabled);
		}, button->lifetime());
	}
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_bottom_nav_about());
}

void BuildThemes(SectionBuilder &builder) {
	const auto showOther = builder.showOther();

	builder.addSkip();
	builder.addButton({
		.id = u"nexo/appearance/themes"_q,
		.title = tr::nexo_appearance_themes(),
		.icon = { &st::menuIconPalette },
		.onClick = [=] { showOther(ChatId()); },
		.keywords = { u"theme"_q, u"night"_q, u"color"_q, u"wallpaper"_q },
	});
	builder.addButton({
		.id = u"nexo/appearance/ayu"_q,
		.title = tr::nexo_appearance_more(),
		.icon = { &st::menuIconCustomize },
		.onClick = [=] { showOther(AyuAppearanceId()); },
		.keywords = { u"font"_q, u"corners"_q, u"avatar"_q },
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = NexoAppearance::Id(),
	.parentId = MainId(),
	.title = &tr::nexo_appearance_title,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	BuildNavigation(builder);
	BuildThemes(builder);
});

} // namespace

NexoAppearance::NexoAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> NexoAppearance::title() {
	return tr::nexo_appearance_title();
}

void NexoAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type NexoAppearanceId() {
	return NexoAppearance::Id();
}

} // namespace Settings
