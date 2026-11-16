// Structure decoration: turns a group of plain blocks into a styled surface
// (fills, patterns, edge trims, top dressing, hanging details, glow).
#include "Recipes.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace deco {

StructureInfo analyzeStructure(std::vector<Rect> const& blocks) {
    StructureInfo info;
    std::set<std::pair<int, int>> cells;
    for (auto const& b : blocks) {
        int x0 = int(std::floor(b.x / BLOCK + 0.01f));
        int x1 = int(std::ceil(b.right() / BLOCK - 0.01f)) - 1;
        int y0 = int(std::floor(b.y / BLOCK + 0.01f));
        int y1 = int(std::ceil(b.top() / BLOCK - 0.01f)) - 1;
        bool any = false;
        for (int x = x0; x <= x1; ++x) {
            for (int y = y0; y <= y1; ++y) {
                float cx = (x + 0.5f) * BLOCK, cy = (y + 0.5f) * BLOCK;
                if (cx >= b.x - 0.5f && cx <= b.right() + 0.5f && cy >= b.y - 0.5f && cy <= b.top() + 0.5f) {
                    cells.insert({ x, y });
                    any = true;
                }
            }
        }
        if (!any) cells.insert({ int(std::floor(b.cx() / BLOCK)), int(std::floor(b.cy() / BLOCK)) });
        if (cells.size() > 2500) break;
    }
    info.cells = int(cells.size());
    if (cells.empty()) return info;

    int minX = INT32_MAX, maxX = INT32_MIN, minY = INT32_MAX, maxY = INT32_MIN;
    for (auto const& [x, y] : cells) {
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    }
    info.bbox = Rect{ minX * BLOCK, minY * BLOCK, (maxX - minX + 1) * BLOCK, (maxY - minY + 1) * BLOCK };
    auto has = [&](int x, int y) { return cells.count({ x, y }) > 0; };

    // greedy meshing: horizontal runs, then merge identical runs vertically
    struct Run { int x0, x1, y0, y1; };
    std::vector<Run> open, done;
    for (int y = minY; y <= maxY; ++y) {
        std::vector<Run> rowRuns;
        for (int x = minX; x <= maxX; ++x) {
            if (!has(x, y)) continue;
            int s = x;
            while (x + 1 <= maxX && has(x + 1, y)) ++x;
            rowRuns.push_back({ s, x, y, y });
        }
        std::vector<Run> next;
        for (auto& r : rowRuns) {
            auto it = std::find_if(open.begin(), open.end(), [&](Run const& o) { return o.x0 == r.x0 && o.x1 == r.x1 && o.y1 == y - 1; });
            if (it != open.end()) {
                Run m = *it;
                m.y1 = y;
                next.push_back(m);
                open.erase(it);
            }
            else next.push_back(r);
        }
        for (auto& o : open) done.push_back(o);
        open = next;
    }
    for (auto& o : open) done.push_back(o);
    for (auto& r : done)
        info.fills.push_back(Rect{ r.x0 * BLOCK, r.y0 * BLOCK, (r.x1 - r.x0 + 1) * BLOCK, (r.y1 - r.y0 + 1) * BLOCK });

    // exposed edges
    for (int y = minY; y <= maxY; ++y) {
        for (int pass = 0; pass < 2; ++pass) {
            int dy = pass == 0 ? 1 : -1;
            for (int x = minX; x <= maxX; ++x) {
                if (!has(x, y) || has(x, y + dy)) continue;
                int s = x;
                while (x + 1 <= maxX && has(x + 1, y) && !has(x + 1, y + dy)) ++x;
                StructureInfo::Seg seg{ s * BLOCK, (x + 1) * BLOCK, (pass == 0 ? y + 1 : y) * BLOCK };
                (pass == 0 ? info.tops : info.bottoms).push_back(seg);
            }
        }
    }
    for (int x = minX; x <= maxX; ++x) {
        for (int pass = 0; pass < 2; ++pass) {
            int dx = pass == 0 ? -1 : 1;
            for (int y = minY; y <= maxY; ++y) {
                if (!has(x, y) || has(x + dx, y)) continue;
                int s = y;
                while (y + 1 <= maxY && has(x, y + 1) && !has(x + dx, y + 1)) ++y;
                StructureInfo::VSeg seg{ s * BLOCK, (y + 1) * BLOCK, (pass == 0 ? x : x + 1) * BLOCK };
                (pass == 0 ? info.lefts : info.rights).push_back(seg);
            }
        }
    }
    return info;
}

