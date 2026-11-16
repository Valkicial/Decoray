// Glue between the portable design engine and Geometry Dash.
#pragma once

#include <Geode/Geode.hpp>
#include "../core/DecoCore.hpp"

namespace aideco {

// Real (unscaled) sprite size of an object id, measured once and cached.
deco::Vec2 measuredSize(int id);

// World rect of an object (hitbox, or sprite bounds for decoration).
cocos2d::CCRect objectRect(GameObject* obj);

// Flood-fills touching solid blocks starting at `seed`.
std::vector<deco::Rect> gatherStructure(LevelEditorLayer* lel, GameObject* seed, size_t maxObjects);
std::vector<deco::Rect> rectsOf(cocos2d::CCArray* objects);

struct PlaceSettings {
    int limit = 80;
    bool pixel = false;
    bool autoColors = true;
    bool noTouch = true;
    bool dontFade = false;
    bool dontEnter = false;
};

struct PlaceResult {
    geode::Ref<cocos2d::CCArray> objects;
    std::string colorInfo;
    int count = 0;
};

// Pastes a design into the editor (one undo step) and selects it.
geode::Result<PlaceResult> placeDesign(EditorUI* ui, deco::Design const& design, PlaceSettings const& settings);

// Deletes objects through the editor so it stays undoable.
void deleteObjects(EditorUI* ui, cocos2d::CCArray* objects);

// ---- user example library ("learn from examples") --------------------------

struct Example {
    std::string name;
    std::vector<std::string> tags;
    std::string objects;       // GD object string, positions relative to center
    int count = 0;
};

class Library {
public:
    static Library& get();
    void load();
    geode::Result<> addFromSelection(EditorUI* ui, std::string const& name);
    std::vector<Example const*> match(std::string const& description, size_t max) const;
    size_t size() const { return m_examples.size(); }
    std::filesystem::path folder() const;
    // compact text for the AI prompt
    static std::string describe(Example const& ex, size_t maxObjects);
    // paste an example centered at `world`
    geode::Result<PlaceResult> place(EditorUI* ui, Example const& ex, cocos2d::CCPoint world);

private:
    std::vector<Example> m_examples;
    bool m_loaded = false;
    void save() const;
};

} // namespace aideco
