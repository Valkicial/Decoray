// Generator: description + target -> primitives, with detail scaled to the
// object limit.
#include "Recipes.hpp"

#include <algorithm>
#include <numeric>

namespace deco {

namespace {

struct Placement {
    Kind kind;
    float S;
    float x, y;
    int layerDelta;
    bool hang;
};

bool hangs(Kind k, Spec const& spec) {
    return k == Kind::Chain || (k == Kind::Lantern && spec.hanging);
}

void drawPlaced(Canvas& c, Placement const& p) {
    auto sv = c.save();
    c.ox = p.x; c.oy = p.y; c.k = 1; c.mirror = false;
    c.layerOffset += p.layerDelta;
    if (p.hang) {
        // hanging props: draw a chain/lantern so its top meets y
        if (p.kind == Kind::Lantern) {
            c.oy = p.y - p.S * 0.7f;
            drawProp(c, p.kind, p.S);
        }
        else {
            c.oy = p.y - p.S;
            drawProp(c, p.kind, p.S);
        }
    }
    else {
        // mirror every other instance for variety where it makes sense
        drawProp(c, p.kind, p.S);
    }
    c.restore(sv);
}

void composeFree(Canvas& c, Spec const& spec, Options const& opts, std::vector<Kind>& used) {
    float S = std::clamp(opts.sizeBlocks, 0.5f, 40.f) * BLOCK * spec.sizeMul;
    std::vector<SubjectReq> subjects = spec.subjects;
    if (subjects.empty()) subjects.push_back({ Kind::Abstract, 1 });

    std::vector<SubjectReq> ground, sky;
    for (auto const& s : subjects) (isSkyKind(s.kind) ? sky : ground).push_back(s);

    std::vector<Placement> places;
    float left = 0, right = 0, top = 0;
    if (!ground.empty()) {
        auto const& main = ground.front();
        int n = std::max(1, main.count);
        float w = propWidth(main.kind, S);
        float total = 0;
        std::vector<float> sizes;
        for (int i = 0; i < n; ++i) {
            float si = n == 1 ? S : S * c.rng.range(0.75f, 1.05f);
            sizes.push_back(si);
            total += propWidth(main.kind, si) * (n == 1 ? 1.f : 0.85f);
        }
        float x = -total / 2;
        for (int i = 0; i < n; ++i) {
            float wi = propWidth(main.kind, sizes[i]) * (n == 1 ? 1.f : 0.85f);
            places.push_back({ main.kind, sizes[i], x + wi / 2, 0, 0, false });
            x += wi;
        }
        left = -total / 2; right = total / 2; top = S;
        (void)w;
        bool toRight = true;
        for (size_t gi = 1; gi < ground.size(); ++gi) {
            auto const& sec = ground[gi];
            for (int i = 0; i < sec.count; ++i) {
                float si = S * c.rng.range(0.5f, 0.68f);
                float wi = propWidth(sec.kind, si);
                if (toRight) { places.push_back({ sec.kind, si, right + wi / 2 + S * 0.04f, 0, 0, false }); right += wi + S * 0.04f; }
                else { places.push_back({ sec.kind, si, left - wi / 2 - S * 0.04f, 0, 0, false }); left -= wi + S * 0.04f; }
                toRight = !toRight;
            }
        }
    }
    float skyY = ground.empty() ? 0 : top * 1.05f;
    float skyS = ground.empty() ? S : S * 0.55f;
    for (size_t i = 0; i < sky.size(); ++i) {
        auto const& s = sky[i];
        for (int j = 0; j < s.count; ++j) {
            float x = ground.empty() ? (j - (s.count - 1) / 2.f) * propWidth(s.kind, skyS) * 1.1f
                                     : (i % 2 ? left : right) * 0.6f + j * skyS * 0.9f;
            places.push_back({ s.kind, skyS * c.rng.range(0.85f, 1.1f), x, skyY + (ground.empty() ? 0 : c.rng.range(0, S * 0.2f)), ground.empty() ? 0 : -1, false });
        }
    }
    for (auto const& p : places) {
        drawPlaced(c, p);
        used.push_back(p.kind);
    }
    if (spec.ground && !ground.empty() && ground.front().kind != Kind::Island && ground.front().kind != Kind::Mountain) {
        float w = right - left + S * 0.3f;
        float cx = (left + right) / 2;
        shade(c.box(cx - w / 2, -S * 0.12f, w, S * 0.12f, MID, Layer::MAIN, 0), 0.8f);
        shade(c.box(cx - w / 2, -S * 0.025f, w, S * 0.025f, LIGHT, Layer::MAIN, 2), 0.8f);
        shade(c.box(cx - w / 2, -S * 0.12f, w, S * 0.04f, BASE, Layer::MAIN, 1), 1.2f);
    }
}

void composeStructure(Canvas& c, Spec& spec, Target const& target, Options const& opts, std::vector<Kind>& used, std::string& style) {
    auto info = analyzeStructure(target.blocks);
    Rect bb = info.bbox;

    // subjects that are really surfaces
    std::vector<SubjectReq> props;
    for (auto const& s : spec.subjects) {
        if (s.kind == Kind::Grass) spec.surfaces.push_back(Surf::Grass);
        else if (s.kind == Kind::Spikes) spec.surfaces.push_back(Surf::Spikes);
        else if (s.kind == Kind::Waves) spec.surfaces.push_back(Surf::Waves);
        else if (s.kind == Kind::Stars) spec.surfaces.push_back(Surf::Stars);
        else if (s.kind == Kind::Flower && !spec.fill) spec.surfaces.push_back(Surf::Flowers);
        else props.push_back(s);
    }

    if (spec.fill && !props.empty()) {
        Kind k = props.front().kind;
        float S = std::min(bb.h * 1.02f, bb.w * 1.08f / propWidth(k, 1.f));
        S = std::max(S, 20.f);
        Placement p{ k, S, bb.cx(), bb.y, 1, false };
        drawPlaced(c, p);
        used.push_back(k);
        style = "shape fill";
        return;
    }

    c.layerOffset += 1;
    decorateStructure(c, target.blocks, style);
    c.layerOffset -= 1;

    if (props.empty()) return;

    auto tops = info.tops;
    std::sort(tops.begin(), tops.end(), [](auto const& a, auto const& b) { return (a.x1 - a.x0) > (b.x1 - b.x0); });
    auto bottoms = info.bottoms;
    std::sort(bottoms.begin(), bottoms.end(), [](auto const& a, auto const& b) { return (a.x1 - a.x0) > (b.x1 - b.x0); });

    float baseS = std::clamp(opts.sizeBlocks * BLOCK, 45.f, 240.f) * spec.sizeMul;
    for (auto const& req : props) {
        int n = std::max(1, req.count);
        if (isSkyKind(req.kind)) {
            for (int i = 0; i < n; ++i) {
                float x = bb.x + bb.w * (i + 0.5f) / n;
                Placement p{ req.kind, baseS * 0.7f, x, bb.top() + baseS * 0.6f, -1, false };
                drawPlaced(c, p);
            }
            used.push_back(req.kind);
            continue;
        }
        if (hangs(req.kind, spec) && !bottoms.empty()) {
            auto const& seg = bottoms.front();
            float len = seg.x1 - seg.x0;
            for (int i = 0; i < n; ++i) {
                float x = seg.x0 + len * (i + 0.5f) / n;
                Placement p{ req.kind, baseS * 0.8f, x, seg.y, 0, true };
                drawPlaced(c, p);
            }
            used.push_back(req.kind);
            continue;
        }
        if (tops.empty()) continue;
        auto const& seg = tops.front();
        float len = seg.x1 - seg.x0;
        float S = baseS;
        float need = propWidth(req.kind, S) * n;
        if (need > len * 1.15f) S *= std::max(0.35f, len * 1.15f / need);
        for (int i = 0; i < n; ++i) {
            float x = seg.x0 + len * (i + 0.5f) / n;
            Placement p{ req.kind, S * (n > 1 ? c.rng.range(0.85f, 1.05f) : 1.f), x, seg.y, 0, false };
            drawPlaced(c, p);
        }
        used.push_back(req.kind);
    }
}

bool isGlowShape(Shape s) { return s == Shape::Glow || s == Shape::EdgeGlow; }

} // namespace

void enforceLimit(std::vector<Prim>& prims, int limit) {
    if (limit <= 0) { prims.clear(); return; }
    if (int(prims.size()) <= limit) return;
    std::vector<char> keep(prims.size(), 1);
    int excess = int(prims.size()) - limit;
    for (int pri = 4; pri >= 1 && excess > 0; --pri) {
        std::vector<size_t> tier;
        for (size_t i = 0; i < prims.size(); ++i)
            if (keep[i] && prims[i].pri == pri) tier.push_back(i);
        if (tier.empty()) continue;
        if (excess >= int(tier.size())) {
            for (auto i : tier) keep[i] = 0;
            excess -= int(tier.size());
        }
        else {
            // remove evenly spread items so what is left stays balanced
            double step = double(tier.size()) / excess;
            for (int r = 0; r < excess; ++r) keep[tier[size_t(r * step)]] = 0;
            excess = 0;
        }
    }
    if (excess > 0) {
        // only silhouettes left: drop the smallest ones
        std::vector<size_t> idx;
        for (size_t i = 0; i < prims.size(); ++i) if (keep[i]) idx.push_back(i);
        std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) { return prims[a].w * prims[a].h < prims[b].w * prims[b].h; });
        for (int r = 0; r < excess && r < int(idx.size()); ++r) keep[idx[r]] = 0;
    }
    std::vector<Prim> out;
    out.reserve(limit);
    for (size_t i = 0; i < prims.size(); ++i) if (keep[i]) out.push_back(prims[i]);
    prims = std::move(out);
}