namespace {

enum class Pat { None, Bricks, Stripes, Grid, Dots, Panels, Checker, Cracks, Stars, Tiles, Strata, Planks, Insets, Bubbles };
enum class Top { None, Cap, Grass, Moss, Snow, Crystals, Spikes, Waves, Frosting, Slime, Crust, Clouds, Flowers };
enum class Bot { None, Icicles, Drips, Vines };

struct Style {
    std::string name;
    int fillSlot = MID;
    float fillShade = 1.f;
    Pat pat = Pat::None;
    Top top = Top::Cap;
    Bot bot = Bot::None;
    bool edgeGlow = false;
    bool outline = false;
    int outlineSlot = LIGHT;
    bool innerLines = false;
    bool rivets = false;
    int dripSlot = MID;
};

Style styleFor(Spec const& spec) {
    Style s;
    auto const& t = spec.theme;
    s.name = t;
    if (t == "neon") { s.fillSlot = BASE; s.pat = Pat::Grid; s.top = Top::None; s.edgeGlow = true; s.outline = true; s.innerLines = true; }
    else if (t == "lava") { s.fillSlot = BASE; s.fillShade = 1.5f; s.pat = Pat::Cracks; s.top = Top::Crust; s.bot = Bot::Drips; s.edgeGlow = true; s.dripSlot = GLOW; }
    else if (t == "ice") { s.fillSlot = MID; s.pat = Pat::Insets; s.top = Top::Snow; s.bot = Bot::Icicles; s.edgeGlow = true; s.outline = true; }
    else if (t == "forest") { s.fillSlot = WOOD; s.fillShade = 0.75f; s.pat = Pat::Dots; s.top = Top::Grass; s.bot = spec.detail > 1.2f ? Bot::Vines : Bot::None; }
    else if (t == "garden") { s.fillSlot = WOOD; s.fillShade = 0.75f; s.pat = Pat::Dots; s.top = Top::Flowers; }
    else if (t == "sakura") { s.fillSlot = WOOD; s.fillShade = 0.9f; s.pat = Pat::Strata; s.top = Top::Flowers; }
    else if (t == "ocean") { s.fillSlot = MID; s.fillShade = 0.8f; s.pat = Pat::Bubbles; s.top = Top::Waves; }
    else if (t == "space") { s.fillSlot = BASE; s.pat = Pat::Stars; s.top = Top::None; s.edgeGlow = true; s.outline = true; s.outlineSlot = MID; }
    else if (t == "crystal") { s.fillSlot = BASE; s.fillShade = 1.4f; s.pat = Pat::Stars; s.top = Top::Crystals; s.edgeGlow = true; s.outline = true; s.outlineSlot = MID; }
    else if (t == "desert") { s.fillSlot = MID; s.fillShade = 0.9f; s.pat = Pat::Strata; s.top = Top::Cap; }
    else if (t == "candy") { s.fillSlot = MID; s.pat = Pat::Stripes; s.top = Top::Frosting; s.dripSlot = LIGHT; }
    else if (t == "spooky") { s.fillSlot = BASE; s.pat = Pat::Cracks; s.top = Top::Spikes; s.bot = Bot::Drips; s.edgeGlow = true; s.outline = true; s.outlineSlot = MID; s.dripSlot = MID; }
    else if (t == "castle") { s.fillSlot = MID; s.fillShade = 0.85f; s.pat = Pat::Bricks; s.top = Top::Cap; }
    else if (t == "gold") { s.fillSlot = MID; s.fillShade = 0.8f; s.pat = Pat::Panels; s.top = Top::Cap; s.edgeGlow = true; s.outline = true; }
    else if (t == "toxic") { s.fillSlot = BASE; s.fillShade = 1.3f; s.pat = Pat::Bubbles; s.top = Top::Slime; s.bot = Bot::Drips; s.edgeGlow = true; s.dripSlot = GLOW; }
    else if (t == "night") { s.fillSlot = BASE; s.fillShade = 1.4f; s.pat = Pat::Tiles; s.top = Top::Cap; s.edgeGlow = true; s.outline = true; s.outlineSlot = MID; }
    else if (t == "sky") { s.fillSlot = LIGHT; s.fillShade = 0.92f; s.pat = Pat::None; s.top = Top::Clouds; }
    else if (t == "tech") { s.fillSlot = MID; s.fillShade = 0.7f; s.pat = Pat::Panels; s.top = Top::Cap; s.edgeGlow = true; s.rivets = true; }
    else if (t == "hell") { s.fillSlot = BASE; s.pat = Pat::Cracks; s.top = Top::Spikes; s.bot = Bot::Drips; s.edgeGlow = true; s.dripSlot = LIGHT; }
    else if (t == "sunset") { s.fillSlot = MID; s.fillShade = 0.75f; s.pat = Pat::Strata; s.top = Top::Cap; s.edgeGlow = true; }
    else if (t == "cozy") { s.fillSlot = MID; s.pat = Pat::Planks; s.top = Top::Cap; }
    else { s.fillSlot = MID; s.fillShade = 0.62f; s.pat = Pat::Insets; s.top = Top::Cap; s.edgeGlow = true; }

    // explicit surface words win
    for (auto sf : spec.surfaces) {
        switch (sf) {
            case Surf::Bricks: s.pat = Pat::Bricks; break;
            case Surf::Stripes: s.pat = Pat::Stripes; break;
            case Surf::Grid: s.pat = Pat::Grid; break;
            case Surf::Dots: s.pat = Pat::Dots; break;
            case Surf::Panels: s.pat = Pat::Panels; break;
            case Surf::Checker: s.pat = Pat::Checker; break;
            case Surf::Cracks: s.pat = Pat::Cracks; break;
            case Surf::Stars: s.pat = Pat::Stars; break;
            case Surf::Tiles: s.pat = Pat::Tiles; break;
            case Surf::Grass: s.top = Top::Grass; break;
            case Surf::Moss: s.top = Top::Moss; if (s.bot == Bot::None) s.bot = Bot::Vines; break;
            case Surf::Snow: s.top = Top::Snow; break;
            case Surf::Crystals: s.top = Top::Crystals; break;
            case Surf::Spikes: s.top = Top::Spikes; break;
            case Surf::Waves: s.top = Top::Waves; break;
            case Surf::Flowers: s.top = Top::Flowers; break;
            case Surf::Lava: s.top = Top::Crust; s.edgeGlow = true; break;
            case Surf::Icicles: s.bot = Bot::Icicles; break;
            case Surf::Drips: s.bot = Bot::Drips; break;
            case Surf::Vines: s.bot = Bot::Vines; break;
            case Surf::Outline: s.outline = true; break;
            case Surf::Glow: s.edgeGlow = true; break;
            case Surf::Rivets: s.rivets = true; if (s.pat == Pat::None) s.pat = Pat::Panels; break;
        }
    }
    if (spec.minimal) {
        bool explicitPat = false;
        for (auto sf : spec.surfaces)
            if (sf == Surf::Bricks || sf == Surf::Stripes || sf == Surf::Grid || sf == Surf::Dots || sf == Surf::Panels || sf == Surf::Checker || sf == Surf::Tiles) explicitPat = true;
        if (!explicitPat) s.pat = Pat::None;
    }
    if (spec.glow == 0) s.edgeGlow = false;
    if (spec.glow == 1) s.edgeGlow = true;
    return s;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

} // namespace

void decorateStructure(Canvas& c, std::vector<Rect> const& blocks, std::string& styleName) {
    auto info = analyzeStructure(blocks);
    if (info.fills.empty()) return;
    Style st = styleFor(*c.spec);
    styleName = st.name;
    auto& rng = c.rng;
    float d = c.detail;

    // ---- base fill
    for (auto const& r : info.fills)
        shade(c.box(r.x, r.y, r.w, r.h, st.fillSlot, Layer::MAIN, 0), st.fillShade);

    // ---- bottom shading band
    for (auto const& s : info.bottoms) {
        float h = std::min(info.bbox.h * 0.3f, 14.f);
        shade(c.box(s.x0, s.y, s.x1 - s.x0, h, st.fillSlot, Layer::MAIN, 1), st.fillShade * 0.72f);
    }

    // ---- pattern
    for (auto const& r : info.fills) {
        float area = r.w * r.h;
        switch (st.pat) {
            case Pat::Bricks: {
                // darker mortar, bricks on top
                c.box(r.x, r.y, r.w, r.h, st.fillSlot, Layer::MAIN, 1).shade = st.fillShade * 0.55f;
                float bh = 15.f, bw = 30.f;
                int row = 0;
                for (float y = r.y; y < r.top() - 0.1f; y += bh, ++row) {
                    float off = (row % 2) ? -bw / 2 : 0;
                    for (float x = r.x + off; x < r.right() - 0.1f; x += bw) {
                        float x0 = std::max(r.x, x) + 1.2f, x1 = std::min(r.right(), x + bw) - 1.2f;
                        if (x1 - x0 < 3) continue;
                        auto& b = c.box(x0, y + 1.2f, x1 - x0, bh - 2.4f, st.fillSlot, Layer::MAIN, 2);
                        b.shade = st.fillShade * rng.range(0.92f, 1.12f);
                    }
                }
                break;
            }
            case Pat::Stripes: {
                for (float x = r.x + 4; x < r.right() - 2; x += 15)
                    shade(c.box(x, r.y, 7, r.h, LIGHT, Layer::FRONT, 2), 0.9f, 0.6f);
                break;
            }
            case Pat::Grid: {
                if (d < 0.7f) break;
                for (float x = r.x + BLOCK; x < r.right() - 1; x += BLOCK)
                    shade(c.box(x - 0.75f, r.y, 1.5f, r.h, GLOW, Layer::FRONT, 3), 0.3f);
                for (float y = r.y + BLOCK; y < r.top() - 1; y += BLOCK)
                    shade(c.box(r.x, y - 0.75f, r.w, 1.5f, GLOW, Layer::FRONT, 3), 0.3f);
                break;
            }
            case Pat::Dots: case Pat::Stars: case Pat::Bubbles: {
                int n = std::max(1, int(area / 900.f * 1.2f * d));
                for (int i = 0; i < n; ++i) {
                    float x = rng.range(r.x + 4, r.right() - 4), y = rng.range(r.y + 4, r.top() - 6);
                    if (st.pat == Pat::Stars) {
                        float sz = rng.range(3, 6);
                        c.shape(Shape::Star, x, y, sz, sz, i % 3 ? LIGHT : GLOW, Layer::FRONT, i < 2 ? 2 : 4).sat = 0.5f;
                    }
                    else if (st.pat == Pat::Bubbles) {
                        float sz = rng.range(3, 8);
                        shade(c.ring(x, y, sz, sz, c.spec->theme == "toxic" ? GLOW : LIGHT, Layer::FRONT, 4), 0.7f);
                    }
                    else {
                        float sz = rng.range(3, 7);
                        auto& p = c.shape(Shape::Hexagon, x, y, sz * 1.3f, sz, st.fillSlot, Layer::FRONT, 4);
                        p.shade = st.fillShade * 0.6f;
                    }
                }
                break;
            }
            case Pat::Panels: {
                float ps = r.w >= 60 && r.h >= 60 ? 60.f : 30.f;
                for (float x = r.x; x < r.right() - 0.1f; x += ps) {
                    for (float y = r.y; y < r.top() - 0.1f; y += ps) {
                        float w = std::min(ps, r.right() - x), h = std::min(ps, r.top() - y);
                        shade(c.box(x + 2.5f, y + 2.5f, w - 5, h - 5, st.fillSlot, Layer::MAIN, 2), st.fillShade * 1.18f);
                        if (st.rivets && w >= 30 && h >= 30 && d >= 0.9f) {
                            for (int k = 0; k < 3; ++k)
                                shade(c.box(x + w * 0.3f, y + h * 0.35f + k * 5.f, w * 0.4f, 2.f, BASE, Layer::FRONT, 3), 1.5f);
                        }
                        if (st.rivets) {
                            for (int cx = 0; cx < 2; ++cx)
                                for (int cy = 0; cy < 2; ++cy)
                                    shade(c.circle(x + 5.5f + cx * (w - 11), y + 5.5f + cy * (h - 11), 2.6f, LIGHT, Layer::FRONT, 4), 0.75f);
                        }
                    }
                }
                break;
            }
            case Pat::Checker: {
                for (float x = r.x; x < r.right() - 0.1f; x += 15)
                    for (float y = r.y; y < r.top() - 0.1f; y += 15)
                        if ((int(x / 15) + int(y / 15)) % 2 == 0)
                            shade(c.box(x, y, 15, 15, st.fillSlot, Layer::MAIN, 2), st.fillShade * 1.3f);
                break;
            }
            case Pat::Cracks: {
                int n = std::max(1, int(area / 1800.f * d));
                bool hot = c.spec->theme == "lava" || c.spec->theme == "hell";
                for (int i = 0; i < n; ++i) {
                    float x = rng.range(r.x + 6, r.right() - 6), y = rng.range(r.y + 6, r.top() - 6);
                    float a = rng.range(0, 2 * PI);
                    for (int seg = 0; seg < 3; ++seg) {
                        float len = rng.range(6, 12);
                        float x2 = clampf(x + std::cos(a) * len, r.x + 2, r.right() - 2);
                        float y2 = clampf(y + std::sin(a) * len, r.y + 2, r.top() - 2);
                        auto& b = c.bar(x, y, x2, y2, hot ? 2.2f : 1.6f, hot ? GLOW : BASE, Layer::FRONT, seg == 0 ? 2 : 3);
                        b.shade = hot ? 0.9f : 1.6f;
                        x = x2; y = y2;
                        a += rng.range(-0.9f, 0.9f);
                    }
                    if (hot && c.glow) c.glowAt(x, y, 18, 18, 0.5f, Layer::FRONT, 4);
                }
                break;
            }
            case Pat::Tiles: {
                for (float x = r.x; x < r.right() - 0.1f; x += BLOCK)
                    for (float y = r.y; y < r.top() - 0.1f; y += BLOCK)
                        shade(c.box(x + 2, y + 2, BLOCK - 4, BLOCK - 4, st.fillSlot, Layer::MAIN, 2), st.fillShade * 1.25f);
                break;
            }
            case Pat::Strata: {
                int i = 0;
                for (float y = r.y; y < r.top() - 0.1f; y += 10, ++i) {
                    if (i % 2) continue;
                    float h = std::min(10.f, r.top() - y);
                    shade(c.box(r.x, y, r.w, h, st.fillSlot, Layer::MAIN, 2), st.fillShade * (i % 4 ? 1.12f : 0.88f));
                }
                break;
            }
            case Pat::Planks: {
                for (float y = r.y + 10; y < r.top() - 0.1f; y += 10)
                    shade(c.box(r.x, y - 0.8f, r.w, 1.6f, BASE, Layer::MAIN, 2), 1.2f);
                int joints = std::max(1, int(area / 600.f * d));
                for (int i = 0; i < joints; ++i) {
                    float y = r.y + std::floor(rng.range(0, r.h / 10.f)) * 10.f;
                    shade(c.box(rng.range(r.x + 5, r.right() - 5), y, 1.6f, 10, BASE, Layer::MAIN, 4), 1.2f);
                }
                break;
            }
            case Pat::Insets: {
                if (r.w >= 20 && r.h >= 20) shade(c.box(r.x + 4, r.y + 4, r.w - 8, r.h - 8, st.fillSlot, Layer::MAIN, 1), st.fillShade * 1.25f);
                if (r.w >= 34 && r.h >= 34) shade(c.box(r.x + 9, r.y + 9, r.w - 18, r.h - 18, st.fillSlot, Layer::MAIN, 2), st.fillShade * 1.5f);
                break;
            }
            default: break;
        }
    }

    // ---- outline
    float t = 2.5f;
    if (st.outline) {
        for (auto const& s : info.tops) c.box(s.x0, s.y - t, s.x1 - s.x0, t, st.outlineSlot, Layer::FRONT, 1);
        for (auto const& s : info.bottoms) c.box(s.x0, s.y, s.x1 - s.x0, t, st.outlineSlot, Layer::FRONT, 1);
        for (auto const& s : info.lefts) c.box(s.x, s.y0, t, s.y1 - s.y0, st.outlineSlot, Layer::FRONT, 1);
        for (auto const& s : info.rights) c.box(s.x - t, s.y0, t, s.y1 - s.y0, st.outlineSlot, Layer::FRONT, 1);
        if (st.innerLines && d >= 0.8f) {
            float in = 6.f, t2 = 1.5f;
            for (auto const& s : info.tops) if (s.x1 - s.x0 > 2 * in) shade(c.box(s.x0 + in, s.y - in - t2, s.x1 - s.x0 - 2 * in, t2, GLOW, Layer::FRONT, 2), 0.7f);
            for (auto const& s : info.bottoms) if (s.x1 - s.x0 > 2 * in) shade(c.box(s.x0 + in, s.y + in, s.x1 - s.x0 - 2 * in, t2, GLOW, Layer::FRONT, 2), 0.7f);
            for (auto const& s : info.lefts) if (s.y1 - s.y0 > 2 * in) shade(c.box(s.x + in, s.y0 + in, t2, s.y1 - s.y0 - 2 * in, GLOW, Layer::FRONT, 2), 0.7f);
            for (auto const& s : info.rights) if (s.y1 - s.y0 > 2 * in) shade(c.box(s.x - in - t2, s.y0 + in, t2, s.y1 - s.y0 - 2 * in, GLOW, Layer::FRONT, 2), 0.7f);
        }
    }

    // ---- edge glow (outside the structure, behind the player)
    if (st.edgeGlow && c.glow) {
        float gh = 26.f;
        float power = c.spec->theme == "modern" ? 0.45f : 0.85f;
        for (auto const& s : info.tops) {
            auto& g = c.shape(Shape::EdgeGlow, (s.x0 + s.x1) / 2, s.y + gh / 2, s.x1 - s.x0, gh, GLOW, Layer::BG, 3);
            g.shade = power;
        }
        for (auto const& s : info.bottoms) {
            auto& g = c.shape(Shape::EdgeGlow, (s.x0 + s.x1) / 2, s.y - gh / 2, s.x1 - s.x0, gh, GLOW, Layer::BG, 3);
            g.rot = 180; g.shade = power * 0.8f;
        }
        for (auto const& s : info.lefts) {
            auto& g = c.shape(Shape::EdgeGlow, s.x - gh / 2, (s.y0 + s.y1) / 2, s.y1 - s.y0, gh, GLOW, Layer::BG, 3);
            g.rot = -90; g.shade = power * 0.9f;
        }
        for (auto const& s : info.rights) {
            auto& g = c.shape(Shape::EdgeGlow, s.x + gh / 2, (s.y0 + s.y1) / 2, s.y1 - s.y0, gh, GLOW, Layer::BG, 3);
            g.rot = 90; g.shade = power * 0.9f;
        }
    }

    // ---- top dressing
    for (auto const& s : info.tops) {
        float len = s.x1 - s.x0;
        switch (st.top) {
            case Top::Cap:
                shade(c.box(s.x0, s.y - 4, len, 4, LIGHT, Layer::FRONT, 1), 0.85f);
                shade(c.box(s.x0, s.y - 5.5f, len, 1.5f, BASE, Layer::FRONT, 2), 1.2f);
                if (st.name == "modern" || st.name == "tech") {
                    // accent line + corner caps, the "finished" look of modern layouts
                    int slot = st.name == "tech" ? GLOW : ACCENT;
                    if (len > 16) shade(c.box(s.x0 + 5, s.y - 10, len - 10, 1.8f, slot, Layer::FRONT, 2), st.name == "tech" ? 0.8f : 1.f);
                    c.box(s.x0, s.y - 7, 5, 7, ACCENT, Layer::FRONT, 2);
                    c.box(s.x1 - 5, s.y - 7, 5, 7, ACCENT, Layer::FRONT, 2);
                }
                break;
            case Top::Grass: case Top::Moss: case Top::Flowers: {
                bool moss = st.top == Top::Moss;
                int grassSlot = FOLIAGE;
                c.box(s.x0, s.y - 7, len, 7, grassSlot, Layer::FRONT, 0);
                shade(c.box(s.x0, s.y - 2, len, 2, FOLIAGE, Layer::FRONT, 2), 1.35f, 0.8f);
                // scalloped lower edge
                for (float x = s.x0 + 4; x < s.x1 - 2; x += 8)
                    c.circle(x, s.y - 7, rng.range(6, 10), grassSlot, Layer::FRONT, 1);
                if (!moss) {
                    int blades = int(len / 6.f * std::min(d, 1.6f));
                    for (int i = 0; i < blades; ++i) {
                        float x = rng.range(s.x0 + 2, s.x1 - 2), h = rng.range(4, 11);
                        auto& b = c.tri(x, s.y + h / 2 - 0.5f, 3.5f, h, grassSlot, Layer::FX, i % 2 ? 2 : 4);
                        b.shade = i % 3 == 0 ? 1.35f : 0.9f;
                        c.rotated(b, rng.range(-15, 15));
                    }
                }
                int flowers = st.top == Top::Flowers ? int(len / 20.f * d) : int(len / 60.f * d);
                for (int i = 0; i < flowers; ++i) {
                    float x = rng.range(s.x0 + 3, s.x1 - 3), fy = s.y + rng.range(2, 6);
                    c.circle(x, fy, 4.5f, BLOOM, Layer::FX, 4);
                    auto& ctr = c.circle(x, fy, 1.8f, BLOOM, Layer::FX, 4);
                    ctr.hue = 50; ctr.shade = 1.4f;
                }
                break;
            }
            case Top::Snow: {
                c.box(s.x0, s.y - 6, len, 6, LIGHT, Layer::FRONT, 0);
                for (float x = s.x0 + 5; x < s.x1 - 3; x += rng.range(8, 13))
                    c.circle(x, s.y - 1, rng.range(9, 15), LIGHT, Layer::FRONT, 1);
                c.circle(s.x0 + 2, s.y - 3, 8, LIGHT, Layer::FRONT, 1);
                c.circle(s.x1 - 2, s.y - 3, 8, LIGHT, Layer::FRONT, 1);
                int sp = int(len / 40.f * d);
                for (int i = 0; i < sp; ++i) {
                    auto& st2 = c.shape(Shape::Star, rng.range(s.x0, s.x1), s.y + rng.range(3, 14), 4, 4, GLOW, Layer::FX, 4);
                    st2.sat = 0.3f;
                }
                break;
            }
            case Top::Crystals: {
                for (float x = s.x0 + rng.range(6, 18); x < s.x1 - 4; x += rng.range(22, 40) / std::max(0.6f, d)) {
                    float h = rng.range(16, 38), w = h * 0.3f, a = rng.range(-22, 22);
                    float r = a * PI / 180.f;
                    float cx = x + std::sin(r) * h * 0.4f, cy = s.y + std::cos(r) * h * 0.4f;
                    if (c.glow) c.glowAt(cx, cy, w * 3.5f, h * 1.4f, 0.55f, Layer::BG, 3);
                    auto& body = c.shape(Shape::Diamond, cx, cy, w, h, MID, Layer::FRONT, 0);
                    c.rotated(body, a);
                    auto& facet = c.shape(Shape::Diamond, cx - std::cos(r) * w * 0.17f, cy + std::sin(r) * w * 0.17f, w * 0.42f, h * 0.88f, LIGHT, Layer::FRONT, 2);
                    facet.shade = 0.92f;
                    c.rotated(facet, a);
                }
                break;
            }
            case Top::Spikes: {
                for (float x = s.x0 + 6; x < s.x1 - 4; x += 12) {
                    float h = rng.range(9, 20);
                    auto& sp = c.tri(x, s.y + h / 2, 9, h, BASE, Layer::FRONT, 0);
                    c.rotated(sp, rng.range(-10, 10));
                    auto& in = c.tri(x - 0.8f, s.y + h * 0.38f, 3.5f, h * 0.65f, MID, Layer::FRONT, 2);
                    in.rot = sp.rot;
                    if (c.glow && rng.chance(0.5f)) c.glowAt(x, s.y + h, 10, 10, 0.8f, Layer::FX, 4);
                }
                break;
            }
            case Top::Waves: {
                for (float x = s.x0 + 5; x < s.x1 - 3; x += 10) c.circle(x, s.y - 3, 12, MID, Layer::FRONT, 1);
                for (float x = s.x0 + 3; x < s.x1 - 3; x += 10) shade(c.circle(x, s.y + 1, 5, LIGHT, Layer::FRONT, 2), 0.95f, 0.5f);
                break;
            }
            case Top::Frosting: {
                c.box(s.x0, s.y - 8, len, 8, LIGHT, Layer::FRONT, 0);
                for (float x = s.x0 + 5; x < s.x1 - 3; x += 9) c.circle(x, s.y - 1, 9, LIGHT, Layer::FRONT, 1);
                for (float x = s.x0 + rng.range(4, 10); x < s.x1 - 4; x += rng.range(10, 18)) {
                    float h = rng.range(5, 16);
                    c.box(x - 2.5f, s.y - 8 - h, 5, h, LIGHT, Layer::FRONT, 2);
                    c.circle(x, s.y - 8 - h, 5, LIGHT, Layer::FRONT, 2);
                }
                int spr = int(len / 12.f * d);
                for (int i = 0; i < spr; ++i) {
                    auto& p = c.rect(rng.range(s.x0 + 2, s.x1 - 2), s.y - rng.range(1, 6), 4, 1.5f, i % 2 ? ACCENT : GLOW, Layer::FX, 4);
                    p.rot = rng.range(0, 180);
                    p.hue = rng.range(-60, 60);
                }
                break;
            }
            case Top::Slime: {
                shade(c.box(s.x0, s.y - 6, len, 6, GLOW, Layer::FRONT, 0), 0.75f);
                for (float x = s.x0 + 5; x < s.x1 - 3; x += 10) shade(c.circle(x, s.y - 1, 8, GLOW, Layer::FRONT, 1), 0.75f);
                break;
            }
            case Top::Crust: {
                shade(c.box(s.x0, s.y - 5, len, 5, GLOW, Layer::FRONT, 0), 0.9f);
                shade(c.box(s.x0, s.y - 7, len, 2, MID, Layer::FRONT, 1), 1.f);
                int embers = int(len / 25.f * d);
                for (int i = 0; i < embers; ++i)
                    c.shape(Shape::Diamond, rng.range(s.x0, s.x1), s.y + rng.range(6, 30), 2.5f, 4, GLOW, Layer::FX, 4);
                break;
            }
            case Top::Clouds: {
                for (float x = s.x0 + 8; x < s.x1 - 4; x += rng.range(12, 20))
                    c.circle(x, s.y - 2, rng.range(16, 26), LIGHT, Layer::FRONT, 1);
                break;
            }
            default: break;
        }
    }

    // ---- hanging details
    for (auto const& s : info.bottoms) {
        switch (st.bot) {
            case Bot::Icicles:
                for (float x = s.x0 + rng.range(3, 8); x < s.x1 - 3; x += rng.range(7, 14)) {
                    float h = rng.range(8, 26);
                    auto& ic = c.tri(x, s.y - h / 2 + 1, rng.range(5, 8), h, LIGHT, Layer::FRONT, 1);
                    ic.rot = 180;
                    ic.shade = rng.range(0.85f, 1.f);
                }
                break;
            case Bot::Drips:
                for (float x = s.x0 + rng.range(4, 10); x < s.x1 - 4; x += rng.range(10, 20)) {
                    float h = rng.range(5, 20);
                    shade(c.box(x - 2, s.y - h, 4, h + 1, st.dripSlot, Layer::FRONT, 1), st.dripSlot == GLOW ? 0.8f : 1.f);
                    shade(c.circle(x, s.y - h, 6, st.dripSlot, Layer::FRONT, 1), st.dripSlot == GLOW ? 0.8f : 1.f);
                }
                break;
            case Bot::Vines:
                for (float x = s.x0 + rng.range(5, 14); x < s.x1 - 5; x += rng.range(16, 28) / std::clamp(d, 0.6f, 1.3f)) {
                    float len = rng.range(18, 55);
                    float sway = rng.range(-5, 5);
                    c.bar(x, s.y + 1, x + sway, s.y - len, 1.8f, FOLIAGE, Layer::FRONT, 2).shade = 0.8f;
                    for (float y = 8; y < len - 2; y += 8) {
                        float t2 = y / len;
                        auto& leaf = c.ellipse(x + sway * t2 + ((int(y) / 8) % 2 ? 3 : -3), s.y - y, 6, 3, FOLIAGE, Layer::FRONT, 4);
                        leaf.rot = ((int(y) / 8) % 2) ? -30.f : 30.f;
                    }
                }
                break;
            default: break;
        }
    }
}

} // namespace deco
