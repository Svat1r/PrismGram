// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#include "nexo/updates/nexo_updates.h"

#include "nexo/nexo_version.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Nexo::Updates {
namespace {

constexpr auto kReleasesApi
	= "https://api.github.com/repos/Svat1r/NexoDesk/releases/latest";
constexpr auto kMaxNotesSize = 256 * 1024;

rpl::variable<int> FeedPluginApiVariable = 0;

[[nodiscard]] QString PreviousListPath() {
	return cWorkingDir() + u"tupdates/previous/files.txt"_q;
}

} // namespace

void SetFeedPluginApi(int version) {
	FeedPluginApiVariable = version;
}

int FeedPluginApi() {
	return FeedPluginApiVariable.current();
}

rpl::producer<int> FeedPluginApiValue() {
	return FeedPluginApiVariable.value();
}

void FetchReleaseNotes(Fn<void(ReleaseNotes)> done) {
	const auto manager = new QNetworkAccessManager();
	auto request = QNetworkRequest(QUrl(QString::fromLatin1(kReleasesApi)));
	request.setRawHeader("Accept", "application/vnd.github+json");
	request.setRawHeader("User-Agent", "NexoDesk");
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	const auto reply = manager->get(request);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		auto result = ReleaseNotes();
		if (reply->error() != QNetworkReply::NoError) {
			result.error = reply->errorString();
		} else {
			const auto object = QJsonDocument::fromJson(
				reply->read(kMaxNotesSize)).object();
			result.tag = object.value(u"tag_name"_q).toString();
			result.title = object.value(u"name"_q).toString();
			result.body = object.value(u"body"_q).toString();
			if (result.tag.isEmpty()) {
				result.error = u"No release information."_q;
			}
		}
		reply->deleteLater();
		manager->deleteLater();
		done(std::move(result));
	});
}

bool HasPreviousVersion() {
	return QFile::exists(PreviousListPath());
}

bool StartRollback(QString *error) {
	const auto updater = cExeDir() + u"Updater.exe"_q;
	if (!QFile::exists(updater)) {
		if (error) {
			*error = u"Updater.exe was not found next to the app."_q;
		}
		return false;
	} else if (!HasPreviousVersion()) {
		if (error) {
			*error = u"No previous version is kept."_q;
		}
		return false;
	}
	// Updater.exe resolves tupdates relative to its working directory.
	const auto started = QProcess::startDetached(
		updater,
		{ u"-rollback"_q, u"-exename"_q, QString::fromLatin1("NexoDesk.exe") },
		cWorkingDir());
	if (!started && error) {
		*error = u"Could not start Updater.exe."_q;
	}
	return started;
}

} // namespace Nexo::Updates
