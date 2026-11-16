#include "Bridge.hpp"

#include <Geode/binding/ColorAction.hpp>
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/GJEffectManager.hpp>
#include <Geode/binding/GJSpriteColor.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <Geode/utils/file.hpp>

#include <algorithm>
#include <array>
#include <deque>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

using namespace geode::prelude;

namespace aideco {

// GD stores object y in level strings 90 units lower than the editor's world
// space (the ground line). Any mismatch is corrected after pasting anyway.
static constexpr float STRING_Y_OFFSET = 90.f;

deco::Vec2 measuredSize(int id) {
    static std::unordered_map<int, deco::Vec2> cache;
    if (auto it = cache.find(id); it != cache.end()) return it->second;
    deco::Vec2 v = deco::defaultObjectSize(id);
    if (auto obj = GameObject::createWithKey(id)) {
        auto cs = obj->getContentSize();
        if (cs.width > 0.5f && cs.height > 0.5f) v = { cs.width, cs.height };
    }
    cache[id] = v;
    return v;
}

CCRect objectRect(GameObject* obj) {
    CCRect r = obj->getObjectRect();
    if (r.size.width < 1.f || r.size.height < 1.f) r = obj->boundingBox();
    return r;
}

static deco::Rect toRect(CCRect const& r) {
    return deco::Rect{ r.origin.x, r.origin.y, r.size.width, r.size.height };
}

static bool blockLike(GameObject* o) {
    return o && o->m_objectType == GameObjectType::Solid && !o->m_isNoTouch;
}

std::vector<deco::Rect> gatherStructure(LevelEditorLayer* lel, GameObject* seed, size_t maxObjects) {
    std::vector<deco::Rect> out;
    if (!seed) return out;
    out.push_back(toRect(objectRect(seed)));
    if (!blockLike(seed)) return out;

    std::unordered_set<GameObject*> seen{ seed };
    std::deque<GameObject*> queue{ seed };
    while (!queue.empty() && out.size() < maxObjects) {
        auto cur = queue.front();
        queue.pop_front();
        auto r = objectRect(cur);
        CCRect probe{ r.origin.x - 2.f, r.origin.y - 2.f, r.size.width + 4.f, r.size.height + 4.f };
        auto arr = lel->objectsInRect(probe, false);
        for (auto other : CCArrayExt<GameObject*>(arr)) {
            if (seen.count(other) || !blockLike(other)) continue;
            seen.insert(other);
            out.push_back(toRect(objectRect(other)));
            queue.push_back(other);
            if (out.size() >= maxObjects) break;
        }
    }
    return out;
}

std::vector<deco::Rect> rectsOf(CCArray* objects) {
    std::vector<deco::Rect> out;
    for (auto obj : CCArrayExt<GameObject*>(objects)) out.push_back(toRect(objectRect(obj)));
    return out;
}

// ------------------------------------------------------------------ colors --

namespace {

std::set<int> channelsInUse(LevelEditorLayer* lel) {
    std::set<int> used;
    for (auto obj : CCArrayExt<GameObject*>(lel->m_objects)) {
        if (obj->m_baseColor) used.insert(obj->m_baseColor->m_colorID);
        if (obj->m_detailColor) used.insert(obj->m_detailColor->m_colorID);
        if (obj->m_objectID == 899) used.insert(obj->m_targetColor);
    }
    return used;
}

ColorAction* colorActionFor(LevelEditorLayer* lel, int channel) {
    if (!lel->m_levelSettings || !lel->m_levelSettings->m_effectManager) return nullptr;
    return lel->m_levelSettings->m_effectManager->getColorAction(channel);
}

bool sameColor(ccColor3B a, deco::RGB b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

void setChannelColor(LevelEditorLayer* lel, int channel, deco::RGB c, bool blending) {
    auto action = colorActionFor(lel, channel);
    if (!action) return;
    ccColor3B col{ c.r, c.g, c.b };
    action->m_color = col;
    action->m_fromColor = col;
    action->m_toColor = col;
    action->m_blending = blending;
    action->m_playerColor = 0;
    action->m_copyID = 0;
    action->m_fromOpacity = 1.f;
    action->m_toOpacity = 1.f;
    action->m_currentOpacity = 1.f;
}

char const* slotName(int s) {
    static char const* names[] = { "base", "mid", "light", "accent", "glow", "foliage", "bloom", "wood" };
    return (s >= 0 && s < deco::SLOT_COUNT) ? names[s] : "?";
}

// remember channels per palette so re-rolling the same theme reuses them
struct PaletteMemo {
    LevelEditorLayer* level = nullptr;
    std::map<std::string, std::array<int, deco::SLOT_COUNT>> byPalette;
};
PaletteMemo& memo() {
    static PaletteMemo m;
    return m;
}

std::string paletteKey(deco::Palette const& p) {
    std::string k;
    for (auto const& c : p.c) k += deco::hex(c);
    return k;
}

Result<PlaceResult> pasteAt(EditorUI* ui, std::string const& str, CCPoint expectedCenter) {
    ui->deselectAll();
    auto arr = ui->pasteObjects(gd::string(str), false, false);
    if (!arr || arr->count() == 0) return Err("Geometry Dash did not create any objects");
    Ref<CCArray> objs = arr;

    // correct for any offset GD applied while pasting
    double ax = 0, ay = 0;
    for (auto obj : CCArrayExt<GameObject*>(objs)) { ax += obj->getPositionX(); ay += obj->getPositionY(); }
    ax /= objs->count();
    ay /= objs->count();
    CCPoint delta{ float(expectedCenter.x - ax), float(expectedCenter.y - ay) };
    if (std::fabs(delta.x) > 0.5f || std::fabs(delta.y) > 0.5f) {
        for (auto obj : CCArrayExt<GameObject*>(objs)) ui->moveObject(obj, delta);
    }
    ui->deselectAll();
    ui->selectObjects(objs, true);
    ui->updateButtons();
    ui->updateObjectInfoLabel();
    PlaceResult res;
    res.objects = objs;
    res.count = int(objs->count());
    return Ok(res);
}

} // namespace

Result<PlaceResult> placeDesign(EditorUI* ui, deco::Design const& design, PlaceSettings const& s) {
    auto lel = ui->m_editorLayer;
    if (!lel) return Err("Editor not ready");

    auto objs = deco::emit(design, measuredSize, s.limit, s.pixel || design.pixel);
    if (objs.empty()) return Err("The design came out empty, try a different description");

    bool usedSlot[deco::SLOT_COUNT] = {};
    for (auto const& o : objs) usedSlot[o.slot] = true;

    int channels[deco::SLOT_COUNT];
    int first = std::clamp(int(Mod::get()->getSettingValue<int64_t>("first-channel")), 1, 990);
    std::string colorInfo;

    if (s.autoColors) {
        auto& m = memo();
        if (m.level != lel) { m.level = lel; m.byPalette.clear(); }
        auto key = paletteKey(design.palette);
        bool reused = false;
        if (auto it = m.byPalette.find(key); it != m.byPalette.end()) {
            reused = true;
            for (int sl = 0; sl < deco::SLOT_COUNT; ++sl) {
                if (!usedSlot[sl]) continue;
                auto a = colorActionFor(lel, it->second[sl]);
                if (!a || it->second[sl] <= 0 || !sameColor(a->m_fromColor, design.palette.c[sl])) reused = false;
            }
            if (reused) std::copy(it->second.begin(), it->second.end(), channels);
        }
        if (!reused) {
            auto taken = channelsInUse(lel);
            int next = first;
            std::array<int, deco::SLOT_COUNT> chosen{};
            for (int sl = 0; sl < deco::SLOT_COUNT; ++sl) {
                if (!usedSlot[sl]) { chosen[sl] = 0; continue; }
                while (next < 999 && taken.count(next)) ++next;
                chosen[sl] = std::min(next, 999);
                taken.insert(chosen[sl]);
            }
            for (int sl = 0; sl < deco::SLOT_COUNT; ++sl) {
                channels[sl] = chosen[sl] ? chosen[sl] : 1011;
                if (chosen[sl]) setChannelColor(lel, chosen[sl], design.palette.c[sl], sl == deco::GLOW);
            }
            m.byPalette[key] = chosen;
            lel->levelSettingsUpdated();
        }
        for (int sl = 0; sl < deco::SLOT_COUNT; ++sl) {
            if (!usedSlot[sl]) continue;
            if (!colorInfo.empty()) colorInfo += "  ";
            colorInfo += fmt::format("{}:{}", slotName(sl), channels[sl]);
        }
    }
    else {
        for (int sl = 0; sl < deco::SLOT_COUNT; ++sl) channels[sl] = first + sl;
        colorInfo = fmt::format("using channels {}-{} (set their colors yourself)", first, first + deco::SLOT_COUNT - 1);
    }

    int layer = lel->m_currentLayer >= 0 ? int(lel->m_currentLayer) : -1;
    auto str = deco::toObjectString(objs, channels, STRING_Y_OFFSET, layer, s.noTouch, s.dontFade, s.dontEnter);

    double ex = 0, ey = 0;
    for (auto const& o : objs) { ex += o.x; ey += o.y; }
    ex /= objs.size();
    ey /= objs.size();

    GEODE_UNWRAP_INTO(auto res, pasteAt(ui, str, CCPoint{ float(ex), float(ey) }));
    res.colorInfo = colorInfo;
    return Ok(res);
}

void deleteObjects(EditorUI* ui, CCArray* objects) {
    if (!objects) return;
    auto alive = CCArray::create();
    for (auto obj : CCArrayExt<GameObject*>(objects))
        if (obj->getParent()) alive->addObject(obj);
    if (alive->count() == 0) return;
    ui->deselectAll();
    ui->selectObjects(alive, true);
    ui->onDeleteSelected(nullptr);
    ui->deselectAll();
    ui->updateButtons();
}

// ----------------------------------------------------------------- library --

namespace {

struct ParsedObj {
    std::vector<std::pair<std::string, std::string>> kv;
    double x = 0, y = 0;
    int id = 0;
};

std::vector<ParsedObj> parseObjects(std::string const& str) {
    std::vector<ParsedObj> out;
    for (auto const& part : string::split(str, ";")) {
        if (part.empty()) continue;
        auto toks = string::split(part, ",");
        ParsedObj o;
        for (size_t i = 0; i + 1 < toks.size(); i += 2) {
            auto const& k = toks[i];
            auto const& v = toks[i + 1];
            if (k == "1") o.id = numFromString<int>(v).unwrapOr(0);
            if (k == "2") o.x = numFromString<double>(v).unwrapOr(0);
            if (k == "3") o.y = numFromString<double>(v).unwrapOr(0);
            o.kv.push_back({ k, v });
        }
        if (o.id > 0) out.push_back(std::move(o));
    }
    return out;
}

std::string writeObjects(std::vector<ParsedObj> const& objs, double dx, double dy) {
    std::string s;
    for (auto const& o : objs) {
        bool firstKv = true;
        for (auto const& [k, v] : o.kv) {
            if (!firstKv) s += ",";
            firstKv = false;
            if (k == "2") s += "2," + fmt::format("{:.2f}", o.x + dx);
            else if (k == "3") s += "3," + fmt::format("{:.2f}", o.y + dy);
            else s += k + "," + v;
        }
        s += ";";
    }
    return s;
}

std::vector<std::string> words(std::string const& text) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : text) {
        if (std::isalnum(static_cast<unsigned char>(ch))) cur += char(std::tolower(static_cast<unsigned char>(ch)));
        else if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    }
    if (!cur.empty()) out.push_back(cur);
    std::erase_if(out, [](std::string const& w) { return w.size() < 3 || w == "the" || w == "and" || w == "with"; });
    return out;
}

} // namespace

