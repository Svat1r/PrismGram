// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/hidden/nexo_hidden.h"

#include "base/openssl_help.h"
#include "base/random.h"
#include "base/timer.h"
#include "data/data_peer.h"
#include "lang/lang_instance.h"
#include "main/main_session.h"
#include "ui/layers/generic_box.h"
#include "ui/rp_widget.h"
#include "ui/widgets/fields/password_input.h"
#include "ui/widgets/labels.h"
#include "window/window_session_controller.h"
#include "styles/style_boxes.h"
#include "styles/style_layers.h"

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>
#include <QtGui/QGuiApplication>

namespace Nexo::Hidden {
namespace {

constexpr auto kLockDelay = crl::time(60 * 1000);

struct State {
	bool loaded = false;
	bool unlocked = false;
	QByteArray salt;
	QByteArray hash;
	// Session user id -> hidden peer ids.
	base::flat_map<uint64, base::flat_set<uint64>> peers;
	rpl::event_stream<> changes;
	std::unique_ptr<base::Timer> lockTimer;
};

State &Store() {
	static auto result = State();
	return result;
}

[[nodiscard]] QString Text(const char *en, const char *ru) {
	return Lang::GetInstance().id().startsWith(u"ru"_q)
		? QString::fromUtf8(ru)
		: QString::fromUtf8(en);
}

[[nodiscard]] QString ConfigPath() {
	return cWorkingDir() + u"tdata/nexo/hidden.json"_q;
}

void Save() {
	const auto &state = Store();
	auto peers = QJsonObject();
	for (const auto &[user, ids] : state.peers) {
		auto list = QJsonArray();
		for (const auto id : ids) {
			list.push_back(QString::number(id));
		}
		peers.insert(QString::number(user), list);
	}
	auto object = QJsonObject();
	object.insert(u"salt"_q, QString::fromLatin1(state.salt.toBase64()));
	object.insert(u"hash"_q, QString::fromLatin1(state.hash.toBase64()));
	object.insert(u"peers"_q, peers);
	auto file = QSaveFile(ConfigPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
		file.commit();
	}
}

void EnsureLoaded() {
	auto &state = Store();
	if (state.loaded) {
		return;
	}
	state.loaded = true;

	// Hide again a minute after PrismGram leaves the foreground.
	state.lockTimer = std::make_unique<base::Timer>([] { Lock(); });
	QObject::connect(
		qApp,
		&QGuiApplication::applicationStateChanged,
		[](Qt::ApplicationState appState) {
			auto &state = Store();
			if (appState == Qt::ApplicationActive) {
				state.lockTimer->cancel();
			} else if (state.unlocked) {
				state.lockTimer->callOnce(kLockDelay);
			}
		});

	auto file = QFile(ConfigPath());
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	const auto object = QJsonDocument::fromJson(file.readAll()).object();
	state.salt = QByteArray::fromBase64(
		object.value(u"salt"_q).toString().toLatin1());
	state.hash = QByteArray::fromBase64(
		object.value(u"hash"_q).toString().toLatin1());
	const auto peers = object.value(u"peers"_q).toObject();
	for (auto i = peers.begin(); i != peers.end(); ++i) {
		auto &ids = state.peers[i.key().toULongLong()];
		for (const auto &value : i.value().toArray()) {
			ids.emplace(value.toString().toULongLong());
		}
	}
}

[[nodiscard]] uint64 UserKey(not_null<Main::Session*> session) {
	return session->userId().bare;
}

[[nodiscard]] QByteArray HashPin(const QString &pin, const QByteArray &salt) {
	const auto data = salt + pin.toUtf8();
	const auto hash = openssl::Sha256(bytes::make_span(data));
	return QByteArray(
		reinterpret_cast<const char*>(hash.data()),
		int(hash.size()));
}

[[nodiscard]] bool HasPin() {
	EnsureLoaded();
	return !Store().hash.isEmpty();
}

[[nodiscard]] bool CheckPin(const QString &pin) {
	const auto &state = Store();
	return !state.hash.isEmpty() && HashPin(pin, state.salt) == state.hash;
}

void SetPin(const QString &pin) {
	auto &state = Store();
	state.salt = QByteArray(16, Qt::Uninitialized);
	base::RandomFill(state.salt.data(), std::size_t(state.salt.size()));
	state.hash = HashPin(pin, state.salt);
	Save();
}

void Fire() {
	Store().changes.fire({});
}

// A box with one PIN field; check() returns false to keep the box open.
void AskPin(
		not_null<Window::SessionController*> controller,
		QString title,
		QString about,
		Fn<bool(QString)> check) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		box->setTitle(rpl::single(title));
		if (!about.isEmpty()) {
			box->addRow(object_ptr<Ui::FlatLabel>(box, about, st::boxLabel));
		}
		const auto wrap = box->addRow(object_ptr<Ui::RpWidget>(box));
		const auto field = Ui::CreateChild<Ui::PasswordInput>(
			wrap,
			st::defaultInputField,
			rpl::single(u"PIN"_q));
		wrap->resize(wrap->width(), field->height());
		wrap->widthValue() | rpl::on_next([=](int width) {
			field->resizeToWidth(width);
			field->moveToLeft(0, 0);
		}, field->lifetime());
		box->setFocusCallback([=] { field->setFocusFast(); });
		const auto submit = [=] {
			if (check(field->getLastText())) {
				box->closeBox();
			} else {
				field->showError();
			}
		};
		QObject::connect(field, &Ui::MaskedInputField::submitted, [=] {
			submit();
		});
		box->addButton(rpl::single(u"OK"_q), submit);
		box->addButton(
			rpl::single(Text("Cancel", "Отмена")),
			[=] { box->closeBox(); });
	}));
}

