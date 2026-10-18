"""Draws the Nexo Desk banner (GitHub social preview, README header).

Usage (from Telegram/Resources/art/nexo):  python make_banner.py
Writes banner.png (1280x640). Uses Windows fonts (Segoe UI, Consolas).
"""
import math
import os
import random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 1280, 640
FONTS = r'C:\Windows\Fonts'
random.seed(7)


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)


def background():
    # Near-black with a soft light spot on the right, like a window at night.
    img = Image.new('L', (W, H), 8)
    px = img.load()
    cx, cy = W * 0.74, H * 0.42
    for y in range(H):
        for x in range(W):
            d = math.hypot((x - cx) / W, (y - cy) / H)
            px[x, y] = int(8 + 38 * max(0.0, 1.0 - d * 2.1) ** 2)
    return img


def rain(img):
    layer = Image.new('L', (W, H), 0)
    draw = ImageDraw.Draw(layer)
    for _ in range(420):
        x = random.uniform(-200, W)
        y = random.uniform(-60, H)
        length = random.uniform(18, 70)
        shade = random.randint(18, 70)
        draw.line([(x, y), (x + length * 0.28, y + length)], fill=shade, width=1)
    layer = layer.filter(ImageFilter.GaussianBlur(0.6))
    return Image.composite(Image.new('L', (W, H), 255), img, layer.point(lambda v: v // 3))


def grain(img, amount=14):
    noise = Image.effect_noise((W, H), amount).point(lambda v: v - 128)
    return Image.eval(Image.merge('L', [img]), lambda v: v).point(lambda v: v) if False else \
        Image.blend(img, Image.effect_noise((W, H), 64), 0.06)


def vignette(img):
    mask = Image.new('L', (W, H), 0)
    ImageDraw.Draw(mask).ellipse([-W * 0.25, -H * 0.4, W * 1.25, H * 1.4], fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(140))
    return Image.composite(img, Image.new('L', (W, H), 0), mask)


def logo(size):
    """Monochrome version of the Nexo logo: ring, N and the orbit dot."""
    s = size * 4
    img = Image.new('LA', (s, s), (0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse([0, 0, s - 1, s - 1], fill=(235, 255))
    w = int(s * 0.105)
    pts = [(0.33, 0.70), (0.33, 0.30), (0.67, 0.70), (0.67, 0.30)]
    pts = [(x * s, y * s) for x, y in pts]
    for a, b in zip(pts, pts[1:]):
        d.line([a, b], fill=(12, 255), width=w)
    for x, y in pts:
        r = (w - 1) / 2
        d.ellipse([x - r, y - r, x + r, y + r], fill=(12, 255))
    cx, cy, r = 0.755 * s, 0.245 * s, 0.045 * s
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(12, 255))
    return img.resize((size, size), Image.LANCZOS)


def chat_mock(img):
    """A few dark message bubbles on the right, the client's own look."""
    layer = Image.new('LA', (W, H), (0, 0))
    d = ImageDraw.Draw(layer)
    f = font('segoeui.ttf', 22)
    small = font('segoeui.ttf', 15)
    bubbles = [
        (False, 'so, what do you feel?', '01:12'),
        (True, 'nothing. and everything.', '01:13'),
        (False, 'same.', '01:13'),
    ]
    y = 190
    for out, text, time in bubbles:
        tw = d.textlength(text, font=f)
        bw, bh = tw + 92, 50
        x = (W - 90 - bw) if out else 740
        d.rounded_rectangle([x, y, x + bw, y + bh], radius=18,
                            fill=(38, 230) if out else (22, 230),
                            outline=(70, 255), width=1)
        d.text((x + 18, y + 11), text, font=f, fill=(225, 255))
        d.text((x + bw - 52, y + 24), time, font=small, fill=(120, 255))
        y += bh + 18
    img = img.convert('LA')
    img.alpha_composite(layer)
    return img.convert('L')


def main():
    img = background()
    img = rain(img)
    img = chat_mock(img)
    img = grain(img)
    img = vignette(img)
    img = img.convert('RGBA')

    lg = logo(132)
    img.alpha_composite(lg.convert('RGBA'), (96, 150))
    d = ImageDraw.Draw(img)
    d.text((92, 312), 'Nexo Desk', font=font('segoeuib.ttf', 88), fill=(240, 240, 240, 255))
    d.text((96, 420), 'Telegram client based on AyuGram', font=font('segoeuil.ttf', 32),
           fill=(170, 170, 170, 255))
    d.text((96, 470), 'plugins  ·  dark themes  ·  proxy  ·  signed updates',
           font=font('consola.ttf', 22), fill=(120, 120, 120, 255))
    d.text((96, 560), 'github.com/Svat1r/NexoDesk', font=font('consola.ttf', 20),
           fill=(95, 95, 95, 255))
    img.convert('RGB').save(os.path.join(HERE, 'banner.png'), optimize=True)
    print('banner.png written')


if __name__ == '__main__':
    main()