Library& Library::get() {
    static Library lib;
    return lib;
}

std::filesystem::path Library::folder() const {
    return Mod::get()->getSaveDir() / "examples";
}

void Library::load() {
    if (m_loaded) return;
    m_loaded = true;
    m_examples.clear();
    std::error_code ec;
    std::filesystem::create_directories(folder(), ec);
    auto files = file::readDirectory(folder()).unwrapOrDefault();
    for (auto const& path : files) {
        if (path.extension() != ".json") continue;
        auto json = file::readJson(path);
        if (!json) { log::warn("Skipping example pack {}: {}", path.string(), json.unwrapErr()); continue; }
        auto arr = json.unwrap()["examples"].asArray();
        if (!arr) continue;
        for (auto const& e : arr.unwrap()) {
            Example ex;
            ex.name = e["name"].asString().unwrapOr("example");
            ex.objects = e["objects"].asString().unwrapOr("");
            if (auto tags = e["tags"].asArray()) {
                for (auto const& t : tags.unwrap()) ex.tags.push_back(t.asString().unwrapOr(""));
            }
            ex.count = int(e["count"].asInt().unwrapOr(0));
            if (!ex.objects.empty()) m_examples.push_back(std::move(ex));
        }
    }
    log::info("Loaded {} decoration examples", m_examples.size());
}

