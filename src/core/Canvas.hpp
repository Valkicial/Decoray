// Drawing helper used by the recipes. All coordinates are local to the
// current transform (origin + uniform scale + optional mirror).
#pragma once
#include "DecoCore.hpp"
#include "Spec.hpp"

namespace deco {

struct Canvas {
    std::vector<Prim> prims;
    Rng rng;
    Spec const* spec = nullptr;
    float detail = 1.f;   // 0.4 .. 3, grows with the object limit
    bool glow = true;

    // transform
    float ox = 0, oy = 0, k = 1;
    bool mirror = false;
    Layer layerShift = Layer::MAIN; // base layer for MAIN-relative layers
    int layerOffset = 0;

    explicit Canvas(uint32_t seed) : rng(seed) {}

    struct Saved { float ox, oy, k; bool mirror; int layerOffset; };
    Saved save() const { return { ox, oy, k, mirror, layerOffset }; }
    void restore(Saved const& s) { ox = s.ox; oy = s.oy; k = s.k; mirror = s.mirror; layerOffset = s.layerOffset; }
    void at(float x, float y, float scale) {
        ox = ox + (mirror ? -x : x) * k;
        oy = oy + y * k;
        k *= scale;
    }

    Layer L(Layer l) const {
        int v = int(l) + layerOffset;
        if (v < 0) v = 0;
        if (v > int(Layer::FX)) v = int(Layer::FX);
        return Layer(v);
    }

    int n(float base) const {
        int v = int(std::round(base * detail));
        return v < 1 ? 1 : v;
    }

    Prim& add(Shape s, float x, float y, float w, float h, int slot, Layer layer, int pri) {
        Prim p;
        p.shape = s;
        p.x = ox + (mirror ? -x : x) * k;
        p.y = oy + y * k;
        p.w = std::fabs(w * k);
        p.h = std::fabs(h * k);
        p.slot = slot;
        p.layer = L(layer);
        p.pri = pri;
        p.flipX = mirror;
        p.order = int(prims.size());
        prims.push_back(p);
        return prims.back();
    }

    Prim& rect(float cx, float cy, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Rect, cx, cy, w, h, slot, l, pri);
    }
    // rect from bottom-left corner
    Prim& box(float x, float y, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Rect, x + w / 2, y + h / 2, w, h, slot, l, pri);
    }
    Prim& circle(float cx, float cy, float d, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Circle, cx, cy, d, d, slot, l, pri);
    }
    Prim& ellipse(float cx, float cy, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Circle, cx, cy, w, h, slot, l, pri);
    }
    Prim& ring(float cx, float cy, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Ring, cx, cy, w, h, slot, l, pri);
    }
    Prim& tri(float cx, float cy, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(Shape::Triangle, cx, cy, w, h, slot, l, pri);
    }
    // right triangle; default has its right angle at the bottom-right (ramp up to the right)
    Prim& slope(float cx, float cy, float w, float h, bool fx, bool fy, int slot, Layer l = Layer::MAIN, int pri = 0) {
        auto& p = add(Shape::Slope, cx, cy, w, h, slot, l, pri);
        p.flipX = fx != mirror;
        p.flipY = fy;
        return p;
    }
    Prim& shape(Shape s, float cx, float cy, float w, float h, int slot, Layer l = Layer::MAIN, int pri = 0) {
        return add(s, cx, cy, w, h, slot, l, pri);
    }
    // additive glow, `power` dims it through HSV value
    Prim* glowAt(float cx, float cy, float w, float h, float power = 1.f, Layer l = Layer::BACK, int pri = 3) {
        if (!glow) return nullptr;
        auto& p = add(Shape::Glow, cx, cy, w, h, GLOW, l, pri);
        p.shade = power;
        return &p;
    }
    // line segment as a rotated rect
    Prim& bar(float x1, float y1, float x2, float y2, float thick, int slot, Layer l = Layer::MAIN, int pri = 0) {
        float dx = x2 - x1, dy = y2 - y1;
        float len = std::sqrt(dx * dx + dy * dy);
        auto& p = add(Shape::Rect, (x1 + x2) / 2, (y1 + y2) / 2, len, thick, slot, l, pri);
        float ang = std::atan2(dy, dx) * 180.f / PI;
        if (mirror) ang = 180.f - ang;
        p.rot = -ang; // GD rotation is clockwise
        p.flipX = false;
        return p;
    }
    Prim& rotated(Prim& p, float degClockwise) {
        p.rot = mirror ? -degClockwise : degClockwise;
        return p;
    }
};

// shading helpers
inline Prim& shade(Prim& p, float v, float s = 1.f) { p.shade = v; p.sat = s; return p; }

} // namespace deco
