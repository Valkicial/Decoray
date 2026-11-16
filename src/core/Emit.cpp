// Emission: primitives -> GD objects -> level string. Also pixel mode,
// JSON helpers and the text blocks used to prompt Claude.
#include "DecoCore.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <sstream>

namespace deco {

// ------------------------------------------------------------------ objects --

int objectIdFor(Prim const& p) {
    float big = std::max(p.w, p.h);
    switch (p.shape) {
        case Shape::Rect: return 211;
        case Shape::Slope: return 693;
        case Shape::Circle: return big > 45 ? 3637 : 3621;
        case Shape::Ring: return big > 45 ? 3636 : 3622;
        case Shape::Triangle: return 3628;
        case Shape::Star: return 3625;
        case Shape::Diamond: return 3624;
        case Shape::Hexagon: return 3629;
        case Shape::Heart: return 3623;
        case Shape::Glow: return 1888;
        case Shape::EdgeGlow: return 1011;
        case Shape::Raw: return p.rawId > 0 ? p.rawId : 211;
    }
    return 211;
}

// Fallback sizes; in game the real sprite sizes are measured instead.
Vec2 defaultObjectSize(int id) {
    switch (id) {
        case 694: return { 60, 30 };
        case 3636: case 3637: case 3638: case 3639: return { 60, 60 };
        case 1888: return { 60, 60 };
        case 1886: return { 40, 40 };
        case 1887: return { 20, 20 };
        case 917: return { 7.5f, 7.5f };
        case 916: return { 15, 15 };
        default: return { 30, 30 };
    }
}

namespace {

int zLayerFor(Layer l) {
    switch (l) {
        case Layer::BG2: return -1;  // B3
        case Layer::BG: return 1;    // B2
        case Layer::BACK: return 3;  // B1
        case Layer::MAIN: return 5;  // T1
        case Layer::FRONT: return 7; // T2
        case Layer::FX: return 9;    // T3
    }
    return 5;
}

bool isGlow(Shape s) { return s == Shape::Glow || s == Shape::EdgeGlow; }

GDObject toObject(Prim const& p, SizeFn const& sizeOf) {
    GDObject o;
    o.id = objectIdFor(p);
    Vec2 sz = sizeOf(o.id);
    if (sz.x <= 0.01f) sz.x = 30;
    if (sz.y <= 0.01f) sz.y = 30;
    o.x = p.x;
    o.y = p.y;
    if (p.shape == Shape::Raw && (p.w <= 0 || p.h <= 0)) { o.scaleX = o.scaleY = 1; }
    else {
        o.scaleX = std::clamp(p.w / sz.x, 0.01f, 200.f);
        o.scaleY = std::clamp(p.h / sz.y, 0.01f, 200.f);
    }
    o.rot = p.rot;
    o.flipX = p.flipX;
    o.flipY = p.flipY;
    o.slot = std::clamp(p.slot, 0, SLOT_COUNT - 1);
    o.hsvH = std::clamp(p.hue, -180.f, 180.f);
    o.hsvS = std::clamp(p.sat, 0.f, 2.f);
    o.hsvV = std::clamp(p.shade, 0.f, 2.f);
    o.hsv = std::fabs(o.hsvH) > 0.5f || std::fabs(o.hsvS - 1) > 0.01f || std::fabs(o.hsvV - 1) > 0.01f;
    o.zLayer = zLayerFor(p.layer);
    return o;
}

void assignZOrder(std::vector<GDObject>& objs, std::vector<std::pair<int, int>> const& keys) {
    // keys[i] = (layer, order); spread ranks of each layer over [-99, 99]
    std::map<int, std::vector<size_t>> byLayer;
    for (size_t i = 0; i < objs.size(); ++i) byLayer[objs[i].zLayer].push_back(i);
    for (auto& [layer, idx] : byLayer) {
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return keys[a].second < keys[b].second; });
        size_t n = idx.size();
        for (size_t r = 0; r < n; ++r) {
            int z = n <= 199 ? int(r) - int(std::min<size_t>(n, 199) / 2) : -99 + int(r * 198 / (n - 1));
            objs[idx[r]].zOrder = std::clamp(z, -99, 99);
        }
    }
}

// ---- pixel mode ------------------------------------------------------------

