// This file is part of PrismGram, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

namespace Nexo::Updates {

// Plugin API version announced by the update feed ("plugin_api" next to
// "released"), 0 when the feed did not say.
void SetFeedPluginApi(int version);
[[nodiscard]] int FeedPluginApi();
[[nodiscard]] rpl::producer<int> FeedPluginApiValue();

struct ReleaseNotes {
	QString tag;
	QString title;
	QString body;
	QString error;
};

// Reads the latest release from the GitHub API:
// https://api.github.com/repos/<owner>/<repo>/releases/latest
void FetchReleaseNotes(Fn<void(ReleaseNotes)> done);

// The previous version kept by Updater.exe after the last update.
[[nodiscard]] bool HasPreviousVersion();
// Starts Updater.exe -rollback. On success the app must quit right away.
bool StartRollback(QString *error);

} // namespace Nexo::Updates
