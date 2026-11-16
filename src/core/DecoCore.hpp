// DecoCore: platform-independent design engine for AI Deco.
// No Geode / cocos2d dependencies here, so it can be compiled and tested
// on any machine (see tools/preview).
#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace deco {

constexpr float BLOCK = 30.f;   // one GD grid block in world units
constexpr float PI = 3.14159265358979f;

struct Vec2 {
    float x = 0, y = 0;
};

// Axis aligned rectangle, (x, y) is the bottom-left corner.
struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    float cx() const { return x + w / 2; }
    float cy() const { return y + h / 2; }
    float right() const { return x + w; }
    float top() const { return y + h; }
};

struct RGB {
    uint8_t r = 255, g = 255, b = 255;
};

// Palette slots. Each slot becomes one GD color channel.
enum Slot : int {
    BASE = 0,   // darkest tone: silhouettes, shadows, mortar
    MID = 1,    // main body color
    LIGHT = 2,  // highlights, rims, snow caps
    ACCENT = 3, // second hue: wood, flowers, windows, gems
    GLOW = 4,   // emissive, uses additive blending
    FOLIAGE = 5,// plants: leaves, grass, moss, vines (green-ish in every theme)
    BLOOM = 6,  // flowers, berries, blossoms
    WOOD = 7,   // trunks, planks, handles, posts
    SLOT_COUNT = 8,
};

struct Palette {
    std::string name;
    RGB c[SLOT_COUNT];
};

// Primitive shapes. Each primitive is emitted as exactly one GD object
// (except in pixel mode, where solids are rasterised into squares).
enum class Shape : uint8_t {
    Rect,      // 211  colored square, scaled freely
    Slope,     // 693  decorative colored slope (right angle bottom-right)
    Circle,    // 3621 / 3637
    Ring,      // 3622 / 3636 hollow circle
    Triangle,  // 3628 isosceles, pointing up
    Star,      // 3625
    Diamond,   // 3624
    Hexagon,   // 3629
    Heart,     // 3623
    Glow,      // 1888 large glow orb (radial, additive)
    EdgeGlow,  // 1011 straight large glow: bright edge at the bottom, fades up
    Raw,       // any object id (used by AI output / examples)
};

// Render layers, mapped to GD Z layers when emitted.
enum class Layer : int8_t {
    BG2 = 0,   // B3: far background
    BG = 1,    // B2: background
    BACK = 2,  // B1: directly behind the main layer (glows)
    MAIN = 3,  // T1: main deco layer
    FRONT = 4, // T2: details on top
    FX = 5,    // T3: front glow / sparkles
};

struct Prim {
    Shape shape = Shape::Rect;
    float x = 0, y = 0;     // center
    float w = BLOCK, h = BLOCK;
    float rot = 0;          // degrees, clockwise (GD convention)
    bool flipX = false, flipY = false;
    int slot = MID;
    float shade = 1.f;      // HSV value multiplier
    float sat = 1.f;        // HSV saturation multiplier
    float hue = 0.f;        // HSV hue shift (degrees)
    Layer layer = Layer::MAIN;
    int order = 0;          // draw order inside the layer (filled automatically)
    int pri = 0;            // 0 = essential silhouette ... 4 = optional sparkle
    int rawId = 0;          // for Shape::Raw
};

struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
    uint32_t next() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return s;
    }
    float f() { return (next() & 0xFFFFFF) / float(0x1000000); }
    float range(float a, float b) { return a + (b - a) * f(); }
    int irange(int a, int b) { return a + int(next() % uint32_t(b - a + 1)); }
    bool chance(float p) { return f() < p; }
};

// What the generator decorates.
struct Target {
    // true when the user hovered/selected blocks: we decorate those blocks.
    bool structure = false;
    std::vector<Rect> blocks;    // world rects of the hovered structure
    Vec2 anchor;                 // world position of the cursor
};

struct Options {
    int limit = 80;              // object limit slider
    uint32_t seed = 1;
    bool pixel = false;          // pixel-art rasterisation
    float sizeBlocks = 4.f;      // free-placement size
};

struct Design {
    std::vector<Prim> prims;     // world coordinates
    Palette palette;
    std::string summary;
    std::vector<std::string> notes;
    bool pixel = false;          // description asked for pixel art
    float detail = 1.f;          // detail level that was used
};

// ---- Generation -------------------------------------------------------------

Design generate(std::string const& description, Target const& target, Options const& opts);

// Enforce the object limit by dropping optional detail first.
void enforceLimit(std::vector<Prim>& prims, int limit);

// Theme palette lookup (used by the AI path when it does not provide colors).
Palette paletteFor(std::string const& description);

// ---- Emission ---------------------------------------------------------------

struct GDObject {
    int id = 211;
    double x = 0, y = 0;
    float scaleX = 1, scaleY = 1;
    float rot = 0;
    bool flipX = false, flipY = false;
    int slot = MID;
    bool hsv = false;
    float hsvH = 0, hsvS = 1, hsvV = 1;
    int zLayer = 5;
    int zOrder = 0;
};

// Returns the unscaled content size of an object id.
using SizeFn = std::function<Vec2(int)>;
Vec2 defaultObjectSize(int id);
int objectIdFor(Prim const& p);

std::vector<GDObject> emit(Design const& design, SizeFn const& sizeOf, int limit, bool pixel);

// GD level-string for a set of objects. channels[slot] = color channel id.
std::string toObjectString(
    std::vector<GDObject> const& objs, int const channels[SLOT_COUNT],
    float yOffset, int editorLayer, bool noTouch, bool dontFade, bool dontEnter
);

// JSON helpers (used for the AI prompt and the preview tool).
std::string primsToJson(std::vector<Prim> const& prims, Vec2 origin);
std::string hex(RGB c);
RGB parseHex(std::string const& s, RGB fallback);
std::string shapeName(Shape s);
bool shapeFromName(std::string const& n, Shape& out);
std::string designRulesText();
std::string objectCatalogText();

} // namespace deco
