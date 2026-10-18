// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/profile/profile_banners.h"

#include "data/data_changes.h"
#include "data/data_user.h"
#include "main/main_session.h"
#include "nexo/plugins/plugin_manager.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Nexo::ProfileBanners {
namespace {

constexpr auto kMaxImageSize = qint64(5 * 1024 * 1024);
constexpr auto kMaxSide = 1600;
constexpr auto kMaxCached = 40;

struct Loading {
	std::vector<Fn<void(QImage)>> callbacks;
};

base::flat_map<QString, QImage> &Cache() {
	static auto result = base::flat_map<QString, QImage>();
	return result;
}

base::flat_map<QString, Loading> &InFlight() {
	static auto result = base::flat_map<QString, Loading>();
	return result;
}

// https only, a real host name: no localhost, .local or IP literals, so a
// bio can't make every viewer probe their own network.
[[nodiscard]] bool IsAllowedLink(const QString &link) {
	const auto url = QUrl(link);
	if (!url.isValid() || url.scheme() != u"https"_q) {
		return false;
	}
	const auto host = url.host().toLower();
	if (host.isEmpty()
		|| host == u"localhost"_q
		|| host.endsWith(u".local"_q)
		|| host.endsWith(u".localhost"_q)
		|| !QHostAddress(host).isNull()
		|| !host.contains('.')) {
		return false;
	}
	return true;
}

void Load(const QString &link, Fn<void(QImage)> done) {
	if (const auto i = Cache().find(link); i != end(Cache())) {
		done(i->second);
		return;
	}
	auto &loading = InFlight()[link];
	loading.callbacks.push_back(std::move(done));
	if (loading.callbacks.size() > 1) {
		return;
	}
	const auto manager = new QNetworkAccessManager();
	auto request = QNetworkRequest(QUrl(link));
	request.setRawHeader("User-Agent", "PrismGram");
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	const auto reply = manager->get(request);
	QObject::connect(
		reply,
		&QNetworkReply::downloadProgress,
		[=](qint64 received, qint64 total) {
			if (received > kMaxImageSize || total > kMaxImageSize) {
				reply->abort();
			}
		});
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		auto image = QImage();
		if (reply->error() == QNetworkReply::NoError) {
			image = QImage::fromData(reply->read(kMaxImageSize));
			if (!image.isNull()
				&& (image.width() > kMaxSide || image.height() > kMaxSide)) {
				image = image.scaled(
					kMaxSide,
					kMaxSide,
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
			}
		}
		reply->deleteLater();
		manager->deleteLater();
		auto &cache = Cache();
		if (int(cache.size()) >= kMaxCached) {
			cache.erase(cache.begin());
		}
		cache[link] = image;
		auto callbacks = base::take(InFlight()[link].callbacks);
		InFlight().remove(link);
		for (const auto &callback : callbacks) {
			callback(image);
		}
	});
}

[[nodiscard]] QString Resolve(not_null<UserData*> user) {
	const auto bio = user->about();
	if (bio.isEmpty()) {
		return QString();
	}
	const auto json = QJsonDocument(QJsonObject{
		{ u"id"_q, QString::number(user->id.value) },
		{ u"username"_q, user->username() },
		{ u"firstName"_q, user->firstName },
		{ u"bio"_q, bio },
	}).toJson(QJsonDocument::Compact);
	const auto link = Plugins::Manager::Instance().resolveBanner(json).trimmed();
	// The link must be written in the bio itself.
	return (!link.isEmpty() && bio.contains(link) && IsAllowedLink(link))
		? link
		: QString();
}

} // namespace

rpl::producer<QImage> Value(not_null<UserData*> user) {
	return [=](auto consumer) {
		auto lifetime = rpl::lifetime();
		const auto guard = lifetime.make_state<base::has_weak_ptr>();
		const auto current = lifetime.make_state<QString>();
		const auto refresh = [=] {
			const auto link = Resolve(user);
			if (link == *current) {
				return;
			}
			*current = link;
			if (link.isEmpty()) {
				consumer.put_next(QImage());
				return;
			}
			Load(link, crl::guard(guard, [=](QImage image) {
				if (*current == link) {
					consumer.put_next(std::move(image));
				}
			}));
		};
		consumer.put_next(QImage());
		rpl::merge(
			user->session().changes().peerFlagsValue(
				user,
				Data::PeerUpdate::Flag::About) | rpl::to_empty,
			Plugins::Manager::Instance().changes()
		) | rpl::on_next(refresh, lifetime);
		return lifetime;
	};
}

} // namespace Nexo::ProfileBanners
