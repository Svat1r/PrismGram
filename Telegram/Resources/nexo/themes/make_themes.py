"""Generates the built-in PrismGram themes.

Usage (from Telegram/Resources/nexo/themes):  python make_themes.py

Every theme starts from the night palette of Telegram Desktop
(Resources/night.tdesktop-theme) and recolors it:
- blue accent colors get the theme accent hue;
- dark background colors get the theme background tint and lightness.
Everything else (reds for errors, greens for online, text colors) stays.
Output: <id>.tdesktop-theme files next to this script.
"""
import colorsys
import io
import os
import re
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(HERE, '..', '..', 'night.tdesktop-theme')

# id: (accent, background, lightness shift for backgrounds)
THEMES = {
    'nexo-night': ('#3E88F7', '#13171C', 0.0),
    'nexo-amoled': ('#3E88F7', '#000000', 0.0),
    'nexo-ocean': ('#14A3B8', '#0F1A20', 0.0),
    'nexo-violet': ('#8B5CF6', '#15121E', 0.0),
    'nexo-forest': ('#34B36B', '#101A14', 0.0),
}

COLOR = re.compile(r'#([0-9a-fA-F]{6})([0-9a-fA-F]{2})?')
BLUE_HUES = (0.52, 0.68)  # hue range of the night theme's blues


def hls(hex6):
    r, g, b = (int(hex6[i:i + 2], 16) / 255 for i in (0, 2, 4))
    return colorsys.rgb_to_hls(r, g, b)


def to_hex(h, l, s):
    r, g, b = colorsys.hls_to_rgb(h, max(0.0, min(1.0, l)), max(0.0, min(1.0, s)))
    return '%02x%02x%02x' % (round(r * 255), round(g * 255), round(b * 255))


def recolor(hex6, accent, background, shift):
    h, l, s = hls(hex6)
    ah, al, as_ = hls(accent[1:])
    bh, bl, bs = hls(background[1:])
    blue = BLUE_HUES[0] <= h <= BLUE_HUES[1]
    if not blue:
        return hex6
    if s > 0.35 and l > 0.3:
        # Accent: keep the original lightness relation, take accent hue.
        return to_hex(ah, l + (al - 0.55) * 0.5, min(1.0, s * 0.6 + as_ * 0.4))
    if l < 0.3:
        # Background: theme tint, lightness relative to the base windowBg.
        base_bg_l = hls('17212b')[1]
        target = bl + (l - base_bg_l) + shift
        return to_hex(bh, target, bs)
    return hex6


def build(theme_id, accent, background, shift):
    with zipfile.ZipFile(BASE) as z:
        colors = z.read('colors.tdesktop-theme').decode('utf-8')
    def replace(match):
        alpha = match.group(2) or ''
        return '#' + recolor(match.group(1), accent, background, shift) + alpha
    lines = []
    for line in colors.splitlines():
        key, sep, rest = line.partition(':')
        if sep and '#' in rest:
            value, comment, tail = rest.partition('//')
            line = key + sep + COLOR.sub(replace, value) + comment + tail
        lines.append(line)
    colors = '\n'.join(lines) + '\n'

    # Plain background in the theme's own color, 1x1 is enough.
    from PIL import Image
    bg = Image.new('RGB', (1, 1), background)
    png = io.BytesIO()
    bg.save(png, 'PNG')

    out = os.path.join(HERE, theme_id + '.tdesktop-theme')
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('colors.tdesktop-theme', colors)
        z.writestr('background.png', png.getvalue())
    return out


def main():
    for theme_id, (accent, background, shift) in THEMES.items():
        print('written', build(theme_id, accent, background, shift))


if __name__ == '__main__':
    main()
