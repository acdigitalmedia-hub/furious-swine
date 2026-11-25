#!/usr/bin/env python3
"""Bakes the game's UI font into assets/fonts/ui.font.

Developer tool (needs Pillow); the game itself only reads the baked file.
Each face is rasterised at several pixel sizes; every glyph is stored twice:
its fill and an outline mask (the glyph grown by a stroke) so text can be
drawn with a cartoon outline in two textured passes.

    python3 tools/make_font_atlas.py path/to/Fredoka.ttf

File layout (little-endian):
    magic "SSFONT2\\0"
    u16 atlasW, u16 atlasH, u32 rleBytes, rle (u8 alpha; 0x00 <run u16> = run of zeros)
    u16 faceCount, then per face:
        u8 faceId, f32 pixelSize, f32 ascent, f32 lineHeight, f32 strokePx, u16 glyphCount
        per glyph: u32 codepoint, f32 advance,
                   fill   i16 x y w h ox oy
                   stroke i16 x y w h ox oy
"""
import os
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

# Face ids must match enum class FontFace in src/text.h.
FACES = [  # (id, weight)
    (0, 500),  # BODY
    (1, 600),  # BOLD
    (2, 700),  # DISPLAY
]
SIZES = [18, 32, 60]
CHARS = [chr(c) for c in range(32, 127)] + ['·', '•', '×', '…']
ATLAS_W = 2048
PAD = 2


def stroke_for(size):
    return max(2, round(size * 0.12))


def bake_face(path, weight, size):
    font = ImageFont.truetype(path, size)
    font.set_variation_by_axes([weight, 100])
    ascent, descent = font.getmetrics()
    sw = stroke_for(size)
    glyphs = []
    for ch in CHARS:
        adv = font.getlength(ch)
        imgs = []
        for stroke in (0, sw):
            bbox = font.getbbox(ch, stroke_width=stroke)
            w, h = max(1, bbox[2] - bbox[0]), max(1, bbox[3] - bbox[1])
            im = Image.new('L', (w, h), 0)
            ImageDraw.Draw(im).text((-bbox[0], -bbox[1]), ch, font=font, fill=255,
                                   stroke_width=stroke, stroke_fill=255)
            if ch == ' ':
                im = Image.new('L', (1, 1), 0)
            imgs.append((im, bbox[0], bbox[1]))
        glyphs.append((ord(ch), adv, imgs))
    return dict(size=size, ascent=ascent, line=ascent + descent, stroke=sw, glyphs=glyphs)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    ttf = sys.argv[1]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(root, 'assets', 'fonts', 'ui.font')

    faces = []
    for fid, weight in FACES:
        for size in SIZES:
            faces.append((fid, bake_face(ttf, weight, size)))

    # Shelf-pack every bitmap into one atlas.
    items = []
    for fi, (_, f) in enumerate(faces):
        for gi, (_, _, imgs) in enumerate(f['glyphs']):
            for k, (im, _, _) in enumerate(imgs):
                items.append((im.size[1], fi, gi, k, im))
    items.sort(key=lambda t: -t[0])
    x = y = shelf = 0
    place = {}
    for h, fi, gi, k, im in items:
        w = im.size[0]
        if x + w + PAD > ATLAS_W:
            x, y, shelf = 0, y + shelf + PAD, 0
        place[(fi, gi, k)] = (x, y)
        x += w + PAD
        shelf = max(shelf, h)
    atlas_h = y + shelf + PAD
    atlas_h = 1 << (atlas_h - 1).bit_length()
    atlas = Image.new('L', (ATLAS_W, atlas_h), 0)
    for (fi, gi, k), (px, py) in place.items():
        atlas.paste(faces[fi][1]['glyphs'][gi][2][k][0], (px, py))

    raw = atlas.tobytes()
    rle = bytearray()
    i = 0
    while i < len(raw):
        if raw[i] == 0:
            j = i
            while j < len(raw) and raw[j] == 0 and j - i < 65535:
                j += 1
            rle += b'\x00' + struct.pack('<H', j - i)
            i = j
        else:
            rle.append(raw[i])
            i += 1

    blob = bytearray(b'SSFONT2\x00')
    blob += struct.pack('<HHI', ATLAS_W, atlas_h, len(rle)) + rle
    blob += struct.pack('<H', len(faces))
    for fi, (fid, f) in enumerate(faces):
        blob += struct.pack('<BffffH', fid, f['size'], f['ascent'], f['line'], f['stroke'], len(f['glyphs']))
        for gi, (cp, adv, imgs) in enumerate(f['glyphs']):
            blob += struct.pack('<If', cp, adv)
            for k, (im, ox, oy) in enumerate(imgs):
                px, py = place[(fi, gi, k)]
                blob += struct.pack('<hhhhhh', px, py, im.size[0], im.size[1], ox, oy)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, 'wb') as fh:
        fh.write(blob)
    print(f'wrote {out}: atlas {ATLAS_W}x{atlas_h}, {len(faces)} faces, {len(blob)} bytes')


if __name__ == '__main__':
    main()