bool pointInPoly(float x, float y, std::vector<Vec2> const& poly) {
    bool in = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].y > y) != (poly[j].y > y)) &&
            (x < (poly[j].x - poly[i].x) * (y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
            in = !in;
    }
    return in;
}

bool contains(Prim const& p, float x, float y) {
    float dx = x - p.x, dy = y - p.y;
    float a = p.rot * PI / 180.f;
    float lx = dx * std::cos(a) - dy * std::sin(a);
    float ly = dx * std::sin(a) + dy * std::cos(a);
    if (p.flipX) lx = -lx;
    if (p.flipY) ly = -ly;
    if (p.w <= 0 || p.h <= 0) return false;
    float u = lx / p.w, v = ly / p.h;
    if (std::fabs(u) > 0.5f || std::fabs(v) > 0.5f) return false;
    switch (p.shape) {
        case Shape::Rect: case Shape::Raw: return true;
        case Shape::Circle: return u * u + v * v <= 0.25f;
        case Shape::Ring: { float r = u * u + v * v; return r <= 0.25f && r >= 0.25f * 0.8f * 0.8f; }
        case Shape::Slope: return v <= u;
        case Shape::Triangle: return std::fabs(u) <= (0.5f - v) / 2.f;
        case Shape::Diamond: return std::fabs(u) + std::fabs(v) <= 0.5f;
        case Shape::Hexagon: return std::fabs(v) <= 0.5f && std::fabs(u) <= 0.5f - std::fabs(v) * 0.5f;
        case Shape::Star: {
            static std::vector<Vec2> star;
            if (star.empty()) {
                for (int i = 0; i < 10; ++i) {
                    float ang = PI / 2 + i * PI / 5;
                    float r = i % 2 ? 0.2f : 0.5f;
                    star.push_back({ std::cos(ang) * r, std::sin(ang) * r });
                }
            }
            return pointInPoly(u, v, star);
        }
        case Shape::Heart: {
            float X = u * 2.3f, Y = v * 2.3f + 0.25f;
            float q = X * X + Y * Y - 1;
            return q * q * q - X * X * Y * Y * Y <= 0;
        }
        default: return false;
    }
}

