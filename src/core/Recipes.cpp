// Prop recipes. Every recipe draws one subject with its base at local (0, 0),
// roughly S units tall. Recipes are written in layers of priority:
//   pri 0  silhouette (always kept)
//   pri 1  shading / secondary forms
//   pri 2  highlights, trims, small structure
//   pri 3  glow
//   pri 4  sparkles, scatter detail
// so the object limit can strip detail gracefully.
#include "Recipes.hpp"

#include <algorithm>

namespace deco {

namespace {

float deg(float rad) { return rad * 180.f / PI; }

// Teardrop flame built from additive glow-channel shapes. Overlaps add up,
// which gives a hot white core for free.
void flame(Canvas& c, float x, float y, float s, Layer l = Layer::FRONT) {
    shade(c.circle(x, y + s * 0.28f, s * 0.56f, GLOW, l, 0), 0.75f);
    shade(c.tri(x, y + s * 0.62f, s * 0.5f, s * 0.72f, GLOW, l, 0), 0.75f);
    shade(c.circle(x, y + s * 0.25f, s * 0.36f, GLOW, l, 1), 0.9f, 0.75f);
    shade(c.tri(x, y + s * 0.5f, s * 0.3f, s * 0.48f, GLOW, l, 1), 0.9f, 0.75f);
    shade(c.circle(x, y + s * 0.22f, s * 0.18f, GLOW, l, 2), 1.f, 0.2f);
    c.glowAt(x, y + s * 0.35f, s * 2.4f, s * 2.4f, 0.8f, Layer::BACK, 3);
    int embers = c.n(2);
    for (int i = 0; i < embers; ++i) {
        auto& e = c.shape(Shape::Diamond, x + c.rng.range(-s * 0.4f, s * 0.4f), y + s * c.rng.range(0.9f, 1.4f), s * 0.07f, s * 0.11f, GLOW, Layer::FX, 4);
        e.sat = 0.6f;
    }
}

void windowArch(Canvas& c, float x, float y, float w, float h, bool lit, Layer l) {
    int slot = lit ? GLOW : BASE;
    auto& r = c.box(x - w / 2, y, w, h - w / 2, slot, l, 1);
    auto& top = c.circle(x, y + h - w / 2, w, slot, l, 1);
    if (lit) { r.shade = 0.85f; top.shade = 0.85f; }
    else { r.shade = 0.9f; top.shade = 0.9f; }
    c.box(x - w * 0.65f, y - w * 0.12f, w * 1.3f, w * 0.18f, LIGHT, l, 2).shade = 0.75f;
    if (lit) c.glowAt(x, y + h / 2, w * 3.f, h * 2.f, 0.45f, Layer::FX, 3);
}

// --------------------------------------------------------------------- nature

void tree(Canvas& c, float S, bool petals) {
    float tw = S * 0.12f, th = S * 0.52f;
    shade(c.box(-tw / 2, 0, tw, th, WOOD, Layer::MAIN, 0), 0.8f);
    shade(c.box(tw * 0.05f, 0, tw * 0.45f, th * 0.96f, WOOD, Layer::MAIN, 1), 0.55f);
    shade(c.slope(-tw / 2 - S * 0.04f, S * 0.035f, S * 0.08f, S * 0.07f, false, false, WOOD, Layer::MAIN, 2), 0.8f);
    shade(c.slope(tw / 2 + S * 0.04f, S * 0.035f, S * 0.08f, S * 0.07f, true, false, WOOD, Layer::MAIN, 2), 0.55f);
    shade(c.bar(0, th * 0.62f, S * 0.17f, th * 0.92f, S * 0.035f, WOOD, Layer::MAIN, 2), 0.7f);
    shade(c.bar(0, th * 0.72f, -S * 0.14f, th * 0.98f, S * 0.03f, WOOD, Layer::MAIN, 2), 0.8f);

    float cy = S * 0.69f, R = S * 0.3f;
    int blobs = std::max(4, c.n(5));
    std::vector<Vec2> pos;
    std::vector<float> size;
    pos.push_back({ 0, cy + R * 0.1f });
    size.push_back(R * 1.25f);
    for (int i = 0; i < blobs; ++i) {
        float a = (i + c.rng.range(-0.25f, 0.25f)) / blobs * 2 * PI;
        pos.push_back({ std::cos(a) * R * 0.62f, cy + std::sin(a) * R * 0.48f });
        size.push_back(R * c.rng.range(0.82f, 1.05f));
    }
    // back (shadow) layer, slightly offset down-right
    int leaf = petals ? BLOOM : FOLIAGE;
    for (size_t i = 0; i < pos.size(); ++i)
        shade(c.circle(pos[i].x + S * 0.035f, pos[i].y - S * 0.035f, size[i], leaf, Layer::FRONT, i == 0 ? 0 : 1), 0.5f);
    for (size_t i = 0; i < pos.size(); ++i)
        shade(c.circle(pos[i].x, pos[i].y, size[i] * 0.94f, leaf, Layer::FRONT, 0), c.rng.range(0.9f, 1.05f));
    int hl = c.n(3);
    for (int i = 0; i < hl; ++i) {
        float a = c.rng.range(0.55f, 0.95f) * PI;
        float rr = R * c.rng.range(0.2f, 0.55f);
        shade(c.circle(std::cos(a) * rr - R * 0.1f, cy + std::sin(a) * rr + R * 0.15f, R * c.rng.range(0.22f, 0.36f), leaf, Layer::FRONT, 2), 1.3f, 0.75f);
    }
    int dots = c.n(petals ? 5 : 3);
    for (int i = 0; i < dots; ++i) {
        float a = c.rng.range(0, 2 * PI), rr = R * c.rng.range(0.2f, 0.85f);
        auto& d = c.circle(std::cos(a) * rr, cy + std::sin(a) * rr * 0.8f, S * c.rng.range(0.025f, 0.04f), BLOOM, Layer::FRONT, 4);
        if (petals) { d.shade = 1.2f; d.sat = 0.5f; }
    }
    if (petals) {
        int falling = c.n(5);
        for (int i = 0; i < falling; ++i) {
            auto& p = c.ellipse(c.rng.range(-S * 0.55f, S * 0.55f), c.rng.range(0, S * 0.45f), S * 0.04f, S * 0.022f, BLOOM, Layer::FX, 4);
            p.shade = 1.2f; p.sat = 0.6f;
            c.rotated(p, c.rng.range(-60, 60));
        }
    }
    if (c.glow && (c.spec->glow == 1 || c.spec->night)) {
        int flies = c.n(3);
        for (int i = 0; i < flies; ++i)
            c.glowAt(c.rng.range(-S * 0.5f, S * 0.5f), c.rng.range(S * 0.2f, S * 0.95f), S * 0.08f, S * 0.08f, 1.f, Layer::FX, 4);
        c.glowAt(0, cy, R * 3.2f, R * 2.8f, 0.3f, Layer::BACK, 3);
    }
}

void pine(Canvas& c, float S) {
    bool snow = c.spec->theme == "ice";
    float tw = S * 0.1f;
    shade(c.box(-tw / 2, 0, tw, S * 0.22f, WOOD, Layer::MAIN, 0), 0.7f);
    int tiers = c.detail > 1.3f ? 4 : 3;
    for (int i = 0; i < tiers; ++i) {
        float f = float(i) / tiers;
        float w = S * 0.82f * (1.f - f * 0.62f);
        float h = S * 0.42f * (1.f - f * 0.25f);
        float y = S * 0.16f + S * 0.68f * f;
        shade(c.tri(S * 0.025f, y + h / 2 - S * 0.02f, w * 1.04f, h * 1.02f, FOLIAGE, Layer::FRONT, 0), 0.5f);
        c.tri(0, y + h / 2, w, h, FOLIAGE, Layer::FRONT, 0);
        shade(c.slope(-w / 4, y + h / 2, w / 2, h, false, false, FOLIAGE, Layer::FRONT, 1), 1.25f, 0.9f);
        if (snow) {
            c.tri(0, y + h * 0.84f, w * 0.32f, h * 0.32f, LIGHT, Layer::FRONT, 2);
            shade(c.box(-w * 0.45f, y - S * 0.005f, w * 0.9f, S * 0.025f, LIGHT, Layer::FRONT, 2), 0.95f);
        }
    }
    int orn = (c.spec->glow == 1 || c.spec->theme == "ice") ? c.n(5) : 0;
    for (int i = 0; i < orn; ++i) {
        float t = c.rng.range(0.1f, 0.85f);
        float y = S * 0.2f + S * 0.65f * t;
        float half = S * 0.4f * (1.f - t * 0.75f) * 0.75f;
        float ox = c.rng.range(-half, half);
        auto& o = c.circle(ox, y, S * 0.045f, i % 2 ? ACCENT : GLOW, Layer::FX, 3);
        o.shade = 1.1f;
        if (c.glow && i % 2 == 0) c.glowAt(ox, y, S * 0.14f, S * 0.14f, 0.9f, Layer::FX, 4);
    }
    if (c.spec->glow == 1 || c.spec->theme == "ice") {
        c.shape(Shape::Star, 0, S * 1.03f, S * 0.14f, S * 0.14f, GLOW, Layer::FX, 2).sat = 0.6f;
        c.glowAt(0, S * 1.03f, S * 0.5f, S * 0.5f, 0.8f, Layer::FX, 3);
    }
}

void palm(Canvas& c, float S) {
    int segs = std::max(5, c.n(7));
    Vec2 prev{ 0, 0 };
    Vec2 top{};
    for (int i = 1; i <= segs; ++i) {
        float t = float(i) / segs;
        Vec2 p{ S * 0.18f * t * t, S * 0.72f * t };
        float th = S * (0.09f - 0.03f * t);
        shade(c.bar(prev.x, prev.y, p.x, p.y + S * 0.01f, th, WOOD, Layer::MAIN, 0), i % 2 ? 0.8f : 0.62f);
        prev = p;
        top = p;
    }
    float angles[] = { 160, 128, 95, 58, 22, -8 };
    for (int pass = 0; pass < 2; ++pass) {
        for (float a : angles) {
            float r = a * PI / 180.f;
            float len = S * 0.46f;
            float cx = top.x + std::cos(r) * len * 0.45f;
            float cy = top.y + std::sin(r) * len * 0.45f - S * 0.05f;
            float droop = a > 90 ? -18.f : 18.f;
            auto& leaf = c.ellipse(cx + (pass ? 0 : S * 0.02f), cy - (pass ? 0 : S * 0.02f), len, S * 0.1f, FOLIAGE, Layer::FRONT, pass ? 0 : 1);
            if (!pass) leaf.shade = 0.5f;
            c.rotated(leaf, -a + droop);
            if (pass) {
                auto& vein = c.ellipse(cx, cy + S * 0.01f, len * 0.8f, S * 0.025f, FOLIAGE, Layer::FRONT, 2);
                vein.shade = 1.4f; vein.sat = 0.7f;
                c.rotated(vein, -a + droop);
            }
        }
    }
    for (int i = 0; i < 3; ++i)
        shade(c.circle(top.x + (i - 1) * S * 0.05f, top.y - S * 0.05f, S * 0.07f, ACCENT, Layer::FRONT, 2), 0.45f);
}

void deadTree(Canvas& c, float S) {
    int depth = c.detail > 1.4f ? 4 : 3;
    struct B { float x, y, a, len, th; int d; };
    std::vector<B> stack{ { 0, 0, 90, S * 0.42f, S * 0.09f, depth } };
    while (!stack.empty()) {
        B b = stack.back();
        stack.pop_back();
        float r = b.a * PI / 180.f;
        float x2 = b.x + std::cos(r) * b.len, y2 = b.y + std::sin(r) * b.len;
        c.bar(b.x, b.y, x2, y2, b.th, BASE, Layer::MAIN, b.d >= depth - 1 ? 0 : (b.d == 0 ? 2 : 1));
        if (b.d > 0) {
            float spread = c.rng.range(20, 36);
            stack.push_back({ x2, y2, b.a + spread + c.rng.range(-6, 6), b.len * 0.68f, b.th * 0.66f, b.d - 1 });
            stack.push_back({ x2, y2, b.a - spread + c.rng.range(-6, 6), b.len * 0.68f, b.th * 0.66f, b.d - 1 });
        }
    }
    shade(c.slope(-S * 0.08f, S * 0.04f, S * 0.08f, S * 0.08f, false, false, BASE, Layer::MAIN, 1), 1.f);
    shade(c.slope(S * 0.08f, S * 0.04f, S * 0.08f, S * 0.08f, true, false, BASE, Layer::MAIN, 1), 1.f);
    if (c.glow) c.glowAt(0, S * 0.6f, S * 1.2f, S * 1.1f, 0.35f, Layer::BG, 3);
}

void bush(Canvas& c, float S) {
    float W = S * 1.3f;
    int blobs = std::max(3, c.n(4));
    shade(c.box(-W / 2, 0, W, S * 0.25f, FOLIAGE, Layer::MAIN, 0), 0.92f);
    std::vector<Vec2> pos;
    std::vector<float> sz;
    for (int i = 0; i < blobs; ++i) {
        float t = blobs == 1 ? 0.5f : float(i) / (blobs - 1);
        float x = (t - 0.5f) * W * 0.78f;
        float bump = 1.f - std::fabs(t - 0.5f) * 1.2f;
        pos.push_back({ x, S * (0.3f + 0.22f * bump) + c.rng.range(-S * 0.03f, S * 0.03f) });
        sz.push_back(S * (0.42f + 0.3f * bump) * c.rng.range(0.9f, 1.08f));
    }
    for (size_t i = 0; i < pos.size(); ++i) shade(c.circle(pos[i].x + S * 0.04f, pos[i].y - S * 0.04f, sz[i], FOLIAGE, Layer::MAIN, 1), 0.5f);
    for (size_t i = 0; i < pos.size(); ++i) shade(c.circle(pos[i].x, pos[i].y, sz[i] * 0.92f, FOLIAGE, Layer::MAIN, 0), c.rng.range(0.92f, 1.05f));
    int hl = c.n(2);
    for (int i = 0; i < hl; ++i) {
        auto const& p = pos[c.rng.irange(0, int(pos.size()) - 1)];
        shade(c.circle(p.x - S * 0.08f, p.y + S * 0.08f, S * 0.16f, FOLIAGE, Layer::FRONT, 2), 1.3f, 0.75f);
    }
    int berries = c.n(3);
    for (int i = 0; i < berries; ++i) {
        auto& b = c.circle(c.rng.range(-W * 0.4f, W * 0.4f), c.rng.range(S * 0.15f, S * 0.6f), S * 0.06f, BLOOM, Layer::FRONT, 4);
        (void)b;
    }
}

void crystal(Canvas& c, float S) {
    int shards = std::max(3, c.n(3) + 2);
    struct Sh { float x, h, w, a; };
    std::vector<Sh> list;
    list.push_back({ 0, S * 1.0f, S * 0.3f, 0 });
    for (int i = 1; i < shards; ++i) {
        float side = (i % 2) ? -1.f : 1.f;
        float x = side * S * (0.12f + 0.1f * ((i + 1) / 2)) + c.rng.range(-S * 0.03f, S * 0.03f);
        float h = S * c.rng.range(0.45f, 0.75f) * (1.f - 0.1f * ((i - 1) / 2));
        list.push_back({ x, h, S * c.rng.range(0.17f, 0.24f), side * c.rng.range(14, 32) });
    }
    // draw tallest last so it sits in front
    std::sort(list.begin(), list.end(), [](Sh const& a, Sh const& b) { return a.h < b.h; });
    for (auto const& s : list) {
        float cy = s.h * 0.44f;
        float r = s.a * PI / 180.f;
        float cxp = s.x + std::sin(r) * cy;
        float cyp = std::cos(r) * cy;
        if (c.glow) c.glowAt(cxp, cyp, s.w * 3.f, s.h * 1.3f, 0.5f, Layer::BACK, 3);
        auto& body = c.shape(Shape::Diamond, cxp, cyp, s.w, s.h, MID, Layer::MAIN, 0);
        c.rotated(body, s.a);
        float off = s.w * 0.17f;
        auto& facet = c.shape(Shape::Diamond, cxp - std::cos(r) * off, cyp + std::sin(r) * off, s.w * 0.42f, s.h * 0.88f, LIGHT, Layer::MAIN, 1);
        facet.shade = 0.92f;
        c.rotated(facet, s.a);
        auto& dark = c.shape(Shape::Diamond, cxp + std::cos(r) * off * 1.3f, cyp - std::sin(r) * off * 1.3f, s.w * 0.3f, s.h * 0.75f, BASE, Layer::MAIN, 2);
        dark.shade = 1.4f;
        c.rotated(dark, s.a);
    }
    int rocks = c.n(3);
    for (int i = 0; i < rocks; ++i) {
        float x = (rocks == 1 ? 0 : (float(i) / (rocks - 1) - 0.5f)) * S * 0.75f;
        auto& r = c.shape(Shape::Hexagon, x, S * 0.03f, S * c.rng.range(0.2f, 0.3f), S * 0.13f, BASE, Layer::FRONT, 1);
        r.shade = 1.3f;
    }
    int sp = c.n(3);
    for (int i = 0; i < sp; ++i) {
        float x = c.rng.range(-S * 0.5f, S * 0.5f), y = c.rng.range(S * 0.25f, S * 1.05f);
        auto& st = c.shape(Shape::Star, x, y, S * 0.06f, S * 0.06f, GLOW, Layer::FX, 4);
        st.sat = 0.4f;
    }
}

void rocks(Canvas& c, float S) {
    int n = std::max(2, c.n(2) + 1);
    for (int i = 0; i < n; ++i) {
        float t = n == 1 ? 0.5f : float(i) / (n - 1);
        float w = S * (i == n / 2 ? 0.75f : c.rng.range(0.4f, 0.55f));
        float h = w * c.rng.range(0.55f, 0.7f);
        float x = (t - 0.5f) * S * 0.9f;
        auto& body = c.shape(Shape::Hexagon, x, h * 0.42f, w, h, MID, Layer::MAIN, 0);
        body.shade = 0.82f;
        c.rotated(body, c.rng.range(-8, 8));
        auto& sh = c.shape(Shape::Hexagon, x + w * 0.1f, h * 0.32f, w * 0.75f, h * 0.6f, BASE, Layer::MAIN, 1);
        sh.shade = 1.2f;
        auto& hl = c.shape(Shape::Hexagon, x - w * 0.16f, h * 0.58f, w * 0.42f, h * 0.36f, LIGHT, Layer::MAIN, 2);
        hl.shade = 0.75f;
        if (c.spec->theme == "forest" || c.spec->has(Surf::Moss)) {
            c.ellipse(x - w * 0.05f, h * 0.78f, w * 0.55f, h * 0.18f, FOLIAGE, Layer::FRONT, 2);
        }
    }
}

void mushroom(Canvas& c, float S) {
    bool glowy = c.spec->glow == 1;
    shade(c.box(-S * 0.09f, 0, S * 0.18f, S * 0.52f, LIGHT, Layer::MAIN, 0), 0.88f, 0.4f);
    shade(c.box(S * 0.02f, 0, S * 0.07f, S * 0.5f, LIGHT, Layer::MAIN, 1), 0.65f, 0.4f);
    c.ellipse(0, S * 0.5f, S * 0.72f, S * 0.2f, BASE, Layer::MAIN, 1);
    int capSlot = glowy ? MID : BLOOM;
    c.ellipse(S * 0.02f, S * 0.6f, S * 0.98f, S * 0.52f, BASE, Layer::FRONT, 1);
    shade(c.ellipse(0, S * 0.63f, S * 0.95f, S * 0.5f, capSlot, Layer::FRONT, 0), 1.f);
    shade(c.ellipse(-S * 0.14f, S * 0.73f, S * 0.36f, S * 0.15f, LIGHT, Layer::FRONT, 2), 0.95f, 0.5f);
    int spots = c.n(4);
    for (int i = 0; i < spots; ++i) {
        float a = c.rng.range(0.15f, 0.85f) * PI;
        float rr = c.rng.range(0.15f, 0.36f);
        auto& sp = c.circle(std::cos(a) * S * rr, S * 0.63f + std::sin(a) * S * rr * 0.45f, S * c.rng.range(0.06f, 0.1f), glowy ? GLOW : LIGHT, Layer::FRONT, 2);
        sp.sat = 0.5f;
    }
    if (c.glow && glowy) {
        c.glowAt(0, S * 0.6f, S * 1.6f, S * 1.2f, 0.55f, Layer::BACK, 3);
        int sp2 = c.n(3);
        for (int i = 0; i < sp2; ++i)
            c.glowAt(c.rng.range(-S * 0.5f, S * 0.5f), c.rng.range(S * 0.1f, S * 0.9f), S * 0.07f, S * 0.07f, 1.f, Layer::FX, 4);
    }
}

void flower(Canvas& c, float S) {
    shade(c.bar(0, 0, S * 0.03f, S * 0.66f, S * 0.045f, FOLIAGE, Layer::MAIN, 0), 0.8f);
    auto& l1 = c.ellipse(-S * 0.1f, S * 0.24f, S * 0.24f, S * 0.08f, FOLIAGE, Layer::MAIN, 1);
    c.rotated(l1, 35);
    auto& l2 = c.ellipse(S * 0.12f, S * 0.36f, S * 0.22f, S * 0.07f, FOLIAGE, Layer::MAIN, 1);
    c.rotated(l2, -35);
    l2.shade = 0.85f;
    float cx = S * 0.03f, cy = S * 0.7f;
    int petals = c.detail > 1.2f ? 7 : 5;
    for (int i = 0; i < petals; ++i) {
        float a = float(i) / petals * 2 * PI + 0.3f;
        auto& p = c.ellipse(cx + std::cos(a) * S * 0.11f, cy + std::sin(a) * S * 0.11f, S * 0.22f, S * 0.12f, BLOOM, Layer::FRONT, 0);
        p.shade = (i % 2) ? 0.9f : 1.f;
        c.rotated(p, -deg(a));
    }
    if (c.detail > 1.f) {
        for (int i = 0; i < petals; ++i) {
            float a = float(i) / petals * 2 * PI + 0.75f;
            auto& p = c.ellipse(cx + std::cos(a) * S * 0.06f, cy + std::sin(a) * S * 0.06f, S * 0.12f, S * 0.07f, BLOOM, Layer::FRONT, 2);
            p.shade = 1.2f; p.sat = 0.7f;
            c.rotated(p, -deg(a));
        }
    }
    auto& ctr = c.circle(cx, cy, S * 0.11f, BLOOM, Layer::FRONT, 1);
    ctr.hue = 50; ctr.shade = 1.4f; ctr.sat = 0.9f;
    if (c.glow && c.spec->glow == 1) c.glowAt(cx, cy, S * 0.6f, S * 0.6f, 0.6f, Layer::BACK, 3);
}

void grass(Canvas& c, float S) {
    int blades = std::max(5, c.n(12));
    float W = S * 1.8f;
    for (int i = 0; i < blades; ++i) {
        float x = (float(i) / (blades - 1) - 0.5f) * W + c.rng.range(-S * 0.04f, S * 0.04f);
        float h = S * c.rng.range(0.18f, 0.42f);
        auto& b = c.tri(x, h / 2, S * 0.06f, h, FOLIAGE, Layer::FRONT, i % 2 ? 1 : 0);
        b.shade = i % 3 == 0 ? 1.35f : c.rng.range(0.8f, 1.05f);
        c.rotated(b, c.rng.range(-18, 18));
    }
    int fl = c.n(2);
    for (int i = 0; i < fl; ++i) {
        auto& f = c.circle(c.rng.range(-W * 0.4f, W * 0.4f), S * c.rng.range(0.2f, 0.35f), S * 0.06f, BLOOM, Layer::FX, 4);
        (void)f;
    }
}

void cactus(Canvas& c, float S) {
    float w = S * 0.2f;
    shade(c.box(-w / 2, 0, w, S * 0.75f, FOLIAGE, Layer::MAIN, 0), 0.95f);
    shade(c.circle(0, S * 0.75f, w, FOLIAGE, Layer::MAIN, 0), 0.95f);
    // left arm
    shade(c.box(-S * 0.3f, S * 0.32f, S * 0.22f, w * 0.75f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    shade(c.box(-S * 0.3f, S * 0.32f, w * 0.75f, S * 0.22f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    shade(c.circle(-S * 0.3f + w * 0.375f, S * 0.54f, w * 0.75f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    // right arm
    shade(c.box(S * 0.08f, S * 0.45f, S * 0.2f, w * 0.7f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    shade(c.box(S * 0.28f - w * 0.7f, S * 0.45f, w * 0.7f, S * 0.18f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    shade(c.circle(S * 0.28f - w * 0.35f, S * 0.63f, w * 0.7f, FOLIAGE, Layer::MAIN, 0), 0.85f);
    for (int i = -1; i <= 1; ++i)
        shade(c.box(i * w * 0.28f - S * 0.006f, S * 0.04f, S * 0.012f, S * 0.68f, FOLIAGE, Layer::FRONT, 2), 0.65f);
    shade(c.box(w * 0.15f, 0, w * 0.35f, S * 0.74f, FOLIAGE, Layer::MAIN, 1), 0.72f);
    c.circle(0, S * 0.86f, S * 0.08f, BLOOM, Layer::FRONT, 2);
}

void island(Canvas& c, float S) {
    float W = S * 1.2f;
    float top = S * 0.7f;
    auto& rock = c.tri(0, top - S * 0.33f, W * 0.9f, S * 0.66f, BASE, Layer::MAIN, 0);
    rock.rot = 180;
    auto& rock2 = c.tri(-W * 0.22f, top - S * 0.2f, W * 0.45f, S * 0.4f, BASE, Layer::MAIN, 1);
    rock2.rot = 180; rock2.shade = 1.4f;
    auto& rock3 = c.tri(W * 0.25f, top - S * 0.16f, W * 0.35f, S * 0.3f, BASE, Layer::MAIN, 1);
    rock3.rot = 180; rock3.shade = 1.25f;
    shade(c.box(-W / 2, top - S * 0.12f, W, S * 0.12f, WOOD, Layer::MAIN, 0), 0.8f);
    c.box(-W / 2 - S * 0.02f, top - S * 0.02f, W + S * 0.04f, S * 0.07f, FOLIAGE, Layer::FRONT, 0);
    shade(c.box(-W / 2, top + S * 0.035f, W, S * 0.015f, FOLIAGE, Layer::FRONT, 2), 1.35f);
    int tufts = c.n(5);
    for (int i = 0; i < tufts; ++i) {
        auto& t = c.tri(c.rng.range(-W * 0.45f, W * 0.45f), top + S * 0.08f, S * 0.04f, S * 0.09f, FOLIAGE, Layer::FRONT, 4);
        c.rotated(t, c.rng.range(-20, 20));
    }
    int vines = c.n(2);
    for (int i = 0; i < vines; ++i) {
        float x = c.rng.range(-W * 0.35f, W * 0.35f);
        float len = S * c.rng.range(0.15f, 0.3f);
        shade(c.bar(x, top - S * 0.1f, x + S * 0.02f, top - S * 0.1f - len, S * 0.02f, FOLIAGE, Layer::FRONT, 2), 0.8f);
    }
    // a small tree on the island
    auto sv = c.save();
    c.at(-W * 0.18f, top + S * 0.04f, 0.5f);
    tree(c, S, false);
    c.restore(sv);
    if (c.glow) c.glowAt(0, top - S * 0.45f, W * 1.2f, S * 0.6f, 0.4f, Layer::BACK, 3);
}

void mountains(Canvas& c, float S) {
    bool lava = c.spec->theme == "lava" || c.spec->theme == "hell";
    struct Pk { float x, w, h; Layer l; int slot; float sh; };
    Pk peaks[] = {
        { -S * 0.55f, S * 1.1f, S * 0.75f, Layer::BG, MID, 0.6f },
        { S * 0.6f, S * 1.2f, S * 0.85f, Layer::BG, MID, 0.6f },
        { 0, S * 1.5f, S * 1.0f, Layer::BACK, MID, 0.9f },
    };
    for (auto const& p : peaks) {
        auto& m = c.tri(p.x, p.h / 2, p.w, p.h, p.slot, p.l, 0);
        m.shade = p.sh;
        shade(c.slope(p.x + p.w / 4, p.h / 2, p.w / 2, p.h, true, false, BASE, p.l, 1), 1.3f);
        if (!lava) {
            c.tri(p.x, p.h * 0.88f, p.w * 0.24f, p.h * 0.24f, LIGHT, p.l, 2);
        }
    }
    if (lava) {
        auto& crater = c.box(-S * 0.12f, S * 0.94f, S * 0.24f, S * 0.06f, GLOW, Layer::MAIN, 1);
        crater.shade = 1.f;
        int streams = c.n(3);
        for (int i = 0; i < streams; ++i) {
            float x = c.rng.range(-S * 0.1f, S * 0.1f);
            shade(c.bar(x, S * 0.95f, x + c.rng.range(-S * 0.3f, S * 0.3f), S * c.rng.range(0.35f, 0.6f), S * 0.03f, GLOW, Layer::MAIN, 2), 0.85f);
        }
        c.glowAt(0, S * 1.0f, S * 0.8f, S * 0.6f, 0.9f, Layer::FX, 3);
        int smoke = c.n(3);
        for (int i = 0; i < smoke; ++i)
            shade(c.circle(c.rng.range(-S * 0.15f, S * 0.15f), S * (1.1f + i * 0.12f), S * (0.14f + i * 0.04f), BASE, Layer::BG, 4), 1.5f);
    }
}

// ---------------------------------------------------------------------- light

void lamp(Canvas& c, float S) {
    float pw = S * 0.065f;
    c.box(-pw / 2, 0, pw, S * 0.86f, BASE, Layer::MAIN, 0);
    shade(c.box(-pw / 2, 0, pw * 0.3f, S * 0.86f, LIGHT, Layer::MAIN, 2), 0.45f);
    c.box(-S * 0.1f, 0, S * 0.2f, S * 0.06f, BASE, Layer::MAIN, 0);
    c.slope(-S * 0.075f, S * 0.08f, S * 0.05f, S * 0.04f, false, false, BASE, Layer::MAIN, 1);
    c.slope(S * 0.075f, S * 0.08f, S * 0.05f, S * 0.04f, true, false, BASE, Layer::MAIN, 1);
    c.bar(0, S * 0.84f, S * 0.24f, S * 0.88f, S * 0.035f, BASE, Layer::MAIN, 0);
    c.circle(0, S * 0.88f, S * 0.07f, BASE, Layer::MAIN, 2);
    float hx = S * 0.24f, hy = S * 0.85f;
    c.tri(hx, hy + S * 0.035f, S * 0.18f, S * 0.08f, BASE, Layer::FRONT, 0);
    shade(c.circle(hx, hy - S * 0.03f, S * 0.08f, LIGHT, Layer::FRONT, 0), 1.f, 0.3f);
    if (c.glow) {
        c.glowAt(hx, hy - S * 0.03f, S * 0.55f, S * 0.55f, 1.f, Layer::FX, 3);
        auto& cone = c.tri(hx, hy * 0.5f, S * 0.6f, hy * 0.95f, GLOW, Layer::BACK, 3);
        cone.shade = 0.16f;
        c.glowAt(hx, S * 0.02f, S * 0.7f, S * 0.14f, 0.5f, Layer::BACK, 3);
    }
    int moths = c.spec->detail > 1.2f ? c.n(2) : 0;
    for (int i = 0; i < moths; ++i)
        c.glowAt(hx + c.rng.range(-S * 0.12f, S * 0.12f), hy + c.rng.range(-S * 0.12f, S * 0.08f), S * 0.04f, S * 0.04f, 1.f, Layer::FX, 4);
}

void torch(Canvas& c, float S) {
    shade(c.bar(0, 0, 0, S * 0.52f, S * 0.07f, WOOD, Layer::MAIN, 0), 0.7f);
    shade(c.bar(S * 0.015f, 0, S * 0.015f, S * 0.5f, S * 0.025f, WOOD, Layer::MAIN, 1), 0.5f);
    c.box(-S * 0.1f, S * 0.48f, S * 0.2f, S * 0.07f, BASE, Layer::MAIN, 0);
    c.slope(-S * 0.075f, S * 0.45f, S * 0.05f, S * 0.05f, true, true, BASE, Layer::MAIN, 1);
    c.slope(S * 0.075f, S * 0.45f, S * 0.05f, S * 0.05f, false, true, BASE, Layer::MAIN, 1);
    shade(c.box(-S * 0.1f, S * 0.53f, S * 0.2f, S * 0.015f, LIGHT, Layer::MAIN, 2), 0.6f);
    flame(c, 0, S * 0.55f, S * 0.38f);
}

void campfire(Canvas& c, float S) {
    shade(c.bar(-S * 0.32f, S * 0.03f, S * 0.3f, S * 0.16f, S * 0.09f, WOOD, Layer::FRONT, 0), 0.65f);
    shade(c.bar(S * 0.32f, S * 0.03f, -S * 0.3f, S * 0.16f, S * 0.09f, WOOD, Layer::FRONT, 0), 0.5f);
    int stones = c.n(5);
    for (int i = 0; i < stones; ++i) {
        float x = (float(i) / std::max(1, stones - 1) - 0.5f) * S * 0.9f;
        shade(c.shape(Shape::Hexagon, x, S * 0.03f, S * 0.14f, S * 0.09f, BASE, Layer::FX, 1), 1.4f);
    }
    flame(c, 0, S * 0.1f, S * 0.55f, Layer::MAIN);
    if (c.glow) c.glowAt(0, S * 0.05f, S * 1.6f, S * 0.4f, 0.6f, Layer::BACK, 3);
}

void candles(Canvas& c, float S) {
    float hs[] = { 0.55f, 0.8f, 0.4f };
    float xs[] = { -0.18f, 0, 0.17f };
    for (int i = 0; i < 3; ++i) {
        float h = S * hs[i], x = S * xs[i];
        shade(c.box(x - S * 0.055f, 0, S * 0.11f, h, LIGHT, Layer::MAIN, 0), 0.9f, 0.4f);
        shade(c.box(x + S * 0.015f, 0, S * 0.04f, h, LIGHT, Layer::MAIN, 1), 0.72f, 0.4f);
        shade(c.circle(x - S * 0.03f, h - S * 0.04f, S * 0.04f, LIGHT, Layer::FRONT, 2), 1.f, 0.3f);
        c.box(x - S * 0.006f, h, S * 0.012f, S * 0.04f, BASE, Layer::MAIN, 2);
        flame(c, x, h + S * 0.03f, S * 0.14f);
    }
}

void lantern(Canvas& c, float S) {
    float cy = S * 0.48f;
    c.bar(0, cy + S * 0.3f, 0, S * 1.0f, S * 0.02f, BASE, Layer::MAIN, 0);
    c.box(-S * 0.13f, cy + S * 0.26f, S * 0.26f, S * 0.06f, BASE, Layer::MAIN, 0);
    c.box(-S * 0.13f, cy - S * 0.32f, S * 0.26f, S * 0.06f, BASE, Layer::MAIN, 0);
    auto& body = c.ellipse(0, cy, S * 0.5f, S * 0.58f, ACCENT, Layer::MAIN, 0);
    body.shade = 0.95f;
    shade(c.ellipse(0, cy, S * 0.3f, S * 0.5f, GLOW, Layer::FRONT, 1), 0.65f);
    for (int i = -1; i <= 1; ++i)
        shade(c.box(i * S * 0.12f - S * 0.007f, cy - S * 0.26f, S * 0.014f, S * 0.52f, ACCENT, Layer::FRONT, 2), 0.55f);
    shade(c.bar(0, cy - S * 0.32f, 0, cy - S * 0.48f, S * 0.025f, ACCENT, Layer::MAIN, 2), 0.8f);
    if (c.glow) c.glowAt(0, cy, S * 1.4f, S * 1.4f, 0.75f, Layer::BACK, 3);
}

// ---------------------------------------------------------------- buildings

void tower(Canvas& c, float S, bool roofed) {
    bool broken = c.spec->broken;
    bool lit = c.spec->litWindows || c.spec->night || c.spec->glow == 1;
    float W = S * 0.4f, H = S * 0.78f;
    c.box(-W / 2, 0, W, H, MID, Layer::MAIN, 0);
    shade(c.box(W * 0.12f, 0, W * 0.38f, H, MID, Layer::MAIN, 1), 0.72f);
    shade(c.box(-W / 2, 0, W * 0.07f, H, LIGHT, Layer::MAIN, 2), 0.7f);
    // stone courses
    int bricks = c.n(9);
    for (int i = 0; i < bricks; ++i) {
        float bx = c.rng.range(-W * 0.38f, W * 0.3f), by = c.rng.range(S * 0.05f, H * 0.95f);
        shade(c.box(bx, by, W * c.rng.range(0.14f, 0.24f), S * 0.03f, i % 2 ? BASE : LIGHT, Layer::MAIN, 4), i % 2 ? 1.6f : 0.62f);
    }
    // battlements
    float bandH = S * 0.07f;
    shade(c.box(-W * 0.6f, H, W * 1.2f, bandH, MID, Layer::MAIN, 0), 1.06f);
    shade(c.box(-W * 0.6f, H - S * 0.02f, W * 1.2f, S * 0.02f, BASE, Layer::MAIN, 1), 1.3f);
    if (!roofed || broken) {
        int m = 4;
        float mw = W * 1.2f / (2 * m - 1);
        for (int i = 0; i < m; ++i) {
            float mh = broken ? S * c.rng.range(0.02f, 0.09f) : S * 0.08f;
            c.box(-W * 0.6f + i * 2 * mw, H + bandH, mw, mh, MID, Layer::MAIN, 0).shade = 1.06f;
        }
    }
    else {
        float rh = S * 0.42f;
        c.tri(0, H + bandH + rh / 2, W * 1.4f, rh, ACCENT, Layer::MAIN, 0).shade = 0.85f;
        shade(c.slope(W * 0.35f, H + bandH + rh / 2, W * 0.7f, rh, true, false, ACCENT, Layer::MAIN, 1), 0.6f);
        c.bar(0, H + bandH + rh, 0, H + bandH + rh + S * 0.14f, S * 0.015f, BASE, Layer::MAIN, 2);
        auto& flag = c.box(0, H + bandH + rh + S * 0.08f, S * 0.12f, S * 0.06f, ACCENT, Layer::MAIN, 2);
        flag.hue = 180; flag.shade = 1.15f;
    }
    // windows
    int rows = c.detail > 1.3f ? 2 : 1;
    for (int r = 0; r < rows; ++r) {
        float wy = H * (0.45f + 0.28f * r);
        windowArch(c, 0, wy, W * 0.22f, S * 0.14f, lit, Layer::FRONT);
    }
    // door
    c.box(-W * 0.17f, 0, W * 0.34f, S * 0.16f, BASE, Layer::FRONT, 0);
    c.circle(0, S * 0.16f, W * 0.34f, BASE, Layer::FRONT, 0);
    shade(c.box(-W * 0.17f, 0, W * 0.05f, S * 0.16f, WOOD, Layer::FRONT, 2), 0.8f);
    if (c.spec->theme == "forest" || c.spec->has(Surf::Vines) || c.spec->has(Surf::Moss)) {
        int v = c.n(3);
        for (int i = 0; i < v; ++i) {
            float x = c.rng.range(-W * 0.45f, W * 0.45f);
            float len = H * c.rng.range(0.2f, 0.5f);
            shade(c.bar(x, H, x + c.rng.range(-S * 0.03f, S * 0.03f), H - len, S * 0.025f, FOLIAGE, Layer::FRONT, 2), 0.9f);
        }
    }
    if (broken) {
        int cracks = c.n(2);
        for (int i = 0; i < cracks; ++i) {
            float x = c.rng.range(-W * 0.3f, W * 0.3f), y = c.rng.range(H * 0.2f, H * 0.8f);
            c.bar(x, y, x + c.rng.range(-S * 0.06f, S * 0.06f), y + S * 0.1f, S * 0.012f, BASE, Layer::FRONT, 2);
        }
    }
}

void castle(Canvas& c, float S) {
    bool roofed = c.spec->roof || (c.spec->theme == "candy" || c.spec->theme == "crystal" || c.spec->theme == "sakura");
    // side towers first (behind)
    for (int side = -1; side <= 1; side += 2) {
        auto sv = c.save();
        c.at(side * S * 0.48f, 0, 0.72f);
        c.layerOffset -= 1;
        tower(c, S, roofed);
        c.restore(sv);
    }
    // wall
    float wallH = S * 0.42f;
    shade(c.box(-S * 0.48f, 0, S * 0.96f, wallH, MID, Layer::BACK, 0), 0.82f);
    shade(c.box(-S * 0.48f, wallH - S * 0.025f, S * 0.96f, S * 0.025f, LIGHT, Layer::BACK, 2), 0.6f);
    int m = 7;
    float mw = S * 0.96f / (2 * m - 1);
    for (int i = 0; i < m; ++i)
        shade(c.box(-S * 0.48f + i * 2 * mw, wallH, mw, S * 0.06f, MID, Layer::BACK, 1), 0.82f);
    int bricks = c.n(8);
    for (int i = 0; i < bricks; ++i)
        shade(c.box(c.rng.range(-S * 0.44f, S * 0.36f), c.rng.range(S * 0.04f, wallH * 0.9f), S * 0.07f, S * 0.025f, BASE, Layer::BACK, 4), 1.5f);
    // keep
    auto sv = c.save();
    c.at(0, 0, 1.05f);
    tower(c, S, roofed);
    c.restore(sv);
    // gate
    c.box(-S * 0.09f, 0, S * 0.18f, S * 0.16f, BASE, Layer::FRONT, 0);
    c.circle(0, S * 0.16f, S * 0.18f, BASE, Layer::FRONT, 0);
    for (int i = -1; i <= 1; ++i)
        shade(c.box(i * S * 0.045f - S * 0.006f, 0, S * 0.012f, S * 0.22f, LIGHT, Layer::FRONT, 2), 0.5f);
    if (c.glow) c.glowAt(0, S * 0.1f, S * 0.6f, S * 0.3f, 0.35f, Layer::FX, 3);
}

void house(Canvas& c, float S) {
    bool lit = c.spec->litWindows || c.spec->night || c.spec->glow == 1 || c.spec->theme == "cozy";
    float W = S * 0.78f, H = S * 0.46f;
    // chimney behind
    shade(c.box(W * 0.2f, H + S * 0.1f, S * 0.1f, S * 0.28f, ACCENT, Layer::BACK, 1), 0.55f);
    c.box(-W / 2, 0, W, H, MID, Layer::MAIN, 0);
    shade(c.box(W * 0.2f, 0, W * 0.3f, H, MID, Layer::MAIN, 1), 0.78f);
    int planks = c.n(4);
    for (int i = 1; i <= planks; ++i)
        shade(c.box(-W / 2, H * i / (planks + 1), W, S * 0.008f, BASE, Layer::MAIN, 2), 1.4f);
    float rh = S * 0.4f;
    c.tri(0, H + rh / 2, W * 1.28f, rh, ACCENT, Layer::MAIN, 0);
    shade(c.slope(W * 0.32f, H + rh / 2, W * 0.64f, rh, true, false, ACCENT, Layer::MAIN, 1), 0.7f);
    shade(c.box(-W * 0.66f, H - S * 0.01f, W * 1.32f, S * 0.035f, BASE, Layer::MAIN, 1), 1.f);
    // door
    shade(c.box(-W * 0.1f, 0, W * 0.2f, H * 0.62f, WOOD, Layer::FRONT, 0), 0.8f);
    shade(c.circle(W * 0.05f, H * 0.3f, S * 0.025f, LIGHT, Layer::FRONT, 2), 1.f);
    // windows
    for (int side = -1; side <= 1; side += 2) {
        float wx = side * W * 0.3f, wy = H * 0.5f, ws = S * 0.12f;
        auto& win = c.rect(wx, wy, ws, ws, lit ? GLOW : BASE, Layer::FRONT, 0);
        win.shade = lit ? 0.85f : 1.2f;
        c.rect(wx, wy, ws, S * 0.012f, BASE, Layer::FRONT, 2);
        c.rect(wx, wy, S * 0.012f, ws, BASE, Layer::FRONT, 2);
        shade(c.rect(wx, wy - ws * 0.58f, ws * 1.25f, S * 0.02f, LIGHT, Layer::FRONT, 2), 0.7f);
        if (lit) c.glowAt(wx, wy, ws * 3.5f, ws * 3.f, 0.5f, Layer::FX, 3);
    }
    int smoke = c.n(3);
    for (int i = 0; i < smoke; ++i)
        shade(c.circle(W * 0.25f + i * S * 0.04f, H + S * 0.5f + i * S * 0.07f, S * (0.05f + 0.015f * i), LIGHT, Layer::BACK, 4), 0.55f, 0.2f);
}

void city(Canvas& c, float S) {
    bool lit = true;
    int n = std::max(4, c.n(5) + 1);
    float W = S * 1.8f;
    float x = -W / 2;
    // back row
    for (int i = 0; i < n + 1; ++i) {
        float bw = W / (n + 1) * c.rng.range(0.8f, 1.2f);
        float bh = S * c.rng.range(0.5f, 1.0f);
        shade(c.box(x, 0, bw, bh, BASE, Layer::BG, 0), 1.6f);
        x += bw * 0.9f;
    }
    x = -W / 2 + S * 0.05f;
    for (int i = 0; i < n; ++i) {
        float bw = W / n * c.rng.range(0.6f, 0.85f);
        float bh = S * c.rng.range(0.35f, 0.8f);
        c.box(x, 0, bw, bh, BASE, Layer::MAIN, 0);
        shade(c.box(x, 0, bw * 0.08f, bh, MID, Layer::MAIN, 2), 1.f);
        if (c.rng.chance(0.5f)) {
            c.box(x + bw * 0.45f, bh, bw * 0.06f, S * 0.12f, BASE, Layer::MAIN, 2);
            if (c.glow) c.glowAt(x + bw * 0.48f, bh + S * 0.12f, S * 0.06f, S * 0.06f, 1.f, Layer::FX, 4);
        }
        if (lit) {
            int cols = std::max(1, int(bw / (S * 0.07f)));
            int rowsN = std::max(1, int(bh / (S * 0.09f)) - 1);
            int budget = c.n(5);
            for (int k = 0; k < budget; ++k) {
                int cx = c.rng.irange(0, cols - 1), cy = c.rng.irange(0, rowsN - 1);
                float wx = x + (cx + 0.5f) * bw / cols;
                float wy = S * 0.06f + cy * S * 0.09f;
                auto& w = c.rect(wx, wy, S * 0.035f, S * 0.045f, GLOW, Layer::FRONT, k < 2 ? 2 : 4);
                w.shade = c.rng.range(0.55f, 1.f);
                if (k % 2) w.hue = 30;
            }
        }
        x += W / n;
    }
    if (c.glow) c.glowAt(0, S * 0.1f, W * 1.1f, S * 0.6f, 0.3f, Layer::BG, 3);
}

void pillar(Canvas& c, float S) {
    bool broken = c.spec->broken;
    float W = S * 0.22f, H = broken ? S * 0.62f : S * 0.82f;
    c.box(-W / 2, S * 0.06f, W, H - S * 0.06f, MID, Layer::MAIN, 0);
    shade(c.box(W * 0.1f, S * 0.06f, W * 0.4f, H - S * 0.06f, MID, Layer::MAIN, 1), 0.72f);
    for (int i = -1; i <= 1; ++i)
        shade(c.box(i * W * 0.25f - S * 0.006f, S * 0.08f, S * 0.012f, H - S * 0.1f, BASE, Layer::FRONT, 2), 1.2f);
    shade(c.box(-W * 0.72f, 0, W * 1.44f, S * 0.06f, LIGHT, Layer::MAIN, 0), 0.85f);
    shade(c.box(-W * 0.6f, S * 0.06f, W * 1.2f, S * 0.03f, MID, Layer::MAIN, 1), 1.1f);
    if (!broken) {
        shade(c.box(-W * 0.6f, H - S * 0.03f, W * 1.2f, S * 0.03f, MID, Layer::MAIN, 1), 1.1f);
        shade(c.box(-W * 0.75f, H, W * 1.5f, S * 0.065f, LIGHT, Layer::MAIN, 0), 0.85f);
        shade(c.box(-W * 0.75f, H + S * 0.05f, W * 1.5f, S * 0.015f, LIGHT, Layer::MAIN, 2), 1.f);
    }
    else {
        c.slope(-W * 0.25f, H + S * 0.04f, W * 0.5f, S * 0.08f, false, false, MID, Layer::MAIN, 1);
        c.slope(W * 0.25f, H + S * 0.02f, W * 0.5f, S * 0.04f, true, false, MID, Layer::MAIN, 1).shade = 0.72f;
        auto& chunk = c.rect(W * 1.2f, S * 0.05f, W * 0.6f, W * 0.45f, MID, Layer::MAIN, 2);
        c.rotated(chunk, 18);
    }
    if (c.spec->theme == "forest" || c.spec->has(Surf::Vines) || c.spec->has(Surf::Moss)) {
        int v = c.n(4);
        for (int i = 0; i < v; ++i) {
            float y = H * (0.2f + 0.18f * i);
            auto& leaf = c.ellipse(c.rng.range(-W * 0.4f, W * 0.4f), y, S * 0.09f, S * 0.04f, FOLIAGE, Layer::FRONT, 2);
            c.rotated(leaf, c.rng.range(-40, 40));
        }
    }
}

void arch(Canvas& c, float S, bool portal) {
    float px = S * 0.36f, ph = S * 0.62f, pw = S * 0.16f;
    for (int side = -1; side <= 1; side += 2) {
        c.box(side * px - pw / 2, 0, pw, ph, MID, Layer::MAIN, 0);
        shade(c.box(side * px + pw * 0.1f, 0, pw * 0.4f, ph, MID, Layer::MAIN, 1), 0.72f);
        shade(c.box(side * px - pw * 0.7f, 0, pw * 1.4f, S * 0.05f, LIGHT, Layer::MAIN, 1), 0.8f);
    }
    int n = std::max(7, c.n(9)) | 1;
    float R = px;
    for (int i = 0; i < n; ++i) {
        float a = PI * (i + 0.5f) / n;
        float x = std::cos(a) * R, y = ph + std::sin(a) * R;
        auto& v = c.rect(x, y, pw, PI * R / n * 1.08f, MID, Layer::MAIN, 0);
        v.shade = (i == n / 2) ? 1.12f : (i % 2 ? 0.88f : 1.f);
        v.rot = -deg(a);
        if (i == n / 2) {
            auto& key = c.rect(x, y, pw * 1.25f, PI * R / n * 1.1f, LIGHT, Layer::FRONT, 2);
            key.shade = 0.85f;
            key.rot = -deg(a);
        }
    }
    if (portal || c.spec->glow == 1 || c.spec->theme == "crystal" || c.spec->theme == "space") {
        float iw = (px - pw / 2) * 2;
        shade(c.box(-iw / 2, 0, iw, ph, GLOW, Layer::BACK, 1), 0.35f);
        shade(c.ellipse(0, ph, iw, (R - pw / 2) * 2, GLOW, Layer::BACK, 1), 0.35f);
        c.glowAt(0, ph * 0.8f, iw * 1.4f, ph * 1.6f, 0.7f, Layer::BACK, 3);
        int sp = c.n(4);
        for (int i = 0; i < sp; ++i) {
            auto& s = c.shape(Shape::Star, c.rng.range(-iw * 0.4f, iw * 0.4f), c.rng.range(S * 0.1f, ph + R * 0.5f), S * 0.05f, S * 0.05f, GLOW, Layer::FX, 4);
            s.sat = 0.4f;
        }
    }
}

void portal(Canvas& c, float S) {
    float cy = S * 0.5f;
    float w = S * 0.62f, h = S * 0.92f;
    c.glowAt(0, cy, w * 2.f, h * 1.6f, 0.8f, Layer::BACK, 3);
    c.ellipse(0, cy, w * 1.12f, h * 1.08f, BASE, Layer::MAIN, 0);
    c.ellipse(0, cy, w, h, MID, Layer::MAIN, 0);
    int swirl = std::max(2, c.n(3));
    for (int i = 0; i < swirl; ++i) {
        float f = 1.f - (i + 1) * 0.8f / (swirl + 1);
        auto& r = c.ring(0, cy, w * f, h * f, i % 2 ? LIGHT : GLOW, Layer::FRONT, i == 0 ? 0 : 1);
        r.shade = 0.7f + 0.3f * f;
        c.rotated(r, i * 12.f);
    }
    shade(c.ellipse(0, cy, w * 0.25f, h * 0.25f, GLOW, Layer::FRONT, 1), 1.f, 0.3f);
    c.ring(0, cy, w * 1.12f, h * 1.08f, LIGHT, Layer::FRONT, 2).shade = 0.6f;
    int stones = c.n(4);
    for (int i = 0; i < stones; ++i) {
        float a = PI * 0.5f + (float(i) / std::max(1, stones - 1) - 0.5f) * PI * 1.6f;
        auto& s = c.shape(Shape::Diamond, std::cos(a) * w * 0.6f, cy + std::sin(a) * h * 0.58f, S * 0.08f, S * 0.12f, ACCENT, Layer::FRONT, 2);
        c.rotated(s, -deg(a) + 90);
    }
    int sp = c.n(5);
    for (int i = 0; i < sp; ++i) {
        float a = c.rng.range(0, 2 * PI), rr = c.rng.range(0.55f, 0.8f);
        auto& st = c.shape(Shape::Star, std::cos(a) * w * rr, cy + std::sin(a) * h * rr, S * 0.045f, S * 0.045f, GLOW, Layer::FX, 4);
        st.sat = 0.3f;
    }
}

void pyramid(Canvas& c, float S) {
    float W = S * 1.4f, H = S * 0.72f;
    c.tri(0, H / 2, W, H, MID, Layer::MAIN, 0);
    shade(c.slope(W / 4, H / 2, W / 2, H, true, false, MID, Layer::MAIN, 1), 0.7f);
    int steps = c.n(5);
    for (int i = 1; i <= steps; ++i) {
        float y = H * i / (steps + 1);
        float w = W * (1.f - y / H);
        shade(c.rect(0, y, w, S * 0.012f, BASE, Layer::FRONT, 2), 1.3f);
    }
    c.box(-S * 0.06f, 0, S * 0.12f, S * 0.14f, BASE, Layer::FRONT, 0);
    c.tri(0, H - S * 0.06f, S * 0.17f, S * 0.12f, LIGHT, Layer::FRONT, 2);
    if (c.glow) c.glowAt(0, H - S * 0.05f, S * 0.4f, S * 0.4f, 0.8f, Layer::FX, 3);
}

void fence(Canvas& c, float S) {
    float W = S * 1.6f;
    int posts = std::max(3, c.n(5));
    shade(c.box(-W / 2, S * 0.14f, W, S * 0.06f, WOOD, Layer::MAIN, 0), 0.62f);
    shade(c.box(-W / 2, S * 0.34f, W, S * 0.06f, WOOD, Layer::MAIN, 0), 0.62f);
    for (int i = 0; i < posts; ++i) {
        float x = (float(i) / (posts - 1) - 0.5f) * (W - S * 0.1f);
        shade(c.box(x - S * 0.04f, 0, S * 0.08f, S * 0.45f, WOOD, Layer::FRONT, 0), 0.85f);
        shade(c.tri(x, S * 0.49f, S * 0.08f, S * 0.08f, WOOD, Layer::FRONT, 0), 0.85f);
        shade(c.box(x + S * 0.005f, 0, S * 0.035f, S * 0.45f, WOOD, Layer::FRONT, 1), 0.6f);
    }
}

void sign(Canvas& c, float S) {
    shade(c.box(-S * 0.035f, 0, S * 0.07f, S * 0.6f, WOOD, Layer::MAIN, 0), 0.55f);
    float by = S * 0.6f;
    shade(c.rect(0, by, S * 0.7f, S * 0.3f, WOOD, Layer::MAIN, 0), 0.85f);
    shade(c.rect(0, by + S * 0.14f, S * 0.7f, S * 0.02f, WOOD, Layer::MAIN, 2), 0.55f);
    shade(c.rect(0, by - S * 0.14f, S * 0.7f, S * 0.02f, WOOD, Layer::MAIN, 2), 0.55f);
    c.rect(-S * 0.05f, by, S * 0.3f, S * 0.07f, LIGHT, Layer::FRONT, 0);
    auto& head = c.tri(S * 0.15f, by, S * 0.16f, S * 0.14f, LIGHT, Layer::FRONT, 0);
    c.rotated(head, 90);
    for (int i = -1; i <= 1; i += 2)
        c.circle(i * S * 0.3f, by, S * 0.025f, BASE, Layer::FRONT, 4);
}

// -------------------------------------------------------------------- sky

void cloud(Canvas& c, float S) {
    struct B { float x, y, d; };
    B bs[] = { { -0.48f, 0.22f, 0.42f }, { -0.2f, 0.36f, 0.6f }, { 0.14f, 0.42f, 0.66f }, { 0.44f, 0.26f, 0.46f }, { 0.0f, 0.2f, 0.5f } };
    shade(c.box(-S * 0.62f, S * 0.02f, S * 1.24f, S * 0.2f, LIGHT, Layer::MAIN, 1), 0.8f, 0.6f);
    for (auto const& b : bs) shade(c.circle(b.x * S, b.y * S - S * 0.035f, b.d * S, LIGHT, Layer::MAIN, 1), 0.8f, 0.6f);
    shade(c.box(-S * 0.6f, S * 0.06f, S * 1.2f, S * 0.2f, LIGHT, Layer::MAIN, 0), 1.f);
    for (auto const& b : bs) shade(c.circle(b.x * S, b.y * S, b.d * S * 0.95f, LIGHT, Layer::MAIN, 0), 1.f);
    int hl = c.n(2);
    for (int i = 0; i < hl; ++i) {
        auto const& b = bs[1 + i % 2];
        shade(c.circle(b.x * S - S * 0.08f, b.y * S + S * 0.1f, b.d * S * 0.35f, LIGHT, Layer::FRONT, 2), 1.15f, 0.3f);
    }
}

void sun(Canvas& c, float S) {
    float cy = S * 0.5f, d = S * 0.48f;
    c.glowAt(0, cy, S * 1.8f, S * 1.8f, 0.85f, Layer::BACK, 3);
    int rays = std::max(8, c.n(10));
    for (int i = 0; i < rays; ++i) {
        float a = float(i) / rays * 2 * PI;
        float r0 = d * 0.62f, r1 = d * (i % 2 ? 0.9f : 1.05f);
        auto& ray = c.bar(std::cos(a) * r0, cy + std::sin(a) * r0, std::cos(a) * r1, cy + std::sin(a) * r1, S * 0.035f, LIGHT, Layer::MAIN, 2);
        ray.shade = 1.f;
    }
    c.circle(0, cy, d, LIGHT, Layer::MAIN, 0);
    shade(c.circle(0, cy, d * 0.82f, GLOW, Layer::FRONT, 1), 0.7f);
    shade(c.circle(-d * 0.15f, cy + d * 0.15f, d * 0.3f, LIGHT, Layer::FRONT, 2), 1.2f, 0.3f);
}

void moon(Canvas& c, float S, bool crescent) {
    float cy = S * 0.5f, d = S * 0.5f;
    c.glowAt(0, cy, S * 1.5f, S * 1.5f, 0.55f, Layer::BACK, 3);
    shade(c.circle(0, cy, d, LIGHT, Layer::MAIN, 0), 0.95f, 0.5f);
    if (crescent) {
        c.circle(d * 0.28f, cy + d * 0.12f, d * 0.88f, BASE, Layer::FRONT, 0);
    }
    else {
        int cr = c.n(4);
        for (int i = 0; i < cr; ++i) {
            float a = c.rng.range(0, 2 * PI), rr = c.rng.range(0, d * 0.3f);
            shade(c.circle(std::cos(a) * rr, cy + std::sin(a) * rr, d * c.rng.range(0.1f, 0.22f), LIGHT, Layer::FRONT, 2), 0.78f, 0.5f);
        }
        shade(c.circle(d * 0.06f, cy - d * 0.04f, d * 0.92f, LIGHT, Layer::MAIN, 1), 0.86f, 0.5f);
        shade(c.circle(-d * 0.05f, cy + d * 0.04f, d * 0.86f, LIGHT, Layer::MAIN, 1), 0.98f, 0.5f);
    }
    int stars = c.n(3);
    for (int i = 0; i < stars; ++i) {
        auto& s = c.shape(Shape::Star, c.rng.range(-S * 0.7f, S * 0.7f), c.rng.range(S * 0.1f, S * 1.f), S * 0.05f, S * 0.05f, LIGHT, Layer::FX, 4);
        s.sat = 0.4f;
    }
}

void planet(Canvas& c, float S) {
    float cy = S * 0.5f, d = S * 0.55f;
    c.glowAt(0, cy, S * 1.5f, S * 1.5f, 0.55f, Layer::BACK, 3);
    c.circle(0, cy, d, BASE, Layer::MAIN, 0);
    c.circle(-d * 0.06f, cy + d * 0.06f, d * 0.86f, MID, Layer::MAIN, 0);
    int bands = c.n(2);
    for (int i = 0; i < bands; ++i) {
        float y = cy + (i - bands / 2.f + 0.5f) * d * 0.22f;
        shade(c.ellipse(-d * 0.06f, y, d * 0.78f, d * 0.08f, MID, Layer::MAIN, 2), 1.18f, 0.8f);
    }
    shade(c.circle(-d * 0.22f, cy + d * 0.22f, d * 0.22f, LIGHT, Layer::MAIN, 2), 0.85f, 0.6f);
    auto& ring = c.ring(0, cy, d * 1.9f, d * 0.5f, ACCENT, Layer::FRONT, 1);
    c.rotated(ring, -14);
    auto& ring2 = c.ring(0, cy, d * 1.7f, d * 0.42f, LIGHT, Layer::FRONT, 2);
    ring2.shade = 0.7f;
    c.rotated(ring2, -14);
    int moons = c.n(2);
    for (int i = 0; i < moons; ++i)
        shade(c.circle(c.rng.range(-S * 0.75f, S * 0.75f), c.rng.range(0, S * 1.0f), S * c.rng.range(0.05f, 0.09f), LIGHT, Layer::BACK, 4), 0.8f);
}

void stars(Canvas& c, float S) {
    int n = std::max(6, c.n(12));
    float W = S * 1.8f, H = S;
    for (int i = 0; i < n; ++i) {
        float x = c.rng.range(-W / 2, W / 2), y = c.rng.range(0, H);
        float s = S * (c.rng.chance(0.25f) ? c.rng.range(0.07f, 0.11f) : c.rng.range(0.025f, 0.05f));
        bool big = s > S * 0.06f;
        if (big) {
            auto& a = c.shape(Shape::Diamond, x, y, s * 0.35f, s * 1.6f, LIGHT, Layer::MAIN, 0);
            a.sat = 0.4f;
            auto& b = c.shape(Shape::Diamond, x, y, s * 1.6f, s * 0.35f, LIGHT, Layer::MAIN, 0);
            b.sat = 0.4f;
            c.glowAt(x, y, s * 3.f, s * 3.f, 0.8f, Layer::BACK, 3);
        }
        else {
            auto& st = c.shape(Shape::Star, x, y, s, s, i % 3 ? LIGHT : GLOW, Layer::MAIN, i < 4 ? 0 : 4);
            st.sat = 0.5f;
        }
    }
}

// ------------------------------------------------------------------ objects

void gear(Canvas& c, float S) {
    float cy = S * 0.5f, D = S * 0.7f;
    int teeth = c.detail > 1.3f ? 12 : 10;
    for (int i = 0; i < teeth; ++i) {
        float a = float(i) / teeth * 2 * PI;
        auto& t = c.rect(std::cos(a) * D * 0.5f, cy + std::sin(a) * D * 0.5f, D * 0.17f, D * 0.17f, MID, Layer::MAIN, 0);
        t.rot = -deg(a);
        t.shade = 0.9f;
    }
    c.circle(0, cy, D * 0.92f, MID, Layer::MAIN, 0);
    shade(c.circle(0, cy, D * 0.7f, MID, Layer::FRONT, 1), 0.78f);
    shade(c.circle(0, cy, D * 0.3f, LIGHT, Layer::FRONT, 0), 0.8f);
    c.circle(0, cy, D * 0.12f, BASE, Layer::FRONT, 1);
    for (int i = 0; i < 4; ++i) {
        float a = i * PI / 2 + PI / 4;
        c.circle(std::cos(a) * D * 0.25f, cy + std::sin(a) * D * 0.25f, D * 0.12f, BASE, Layer::FRONT, 2);
    }
    shade(c.ring(0, cy, D * 0.92f, D * 0.92f, LIGHT, Layer::FRONT, 2), 0.55f);
    if (c.glow && (c.spec->glow == 1 || c.spec->theme == "neon")) c.glowAt(0, cy, D * 1.8f, D * 1.8f, 0.4f, Layer::BACK, 3);
}

void chain(Canvas& c, float S) {
    int links = std::max(4, c.n(6) + 2);
    float lh = S / (links + 0.5f);
    for (int i = 0; i < links; ++i) {
        float y = S - (i + 0.5f) * lh;
        if (i % 2 == 0) {
            c.ring(0, y, lh * 0.55f, lh * 1.2f, MID, Layer::MAIN, 0);
            shade(c.ring(-lh * 0.04f, y + lh * 0.04f, lh * 0.45f, lh * 1.05f, LIGHT, Layer::MAIN, 2), 0.7f);
        }
        else {
            shade(c.rect(0, y, lh * 0.18f, lh * 1.2f, MID, Layer::FRONT, 0), 0.82f);
        }
    }
    c.box(-S * 0.08f, S - S * 0.02f, S * 0.16f, S * 0.04f, BASE, Layer::MAIN, 1);
}

void spikes(Canvas& c, float S) {
    int n = std::max(3, c.n(5));
    float W = S * 1.4f;
    c.box(-W / 2, 0, W, S * 0.08f, BASE, Layer::MAIN, 0);
    for (int i = 0; i < n; ++i) {
        float x = (float(i) / (n - 1) - 0.5f) * (W - S * 0.2f);
        float h = S * (i % 2 ? c.rng.range(0.45f, 0.6f) : c.rng.range(0.7f, 0.95f));
        float w = S * 0.2f;
        auto& sp = c.tri(x, h / 2 + S * 0.04f, w, h, BASE, Layer::MAIN, 0);
        c.rotated(sp, c.rng.range(-8, 8));
        auto& in = c.tri(x - w * 0.08f, h * 0.42f + S * 0.04f, w * 0.38f, h * 0.7f, MID, Layer::FRONT, 2);
        in.rot = sp.rot;
        if (c.glow) c.glowAt(x, h + S * 0.02f, S * 0.18f, S * 0.18f, 0.9f, Layer::FX, 3);
    }
    if (c.glow) c.glowAt(0, S * 0.1f, W * 1.2f, S * 0.5f, 0.4f, Layer::BACK, 3);
}

void waterfall(Canvas& c, float S) {
    float W = S * 0.6f, H = S;
    c.box(-W / 2, 0, W, H, MID, Layer::MAIN, 0);
    shade(c.box(-W / 2, 0, W * 0.12f, H, MID, Layer::MAIN, 1), 0.7f);
    shade(c.box(W * 0.38f, 0, W * 0.12f, H, MID, Layer::MAIN, 1), 0.7f);
    int streaks = c.n(7);
    for (int i = 0; i < streaks; ++i) {
        float x = c.rng.range(-W * 0.4f, W * 0.4f);
        float len = H * c.rng.range(0.2f, 0.55f), y = c.rng.range(len / 2, H - len / 2);
        auto& s = c.rect(x, y, S * 0.018f, len, LIGHT, Layer::FRONT, i < 3 ? 1 : 4);
        s.shade = c.rng.range(0.75f, 1.1f);
    }
    shade(c.box(-W * 0.62f, H - S * 0.05f, W * 1.24f, S * 0.07f, BASE, Layer::FRONT, 0), 1.3f);
    int foam = c.n(5);
    for (int i = 0; i < foam; ++i) {
        float x = (float(i) / std::max(1, foam - 1) - 0.5f) * W * 1.1f;
        shade(c.circle(x, S * 0.03f, S * c.rng.range(0.1f, 0.16f), LIGHT, Layer::FRONT, 1), 1.f, 0.5f);
    }
    if (c.glow) c.glowAt(0, S * 0.05f, W * 2.2f, S * 0.35f, 0.5f, Layer::FX, 3);
}

void waves(Canvas& c, float S) {
    float W = S * 1.8f, h = S * 0.35f;
    c.box(-W / 2, 0, W, h, MID, Layer::MAIN, 0);
    shade(c.box(-W / 2, 0, W, h * 0.4f, MID, Layer::MAIN, 1), 0.75f);
    int n = std::max(4, c.n(6));
    for (int i = 0; i < n; ++i) {
        float x = (float(i) / (n - 1) - 0.5f) * W * 0.95f;
        c.circle(x, h, W / n * 1.2f, MID, Layer::MAIN, 0);
        shade(c.circle(x - W / n * 0.18f, h + W / n * 0.18f, W / n * 0.55f, LIGHT, Layer::FRONT, 2), 0.9f, 0.6f);
    }
    int bub = c.n(4);
    for (int i = 0; i < bub; ++i)
        shade(c.ring(c.rng.range(-W * 0.45f, W * 0.45f), c.rng.range(h * 0.2f, h * 0.9f), S * 0.06f, S * 0.06f, LIGHT, Layer::FRONT, 4), 0.85f);
}

void orb(Canvas& c, float S) {
    float cy = S * 0.5f, d = S * 0.42f;
    c.glowAt(0, cy, S * 1.6f, S * 1.6f, 0.9f, Layer::BACK, 3);
    shade(c.circle(0, cy, d, MID, Layer::MAIN, 0), 1.f);
    shade(c.circle(0, cy, d * 0.7f, GLOW, Layer::MAIN, 0), 0.8f);
    shade(c.circle(-d * 0.12f, cy + d * 0.12f, d * 0.3f, LIGHT, Layer::FRONT, 1), 1.2f, 0.2f);
    auto& r1 = c.ring(0, cy, d * 1.7f, d * 0.55f, ACCENT, Layer::FRONT, 1);
    c.rotated(r1, 28);
    auto& r2 = c.ring(0, cy, d * 1.7f, d * 0.55f, LIGHT, Layer::FRONT, 2);
    r2.shade = 0.7f;
    c.rotated(r2, -28);
    int sp = c.n(5);
    for (int i = 0; i < sp; ++i) {
        float a = c.rng.range(0, 2 * PI), rr = d * c.rng.range(0.9f, 1.3f);
        auto& s = c.shape(Shape::Diamond, std::cos(a) * rr, cy + std::sin(a) * rr, S * 0.03f, S * 0.07f, GLOW, Layer::FX, 4);
        c.rotated(s, -deg(a) + 90);
    }
}

void heart(Canvas& c, float S) {
    float cy = S * 0.5f;
    c.glowAt(0, cy, S * 1.5f, S * 1.4f, 0.6f, Layer::BACK, 3);
    shade(c.shape(Shape::Heart, S * 0.02f, cy - S * 0.02f, S * 0.86f, S * 0.8f, BASE, Layer::MAIN, 1), 1.f);
    c.shape(Shape::Heart, 0, cy, S * 0.8f, S * 0.74f, MID, Layer::MAIN, 0);
    shade(c.shape(Shape::Heart, -S * 0.03f, cy + S * 0.03f, S * 0.58f, S * 0.52f, MID, Layer::FRONT, 2), 1.18f, 0.85f);
    auto& hl = c.ellipse(-S * 0.17f, cy + S * 0.14f, S * 0.14f, S * 0.09f, LIGHT, Layer::FRONT, 2);
    hl.shade = 1.2f; hl.sat = 0.3f;
    c.rotated(hl, -35);
    int sp = c.n(3);
    for (int i = 0; i < sp; ++i) {
        auto& s = c.shape(Shape::Star, c.rng.range(-S * 0.55f, S * 0.55f), c.rng.range(S * 0.1f, S * 0.95f), S * 0.06f, S * 0.06f, LIGHT, Layer::FX, 4);
        s.sat = 0.4f;
    }
}

void ghost(Canvas& c, float S) {
    c.glowAt(0, S * 0.5f, S * 1.4f, S * 1.4f, 0.45f, Layer::BACK, 3);
    int body = LIGHT;
    shade(c.circle(0, S * 0.62f, S * 0.6f, body, Layer::MAIN, 0), 0.95f, 0.4f);
    shade(c.box(-S * 0.3f, S * 0.2f, S * 0.6f, S * 0.42f, body, Layer::MAIN, 0), 0.95f, 0.4f);
    for (int i = -1; i <= 1; ++i)
        shade(c.circle(i * S * 0.2f, S * 0.2f, S * 0.2f, body, Layer::MAIN, 0), 0.95f, 0.4f);
    shade(c.box(S * 0.12f, S * 0.18f, S * 0.18f, S * 0.5f, body, Layer::MAIN, 1), 0.8f, 0.4f);
    auto& armL = c.ellipse(-S * 0.33f, S * 0.45f, S * 0.2f, S * 0.08f, body, Layer::MAIN, 2);
    armL.shade = 0.9f; armL.sat = 0.4f;
    c.rotated(armL, 30);
    c.ellipse(-S * 0.1f, S * 0.66f, S * 0.09f, S * 0.15f, BASE, Layer::FRONT, 0);
    c.ellipse(S * 0.1f, S * 0.66f, S * 0.09f, S * 0.15f, BASE, Layer::FRONT, 0);
    c.ellipse(0, S * 0.48f, S * 0.12f, S * 0.08f, BASE, Layer::FRONT, 1);
    if (c.glow) {
        c.glowAt(-S * 0.1f, S * 0.66f, S * 0.12f, S * 0.12f, 0.8f, Layer::FX, 4);
        c.glowAt(S * 0.1f, S * 0.66f, S * 0.12f, S * 0.12f, 0.8f, Layer::FX, 4);
    }
}

void skull(Canvas& c, float S) {
    shade(c.circle(S * 0.02f, S * 0.54f, S * 0.62f, LIGHT, Layer::MAIN, 1), 0.7f, 0.3f);
    shade(c.circle(0, S * 0.56f, S * 0.6f, LIGHT, Layer::MAIN, 0), 0.92f, 0.3f);
    shade(c.box(-S * 0.18f, S * 0.12f, S * 0.36f, S * 0.22f, LIGHT, Layer::MAIN, 0), 0.92f, 0.3f);
    c.circle(-S * 0.13f, S * 0.52f, S * 0.18f, BASE, Layer::FRONT, 0);
    c.circle(S * 0.13f, S * 0.52f, S * 0.18f, BASE, Layer::FRONT, 0);
    c.tri(0, S * 0.36f, S * 0.08f, S * 0.08f, BASE, Layer::FRONT, 1).rot = 180;
    for (int i = -2; i <= 2; ++i)
        c.box(i * S * 0.065f - S * 0.006f, S * 0.12f, S * 0.012f, S * 0.12f, BASE, Layer::FRONT, 2);
    c.bar(S * 0.05f, S * 0.85f, S * 0.12f, S * 0.7f, S * 0.012f, BASE, Layer::FRONT, 2);
    if (c.glow) {
        shade(c.circle(-S * 0.13f, S * 0.52f, S * 0.06f, GLOW, Layer::FX, 3), 1.f);
        shade(c.circle(S * 0.13f, S * 0.52f, S * 0.06f, GLOW, Layer::FX, 3), 1.f);
        c.glowAt(0, S * 0.52f, S * 0.6f, S * 0.3f, 0.6f, Layer::FX, 3);
    }
}

void rocket(Canvas& c, float S) {
    float bw = S * 0.24f;
    shade(c.box(-bw / 2, S * 0.22f, bw, S * 0.52f, LIGHT, Layer::MAIN, 0), 0.92f, 0.3f);
    shade(c.box(bw * 0.1f, S * 0.22f, bw * 0.4f, S * 0.52f, LIGHT, Layer::MAIN, 1), 0.72f, 0.3f);
    c.tri(0, S * 0.74f + bw * 0.55f, bw, bw * 1.1f, ACCENT, Layer::MAIN, 0);
    shade(c.slope(bw / 4, S * 0.74f + bw * 0.55f, bw / 2, bw * 1.1f, true, false, ACCENT, Layer::MAIN, 1), 0.7f);
    c.slope(-bw / 2 - S * 0.07f, S * 0.3f, S * 0.14f, S * 0.18f, false, false, ACCENT, Layer::MAIN, 0);
    c.slope(bw / 2 + S * 0.07f, S * 0.3f, S * 0.14f, S * 0.18f, true, false, ACCENT, Layer::MAIN, 0).shade = 0.7f;
    c.circle(0, S * 0.56f, bw * 0.62f, BASE, Layer::FRONT, 0);
    shade(c.circle(0, S * 0.56f, bw * 0.46f, GLOW, Layer::FRONT, 1), 0.8f);
    shade(c.box(-bw / 2, S * 0.36f, bw, S * 0.03f, ACCENT, Layer::FRONT, 2), 0.9f);
    c.box(-bw * 0.35f, S * 0.16f, bw * 0.7f, S * 0.06f, BASE, Layer::MAIN, 1);
    // exhaust flame pointing down
    float fy = S * 0.16f, fs = S * 0.22f;
    shade(c.circle(0, fy - fs * 0.28f, fs * 0.56f, GLOW, Layer::BACK, 0), 0.75f);
    shade(c.tri(0, fy - fs * 0.62f, fs * 0.5f, fs * 0.72f, GLOW, Layer::BACK, 0), 0.75f).rot = 180;
    shade(c.circle(0, fy - fs * 0.25f, fs * 0.3f, GLOW, Layer::BACK, 1), 1.f, 0.3f);
    c.glowAt(0, fy - fs * 0.4f, fs * 2.2f, fs * 2.4f, 0.8f, Layer::BACK, 3);
}

void ufo(Canvas& c, float S) {
    float cy = S * 0.62f;
    if (c.glow) {
        auto& beam = c.tri(0, cy * 0.45f, S * 0.7f, cy * 0.9f, GLOW, Layer::BACK, 3);
        beam.shade = 0.3f;
    }
    shade(c.ellipse(0, cy + S * 0.1f, S * 0.36f, S * 0.32f, LIGHT, Layer::MAIN, 0), 0.8f, 0.5f);
    shade(c.ellipse(-S * 0.05f, cy + S * 0.15f, S * 0.12f, S * 0.08f, LIGHT, Layer::MAIN, 2), 1.2f, 0.2f);
    c.ellipse(0, cy, S * 0.95f, S * 0.22f, MID, Layer::FRONT, 0);
    shade(c.ellipse(0, cy - S * 0.04f, S * 0.95f, S * 0.08f, BASE, Layer::FRONT, 1), 1.f);
    int lights = std::max(3, c.n(5));
    for (int i = 0; i < lights; ++i) {
        float x = (float(i) / (lights - 1) - 0.5f) * S * 0.7f;
        c.circle(x, cy, S * 0.045f, GLOW, Layer::FX, 2);
    }
    if (c.glow) c.glowAt(0, cy, S * 1.3f, S * 0.6f, 0.5f, Layer::BACK, 3);
}

void crown(Canvas& c, float S) {
    float W = S * 0.75f;
    c.glowAt(0, S * 0.35f, S * 1.3f, S * 0.9f, 0.5f, Layer::BACK, 3);
    int spikes = 5;
    for (int i = 0; i < spikes; ++i) {
        float x = (float(i) / (spikes - 1) - 0.5f) * W * 0.85f;
        float h = (i == spikes / 2) ? S * 0.32f : S * 0.25f;
        c.tri(x, S * 0.28f + h / 2, W / spikes * 1.25f, h, MID, Layer::MAIN, 0);
        shade(c.circle(x, S * 0.28f + h, S * 0.06f, LIGHT, Layer::FRONT, 2), 0.95f);
    }
    c.box(-W / 2, S * 0.08f, W, S * 0.22f, MID, Layer::MAIN, 0);
    shade(c.box(-W / 2, S * 0.08f, W, S * 0.05f, MID, Layer::MAIN, 1), 0.7f);
    shade(c.box(-W / 2, S * 0.26f, W, S * 0.025f, LIGHT, Layer::FRONT, 2), 0.9f);
    c.shape(Shape::Diamond, 0, S * 0.19f, S * 0.09f, S * 0.12f, ACCENT, Layer::FRONT, 1);
    for (int i = -1; i <= 1; i += 2)
        c.circle(i * W * 0.3f, S * 0.19f, S * 0.07f, ACCENT, Layer::FRONT, 1).hue = 120.f * i;
}

void snowman(Canvas& c, float S) {
    float ys[] = { 0.24f, 0.6f, 0.86f }, ds[] = { 0.48f, 0.36f, 0.26f };
    for (int i = 0; i < 3; ++i) shade(c.circle(S * 0.025f, S * ys[i] - S * 0.02f, S * ds[i], LIGHT, Layer::MAIN, 1), 0.78f, 0.5f);
    for (int i = 0; i < 3; ++i) shade(c.circle(0, S * ys[i], S * ds[i] * 0.96f, LIGHT, Layer::MAIN, 0), 1.f, 0.3f);
    c.circle(-S * 0.045f, S * 0.89f, S * 0.035f, BASE, Layer::FRONT, 1);
    c.circle(S * 0.045f, S * 0.89f, S * 0.035f, BASE, Layer::FRONT, 1);
    auto& nose = c.tri(S * 0.06f, S * 0.85f, S * 0.04f, S * 0.12f, ACCENT, Layer::FRONT, 1);
    nose.rot = 90; nose.hue = 0;
    for (int i = 0; i < 3; ++i) c.circle(0, S * (0.5f + i * 0.08f), S * 0.035f, BASE, Layer::FRONT, 2);
    c.box(-S * 0.1f, S * 0.97f, S * 0.2f, S * 0.14f, BASE, Layer::FRONT, 0);
    c.box(-S * 0.16f, S * 0.96f, S * 0.32f, S * 0.03f, BASE, Layer::FRONT, 0);
    c.box(-S * 0.17f, S * 0.71f, S * 0.34f, S * 0.05f, ACCENT, Layer::FRONT, 2);
}

void abstractArt(Canvas& c, float S) {
    float cy = S * 0.5f;
    int motif = int(c.rng.next() % 3);
    c.glowAt(0, cy, S * 1.4f, S * 1.4f, 0.6f, Layer::BACK, 3);
    if (motif == 0) {
        float sizes[] = { 0.82f, 0.64f, 0.46f, 0.3f, 0.16f };
        int slots[] = { BASE, MID, LIGHT, ACCENT, GLOW };
        for (int i = 0; i < 5; ++i) {
            auto& r = c.rect(0, cy, S * sizes[i], S * sizes[i], slots[i], Layer::MAIN, i < 3 ? 0 : 2);
            r.rot = 45.f + (i % 2) * 15.f * (c.rng.chance(0.5f) ? 1 : -1);
            if (slots[i] == LIGHT) r.shade = 0.7f;
        }
    }
    else if (motif == 1) {
        int n = std::max(5, c.n(7));
        for (int i = 0; i < n; ++i) {
            float f = 1.f - float(i) / n;
            auto& r = c.ring(0, cy, S * 0.9f * f, S * 0.9f * f, i % 2 ? LIGHT : MID, Layer::MAIN, i < 3 ? 0 : 2);
            r.shade = 0.6f + 0.4f * (1 - f);
        }
        c.circle(0, cy, S * 0.12f, GLOW, Layer::FRONT, 0);
    }
    else {
        int n = std::max(6, c.n(10));
        for (int i = 0; i < n; ++i) {
            float a = float(i) / n * 2 * PI;
            auto& b = c.bar(std::cos(a) * S * 0.12f, cy + std::sin(a) * S * 0.12f, std::cos(a) * S * 0.45f, cy + std::sin(a) * S * 0.45f, S * 0.05f, i % 2 ? MID : LIGHT, Layer::MAIN, 1);
            b.shade = i % 2 ? 1.f : 0.75f;
        }
        c.shape(Shape::Hexagon, 0, cy, S * 0.3f, S * 0.3f, ACCENT, Layer::FRONT, 0);
        c.circle(0, cy, S * 0.12f, GLOW, Layer::FRONT, 1);
    }
    int orbit = c.n(4);
    for (int i = 0; i < orbit; ++i) {
        float a = c.rng.range(0, 2 * PI);
        auto& d = c.shape(Shape::Diamond, std::cos(a) * S * 0.5f, cy + std::sin(a) * S * 0.5f, S * 0.05f, S * 0.09f, i % 2 ? ACCENT : LIGHT, Layer::FX, 4);
        c.rotated(d, -deg(a) + 90);
    }
}

} // namespace

float propWidth(Kind k, float S) {
    switch (k) {
        case Kind::Castle: return S * 1.35f;
        case Kind::City: case Kind::Mountain: return S * 2.0f;
        case Kind::Pyramid: return S * 1.4f;
        case Kind::Fence: case Kind::Grass: case Kind::Waves: case Kind::Stars: return S * 1.8f;
        case Kind::Bush: case Kind::Cloud: case Kind::Island: case Kind::Spikes: return S * 1.3f;
        case Kind::Planet: return S * 1.1f;
        case Kind::Tree: case Kind::Sakura: case Kind::Pine: case Kind::Crystal: case Kind::House: return S * 0.95f;
        case Kind::Palm: return S * 1.0f;
        case Kind::Lamp: case Kind::Torch: case Kind::Candle: case Kind::Pillar: case Kind::Chain: case Kind::Lantern: return S * 0.45f;
        default: return S * 0.85f;
    }
}

void drawProp(Canvas& c, Kind k, float S) {
    switch (k) {
        case Kind::Tree: tree(c, S, false); break;
        case Kind::Sakura: tree(c, S, true); break;
        case Kind::Pine: pine(c, S); break;
        case Kind::Palm: palm(c, S); break;
        case Kind::DeadTree: deadTree(c, S); break;
        case Kind::Bush: bush(c, S * 0.6f); break;
        case Kind::Crystal: crystal(c, S); break;
        case Kind::Rock: rocks(c, S * 0.6f); break;
        case Kind::Mushroom: mushroom(c, S * 0.8f); break;
        case Kind::Flower: flower(c, S * 0.7f); break;
        case Kind::Grass: grass(c, S * 0.6f); break;
        case Kind::Cactus: cactus(c, S); break;
        case Kind::Island: island(c, S); break;
        case Kind::Mountain: mountains(c, S); break;
        case Kind::Lamp: lamp(c, S); break;
        case Kind::Torch: torch(c, S * 0.8f); break;
        case Kind::Campfire: campfire(c, S * 0.7f); break;
        case Kind::Candle: candles(c, S * 0.6f); break;
        case Kind::Lantern: lantern(c, S * 0.7f); break;
        case Kind::Tower: tower(c, S, c.spec->roof); break;
        case Kind::Castle: castle(c, S); break;
        case Kind::House: house(c, S); break;
        case Kind::City: city(c, S); break;
        case Kind::Pillar: pillar(c, S); break;
        case Kind::Arch: arch(c, S, false); break;
        case Kind::Portal: portal(c, S); break;
        case Kind::Pyramid: pyramid(c, S); break;
        case Kind::Fence: fence(c, S * 0.6f); break;
        case Kind::Sign: sign(c, S * 0.8f); break;
        case Kind::Cloud: cloud(c, S * 0.6f); break;
        case Kind::Sun: sun(c, S); break;
        case Kind::Moon: moon(c, S, false); break;
        case Kind::Planet: planet(c, S); break;
        case Kind::Stars: stars(c, S); break;
        case Kind::Gear: gear(c, S); break;
        case Kind::Chain: chain(c, S); break;
        case Kind::Spikes: spikes(c, S * 0.6f); break;
        case Kind::Waterfall: waterfall(c, S); break;
        case Kind::Waves: waves(c, S * 0.6f); break;
        case Kind::Orb: orb(c, S); break;
        case Kind::Heart: heart(c, S); break;
        case Kind::Ghost: ghost(c, S); break;
        case Kind::Skull: skull(c, S); break;
        case Kind::Rocket: rocket(c, S); break;
        case Kind::Ufo: ufo(c, S); break;
        case Kind::Crown: crown(c, S * 0.8f); break;
        case Kind::Snowman: snowman(c, S); break;
        default: abstractArt(c, S); break;
    }
}

void drawMoonCrescent(Canvas& c, float S) { moon(c, S, true); }
void drawFlame(Canvas& c, float x, float y, float s) { flame(c, x, y, s); }

} // namespace deco