Design generate(std::string const& description, Target const& target, Options const& opts) {
    Spec spec = parseSpec(description);
    Design design;
    design.palette = spec.pal;
    design.pixel = spec.pixel;

    std::vector<Kind> used;
    std::string style;
    auto build = [&](float detail) {
        Spec local = spec;
        Canvas c(opts.seed);
        c.spec = &local;
        c.detail = detail;
        c.glow = spec.glow != 0;
        if (spec.background) c.layerOffset = -2;
        else if (spec.foreground) c.layerOffset = 1;
        used.clear();
        style.clear();
        if (target.structure && !target.blocks.empty()) composeStructure(c, local, target, opts, used, style);
        else composeFree(c, local, opts, used);
        return std::move(c.prims);
    };

    int limit = std::max(1, opts.limit);
    float d = spec.detail;
    float maxD = std::min(3.f, spec.detail * 2.5f);
    auto prims = build(d);
    for (int it = 0; it < 4; ++it) {
        int n = std::max(1, int(prims.size()));
        if (n > limit && d > 0.35f) d = std::max(0.35f, d * std::max(0.45f, float(limit) / n));
        else if (n < limit * 0.55f && d < maxD) d = std::min(maxD, d * std::min(1.7f, limit * 0.8f / n));
        else break;
        prims = build(d);
    }
    design.detail = d;
    enforceLimit(prims, limit);

    // free placement: center the design on the cursor
    if (!(target.structure && !target.blocks.empty())) {
        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        for (auto const& p : prims) {
            if (isGlowShape(p.shape)) continue;
            float r = std::max(p.w, p.h) / 2;
            minX = std::min(minX, p.x - r); maxX = std::max(maxX, p.x + r);
            minY = std::min(minY, p.y - r); maxY = std::max(maxY, p.y + r);
        }
        if (minX > maxX) { minX = maxX = minY = maxY = 0; }
        float dx = target.anchor.x - (minX + maxX) / 2;
        float dy = target.anchor.y - (minY + maxY) / 2;
        for (auto& p : prims) { p.x += dx; p.y += dy; }
    }

    if (spec.background) {
        for (auto& p : prims) {
            if (p.slot == GLOW) continue;
            p.shade *= 0.72f;
            p.sat *= 0.85f;
        }
    }

    design.prims = std::move(prims);

    // summary
    std::string what;
    std::vector<Kind> uniq;
    for (auto k : used) if (std::find(uniq.begin(), uniq.end(), k) == uniq.end()) uniq.push_back(k);
    for (size_t i = 0; i < uniq.size() && i < 3; ++i) {
        if (!what.empty()) what += ", ";
        what += kindName(uniq[i]);
    }
    if (target.structure && !target.blocks.empty()) {
        if (!what.empty()) what = style + " surface + " + what;
        else what = style + " surface";
    }
    if (what.empty()) what = "design";
    design.summary = what + " | " + spec.theme + " palette";
    if (spec.pixel) design.notes.push_back("pixel style requested");
    return design;
}

} // namespace deco