std::vector<GDObject> emitPixel(std::vector<Prim> prims, SizeFn const& sizeOf, int limit) {
    std::vector<Prim> solids, glows;
    for (auto const& p : prims) (isGlow(p.shape) ? glows : solids).push_back(p);
    int glowBudget = std::min<int>(int(glows.size()), std::max(0, limit / 8));
    enforceLimit(glows, glowBudget);
    int budget = std::max(1, limit - int(glows.size()));

    // topmost first
    std::stable_sort(solids.begin(), solids.end(), [](Prim const& a, Prim const& b) {
        if (a.layer != b.layer) return a.layer > b.layer;
        return a.order > b.order;
    });
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (auto const& p : solids) {
        float r = std::sqrt(p.w * p.w + p.h * p.h) / 2;
        minX = std::min(minX, p.x - r); maxX = std::max(maxX, p.x + r);
        minY = std::min(minY, p.y - r); maxY = std::max(maxY, p.y + r);
    }
    std::vector<GDObject> out;
    std::vector<std::pair<int, int>> keys;
    if (!solids.empty()) {
        float area = (maxX - minX) * (maxY - minY);
        float g = std::max(3.75f, std::sqrt(area / (budget * 6.f)));
        g = std::round(g / 3.75f) * 3.75f;
        if (g < 3.75f) g = 3.75f;
        struct R { int x0, x1, y0, y1, key; };
        std::vector<R> rects;
        std::vector<Prim const*> keyPrim;
        for (int attempt = 0; attempt < 12; ++attempt) {
            int cols = int(std::ceil((maxX - minX) / g)), rows = int(std::ceil((maxY - minY) / g));
            std::vector<int> grid(size_t(cols) * rows, -1);
            std::map<std::tuple<int, int, int, int, int>, int> keyIds;
            keyPrim.clear();
            for (int ry = 0; ry < rows; ++ry) {
                for (int cx = 0; cx < cols; ++cx) {
                    float x = minX + (cx + 0.5f) * g, y = minY + (ry + 0.5f) * g;
                    for (auto const& p : solids) {
                        if (!contains(p, x, y)) continue;
                        auto k = std::make_tuple(p.slot, int(std::round(p.shade * 20)), int(std::round(p.sat * 20)), int(std::round(p.hue)), int(p.layer));
                        auto it = keyIds.find(k);
                        int id;
                        if (it == keyIds.end()) { id = int(keyPrim.size()); keyIds[k] = id; keyPrim.push_back(&p); }
                        else id = it->second;
                        grid[size_t(ry) * cols + cx] = id;
                        break;
                    }
                }
            }
            rects.clear();
            std::vector<R> open;
            for (int ry = 0; ry < rows; ++ry) {
                std::vector<R> row, next;
                for (int cx = 0; cx < cols; ++cx) {
                    int k = grid[size_t(ry) * cols + cx];
                    if (k < 0) continue;
                    int s = cx;
                    while (cx + 1 < cols && grid[size_t(ry) * cols + cx + 1] == k) ++cx;
                    row.push_back({ s, cx, ry, ry, k });
                }
                for (auto& r : row) {
                    auto it = std::find_if(open.begin(), open.end(), [&](R const& o) { return o.x0 == r.x0 && o.x1 == r.x1 && o.key == r.key && o.y1 == ry - 1; });
                    if (it != open.end()) { R m = *it; m.y1 = ry; next.push_back(m); open.erase(it); }
                    else next.push_back(r);
                }
                for (auto& o : open) rects.push_back(o);
                open = next;
            }
            for (auto& o : open) rects.push_back(o);
            if (int(rects.size()) <= budget || g >= 30.f) break;
            g = std::min(30.f, g * 1.25f);
        }
        for (auto const& r : rects) {
            Prim q = *keyPrim[size_t(r.key)];
            q.shape = Shape::Rect;
            q.rot = 0; q.flipX = q.flipY = false;
            q.w = (r.x1 - r.x0 + 1) * g;
            q.h = (r.y1 - r.y0 + 1) * g;
            q.x = minX + r.x0 * g + q.w / 2;
            q.y = minY + r.y0 * g + q.h / 2;
            out.push_back(toObject(q, sizeOf));
            keys.push_back({ int(q.layer), q.order });
        }
        if (int(out.size()) > budget) out.resize(size_t(budget));
        keys.resize(out.size());
    }
    for (auto const& p : glows) {
        out.push_back(toObject(p, sizeOf));
        keys.push_back({ int(p.layer), p.order });
    }
    assignZOrder(out, keys);
    return out;
}

std::string num(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", v);
    std::string s = buf;
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    if (s == "-0") s = "0";
    return s;
}

} // namespace

std::vector<GDObject> emit(Design const& design, SizeFn const& sizeOf, int limit, bool pixel) {
    std::vector<Prim> prims = design.prims;
    enforceLimit(prims, limit);
    if (pixel) {
        auto px = emitPixel(prims, sizeOf, limit);
        if (!px.empty()) return px;
        // shapes too small for the pixel grid: fall back to normal objects
    }
    std::vector<GDObject> out;
    std::vector<std::pair<int, int>> keys;
    out.reserve(prims.size());
    for (auto const& p : prims) {
        out.push_back(toObject(p, sizeOf));
        keys.push_back({ int(p.layer), p.order });
    }
    assignZOrder(out, keys);
    return out;
}

std::string toObjectString(
    std::vector<GDObject> const& objs, int const channels[SLOT_COUNT],
    float yOffset, int editorLayer, bool noTouch, bool dontFade, bool dontEnter
) {
    std::string s;
    s.reserve(objs.size() * 96);
    for (auto const& o : objs) {
        int ch = channels[std::clamp(o.slot, 0, SLOT_COUNT - 1)];
        s += "1," + std::to_string(o.id);
        s += ",2," + num(o.x);
        s += ",3," + num(o.y - yOffset);
        if (std::fabs(o.rot) > 0.01f) s += ",6," + num(o.rot);
        if (o.flipX) s += ",4,1";
        if (o.flipY) s += ",5,1";
        if (std::fabs(o.scaleX - 1) > 0.0005f) s += ",128," + num(o.scaleX);
        if (std::fabs(o.scaleY - 1) > 0.0005f) s += ",129," + num(o.scaleY);
        s += ",21," + std::to_string(ch) + ",22," + std::to_string(ch);
        s += ",24," + std::to_string(o.zLayer) + ",25," + std::to_string(o.zOrder);
        if (o.hsv) {
            std::string hsv = num(o.hsvH) + "a" + num(o.hsvS) + "a" + num(o.hsvV) + "a0a0";
            s += ",41,1,43," + hsv + ",42,1,44," + hsv;
        }
        if (editorLayer >= 0) s += ",20," + std::to_string(editorLayer);
        if (noTouch) s += ",121,1";
        if (dontFade) s += ",64,1";
        if (dontEnter) s += ",67,1";
        s += ";";
    }
    return s;
}

