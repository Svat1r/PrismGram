// Profile banners for PrismGram (plugin API v1, "profile.banner").
//
// A banner is set by writing a link in your own bio:
//   banner: https://i.imgur.com/XXXX.jpg
// or by putting a direct image link (.jpg .jpeg .png .webp .gif) there.
// PrismGram downloads the link only if it really is in that user's bio.

var MARKED = /banner\s*[:=]\s*(https:\/\/\S+)/i;
var IMAGE = /(https:\/\/\S+\.(?:jpe?g|png|webp|gif))(?:\?\S*)?/i;

function trimLink(link) {
	// Drop punctuation that often follows a link in text.
	return link.replace(/[)\],.;!]+$/, "");
}

function onLoad() {
	nexo.profile.setBannerResolver(function (user) {
		var bio = user.bio || "";
		var marked = bio.match(MARKED);
		if (marked) {
			return trimLink(marked[1]);
		}
		var image = bio.match(IMAGE);
		return image ? trimLink(image[0]) : null;
	});
	nexo.log("Profile banners ready");
}
