#!/usr/bin/env python3
# python3 tools/crop-screenshots.py <shots folder> <output folder>   (needs Pillow)
# Crop each screenshot to its content (the page's background below the card goes), and size it for a README.
import sys, os
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
for name in sorted(os.listdir(src)):
    im = Image.open(os.path.join(src, name)).convert("RGB")
    w, h = im.size
    bg = im.getpixel((10, h - 10))
    px = im.load()
    bottom = h - 1
    def blank(y):
        return all(sum(abs(a - b) for a, b in zip(px[x, y], bg)) < 12 for x in range(0, w - int(40 * w / 1040), 7))
    while bottom > 0 and blank(bottom):
        bottom -= 1
    im = im.crop((0, 0, w, min(h, bottom + int(48 * w / 1040))))
    im = im.resize((1200, int(im.size[1] * 1200 / w)), Image.LANCZOS)
    im.save(os.path.join(dst, name), optimize=True)
    print(name, im.size, os.path.getsize(os.path.join(dst, name)) // 1024, "KB")