// ---------------------------------------------------------------------- json --

std::string hex(RGB c) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

RGB parseHex(std::string const& in, RGB fallback) {
    std::string s = in;
    if (!s.empty() && s[0] == '#') s = s.substr(1);
    if (s.size() != 6) return fallback;
    unsigned v = 0;
    for (char ch : s) {
        v <<= 4;
        if (ch >= '0' && ch <= '9') v |= unsigned(ch - '0');
        else if (ch >= 'a' && ch <= 'f') v |= unsigned(ch - 'a' + 10);
        else if (ch >= 'A' && ch <= 'F') v |= unsigned(ch - 'A' + 10);
        else return fallback;
    }
    return RGB{ uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v) };
}

std::string shapeName(Shape s) {
    switch (s) {
        case Shape::Rect: return "rect"; case Shape::Slope: return "slope"; case Shape::Circle: return "circle";
        case Shape::Ring: return "ring"; case Shape::Triangle: return "triangle"; case Shape::Star: return "star";
        case Shape::Diamond: return "diamond"; case Shape::Hexagon: return "hexagon"; case Shape::Heart: return "heart";
        case Shape::Glow: return "glow"; case Shape::EdgeGlow: return "edgeglow"; case Shape::Raw: return "obj";
    }
    return "rect";
}

bool shapeFromName(std::string const& n, Shape& out) {
    static std::map<std::string, Shape> m = {
        { "rect", Shape::Rect }, { "square", Shape::Rect }, { "box", Shape::Rect }, { "slope", Shape::Slope },
        { "circle", Shape::Circle }, { "ellipse", Shape::Circle }, { "ring", Shape::Ring }, { "triangle", Shape::Triangle },
        { "tri", Shape::Triangle }, { "star", Shape::Star }, { "diamond", Shape::Diamond }, { "hexagon", Shape::Hexagon },
        { "hex", Shape::Hexagon }, { "heart", Shape::Heart }, { "glow", Shape::Glow }, { "edgeglow", Shape::EdgeGlow },
        { "obj", Shape::Raw }, { "raw", Shape::Raw },
    };
    auto it = m.find(n);
    if (it == m.end()) return false;
    out = it->second;
    return true;
}

std::string primsToJson(std::vector<Prim> const& prims, Vec2 origin) {
    std::ostringstream o;
    o << "[";
    bool first = true;
    for (auto const& p : prims) {
        if (!first) o << ",";
        first = false;
        o << "{\"s\":\"" << shapeName(p.shape) << "\"";
        if (p.shape == Shape::Raw) o << ",\"id\":" << p.rawId;
        o << ",\"x\":" << num(std::round((p.x - origin.x) * 2) / 2) << ",\"y\":" << num(std::round((p.y - origin.y) * 2) / 2)
          << ",\"w\":" << num(std::round(p.w * 2) / 2) << ",\"h\":" << num(std::round(p.h * 2) / 2);
        if (std::fabs(p.rot) > 0.05f) o << ",\"r\":" << num(std::round(p.rot));
        if (p.flipX) o << ",\"fx\":1";
        if (p.flipY) o << ",\"fy\":1";
        o << ",\"c\":" << p.slot;
        if (std::fabs(p.shade - 1) > 0.01f) o << ",\"v\":" << num(std::round(p.shade * 100) / 100);
        if (std::fabs(p.sat - 1) > 0.01f) o << ",\"sat\":" << num(std::round(p.sat * 100) / 100);
        if (std::fabs(p.hue) > 0.5f) o << ",\"hue\":" << num(std::round(p.hue));
        o << ",\"l\":" << int(p.layer) << "}";
    }
    o << "]";
    return o.str();
}

