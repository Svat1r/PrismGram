"""Generates every PrismGram app icon from one drawing.

Usage (from Telegram/Resources/art/nexo):  python make_icons.py
Writes logo.svg here and replaces the app icon files in Resources/art
that the client and the Windows resource file load. Requires Pillow.
"""
import os
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ART = os.path.dirname(HERE)

BG = (21, 23, 28, 255)        # PrismGram night, #15171C
FG = (255, 255, 255, 255)
SCALE = 1024                  # master size, everything is drawn in 0..1

# A prism: white beam in from the left, a spectrum out to the right.
PRISM = [(0.50, 0.24), (0.25, 0.70), (0.75, 0.70)]
EDGE = 0.042                  # prism outline width
BEAM = [(0.06, 0.62), (0.375, 0.47)]
BEAM_WIDTH = 0.05
SPECTRUM = ['#FF4D5E', '#FF9F43', '#FFE14D', '#4DE38A', '#3E9BFF', '#9B6BFF']
RAY_FROM = [(0.600, 0.400), (0.640, 0.474)]   # along the right edge
RAY_TO_X = 0.97
RAY_TO_Y = (0.42, 0.80)
RAY_WIDTH = 0.034


def lerp(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


def rays():
    """Start and end point of every spectrum ray, top to bottom."""
    result = []
    n = len(SPECTRUM)
    for i, color in enumerate(SPECTRUM):
        t = i / (n - 1)
        start = lerp(RAY_FROM[0], RAY_FROM[1], t)
        end = (RAY_TO_X, RAY_TO_Y[0] + (RAY_TO_Y[1] - RAY_TO_Y[0]) * t)
        result.append((color, start, end))
    return result


def svg():
    s = 512
    f = lambda v: '%.1f' % (v * s)
    out = ['<svg width="512" height="512" viewBox="0 0 512 512" xmlns="http://www.w3.org/2000/svg">',
           '<defs><clipPath id="c"><circle cx="256" cy="256" r="256"/></clipPath></defs>',
           '<circle cx="256" cy="256" r="256" fill="#15171C"/>',
           '<g clip-path="url(#c)" stroke-linecap="round">']
    (bx0, by0), (bx1, by1) = BEAM
    out.append('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#FFFFFF" stroke-width="%s"/>'
               % (f(bx0), f(by0), f(bx1), f(by1), f(BEAM_WIDTH)))
    for color, (x0, y0), (x1, y1) in rays():
        out.append('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="%s"/>'
                   % (f(x0), f(y0), f(x1), f(y1), color, f(RAY_WIDTH)))
    out.append('</g>')
    pts = ' '.join('%s,%s' % (f(x), f(y)) for x, y in PRISM)
    out.append('<polygon points="%s" fill="#15171C" fill-opacity="0.55" stroke="#FFFFFF" stroke-width="%s" stroke-linejoin="round"/>'
               % (pts, f(EDGE)))
    out.append('</svg>')
    return '\n'.join(out) + '\n'


def hex_rgba(color):
    return tuple(int(color[i:i + 2], 16) for i in (1, 3, 5)) + (255,)


def master(size=SCALE, margin=0.0):
    """Round icon on a transparent square, optional empty margin."""
    supersample = 4
    big = size * supersample
    pad = margin * big
    inner = big - 2 * pad

    def at(x, y):
        return (pad + x * inner, pad + y * inner)

    def line(draw, a, b, color, width):
        draw.line([a, b], fill=color, width=width)
        r = (width - 1) / 2
        for x, y in (a, b):
            draw.ellipse([x - r, y - r, x + r, y + r], fill=color)

    image = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    ImageDraw.Draw(image).ellipse([pad, pad, pad + inner, pad + inner], fill=BG)

    light = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(light)
    line(draw, at(*BEAM[0]), at(*BEAM[1]), FG, int(BEAM_WIDTH * inner))
    for color, start, end in rays():
        line(draw, at(*start), at(*end), hex_rgba(color), int(RAY_WIDTH * inner))
    mask = Image.new('L', (big, big), 0)
    ImageDraw.Draw(mask).ellipse([pad, pad, pad + inner, pad + inner], fill=255)
    image.paste(light, (0, 0), Image.composite(light, Image.new('RGBA', (big, big)), mask).getchannel('A'))

    prism = Image.new('RGBA', (big, big), (0, 0, 0, 0))
    draw = ImageDraw.Draw(prism)
    points = [at(x, y) for x, y in PRISM]
    draw.polygon(points, fill=BG[:3] + (140,))
    width = int(EDGE * inner)
    for a, b in zip(points, points[1:] + points[:1]):
        line(draw, a, b, FG, width)
    image = Image.alpha_composite(image, prism)
    return image.resize((size, size), Image.LANCZOS)


def save_png(name, size, margin=0.0):
    master(size, margin).save(os.path.join(ART, name))


def main():
    with open(os.path.join(HERE, 'logo.svg'), 'w', encoding='utf-8') as f:
        f.write(svg())
    with open(os.path.join(ART, 'ayu', 'default', 'app.svg'), 'w',
              encoding='utf-8') as f:
        f.write(svg())

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
    print('PrismGram icons written.')


if __name__ == '__main__':
    main()
