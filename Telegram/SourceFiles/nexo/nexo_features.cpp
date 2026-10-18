// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/nexo_features.h"

#include "lang/lang_tag.h"
#include "nexo/nexo_settings.h"
#include "ui/widgets/fields/input_field.h"

#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>

namespace Nexo::Features {
namespace {

// Twice the base client's parallelism; more makes FLOOD_WAIT errors
// from Telegram likely.
constexpr auto kBoostMultiplier = 2;

} // namespace

void Init() {
	static auto lifetime = rpl::lifetime();
	Settings::Instance().value(
		Flag::NoNumberRounding
	) | rpl::on_next([](bool enabled) {
		Lang::SetCountShorteningDisabled(enabled);
	}, lifetime);
}

void AfterMentionInserted(
		not_null<Ui::InputField*> field,
		const QString &inserted) {
	if (!Settings::Instance().get(Flag::CommaAfterMention)) {
		return;
	}
	auto cursor = field->textCursor();
	const auto position = cursor.position();
	// The document itself: getLastText() may lag behind insertTag().
	const auto text = cursor.document()->toPlainText();
	if (position < 1
		|| position > text.size()
		|| text[position - 1] != QChar(' ')) {
		return;
	}
	// Only when the message starts with this mention.
	if (text.left(position - 1) != inserted) {
		return;
	}
	// Replace the space with ", " keeping the space's (plain) format,
	// so the comma does not become part of the mention link.
	cursor.setPosition(position - 1);
	cursor.setPosition(position, QTextCursor::KeepAnchor);
	const auto format = cursor.charFormat();
	cursor.insertText(u", "_q, format);
	field->setTextCursor(cursor);
}

int TransferBoost() {
	return Settings::Instance().get(Flag::SpeedBoost)
		? kBoostMultiplier
		: 1;
}

} // namespace Nexo::Features