std::string designRulesText() {
    return
R"(You are an expert Geometry Dash decorator. You design decoration as a list of simple primitives
that a mod converts into GD objects. Coordinates are GD units: 30 units = 1 grid block, x to the right,
y UP. Rotation "r" is in degrees CLOCKWISE (GD convention). Every primitive costs exactly one object.

PRIMITIVES (field "s"):
- rect: solid colored square scaled to w x h. The workhorse for bodies, trims, lines (h=1.5..3), panels.
- slope: right triangle filling w x h with the right angle at the BOTTOM-RIGHT (a ramp rising to the right).
  "fx":1 mirrors it (right angle bottom-left), "fy":1 flips it vertically. Halves of triangles = slopes.
- circle: filled ellipse w x h. Great for foliage blobs, clouds, bubbles, round windows, snow lumps.
- ring: hollow ellipse outline (thin). Halos, bubbles, chain links, planetary rings.
- triangle: isosceles triangle pointing UP inside w x h (r:180 points down: icicles, drips, roofs upside down).
- star, diamond, hexagon, heart: symmetric decorative shapes (diamond stretched tall = crystal shard).
- glow: soft additive radial glow, w x h. Use behind light sources and around emissive things.
- edgeglow: soft additive strip, bright at its BOTTOM edge fading upward. Put it on top of a surface
  (r:180 below a surface, r:90 for a right edge, r:-90 for a left edge).
- obj: any raw GD object id ("id"), sized by w/h (0 = default size). Only use ids from the catalog.

COLORS: "c" is a palette slot: 0 BASE (darkest, shadows/silhouettes), 1 MID (main body), 2 LIGHT
(highlights, rims, snow), 3 ACCENT (second hue: wood, flowers, windows, gems), 4 GLOW (emissive, additive
blending). Shade with "v" (HSV brightness multiplier 0..2), "sat" (saturation multiplier 0..2) and
"hue" (hue shift in degrees). Prefer shading over new colors.

LAYERS: "l" 0=far background, 1=background, 2=behind main (glows), 3=main, 4=front details, 5=front fx.
Inside a layer later primitives draw on top of earlier ones.

WHAT MAKES GD DECO LOOK GOOD (follow these):
1. Silhouette first: one clear readable shape. Then shading, then highlights, then sparkle.
2. Three values: dark base, mid body, light highlight. Light comes from the TOP-LEFT: highlights on the
   upper-left, shadow strips on the right/bottom (same slot with v 0.6-0.8).
3. Layer for depth: back copies offset down-right in BASE, body in MID, small highlights in LIGHT.
4. Contrast and restraint: one focal point, at most one accent hue, glow only on things that emit light.
   Glows sit behind (layer 2) with v 0.3-0.9; never cover a whole design in glow.
5. Consistent detail density; clean straight edges; symmetric structures stay symmetric.
6. Odd counts (3, 5, 7) of repeated elements, varied sizes, slight random rotation for organic things.
7. Thin trims (2-3 units) along edges make blocks look finished. Rounded lumps (circles) soften tops.
8. Never leave gaps inside a solid body; overlap pieces slightly.
9. Respect the object budget: spend objects on silhouette and big shading before tiny scatter.)";
}

std::string objectCatalogText() {
    return
R"(Useful raw object ids for "obj" (optional, the primitives above already cover most needs):
211 colored square | 917 1/16 square | 916 quarter square | 693 slope | 694 wide slope (2x1)
1906 gradient slope | 1907 gradient wide slope | 1903 gradient slab | 1904 gradient slab middle
1753 thin line | 1757 thin half line | 1764 tiny circle | 1765 tiny cross | 1766 tiny triangle
1767 tiny rectangle | 1768 tiny arrow | 1886 medium glow orb | 1887 small glow orb | 1888 large glow orb
503 straight medium glow | 504 outer corner medium glow | 505 inner corner medium glow
1011 straight large glow | 1012 large glow outer corner | 1292 straight small glow
3621 circle | 3622 hollow circle | 3623 heart | 3624 diamond | 3625 star | 3626 music notes
3627 square | 3628 triangle | 3629 hexagon | 3630 large arrow | 3633 exclamation | 3634 question mark
3635 large cross | 3636 large hollow circle | 3637 large circle | 3638 large square | 3639 large hollow square
1191-1201 neon outline set | 467-475 outline set | 1260-1265 outer outlines
186 large outline blade | 187 medium outline blade | 188 small outline blade
50 pulsing filled circle | 51 pulsing circle | 148 pulsing square | 149 pulsing triangle)";
}

} // namespace deco
