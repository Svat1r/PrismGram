// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/themes/nexo_themes.h"

#include "window/themes/window_theme.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace Nexo::Themes {
namespace {

[[nodiscard]] QString ThemesDir() {
	return cWorkingDir() + u"tdata/nexo/themes/"_q;
}

[[nodiscard]] QString ThemePath(const QString &id) {
	return ThemesDir() + id + u".tdesktop-theme"_q;
}

// Themes are applied from a real file, because Telegram Desktop keeps
// the path of the applied theme and reloads it on the next start.
// The file is refreshed whenever the bundled copy differs.
[[nodiscard]] bool ExtractTheme(const QString &id) {
	auto bundled = QFile(u":/nexo/themes/"_q + id + u".tdesktop-theme"_q);
	if (!bundled.open(QIODevice::ReadOnly)) {
		LOG(("Nexo Themes: no bundled theme '%1'.").arg(id));
		return false;
	}
	const auto content = bundled.readAll();
	const auto path = ThemePath(id);
	auto existing = QFile(path);
	if (existing.open(QIODevice::ReadOnly)) {
		if (existing.readAll() == content) {
			return true;
		}
		existing.close();
	}
	QDir().mkpath(ThemesDir());
	auto out = QFile(path);
	if (!out.open(QIODevice::WriteOnly)
		|| out.write(content) != content.size()) {
		LOG(("Nexo Themes: could not write '%1'.").arg(path));
		return false;
	}
	return true;
}

} // namespace

const std::vector<Theme> &List() {
	static const auto result = std::vector<Theme>{
		{ u"nexo-night"_q, u"Prism Night"_q, QColor(0x3E, 0x88, 0xF7), QColor(0x13, 0x17, 0x1C) },
		{ u"nexo-amoled"_q, u"AMOLED"_q, QColor(0x3E, 0x88, 0xF7), QColor(0x00, 0x00, 0x00) },
		{ u"nexo-ocean"_q, u"Ocean"_q, QColor(0x14, 0xA3, 0xB8), QColor(0x0F, 0x1A, 0x20) },
		{ u"nexo-violet"_q, u"Violet"_q, QColor(0x8B, 0x5C, 0xF6), QColor(0x15, 0x12, 0x1E) },
		{ u"nexo-forest"_q, u"Forest"_q, QColor(0x34, 0xB3, 0x6B), QColor(0x10, 0x1A, 0x14) },
	};
	return result;
}

QString CurrentId() {
	const auto &path = Window::Theme::Background()->themeObject().pathAbsolute;
	if (path.isEmpty()) {
		return QString();
	}
	const auto current = QFileInfo(path).absoluteFilePath();
	for (const auto &theme : List()) {
		if (QFileInfo(ThemePath(theme.id)).absoluteFilePath() == current) {
			return theme.id;
		}
	}
	return QString();
}

bool Apply(const QString &id) {
	if (!ExtractTheme(id)) {
		return false;
	}
	if (!Window::Theme::Apply(ThemePath(id))) {
		LOG(("Nexo Themes: could not apply '%1'.").arg(id));
		return false;
	}
	Window::Theme::KeepApplied();
	return true;
}

} // namespace Nexo::Themes
