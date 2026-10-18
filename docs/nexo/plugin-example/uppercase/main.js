// Example Nexo Desk plugin (plugin API v1).
// Globals: `nexo` (API), optional `onLoad()` and `onUnload()`.

function onLoad() {
	nexo.log("Loaded in Nexo Desk " + nexo.version);
	nexo.menu.addMessageItem("Copy in uppercase", function (message) {
		// `message.text` is present only with the "messages.read" permission.
		if (!message.text) {
			nexo.toast("This message has no text.");
			return;
		}
		nexo.copyText(message.text.toUpperCase());
		var used = nexo.settings.get("used", 0) + 1;
		nexo.settings.set("used", used);
		nexo.toast("Copied. Used " + used + " times.");
	});
}

function onUnload() {
	nexo.log("Unloaded");
}
