#include "Claude.hpp"
#include "../core/Recipes.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace aideco {

namespace {

// Frame the model works in: structure -> bottom-left of the blocks' bbox,
// free placement -> the cursor (design centered on 0,0).
deco::Vec2 frameOrigin(AIRequest const& req) {
    if (req.structure && !req.blocks.empty()) {
        auto info = deco::analyzeStructure(req.blocks);
        return { info.bbox.x, info.bbox.y };
    }
    return req.anchor;
}

std::string fmtNum(float v) { return fmt::format("{:.0f}", v); }

std::string targetText(AIRequest const& req) {
    if (req.structure && !req.blocks.empty()) {
        auto info = deco::analyzeStructure(req.blocks);
        auto o = frameOrigin(req);
        std::string s = fmt::format(
            "TARGET: decorate an existing structure of {} grid cells. Its bounding box is (0,0) to ({},{}).\n"
            "Your design must COVER every solid area below (the plain blocks will be hidden behind it), follow the\n"
            "outline, and may add details above top edges and below bottom edges. Use layers 3-5 for the cover.\n"
            "Solid areas (x,y,w,h): ",
            info.cells, fmtNum(info.bbox.w), fmtNum(info.bbox.h));
        for (size_t i = 0; i < info.fills.size() && i < 40; ++i) {
            auto const& r = info.fills[i];
            s += fmt::format("({},{},{},{}) ", fmtNum(r.x - o.x), fmtNum(r.y - o.y), fmtNum(r.w), fmtNum(r.h));
        }
        s += "\nExposed top edges (x0..x1 @ y): ";
        for (size_t i = 0; i < info.tops.size() && i < 30; ++i)
            s += fmt::format("({}..{} @ {}) ", fmtNum(info.tops[i].x0 - o.x), fmtNum(info.tops[i].x1 - o.x), fmtNum(info.tops[i].y - o.y));
        s += "\nExposed bottom edges: ";
        for (size_t i = 0; i < info.bottoms.size() && i < 30; ++i)
            s += fmt::format("({}..{} @ {}) ", fmtNum(info.bottoms[i].x0 - o.x), fmtNum(info.bottoms[i].x1 - o.x), fmtNum(info.bottoms[i].y - o.y));
        return s;
    }
    float h = req.sizeBlocks * deco::BLOCK;
    return fmt::format(
        "TARGET: a free-standing design centered on (0,0), roughly {} units ({} blocks) tall. "
        "Keep the whole design within about +-{} units of the center.",
        fmtNum(h), fmtNum(req.sizeBlocks), fmtNum(h * 0.9f));
}

char const* SLOT_KEYS[deco::SLOT_COUNT] = { "base", "mid", "light", "accent", "glow", "foliage", "bloom", "wood" };

} // namespace

bool claudeConfigured() {
    return !Mod::get()->getSettingValue<std::string>("claude-api-key").empty();
}

web::WebRequest buildClaudeRequest(AIRequest const& req) {
    auto key = Mod::get()->getSettingValue<std::string>("claude-api-key");
    auto model = Mod::get()->getSettingValue<std::string>("claude-model");
    if (model.empty()) model = "claude-sonnet-5-5";

    std::string system = deco::designRulesText() + "\n\n" + deco::objectCatalogText() +
        "\n\nSlots 5 FOLIAGE (leaves, grass, moss, vines), 6 BLOOM (flowers, berries, blossoms) and 7 WOOD "
        "(trunks, planks, handles) also exist; use them for plants and wood so they stay natural in any theme.";

    auto origin = frameOrigin(req);
    deco::Design draft = req.draft;
    deco::enforceLimit(draft.prims, std::min(req.limit, 140));

    std::string palette;
    for (int s = 0; s < deco::SLOT_COUNT; ++s)
        palette += fmt::format("\"{}\":\"{}\"{}", SLOT_KEYS[s], deco::hex(draft.palette.c[s]), s + 1 < deco::SLOT_COUNT ? "," : "");

    std::string user = fmt::format(
        "DESIGN REQUEST: \"{}\"\n\n{}\n\n"
        "OBJECT BUDGET: at most {} primitives. Use most of it when the subject deserves detail, but never exceed it.\n\n"
        "SUGGESTED PALETTE (change it if the request implies other colors): {{{}}}\n\n"
        "PROCEDURAL DRAFT (same coordinate frame; improve on it or replace it entirely, it is only a starting point):\n{}\n\n",
        req.description, targetText(req), req.limit, palette, deco::primsToJson(draft.prims, origin));
    if (!req.examples.empty()) {
        user += "REFERENCE DESIGNS THE USER SAVED (raw GD objects, positions relative to their center; match their style):\n" + req.examples + "\n";
    }
    user +=
        "Reply with ONLY one JSON object, no prose, no code fences:\n"
        "{\"title\":\"short name\",\"palette\":{\"base\":\"#rrggbb\",...all 8 slots...},"
        "\"prims\":[{\"s\":\"rect\",\"x\":0,\"y\":0,\"w\":30,\"h\":30,\"r\":0,\"c\":1,\"v\":1,\"l\":3}, ...]}\n"
        "Optional per-primitive fields: fx, fy, sat, hue, id (for s=\"obj\").";

    int maxTokens = std::clamp(1500 + req.limit * 60, 2000, 32000);
    auto body = matjson::makeObject({
        { "model", model },
        { "max_tokens", matjson::Value(static_cast<std::intmax_t>(maxTokens)) },
        { "system", system },
        { "messages", std::vector<matjson::Value>{ matjson::makeObject({ { "role", "user" }, { "content", user } }) } },
    });

    web::WebRequest r;
    r.header("x-api-key", key);
    r.header("anthropic-version", "2023-06-01");
    r.header("content-type", "application/json");
    r.timeout(std::chrono::seconds(180));
    r.bodyJSON(body);
    return r;
}

