// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo::LiveWallpaper {

enum class Effect {
	None = 0,
	Rain = 1,
	Snow = 2,
	Grain = 3,
	Stars = 4,
};

struct Options {
	Effect effect = Effect::None;
	int density = 50; // 10..100, 50 is normal.
	int speed = 100; // 25..200 percent.
};

[[nodiscard]] Options CurrentOptions();
void SetOptions(const Options &options);

// Paints the effect over the chat background of the widget (call right
// after the background) and keeps repainting it at up to 20 frames a
// second while an effect is on and the window is active.
void Paint(not_null<QWidget*> widget, QPainter &p, QRect rect);

} // namespace Nexo::LiveWallpaper
