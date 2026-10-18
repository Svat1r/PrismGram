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
#include "nexo/ui/theme_picker.h"
#include "base/options.h"
#include "ui/widgets/discrete_sliders.h"
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

void BuildNexoThemes(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/appearance/themes_title"_q,
		.title = tr::nexo_appearance_nexo_themes(),
		.keywords = { u"theme"_q, u"amoled"_q, u"dark"_q, u"night"_q },
	});
	builder.add([](const WidgetContext &ctx) {
		return SectionBuilder::WidgetToAdd{
			.widget = object_ptr<Nexo::Ui::ThemePicker>(ctx.container),
		};
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_nexo_themes_about());
}

void AddFlagToggle(
		SectionBuilder &builder,
		const QString &id,
		rpl::producer<QString> title,
		Nexo::Flag flag,
		QStringList keywords) {
	auto &settings = Nexo::Settings::Instance();
	const auto button = builder.addButton({
		.id = id,
		.title = std::move(title),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings.get(flag)),
		.keywords = std::move(keywords),
	});
	if (button) {
		button->toggledChanges(
		) | rpl::on_next([=, &settings](bool enabled) {
			settings.set(flag, enabled);
		}, button->lifetime());
	}
}

void BuildNavigation(SectionBuilder &builder) {
	builder.addSkip();
	AddFlagToggle(
		builder,
		u"nexo/appearance/bottom_nav"_q,
		tr::nexo_appearance_bottom_nav(),
		Nexo::Flag::BottomNavBar,
		{ u"bottom"_q, u"navigation"_q, u"panel"_q, u"bar"_q });
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_bottom_nav_about());

	builder.addSkip();
	AddFlagToggle(
		builder,
		u"nexo/appearance/proxy_chip"_q,
		tr::nexo_appearance_proxy_chip(),
		Nexo::Flag::ProxyChip,
		{ u"proxy"_q, u"search"_q, u"chip"_q });
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_proxy_chip_about());
}

void BuildChats(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/appearance/chats"_q,
		.title = tr::nexo_appearance_chats(),
		.keywords = { u"chat"_q, u"messages"_q },
	});
	AddFlagToggle(
		builder,
		u"nexo/appearance/centered_title"_q,
		tr::nexo_appearance_centered_title(),
		Nexo::Flag::CenteredTitle,
		{ u"title"_q, u"center"_q, u"header"_q });
	AddFlagToggle(
		builder,
		u"nexo/appearance/no_rounding"_q,
		tr::nexo_appearance_no_rounding(),
		Nexo::Flag::NoNumberRounding,
		{ u"numbers"_q, u"rounding"_q, u"views"_q, u"subscribers"_q });
	AddFlagToggle(
		builder,
		u"nexo/appearance/mention_comma"_q,
		tr::nexo_appearance_mention_comma(),
		Nexo::Flag::CommaAfterMention,
		{ u"mention"_q, u"comma"_q });
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_chats_about());

	builder.addSkip();
	builder.addSubsectionTitle({
		.id = u"nexo/appearance/sticker_size"_q,
		.title = tr::nexo_appearance_sticker_size(),
		.keywords = { u"sticker"_q, u"size"_q },
	});
	builder.add([](const WidgetContext &ctx) {
		// Uses the base client's "sticker-size" option (0 = default size).
		static const auto kSizes = std::array{ 0, 192, 144, 112 };
		auto slider = object_ptr<Ui::SettingsSlider>(
			ctx.container,
			st::settingsSlider);
		const auto raw = slider.data();
		raw->addSection(tr::nexo_appearance_sticker_default(tr::now));
		raw->addSection(tr::nexo_appearance_sticker_medium(tr::now));
		raw->addSection(tr::nexo_appearance_sticker_small(tr::now));
		raw->addSection(tr::nexo_appearance_sticker_tiny(tr::now));
		auto &option = base::options::lookup<int>("sticker-size");
		const auto i = ranges::find(kSizes, option.value());
		raw->setActiveSectionFast((i != end(kSizes))
			? int(i - begin(kSizes))
			: 0);
		raw->sectionActivated(
		) | rpl::on_next([](int index) {
			base::options::lookup<int>("sticker-size").set(kSizes[index]);
		}, raw->lifetime());
		const auto &padding = st::settingsButtonNoIcon.padding;
		return SectionBuilder::WidgetToAdd{
			.widget = std::move(slider),
			.margin = { padding.left(), 0, padding.right(), 0 },
		};
	});
	builder.addSkip();
	builder.addDividerText(tr::nexo_appearance_sticker_about());
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
	BuildNexoThemes(builder);
	BuildNavigation(builder);
	BuildChats(builder);
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