void Library::save() const {
    std::vector<matjson::Value> arr;
    for (auto const& ex : m_examples) {
        std::vector<matjson::Value> tags;
        for (auto const& t : ex.tags) tags.push_back(t);
        arr.push_back(matjson::makeObject({
            { "name", ex.name },
            { "tags", tags },
            { "count", matjson::Value(static_cast<std::intmax_t>(ex.count)) },
            { "objects", ex.objects },
        }));
    }
    auto root = matjson::makeObject({ { "examples", arr } });
    auto res = file::writeStringSafe(folder() / "my-examples.json", root.dump());
    if (!res) log::error("Failed to save examples: {}", res.unwrapErr());
}

Result<> Library::addFromSelection(EditorUI* ui, std::string const& name) {
    this->load();
    auto sel = ui->getSelectedObjects();
    if (!sel || sel->count() == 0) return Err("Select the objects you want to save first");
    auto str = std::string(ui->copyObjects(sel, false, false));
    auto objs = parseObjects(str);
    if (objs.empty()) return Err("Could not read the selected objects");
    double cx = 0, cy = 0;
    for (auto const& o : objs) { cx += o.x; cy += o.y; }
    cx /= objs.size();
    cy /= objs.size();
    for (auto& o : objs) { o.x -= cx; o.y -= cy; }
    Example ex;
    ex.name = name.empty() ? fmt::format("example {}", m_examples.size() + 1) : name;
    ex.tags = words(ex.name);
    ex.objects = writeObjects(objs, 0, 0);
    ex.count = int(objs.size());
    // only our own file is rewritten, packs dropped into the folder stay untouched
    m_examples.push_back(std::move(ex));
    this->save();
    return Ok();
}

