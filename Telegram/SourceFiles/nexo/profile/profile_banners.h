// This file is part of Nexo Desk, based on AyuGram Desktop and
// Telegram Desktop. Licensed under GPLv3, see LEGAL.
#pragma once

class UserData;

namespace Nexo::ProfileBanners {

// Banner image for a user's profile header, provided by a plugin with the
// "profile.banner" permission. Null image when there is no banner.
//
// The plugin only names a link; the client downloads it, and only if the
// exact link is written in that user's bio. So banners are set by the
// profile owner, and a plugin can't use banner links to send data out.
[[nodiscard]] rpl::producer<QImage> Value(not_null<UserData*> user);

} // namespace Nexo::ProfileBanners
