// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/banners/nexo_banners.h"

#include "lang_auto.h"
#include "core/file_utilities.h"
#include "data/data_changes.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_user.h"
#include "main/main_session.h"
#include "mainwindow.h"
#include "nexo/profile/profile_banners.h"
#include "ui/image/image_prepare.h"
#include "ui/userpic_view.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>
#include <QtGui/QImageReader>
#include <QtGui/QPainter>

namespace Nexo::Banners {
namespace {

constexpr auto kMaxSide = 1600;
constexpr auto kAvatarSide = 640;
constexpr auto kMaxSourceSize = qint64(30 * 1024 * 1024);

enum class Source {
	None,
	Image,
	Avatar,
};

struct State {
	Options options;
	base::flat_map<uint64, Source> peers;
	rpl::event_stream<> changes;
	bool loaded = false;
};

State &Store() {
	static auto result = State();
	return result;
}

[[nodiscard]] QString Dir() {
	return cWorkingDir() + u"tdata/nexo/banners/"_q;
}

[[nodiscard]] QString ConfigPath() {
	return cWorkingDir() + u"tdata/nexo/banners.json"_q;
}

[[nodiscard]] QString ImagePath(uint64 peerId) {
	return Dir() + QString::number(peerId) + u".jpg"_q;
}

[[nodiscard]] QString CropName(Crop crop) {
	switch (crop) {
	case Crop::Top: return u"top"_q;
	case Crop::Bottom: return u"bottom"_q;
	case Crop::Center: break;
	}
	return u"center"_q;
}

void Save() {
	const auto &data = Store();
	auto peers = QJsonObject();
	for (const auto &[id, source] : data.peers) {
		peers.insert(
			QString::number(id),
			(source == Source::Image) ? u"image"_q : u"avatar"_q);
	}
	const auto object = QJsonObject{
		{ u"version"_q, 1 },
		{ u"blur"_q, data.options.blur },
		{ u"darken"_q, data.options.darken },
		{ u"crop"_q, CropName(data.options.crop) },
		{ u"profileAvatars"_q, data.options.profileAvatars },
		{ u"peers"_q, peers },
	};
	QDir().mkpath(QFileInfo(ConfigPath()).absolutePath());
	auto file = QSaveFile(ConfigPath());
	if (!file.open(QIODevice::WriteOnly)
		|| file.write(QJsonDocument(object).toJson()) < 0
		|| !file.commit()) {
		LOG(("Nexo Banners: could not write %1.").arg(ConfigPath()));
	}
}

void EnsureLoaded() {
	auto &data = Store();
	if (data.loaded) {
		return;
	}
	data.loaded = true;
	auto file = QFile(ConfigPath());
	if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) {
		return;
	}
	const auto object = QJsonDocument::fromJson(file.readAll()).object();
	const auto number = [&](const char *key, int fallback) {
		const auto value = object.value(QString::fromLatin1(key));
		return value.isDouble()
			? std::clamp(value.toInt(), 0, 100)
			: fallback;
	};
	data.options.blur = number("blur", data.options.blur);
	data.options.darken = number("darken", data.options.darken);
	const auto crop = object.value(u"crop"_q).toString();
	data.options.crop = (crop == u"top"_q)
		? Crop::Top
		: (crop == u"bottom"_q)
		? Crop::Bottom
		: Crop::Center;
	data.options.profileAvatars = object.value(
		u"profileAvatars"_q).toBool(false);
	const auto peers = object.value(u"peers"_q).toObject();
	for (auto i = peers.begin(); i != peers.end(); ++i) {
		const auto id = i.key().toULongLong();
		if (!id) {
			continue;
		} else if (i.value().toString() == u"image"_q) {
			data.peers[id] = Source::Image;
		} else if (i.value().toString() == u"avatar"_q) {
			data.peers[id] = Source::Avatar;
		}
	}
}

[[nodiscard]] Source SourceFor(not_null<PeerData*> peer) {
	EnsureLoaded();
	const auto i = Store().peers.find(peer->id.value);
	return (i != end(Store().peers)) ? i->second : Source::None;
}

void SetSource(not_null<PeerData*> peer, Source source) {
	EnsureLoaded();
	auto &data = Store();
	if (source == Source::None) {
		data.peers.remove(peer->id.value);
		QFile::remove(ImagePath(peer->id.value));
	} else {
		data.peers[peer->id.value] = source;
	}
	Save();
	data.changes.fire({});
}

// Blur and darken, as in the original plugin.
[[nodiscard]] QImage ApplyEffects(QImage image, bool smallSource) {
	if (image.isNull()) {
		return image;
	}
	const auto &options = Store().options;
	image = std::move(image).convertToFormat(
		QImage::Format_ARGB32_Premultiplied);
	auto radius = (options.blur * 24) / 100;
	if (smallSource) {
		// Avatars are small: always blur a bit to hide the scaling.
		radius = std::max(radius, 6);
	}
	if (radius > 0) {
		image = Images::BlurLargeImage(std::move(image), radius);
	}
	if (options.darken > 0) {
		auto p = QPainter(&image);
		p.fillRect(
			image.rect(),
			QColor(0, 0, 0, (options.darken * 230) / 100));
	}
	return image;
}

[[nodiscard]] QImage LoadCustom(uint64 peerId) {
	auto reader = QImageReader(ImagePath(peerId));
	reader.setAutoTransform(true);
	return reader.read();
}

void ChooseImage(
		not_null<PeerData*> peer,
		not_null<Window::SessionController*> controller) {
	const auto weak = base::make_weak(controller);
	FileDialog::GetOpenPath(
		controller->widget().get(),
		tr::nexo_banners_choose(tr::now),
		FileDialog::ImagesFilter(),
		[=](FileDialog::OpenResult &&result) {
			const auto strong = weak.get();
			if (!strong || result.paths.isEmpty()) {
				return;
			}
			auto file = QFile(result.paths.front());
			if (file.size() > kMaxSourceSize) {
				strong->showToast(tr::nexo_banners_too_big(tr::now));
				return;
			}
			auto reader = QImageReader(result.paths.front());
			reader.setAutoTransform(true);
			auto image = reader.read();
			if (image.isNull()) {
				strong->showToast(tr::nexo_banners_bad_image(tr::now));
				return;
			}
			if (image.width() > kMaxSide || image.height() > kMaxSide) {
				image = image.scaled(
					kMaxSide,
					kMaxSide,
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
			}
			QDir().mkpath(Dir());
			if (!image.save(ImagePath(peer->id.value), "JPG", 92)) {
				strong->showToast(tr::nexo_banners_bad_image(tr::now));
				return;
			}
			SetSource(peer, Source::Image);
			strong->showToast(tr::nexo_banners_set(tr::now));
		});
}

// The avatar as a square image, once it is loaded.
[[nodiscard]] rpl::producer<QImage> AvatarValue(not_null<PeerData*> peer) {
	return [=](auto consumer) {
		auto lifetime = rpl::lifetime();
		const auto view = lifetime.make_state<::Ui::PeerUserpicView>();
		const auto last = lifetime.make_state<const QImage*>(nullptr);
		const auto check = [=] {
			*view = peer->createUserpicView();
			const auto image = view->cloud.get();
			if (image && image != *last && !image->isNull()) {
				*last = image;
				consumer.put_next(image->scaled(
					kAvatarSide,
					kAvatarSide,
					Qt::IgnoreAspectRatio,
					Qt::SmoothTransformation));
			}
		};
		peer->loadUserpic();
		check();
		rpl::merge(
			peer->session().changes().peerFlagsValue(
				peer,
				::Data::PeerUpdate::Flag::Photo) | rpl::to_empty,
			peer->session().downloaderTaskFinished()
		) | rpl::filter([=] {
			return !*last;
		}) | rpl::on_next(check, lifetime);
		return lifetime;
	};
}

} // namespace

Options CurrentOptions() {
	EnsureLoaded();
	return Store().options;
}

void SetOptions(const Options &options) {
	EnsureLoaded();
	auto &data = Store();
	data.options = options;
	data.options.blur = std::clamp(options.blur, 0, 100);
	data.options.darken = std::clamp(options.darken, 0, 100);
	Save();
	data.changes.fire({});
}

rpl::producer<> Changes() {
	return Store().changes.events();
}

float64 CropAlign() {
	switch (CurrentOptions().crop) {
	case Crop::Top: return 0.;
	case Crop::Bottom: return 1.;
	case Crop::Center: break;
	}
	return 0.5;
}

rpl::producer<QImage> ProfileValue(not_null<PeerData*> peer) {
	return rpl::single(rpl::empty) | rpl::then(
		Changes()
	) | rpl::map([=]() -> rpl::producer<QImage> {
		const auto source = SourceFor(peer);
		if (source == Source::Image) {
			return rpl::single(ApplyEffects(LoadCustom(peer->id.value), false));
		} else if (source == Source::Avatar
			|| CurrentOptions().profileAvatars) {
			return AvatarValue(peer) | rpl::map([](QImage image) {
				return ApplyEffects(std::move(image), true);
			});
		} else if (const auto user = peer->asUser()) {
			return ProfileBanners::Value(user);
		}
		return rpl::single(QImage());
	}) | rpl::flatten_latest();
}

void AddMenuActions(
		not_null<PeerData*> peer,
		not_null<Window::SessionController*> controller,
		const ::Ui::Menu::MenuCallback &addAction) {
	addAction({
		.text = tr::nexo_banners_menu(tr::now),
		.handler = nullptr,
		.icon = &st::menuIconPhoto,
		.fillSubmenu = [=](not_null<::Ui::PopupMenu*> menu) {
			menu->addAction(
				tr::nexo_banners_choose(tr::now),
				[=] { ChooseImage(peer, controller); },
				&st::menuIconPhoto);
			menu->addAction(
				tr::nexo_banners_use_avatar(tr::now),
				[=] {
					SetSource(peer, Source::Avatar);
					controller->showToast(tr::nexo_banners_set(tr::now));
				},
				&st::menuIconProfile);
			if (SourceFor(peer) != Source::None) {
				menu->addAction(
					tr::nexo_banners_remove(tr::now),
					[=] { SetSource(peer, Source::None); },
					&st::menuIconDelete);
			}
		},
	});
}

} // namespace Nexo::Banners
