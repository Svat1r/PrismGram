# Releasing PrismGram and how updates work

## Keys (once)

```
python Telegram\build\nexo_update_keys.py --keys-dir C:\TBuild\nexo-keys create
```

- `root-private.pem` (Ed25519) signs the update manifest. Keep offline.
- `release-private.pem` (Ed25519, id in `release-key-id.txt`) signs packages.
- Public parts go to `Telegram/Resources/update/` and are built into the
  client. The private keys never go into git. Back up `C:\TBuild\nexo-keys`:
  without it no update can ever be published for installed clients.

## Versions

- `nexo/nexo_version.h`: `kVersion` (shown to users), `kUpdateCounter`
  (+1 for every release), `kPluginApiVersion`.
- Update version = `(AppVersion << 32) | kUpdateCounter`, so a new Nexo
  release on the same AyuGram base and any new AyuGram base are newer.

## Release steps

1. Bump `kVersion` and `kUpdateCounter`, build Release (auto-update on).
2. `python Telegram\build\nexo_release.py --keys-dir C:\TBuild\nexo-keys`
   writes to `out/release-<version>/`: the signed package
   `td-update-win-x64-<base>-nexo<counter>`, the feed `current6`, the
   installer and `NOTES.md` (edit it: Nexo changes, AyuGram changes,
   security fixes, plugin API changes).
3. Add `--publish` to upload with `gh` to github.com/Svat1r/PrismGram as the
   latest release. Do this only after testing.

## Client side

- Feed: `https://github.com/Svat1r/PrismGram/releases/latest/download/current6`
  `{"win64":{"stable":{"released":"<base>:<counter>","link":"/<package>","plugin_api":1}}}`
- Only v2 packages are accepted. The package must be signed by a key the
  root-signed manifest lists for the channel; v1 (RSA) packages, including
  AyuGram's, are always rejected. HTTPS alone is not trusted.
- Files are replaced by `Updater.exe` only after the whole package is
  verified. Before replacing, every file is backed up; a failed copy puts
  the backups back. After success the backups stay in
  `<data>/tupdates/previous`, and Settings > PrismGram > "Go back to the
  previous version" restores them (`Updater.exe -rollback`).
- If the feed's `plugin_api` differs from the running one and plugins are
  on, Settings shows a warning before the update is installed.

## Tested (2026-10-18, local server)

Feed found, package downloaded, v2 signature verified, files unpacked,
installed by Updater.exe on restart with backups in tupdates/previous,
then `Updater.exe -rollback` restored the files and restarted the app.

## Known limits

- After a rollback, auto-update will offer the same version again; turn
  auto-update off in Settings until a fixed release is out.
- A new version that crashes on start is not rolled back automatically;
  use the button or reinstall the previous installer.