std::vector<Example const*> Library::match(std::string const& description, size_t max) const {
    auto want = words(description);
    std::vector<std::pair<int, Example const*>> scored;
    for (auto const& ex : m_examples) {
        // an example matches when most of its name words appear in the description
        int hits = 0;
        for (auto const& t : ex.tags) {
            if (std::find(want.begin(), want.end(), t) != want.end()) ++hits;
        }
        int needed = std::max(1, int(std::ceil(ex.tags.size() * 0.6)));
        if (hits >= needed) scored.push_back({ hits * 10 - int(ex.tags.size()), &ex });
    }
    std::stable_sort(scored.begin(), scored.end(), [](auto const& a, auto const& b) { return a.first > b.first; });
    std::vector<Example const*> out;
    for (size_t i = 0; i < scored.size() && i < max; ++i) out.push_back(scored[i].second);
    return out;
}

std::string Library::describe(Example const& ex, size_t maxObjects) {
    auto objs = parseObjects(ex.objects);
    std::string out = fmt::format("\"{}\" ({} objects; id,x,y,rot,scaleX,scaleY,color):\n", ex.name, objs.size());
    size_t n = 0;
    for (auto const& o : objs) {
        if (n++ >= maxObjects) break;
        std::string rot = "0", sx = "1", sy = "1", col = "0";
        for (auto const& [k, v] : o.kv) {
            if (k == "6") rot = v;
            else if (k == "128" || k == "32") sx = v;
            else if (k == "129") sy = v;
            else if (k == "21") col = v;
        }
        if (sy == "1" && sx != "1") sy = sx;
        out += fmt::format("{},{:.0f},{:.0f},{},{},{},{}\n", o.id, o.x, o.y, rot, sx, sy, col);
    }
    return out;
}

Result<PlaceResult> Library::place(EditorUI* ui, Example const& ex, CCPoint world) {
    auto objs = parseObjects(ex.objects);
    if (objs.empty()) return Err("Example is empty");
    auto str = writeObjects(objs, world.x, world.y - STRING_Y_OFFSET);
    return pasteAt(ui, str, world);
}

} // namespace aideco