Result<deco::Design> parseClaudeResponse(web::WebResponse const& resp, AIRequest const& req) {
    if (!resp.ok()) {
        auto text = resp.string().unwrapOr("");
        std::string msg = fmt::format("HTTP {}", resp.code());
        if (auto parsedErr = matjson::parse(text)) {
            matjson::Value const err = parsedErr.unwrap();
            auto m = err["error"]["message"].asString();
            if (m) msg += ": " + m.unwrap();
        }
        else if (!text.empty()) msg += ": " + text.substr(0, 160);
        return Err(msg);
    }
    auto jsonRes = resp.json();
    if (!jsonRes) return Err("Bad JSON from API: " + jsonRes.unwrapErr());
    matjson::Value const json = jsonRes.unwrap();

    std::string text;
    if (auto content = json["content"].asArray()) {
        for (auto const& block : content.unwrap()) {
            if (block["type"].asString().unwrapOr("") == "text") text += block["text"].asString().unwrapOr("");
        }
    }
    if (text.empty()) return Err("Claude returned no text");

    auto start = text.find('{');
    auto end = text.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return Err("Claude's reply did not contain a design (maybe it ran out of tokens; try a lower object limit)");
    auto parsed = matjson::parse(std::string_view(text).substr(start, end - start + 1));
    if (!parsed) return Err("Could not read Claude's design JSON");
    matjson::Value const doc = parsed.unwrap();

    deco::Design design;
    design.palette = req.draft.palette;
    for (int s = 0; s < deco::SLOT_COUNT; ++s) {
        auto v = doc["palette"][SLOT_KEYS[s]].asString();
        if (v) design.palette.c[s] = deco::parseHex(v.unwrap(), design.palette.c[s]);
    }
    design.palette.name = "claude";

    auto origin = frameOrigin(req);
    int layerShift = req.structure ? 1 : 0;
    auto prims = doc["prims"].asArray();
    if (!prims) return Err("Claude's design had no primitives");
    int order = 0;
    for (auto const& p : prims.unwrap()) {
        deco::Prim prim;
        if (!deco::shapeFromName(p["s"].asString().unwrapOr("rect"), prim.shape)) continue;
        prim.x = float(p["x"].asDouble().unwrapOr(0)) + origin.x;
        prim.y = float(p["y"].asDouble().unwrapOr(0)) + origin.y;
        prim.w = float(p["w"].asDouble().unwrapOr(30));
        prim.h = float(p["h"].asDouble().unwrapOr(prim.w));
        prim.rot = float(p["r"].asDouble().unwrapOr(0));
        prim.flipX = p["fx"].asInt().unwrapOr(0) != 0 || p["fx"].asBool().unwrapOr(false);
        prim.flipY = p["fy"].asInt().unwrapOr(0) != 0 || p["fy"].asBool().unwrapOr(false);
        prim.slot = std::clamp(int(p["c"].asInt().unwrapOr(1)), 0, deco::SLOT_COUNT - 1);
        prim.shade = float(p["v"].asDouble().unwrapOr(1));
        prim.sat = float(p["sat"].asDouble().unwrapOr(1));
        prim.hue = float(p["hue"].asDouble().unwrapOr(0));
        int layer = std::clamp(int(p["l"].asInt().unwrapOr(3)) + layerShift, 0, 5);
        prim.layer = deco::Layer(layer);
        prim.rawId = int(p["id"].asInt().unwrapOr(0));
        prim.order = order++;
        // keep the model's own ordering as priority: later items are details
        prim.pri = std::min(4, int(order * 5 / std::max<size_t>(1, prims.unwrap().size())));
        if (prim.w <= 0.1f || prim.h <= 0.1f) {
            if (prim.shape != deco::Shape::Raw) continue;
        }
        design.prims.push_back(prim);
    }
    if (design.prims.empty()) return Err("Claude's design was empty");
    design.summary = doc["title"].asString().unwrapOr("Claude design") + " | by Claude";
    return Ok(design);
}

} // namespace aideco