void CreatePin(
		not_null<Window::SessionController*> controller,
		Fn<void()> done) {
	AskPin(
		controller,
		Text("New PIN for hidden chats", "Новый PIN для скрытых чатов"),
		Text(
			"4 to 12 digits. Without it the hidden chats can't be shown again.",
			"От 4 до 12 цифр. Без него скрытые чаты не показать."),
		[=](QString pin) {
			static const auto re = QRegularExpression(u"^\\d{4,12}$"_q);
			if (!re.match(pin).hasMatch()) {
				return false;
			}
			SetPin(pin);
			if (done) {
				done();
			}
			return true;
		});
}

} // namespace

bool IsHidden(not_null<PeerData*> peer) {
	EnsureLoaded();
	const auto &peers = Store().peers;
	const auto i = peers.find(UserKey(&peer->session()));
	return (i != peers.end()) && i->second.contains(peer->id.value);
}

bool ShouldHide(not_null<PeerData*> peer) {
	EnsureLoaded();
	return !Store().unlocked && IsHidden(peer);
}

bool Active(not_null<Main::Session*> session) {
	EnsureLoaded();
	return !Store().unlocked && Count(session) > 0;
}

bool Locked() {
	EnsureLoaded();
	return !Store().unlocked;
}

int Count(not_null<Main::Session*> session) {
	EnsureLoaded();
	const auto &peers = Store().peers;
	const auto i = peers.find(UserKey(session));
	return (i != peers.end()) ? int(i->second.size()) : 0;
}

rpl::producer<> Changes() {
	return Store().changes.events();
}

void Lock() {
	EnsureLoaded();
	auto &state = Store();
	if (state.unlocked) {
		state.unlocked = false;
		Fire();
	}
}

void Unlock(
		not_null<Window::SessionController*> controller,
		Fn<void()> done) {
	const auto open = [=] {
		Store().unlocked = true;
		Fire();
		if (done) {
			done();
		}
	};
	if (!HasPin()) {
		CreatePin(controller, open);
		return;
	}
	AskPin(controller, Text("Enter PIN", "Введите PIN"), QString(), [=](
			QString pin) {
		if (!CheckPin(pin)) {
			return false;
		}
		open();
		return true;
	});
}

void Toggle(
		not_null<Window::SessionController*> controller,
		not_null<PeerData*> peer) {
	const auto weak = base::make_weak(controller);
	const auto apply = [=] {
		auto &ids = Store().peers[UserKey(&peer->session())];
		const auto hide = !ids.contains(peer->id.value);
		if (hide) {
			ids.emplace(peer->id.value);
		} else {
			ids.remove(peer->id.value);
		}
		Save();
		Fire();
		if (const auto strong = weak.get()) {
			strong->showToast(hide
				? Text(
					"Chat hidden. Show hidden chats: Settings > PrismGram > Hidden chats.",
					"Чат скрыт. Показать скрытые: Настройки → PrismGram → Скрытые чаты.")
				: Text("The chat is visible again.", "Чат снова виден."));
		}
	};
	if (!HasPin()) {
		CreatePin(controller, apply);
	} else {
		apply();
	}
}

void ChangePin(not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	const auto changed = [=] {
		if (const auto strong = weak.get()) {
			strong->showToast(Text("PIN changed.", "PIN изменён."));
		}
	};
	if (!HasPin()) {
		CreatePin(controller, changed);
		return;
	}
	AskPin(controller, Text("Current PIN", "Текущий PIN"), QString(), [=](
			QString pin) {
		if (!CheckPin(pin)) {
			return false;
		}
		if (const auto strong = weak.get()) {
			CreatePin(strong, changed);
		}
		return true;
	});
}

} // namespace Nexo::Hidden
