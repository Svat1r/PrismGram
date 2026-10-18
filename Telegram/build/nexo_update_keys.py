"""Nexo Desk update keys: create once, then use for every release.

  python nexo_update_keys.py --keys-dir C:\\TBuild\\nexo-keys create

Creates (never overwrites) in --keys-dir:
  root-private.pem       Ed25519 root key. Signs the manifest only.
                         Keep it offline; losing it means no new manifests.
  release-private.pem    Ed25519 release key "nexo-rl-<year>a". Signs
                         update packages (packer -local-key).
and writes the public side into Telegram/Resources/update/:
  root-public.pem, manifest.min.json, manifest.sig

  python nexo_update_keys.py --keys-dir ... show

prints the key ids and public keys.

The format is Telegram Desktop's v2 update manifest (see
SourceFiles/core/update_verify.cpp): the root key signs the exact
manifest bytes, the manifest lists the keys allowed for each channel.
Requires the `openssl` command (OpenSSL 3).
"""
import argparse
import base64
import datetime
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
UPDATE_DIR = os.path.join(HERE, '..', 'Resources', 'update')
YEARS_VALID = 5


def run(*args, data=None):
    return subprocess.run(args, input=data, check=True, capture_output=True).stdout


def b64url(raw):
    return base64.urlsafe_b64encode(raw).rstrip(b'=').decode('ascii')


def raw_public(private_pem):
    der = run('openssl', 'pkey', '-in', private_pem, '-pubout', '-outform', 'DER')
    return der[-32:]  # Ed25519 SubjectPublicKeyInfo ends with the 32 raw bytes.


def create(keys_dir):
    os.makedirs(keys_dir, exist_ok=True)
    root = os.path.join(keys_dir, 'root-private.pem')
    release = os.path.join(keys_dir, 'release-private.pem')
    for path in (root, release):
        if os.path.exists(path):
            sys.exit('Refusing to overwrite ' + path)
    run('openssl', 'genpkey', '-algorithm', 'ed25519', '-out', root)
    run('openssl', 'genpkey', '-algorithm', 'ed25519', '-out', release)

    now = datetime.datetime.now(datetime.timezone.utc)
    release_id = 'nexo-rl-%da' % now.year
    with open(os.path.join(keys_dir, 'release-key-id.txt'), 'w') as f:
        f.write(release_id + '\n')

    manifest = {
        'format': 1,
        'manifest_version': 1,
        'issued': int(now.timestamp()),
        'expires': int(now.replace(year=now.year + YEARS_VALID).timestamp()),
        'keys': [{
            'id': release_id,
            'alg': 'Ed25519',
            'x': b64url(raw_public(release)),
        }],
        'channels': {
            'stable': [[release_id]],
            'beta': [[release_id]],
        },
        'revoked': [],
    }
    manifest_bytes = json.dumps(manifest, separators=(',', ':')).encode('utf-8')
    manifest_path = os.path.join(UPDATE_DIR, 'manifest.min.json')
    with open(manifest_path, 'wb') as f:
        f.write(manifest_bytes)
    signature = run(
        'openssl', 'pkeyutl', '-sign', '-inkey', root,
        '-rawin', '-in', manifest_path)
    if len(signature) != 64:
        sys.exit('Unexpected signature size %d' % len(signature))
    with open(os.path.join(UPDATE_DIR, 'manifest.sig'), 'wb') as f:
        f.write(signature)
    root_public = run('openssl', 'pkey', '-in', root, '-pubout')
    with open(os.path.join(UPDATE_DIR, 'root-public.pem'), 'wb') as f:
        f.write(root_public.replace(b'\r\n', b'\n'))
    print('Keys created in', keys_dir)
    print('Release key id:', release_id)
    print('Public files written to', os.path.normpath(UPDATE_DIR))
    print('Now back up', keys_dir, 'somewhere safe (not in git).')


def show(keys_dir):
    for name in ('root-private.pem', 'release-private.pem'):
        path = os.path.join(keys_dir, name)
        print(name, b64url(raw_public(path)) if os.path.exists(path) else 'missing')
    id_path = os.path.join(keys_dir, 'release-key-id.txt')
    if os.path.exists(id_path):
        print('release key id', open(id_path).read().strip())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--keys-dir', required=True)
    parser.add_argument('command', choices=['create', 'show'])
    args = parser.parse_args()
    if args.command == 'create':
        create(args.keys_dir)
    else:
        show(args.keys_dir)


if __name__ == '__main__':
    main()
