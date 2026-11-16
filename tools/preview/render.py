#!/usr/bin/env python3
"""Approximate renderer for AI Deco output (preview only, not pixel exact).

usage: render.py in.json out.png [--title text]
Shapes follow the same assumptions as the mod (see Emit.cpp).
"""
import json
import math
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

SIZES = {694: (60, 30), 3636: (60, 60), 3637: (60, 60), 1888: (60, 60)}
SCALE = 3.0  # pixels per GD unit


def hsv_adjust(rgb, h, s, v):
    import colorsys
    r, g, b = [c / 255 for c in rgb]
    hh, ss, vv = colorsys.rgb_to_hsv(r, g, b)
    hh = (hh + h / 360.0) % 1.0
    ss = min(1, max(0, ss * s))
    vv = min(1, max(0, vv * v))
    return np.array(colorsys.hsv_to_rgb(hh, ss, vv))


def shape_mask(oid, u, v):
    """u, v in [-0.5, 0.5] local normalized coords; returns coverage 0..1"""
    inside = (np.abs(u) <= 0.5) & (np.abs(v) <= 0.5)
    if oid in (211, 3627, 3638):
        m = inside
    elif oid in (693, 694):
        m = inside & (v <= u)
    elif oid in (3621, 3637):
        m = (u * u + v * v) <= 0.25
    elif oid in (3622, 3636):
        r = u * u + v * v
        m = (r <= 0.25) & (r >= 0.25 * 0.8 * 0.8)
    elif oid == 3628:
        m = inside & (np.abs(u) <= (0.5 - v) / 2)
    elif oid == 3624:
        m = (np.abs(u) + np.abs(v)) <= 0.5
    elif oid == 3629:
        m = inside & (np.abs(u) <= 0.5 - np.abs(v) * 0.5)
    elif oid == 3625:
        ang = np.arctan2(v, u) - math.pi / 2
        k = np.mod(ang, 2 * math.pi / 5) / (2 * math.pi / 5)
        tri = np.abs(k - 0.5) * 2  # 1 at points, 0 between
        rad = 0.2 + 0.3 * tri
        m = np.sqrt(u * u + v * v) <= rad
    elif oid == 3623:
        X = u * 2.3
        Y = v * 2.3 + 0.25
        q = X * X + Y * Y - 1
        m = (q * q * q - X * X * Y * Y * Y) <= 0
    elif oid == 1888:
        r = np.sqrt(u * u + v * v) * 2
        return np.clip(1 - r, 0, 1) ** 1.6
    elif oid == 1011:
        # bright at the bottom (v=-0.5), fading upward
        t = np.clip(0.5 - v, 0, 1)
        f = np.clip(1 - (v + 0.5), 0, 1) ** 1.8
        return np.where(inside, f, 0) * (t > 0)
    else:
        m = inside
    return m.astype(np.float32)


def render(data, out, title=None):
    objs = data["objects"]
    pal = [np.array([int(c[i:i + 2], 16) for i in (1, 3, 5)]) for c in data["palette"]]
    if not objs:
        print("no objects")
        return
    xs, ys = [], []
    for o in objs:
        w, h = SIZES.get(o["id"], (30, 30))
        r = math.hypot(w * o["sx"], h * o["sy"]) / 2
        xs += [o["x"] - r, o["x"] + r]
        ys += [o["y"] - r, o["y"] + r]
    for b in data.get("blocks", []):
        xs += [b[0], b[0] + b[2]]
        ys += [b[1], b[1] + b[3]]
    pad = 20
    minx, maxx, miny, maxy = min(xs) - pad, max(xs) + pad, min(ys) - pad, max(ys) + pad
    W = int((maxx - minx) * SCALE)
    H = int((maxy - miny) * SCALE)
    scale = SCALE
    if max(W, H) > 1100:
        scale = SCALE * 1100 / max(W, H)
        W = int((maxx - minx) * scale)
        H = int((maxy - miny) * scale)
    img = np.zeros((H, W, 3), np.float32)
    img[:] = np.array([0x22, 0x2a, 0x48]) / 255.0  # GD-ish dark blue background
    gy, gx = np.mgrid[0:H, 0:W]
    wx = minx + (gx + 0.5) / scale
    wy = maxy - (gy + 0.5) / scale

    # ground blocks (the user's structure) drawn as default GD blocks underneath
    for b in data.get("blocks", []):
        m = (wx >= b[0]) & (wx <= b[0] + b[2]) & (wy >= b[1]) & (wy <= b[1] + b[3])
        img[m] = [0.1, 0.1, 0.1]

    order = sorted(range(len(objs)), key=lambda i: (objs[i]["zl"], objs[i]["zo"], i))
    for i in order:
        o = objs[i]
        w, h = SIZES.get(o["id"], (30, 30))
        w *= o["sx"]
        h *= o["sy"]
        r = math.hypot(w, h) / 2
        x0 = int(max(0, (o["x"] - r - minx) * scale))
        x1 = int(min(W, (o["x"] + r - minx) * scale + 1))
        y0 = int(max(0, (maxy - (o["y"] + r)) * scale))
        y1 = int(min(H, (maxy - (o["y"] - r)) * scale + 1))
        if x1 <= x0 or y1 <= y0:
            continue
        dx = wx[y0:y1, x0:x1] - o["x"]
        dy = wy[y0:y1, x0:x1] - o["y"]
        a = math.radians(o["r"])
        lx = dx * math.cos(a) - dy * math.sin(a)
        ly = dx * math.sin(a) + dy * math.cos(a)
        if o["fx"]:
            lx = -lx
        if o["fy"]:
            ly = -ly
        u = lx / max(w, 1e-3)
        v = ly / max(h, 1e-3)
        cov = shape_mask(o["id"], u, v)
        col = hsv_adjust(pal[o["slot"]], o["h"], o["s"], o["v"])
        region = img[y0:y1, x0:x1]
        if o["slot"] == 4:  # GLOW channel uses additive blending
            region += cov[..., None] * col[None, None, :]
        else:
            region[:] = region * (1 - cov[..., None]) + cov[..., None] * col[None, None, :]
    img = np.clip(img, 0, 1)
    im = Image.fromarray((img * 255).astype(np.uint8))
    if title:
        canvas = Image.new("RGB", (max(W, 360), H + 34), (18, 18, 28))
        canvas.paste(im, (0, 34))
        d = ImageDraw.Draw(canvas)
        d.text((8, 4), title, fill=(230, 230, 240))
        d.text((8, 18), f"{data['summary']}  |  {data['count']} objects", fill=(150, 160, 190))
        im = canvas
    im.save(out)


if __name__ == "__main__":
    data = json.load(open(sys.argv[1]))
    title = None
    if "--title" in sys.argv:
        title = sys.argv[sys.argv.index("--title") + 1]
    render(data, sys.argv[2], title)
