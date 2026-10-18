"""Generates every Nexo Desk app icon from one drawing.

Usage (from Telegram/Resources/art/nexo):  python make_icons.py
Writes logo.svg here and replaces the app icon files in Resources/art
that the client and the Windows resource file load. Requires Pillow.
"""
import os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.dirname(HERE)

BG = (62, 136, 247, 255)      # Nexo blue, #3E88F7
FG = (255, 255, 255, 255)
SCALE = 1024                  # master size, everything is drawn in 0..1

# The "N" stroke and the small orbit dot, in unit coordinates.
N_POINTS = [(0.33, 0.70), (0.33, 0.30), (0.67, 0.70), (0.67, 0.30)]
N_WIDTH = 0.105
DOT = (0.755, 0.245, 0.045)   # center x, center y, radius

SVG = """<svg width="512" height="512" viewBox="0 0 512 512" xmlns="http://www.w3.org/2000/svg">
<circle cx="256" cy="256" r="256" fill="#3E88F7"/>
<path d="M169 358V154L343 358V154" fill="none" stroke="#FFFFFF" stroke-width="54" stroke-linecap="round" stroke-linejoin="round"/>
<circle cx="387" cy="125" r="23" fill="#FFFFFF"/>
</svg>
"""


def master(size=SCALE, margin=0.0):
    """Round icon on a transparent square, optional empty margin."""
    supersample = 4
    big = size * supersample
    image = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    pad = margin * big
    inner = big - 2 * pad

    def at(x, y):
        return (pad + x * inner, pad + y * inner)

    draw.ellipse([pad, pad, pad + inner, pad + inner], fill=BG)
    width = int(N_WIDTH * inner)
    points = [at(x, y) for x, y in N_POINTS]
    for a, b in zip(points, points[1:]):
        draw.line([a, b], fill=FG, width=width)
    for x, y in points:  # round caps and joints
        r = (width - 1) / 2
        draw.ellipse([x - r, y - r, x + r, y + r], fill=FG)
    cx, cy, r = DOT
    cx, cy = at(cx, cy)
    r *= inner
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=FG)
    return image.resize((size, size), Image.LANCZOS)


def save_png(name, size, margin=0.0):
    master(size, margin).save(os.path.join(ART, name))


def main():
    with open(os.path.join(HERE, 'logo.svg'), 'w', encoding='utf-8') as f:
        f.write(SVG)
    with open(os.path.join(ART, 'ayu', 'default', 'app.svg'), 'w',
              encoding='utf-8') as f:
        f.write(SVG)

    for size in (16, 32, 48, 64, 128, 256, 512):
        save_png('icon%d.png' % size, size)
        save_png('icon%d@2x.png' % size, size * 2)
    save_png('icon_round512@2x.png', 1024)
    save_png('logo_256.png', 256)
    save_png('logo_256_no_margin.png', 256)

    sizes = [(s, s) for s in (16, 24, 32, 48, 64, 128, 256)]
    ico = master(256)
    ico.save(os.path.join(ART, 'icon256.ico'), sizes=sizes)
    ico.save(os.path.join(ART, 'ayu', 'default', 'app_icon.ico'), sizes=sizes)
    master(512).save(os.path.join(HERE, 'logo_512.png'))
    print('Nexo Desk icons written.')


if __name__ == '__main__':
    main()
