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
	Entry{ "nexo_proxy_chip", "Прокси" },
	Entry{ "nexo_appearance_proxy_chip", "Кнопка прокси в поиске" },
	Entry{ "nexo_appearance_proxy_chip_about", "Показывает, используется ли прокси. Нажмите, чтобы открыть настройки подключения." },
	Entry{ "nexo_appearance_nexo_themes", "Темы Nexo" },
	Entry{ "nexo_appearance_nexo_themes_about", "Тёмные темы, сделанные для Nexo Desk. Темы Telegram, акцентные цвета и обои по-прежнему доступны ниже." },
	Entry{ "nexo_plugins_title", "Плагины" },
	Entry{ "nexo_plugins_import", "Импорт плагина" },
	Entry{ "nexo_plugins_open_folder", "Открыть папку плагинов" },
	Entry{ "nexo_plugins_about", "Плагины — это JavaScript, который Nexo Desk выполняет с разрешёнными вами правами. Они не изолированы от приложения: устанавливайте только плагины, которым доверяете. Плагины exteraGram и AyuGram для Android здесь не работают." },
	Entry{ "nexo_plugins_installed", "Установленные" },
	Entry{ "nexo_plugins_empty", "Плагины не установлены. Импортируйте файл .nexoplugin." },
	Entry{ "nexo_plugins_state_running", "Работает" },
	Entry{ "nexo_plugins_state_disabled", "Выключен" },
	Entry{ "nexo_plugins_state_incompatible", "Несовместим" },
	Entry{ "nexo_plugins_state_broken", "Повреждён" },
	Entry{ "nexo_plugins_state_error", "Ошибка" },
	Entry{ "nexo_plugins_field_version", "Версия" },
	Entry{ "nexo_plugins_field_author", "Автор" },
	Entry{ "nexo_plugins_field_source", "Источник" },
	Entry{ "nexo_plugins_source_none", "не указан" },
	Entry{ "nexo_plugins_field_permissions", "Разрешения" },
	Entry{ "nexo_plugins_permissions_none", "нет" },
	Entry{ "nexo_plugins_field_state", "Состояние" },
	Entry{ "nexo_plugins_perm_menu_message", "Добавлять пункты в меню сообщения" },
	Entry{ "nexo_plugins_perm_settings", "Хранить свои настройки" },
	Entry{ "nexo_plugins_perm_messages_read", "Читать текст сообщения, которое вы выбрали в меню" },
	Entry{ "nexo_plugins_perm_clipboard", "Копировать текст в буфер обмена, когда вы нажимаете его пункт" },
	Entry{ "nexo_plugins_enable", "Включить" },
	Entry{ "nexo_plugins_disable", "Выключить" },
	Entry{ "nexo_plugins_show_log", "Журнал" },
	Entry{ "nexo_plugins_log_empty", "Журнал пуст." },
	Entry{ "nexo_plugins_remove", "Удалить" },
	Entry{ "nexo_plugins_remove_sure", "Удалить плагин {name}? Его настройки сохранятся." },
	Entry{ "nexo_plugins_install_title", "Установить плагин?" },
	Entry{ "nexo_plugins_install", "Установить" },
	Entry{ "nexo_plugins_update", "Обновить" },
	Entry{ "nexo_plugins_install_unverified", "Источник не указан, автор не проверен." },
	Entry{ "nexo_plugins_install_warning", "Плагин будет выполнять код внутри Nexo Desk. После установки он выключен, включите его в списке." },
	Entry{ "nexo_plugins_incompatible_warning", "В этой версии он работать не будет: {reason}" },
	Entry{ "nexo_plugins_installed_toast", "Плагин {name} установлен. Включите его в списке." },
	Entry{ "nexo_plugins_install_failed", "Не удалось установить плагин: {error}" },
	Entry{ "nexo_plugins_action_failed", "Плагин {name} не смог выполнить действие. Подробности в журнале." },
	Entry{ "nexo_plugins_stopped", "Плагин {name} остановлен после повторных ошибок." },
	Entry{ "nexo_about_based", "На основе {base} {version}." },
	Entry{ "nexo_about_plugin_api", "Версия API плагинов: {version}" },
	Entry{ "nexo_appearance_chats", "Чаты" },
	Entry{ "nexo_appearance_centered_title", "Название чата по центру" },
	Entry{ "nexo_appearance_no_rounding", "Точные числа (1 234 вместо 1,2K)" },
	Entry{ "nexo_appearance_mention_comma", "Запятая после упоминания в начале" },
	Entry{ "nexo_appearance_chats_about", "Запятая ставится, когда вы выбираете человека из подсказок в начале сообщения." },
	Entry{ "nexo_appearance_sticker_size", "Размер стикеров" },
	Entry{ "nexo_appearance_sticker_default", "Обычный" },
	Entry{ "nexo_appearance_sticker_medium", "Средний" },
	Entry{ "nexo_appearance_sticker_small", "Маленький" },
	Entry{ "nexo_appearance_sticker_tiny", "Мини" },
	Entry{ "nexo_appearance_sticker_about", "Применяется, когда чат открыт заново." },
	Entry{ "nexo_connection_experimental", "Экспериментально" },
	Entry{ "nexo_connection_speed_boost", "Ускорить загрузку и отправку" },
	Entry{ "nexo_connection_speed_boost_about", "Отправляет вдвое больше параллельных запросов за файлами. Telegram может притормаживать слишком активные аккаунты: если загрузка застревает, выключите." },
	Entry{ "nexo_client_title", "Nexo Desk" },
	Entry{ "nexo_client_version", "Версия" },
	Entry{ "nexo_client_base", "Основа" },
	Entry{ "nexo_client_plugin_api", "API плагинов" },
	Entry{ "nexo_updates_title", "Обновления" },
	Entry{ "nexo_updates_notes", "Что нового" },
	Entry{ "nexo_updates_notes_loading", "Загружаем описание релиза..." },
	Entry{ "nexo_updates_notes_failed", "Не удалось загрузить описание: {error}" },
	Entry{ "nexo_updates_notes_empty", "У этого релиза нет описания." },
	Entry{ "nexo_updates_rollback", "Вернуть предыдущую версию" },
	Entry{ "nexo_updates_rollback_sure", "Nexo Desk закроется, вернёт версию до последнего обновления и запустится снова. Данные сохранятся." },
	Entry{ "nexo_updates_rollback_button", "Вернуть" },
	Entry{ "nexo_updates_plugins_warning", "Новая версия использует API плагинов {version}. Плагины для текущей версии после обновления выключатся, их файлы и настройки сохранятся." },
	Entry{ "nexo_updates_about", "Обновления загружаются с github.com/Svat1r/NexoDesk и устанавливаются, только если подпись совпадает с ключом Nexo Desk." },
	Entry{ "nexo_backup_title", "Резервная копия" },
	Entry{ "nexo_backup_export", "Сохранить настройки в файл" },
	Entry{ "nexo_backup_import", "Загрузить настройки из файла" },
	Entry{ "nexo_backup_exported", "Настройки сохранены." },
	Entry{ "nexo_backup_failed", "Не получилось: {error}" },
	Entry{ "nexo_backup_import_sure", "Загрузить настройки из копии Nexo Desk {version} от {date}? Текущие настройки Nexo, AyuGram и плагинов будут заменены, приложение перезапустится." },
	Entry{ "nexo_backup_import_restart", "Загрузить и перезапустить" },
	Entry{ "nexo_backup_about", "В файле настройки Nexo и AyuGram, плагины и их настройки. Аккаунта, ключей входа и сообщений в нём нет." },
	Entry{ "nexo_banners_title", "Баннеры" },
	Entry{ "nexo_banners_profile_avatars", "Аватарка как баннер в каждом профиле" },
	Entry{ "nexo_banners_blur", "Размытие" },
	Entry{ "nexo_banners_darken", "Затемнение" },
	Entry{ "nexo_banners_crop", "Обрезка" },
	Entry{ "nexo_banners_crop_top", "Сверху" },
	Entry{ "nexo_banners_crop_center", "По центру" },
	Entry{ "nexo_banners_crop_bottom", "Снизу" },
	Entry{ "nexo_banners_about", "Поставить картинку на любой профиль или канал: откройте профиль, затем ⋮ → Баннер. Баннеры хранятся на этом компьютере, их видите только вы." },
	Entry{ "nexo_banners_menu", "Баннер" },
	Entry{ "nexo_banners_choose", "Выбрать картинку" },
	Entry{ "nexo_banners_use_avatar", "Сделать из аватарки" },
	Entry{ "nexo_banners_remove", "Убрать баннер" },
	Entry{ "nexo_banners_set", "Баннер установлен." },
	Entry{ "nexo_banners_too_big", "Картинка больше 30 МБ." },
	Entry{ "nexo_banners_bad_image", "Не удалось прочитать картинку." },
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
