// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Ui {
class InputField;
} // namespace Ui

namespace Nexo::Features {

// Connects Nexo settings to the parts of the base client they control.
void Init();

// "Comma after mention": called right after the chat field autocomplete
// inserted a mention. Turns "@name |" into "@name, |" when the mention is
// the first thing in the message.
void AfterMentionInserted(
	not_null<Ui::InputField*> field,
	const QString &inserted);

// "Speed boost" (experimental): multiplier for parallel download and
// upload requests, 1 when off.
[[nodiscard]] int TransferBoost();

} // namespace Nexo::Features
