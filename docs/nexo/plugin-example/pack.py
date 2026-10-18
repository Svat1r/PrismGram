"""Packs a plugin folder into <id>.nexoplugin (a zip). Usage: python pack.py uppercase"""
import json, os, sys, zipfile
folder = sys.argv[1]
with open(os.path.join(folder, 'manifest.json'), encoding='utf-8') as f:
    plugin_id = json.load(f)['id']
out = plugin_id + '.nexoplugin'
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
    for name in sorted(os.listdir(folder)):
        z.write(os.path.join(folder, name), name)
print('written', out)
