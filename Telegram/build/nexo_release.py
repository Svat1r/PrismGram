"""Builds the release files of PrismGram for Windows x64.

  python nexo_release.py --keys-dir C:\\TBuild\\nexo-keys
  python nexo_release.py --keys-dir ... --publish   (asks gh to upload)

Expects a finished Release build with auto-update enabled in out/Release
(PrismGram.exe, Updater.exe, Packer.exe). Produces in out/release-<version>:

  td-update-win-x64-<base>-nexo<counter>   signed v2 update package
  current6                                 update feed for the client
  PrismGram-Setup-x64-<version>.exe         installer (Inno Setup 6)
  NOTES.md                                 release notes to edit

The feed and the package go to the GitHub release; the client reads
https://github.com/Svat1r/PrismGram/releases/latest/download/current6.
Nothing is uploaded without --publish, and --publish uses the gh CLI
with your own GitHub login.
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
SOURCES = os.path.join(ROOT, 'Telegram', 'SourceFiles')
RELEASE = os.path.join(ROOT, 'out', 'Release')
UPDATE_KEYS = os.path.join(ROOT, 'Telegram', 'Resources', 'update')
REPO = 'Svat1r/PrismGram'
ISCC_CANDIDATES = [
    os.path.expandvars(r'%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe'),
    r'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
]


def read(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def nexo_versions():
    text = read(os.path.join(SOURCES, 'nexo', 'nexo_version.h'))
    version = re.search(r'kVersion = "([0-9.]+)"', text).group(1)
    counter = int(re.search(r'kUpdateCounter = (\d+);', text).group(1))
    api = int(re.search(r'kPluginApiVersion = (\d+);', text).group(1))
    base = int(re.search(r'AppVersion = (\d+);', read(
        os.path.join(SOURCES, 'core', 'version.h'))).group(1))
    return version, counter, api, base


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--keys-dir', required=True)
    parser.add_argument('--publish', action='store_true')
    args = parser.parse_args()

    version, counter, api, base = nexo_versions()
    key = os.path.join(args.keys_dir, 'release-private.pem')
    key_id = read(os.path.join(args.keys_dir, 'release-key-id.txt')).strip()
    for path in ('PrismGram.exe', 'Updater.exe', 'Packer.exe'):
        if not os.path.exists(os.path.join(RELEASE, path)):
            sys.exit('Missing out/Release/%s: build with auto-update on.' % path)

    out = os.path.join(ROOT, 'out', 'release-' + version)
    os.makedirs(out, exist_ok=True)

    # 1. Signed update package (same packer call as build.bat, local key).
    subprocess.run([
        os.path.join(RELEASE, 'Packer.exe'),
        '-version', str(base),
        '-path', 'PrismGram.exe',
        '-path', 'Updater.exe',
        '-path', r'modules\x64\d3d\d3dcompiler_47.dll',
        '-target', 'win64',
        '-channel', 'stable',
        '-counter', str(counter),
        '-keys-loc', UPDATE_KEYS,
        '-local-key', key,
        '-local-key-id', key_id,
    ], cwd=RELEASE, check=True)
    package = 'td-update-win-x64-%d-nexo%d' % (base, counter)
    shutil.move(os.path.join(RELEASE, package), os.path.join(out, package))

    # 2. Feed read by the client's HttpChecker.
    feed = {
        'win64': {
            'stable': {
                'released': '%d:%d' % (base, counter),
                'link': '/' + package,
                'plugin_api': api,
            },
        },
    }
    with open(os.path.join(out, 'current6'), 'w', encoding='utf-8') as f:
        json.dump(feed, f, separators=(',', ':'))

    # 3. Installer.
    iscc = next((p for p in ISCC_CANDIDATES if os.path.exists(p)), None)
    if not iscc:
        sys.exit('Inno Setup 6 (ISCC.exe) not found.')
    subprocess.run([
        iscc,
        '/DMyAppVersion=' + version,
        '/DReleasePath=' + RELEASE,
        '/O' + out,
        os.path.join(HERE, 'nexo_setup.iss'),
    ], check=True)

    notes = os.path.join(out, 'NOTES.md')
    if not os.path.exists(notes):
        with open(notes, 'w', encoding='utf-8') as f:
            f.write('## PrismGram %s\n\nBased on AyuGram Desktop %d.\n'
                    'Plugin API: %d.\n\n### PrismGram\n\n- \n\n'
                    '### AyuGram changes\n\n- \n\n### Security\n\n- \n'
                    % (version, base, api))
    print('Release files are in', out)

    if args.publish:
        tag = 'v' + version
        files = [os.path.join(out, name) for name in (
            package, 'current6', 'PrismGram-Setup-x64-%s.exe' % version)]
        subprocess.run([
            'gh', 'release', 'create', tag, *files,
            '--repo', REPO,
            '--title', 'PrismGram ' + version,
            '--notes-file', notes,
        ], check=True)
        print('Published', tag)


if __name__ == '__main__':
    main()
