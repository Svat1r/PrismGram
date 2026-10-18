// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
//
// Built-in translations for nexo_* strings. English defaults live in
// Resources/langs/lang.strings; other languages are applied here on top
// of the current language pack, without any network requests.
#include "nexo/nexo_lang.h"

#include "lang/lang_instance.h"

namespace Nexo::Lang {
namespace {

struct Entry {
	const char *key;
	const char *value;
};

// Russian plural forms: #one, #few, #many, #other.
constexpr auto kRussian = std::array{
	Entry{ "nexo_connection_title", "Подключение" },
	Entry{ "nexo_connection_proxy_list", "Список прокси" },
	Entry{ "nexo_connection_import_title", "Импорт" },
	Entry{ "nexo_connection_import_clipboard", "Импорт ссылок из буфера обмена" },
	Entry{ "nexo_connection_import_file", "Импорт из файла" },
	Entry{ "nexo_connection_import_file_title", "Выберите файл со ссылками на прокси" },
	Entry{ "nexo_connection_import_about", "Вставьте или загрузите любой текст со ссылками tg://proxy, tg://socks или t.me/proxy. Повторы пропускаются, текущий прокси не меняется." },
	Entry{ "nexo_connection_import_result", "Добавлено: {added}. Уже в списке: {existing}. С ошибкой: {invalid}." },
	Entry{ "nexo_connection_import_none", "Ссылки на прокси не найдены." },
	Entry{ "nexo_connection_import_too_big", "Файл слишком большой. Максимум 1 МБ." },
	Entry{ "nexo_connection_import_read_error", "Не удалось прочитать файл." },
	Entry{ "nexo_connection_checks_title", "Проверка" },
	Entry{ "nexo_connection_pick_fastest", "Проверить все и выбрать самый быстрый" },
	Entry{ "nexo_connection_checking", "Проверяем..." },
	Entry{ "nexo_connection_picked", "Подключено через {server}, пинг {ping} мс." },
	Entry{ "nexo_connection_none_available", "Ни один прокси не ответил." },
	Entry{ "nexo_connection_empty", "Сначала добавьте прокси." },
	Entry{ "nexo_connection_remove_dead", "Удалить нерабочие прокси" },
	Entry{ "nexo_connection_remove_dead_none", "Все прокси отвечают." },
	Entry{ "nexo_connection_remove_dead_confirm#one", "Удалить {count} прокси, который не ответил?" },
	Entry{ "nexo_connection_remove_dead_confirm#few", "Удалить {count} прокси, которые не ответили?" },
	Entry{ "nexo_connection_remove_dead_confirm#many", "Удалить {count} прокси, которые не ответили?" },
	Entry{ "nexo_connection_remove_dead_confirm#other", "Удалить {count} прокси, которые не ответили?" },
	Entry{ "nexo_connection_removed#one", "Удалён {count} прокси." },
	Entry{ "nexo_connection_removed#few", "Удалено {count} прокси." },
	Entry{ "nexo_connection_removed#many", "Удалено {count} прокси." },
	Entry{ "nexo_connection_removed#other", "Удалено {count} прокси." },
	Entry{ "nexo_connection_checks_about", "Каждый прокси проверяется подключением к Telegram через него. Сообщения остаются зашифрованными, но владелец прокси видит ваш IP-адрес и время, когда вы в сети." },
	Entry{ "nexo_nav_chats", "Чаты" },
	Entry{ "nexo_nav_contacts", "Контакты" },
	Entry{ "nexo_nav_settings", "Настройки" },
	Entry{ "nexo_nav_profile", "Профиль" },
	Entry{ "nexo_appearance_title", "Оформление" },
	Entry{ "nexo_appearance_bottom_nav", "Нижняя панель навигации" },
	Entry{ "nexo_appearance_bottom_nav_about", "Кнопки «Чаты», «Контакты», «Настройки» и «Профиль» под списком чатов. Скрывается, когда список чатов слишком узкий." },
	Entry{ "nexo_appearance_themes", "Темы, цвета и обои" },
	Entry{ "nexo_appearance_more", "Шрифты, скругления и другое" },
};

[[nodiscard]] bool IsRussian() {
	const auto &instance = ::Lang::GetInstance();
	return instance.id().startsWith(u"ru"_q)
		|| instance.baseId().startsWith(u"ru"_q);
}

void Apply() {
	if (!IsRussian()) {
		return;
	}
	auto &instance = ::Lang::GetInstance();
	for (const auto &entry : kRussian) {
		instance.resetValue(entry.key);
		instance.applyValue(entry.key, entry.value);
	}
	instance.updatePluralRules();
}

} // namespace

void Init() {
	static auto lifetime = rpl::lifetime();
	Apply();
	// Re-apply after every language pack change or update.
	::Lang::GetInstance().updated() | rpl::on_next(Apply, lifetime);
}

} // namespace Nexo::Lang
