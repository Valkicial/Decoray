#include "Spec.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace deco {

// ---------------------------------------------------------------- palettes --

namespace {

RGB rgbHex(uint32_t v) {
    return RGB{ uint8_t((v >> 16) & 0xFF), uint8_t((v >> 8) & 0xFF), uint8_t(v & 0xFF) };
}

RGB hsv(float h, float s, float v) {
    h = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f);
    s = std::clamp(s, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);
    float c = v * s;
    float x = c * (1 - std::fabs(std::fmod(h / 60.f, 2.f) - 1));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    if (h < 60) { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x; b = c; }
    else { r = c; b = x; }
    return RGB{ uint8_t((r + m) * 255), uint8_t((g + m) * 255), uint8_t((b + m) * 255) };
}

struct ThemeDef {
    char const* name;
    uint32_t base, mid, light, accent, glow;
    std::vector<char const*> words;
    uint32_t foliage = 0x3d8b40, bloom = 0xff6b9a, wood = 0x6d4c2f;
};

// Hand-tuned palettes. Every palette follows the same recipe used by most
// well decorated GD levels: a very dark base, one saturated body color, a
// bright highlight, one contrasting accent and a glow tint.
std::vector<ThemeDef> const& themes() {
    static std::vector<ThemeDef> t = {
        { "neon",    0x0b0820, 0x2a0f6b, 0xff3fd2, 0x29f0ff, 0x18d8ff, { "neon", "cyber", "cyberpunk", "synthwave", "retro", "vaporwave", "arcade", "rgb", "laser", "electric", "disco", "club" } , 0x1de9b6, 0xff3fd2, 0x1a1030 },
        { "lava",    0x1a0505, 0x5c1010, 0xff7b00, 0xffc300, 0xff5400, { "lava", "magma", "volcano", "volcanic", "fire", "fiery", "inferno", "molten", "ember", "embers", "burning", "hot", "flame", "flames" } , 0x4a5a1a, 0xffb703, 0x2a1a14 },
        { "ice",     0x0b2545, 0x3e7cb1, 0xe6f6ff, 0x9bd7ff, 0x7fe7ff, { "ice", "icy", "frost", "frosty", "frozen", "snow", "snowy", "winter", "glacier", "arctic", "cold", "blizzard", "christmas", "xmas" } , 0x2e6b5e, 0x9bd7ff, 0x5e4a3c },
        { "forest",  0x0f2d1c, 0x2e7d32, 0x9ccc65, 0x6d4c2f, 0xd4ff6b, { "forest", "nature", "jungle", "grass", "grassy", "moss", "mossy", "woods", "wood", "leaf", "leaves", "green", "garden", "meadow", "swamp", "plant", "plants", "overgrown" } , 0x2e7d32, 0xff6b9a },
        { "ocean",   0x03203a, 0x0a6e8a, 0x8ee3f5, 0xffd166, 0x3ff0ff, { "ocean", "sea", "water", "underwater", "aqua", "beach", "coral", "reef", "wave", "waves", "lake", "river", "aquatic" } , 0x1f8a70, 0xff7f6b },
        { "space",   0x0a0618, 0x3b1f6b, 0xc084fc, 0xf472b6, 0xa78bfa, { "space", "galaxy", "cosmic", "cosmos", "nebula", "universe", "alien", "starry", "astral", "celestial", "void" } , 0x7b2cbf, 0xf472b6, 0x3b2a4a },
        { "crystal", 0x120a2a, 0x4c2a85, 0x9be7ff, 0xff6bd6, 0x7df9ff, { "crystal", "crystals", "gem", "gems", "amethyst", "diamond", "diamonds", "magic", "magical", "fantasy", "fairy", "mystic", "mystical", "arcane", "enchanted" } , 0x3f7d6e, 0xff6bd6, 0x4a3a5a },
        { "desert",  0x4a2c17, 0xc08a4b, 0xf3d9a4, 0x2f8f83, 0xffb347, { "desert", "sand", "sandy", "egypt", "egyptian", "dune", "dunes", "canyon", "western", "wild", "dry", "savanna" } , 0x6b8e23, 0xff5d73 },
        { "candy",   0x8e3b6b, 0xff8fc7, 0xfff0f8, 0x7fdbff, 0xff9be0, { "candy", "cute", "pastel", "sweet", "kawaii", "bubblegum", "cake", "cupcake", "dessert", "sugar", "lollipop" } , 0x7bdc9b, 0xff6fb5, 0xb07a5a },
        { "spooky",  0x0d0614, 0x3b1450, 0x9b5de5, 0xff7b00, 0x7cff6b, { "spooky", "haunted", "halloween", "ghost", "ghostly", "creepy", "horror", "scary", "graveyard", "cemetery", "witch", "eerie", "cursed" } , 0x3a4a2a, 0xff7b00, 0x2b1d2e },
        { "castle",  0x1f2226, 0x5b6169, 0xa9b0b8, 0x6b4a2f, 0xffb347, { "castle", "stone", "medieval", "ruins", "ruin", "dungeon", "fortress", "kingdom", "knight", "rock", "rocky", "cave", "brick", "bricks", "ancient" } , 0x3d6b35, 0xe85d75 },
        { "gold",    0x3b2306, 0xb8860b, 0xffe08a, 0xc1121f, 0xffd166, { "gold", "golden", "royal", "temple", "treasure", "luxury", "king", "palace", "aztec", "mayan", "rich" } , 0x3f6b2a, 0xc1121f },
        { "toxic",   0x0b1a0b, 0x2f6f1f, 0xb6ff3b, 0x5b2a86, 0x9dff00, { "toxic", "acid", "radioactive", "nuclear", "slime", "poison", "poisonous", "sewer", "waste", "zombie" } , 0x4caf1f, 0xb6ff3b, 0x2e2a1a },
        { "night",   0x070b1a, 0x1b2a4a, 0x8aa2c8, 0xffd27a, 0x7aa8ff, { "night", "dark", "shadow", "shadows", "midnight", "moonlit", "evening", "noir", "gloomy" } , 0x1f4a3a, 0xffd27a, 0x2e2620 },
        { "sky",     0x2c6fbf, 0x7fb3ff, 0xffffff, 0xffd166, 0xffffff, { "sky", "clouds", "cloudy", "heaven", "heavenly", "angel", "airy", "day", "daytime", "sunny", "bright" } , 0x5cb85c, 0xff8fab },
        { "tech",    0x101820, 0x37474f, 0xb0bec5, 0xff9800, 0x00e5ff, { "tech", "factory", "metal", "metallic", "industrial", "machine", "mechanical", "robot", "robotic", "steel", "sci", "scifi", "futuristic", "lab", "circuit", "steampunk" } , 0x4a6b4a, 0xff9800, 0x4a3b30 },
        { "hell",    0x0a0000, 0x5e0b15, 0xd00000, 0x2b2b2b, 0xff1e1e, { "hell", "demon", "demonic", "blood", "bloody", "evil", "satanic", "nightmare", "extreme" } , 0x3a1a10, 0xd00000, 0x1a0a0a },
        { "sunset",  0x2b1055, 0xc2185b, 0xffb74d, 0xff7043, 0xffcc80, { "sunset", "sunrise", "dusk", "dawn", "tropical", "summer" } , 0x3e7d4a, 0xff7043 },
        { "cozy",    0x2d1b14, 0x8d5a3b, 0xf2d0a4, 0xc0392b, 0xffc46b, { "cozy", "village", "cottage", "warm", "rustic", "farm", "cabin", "wooden", "autumn", "fall" } , 0x5a7d2c, 0xe74c3c },
        { "garden",  0x1b3a1f, 0x3fa34d, 0xb5e48c, 0xff5d8f, 0xfff3b0, { "flowery", "floral", "spring", "bloom", "blooming" } , 0x3fa34d, 0xff5d8f },
        { "sakura",  0x8c3b5e, 0xf7a1c4, 0xffe3ef, 0x5a3a2e, 0xffd1e6, { "sakura", "cherry", "blossom", "japan", "japanese", "zen", "oriental" } , 0x4f7d4a, 0xf7a1c4, 0x5a3a2e },
        { "modern",  0x111827, 0x374151, 0xe5e7eb, 0x60a5fa, 0x93c5fd, { "modern", "clean", "minimal", "minimalist", "simple", "flat", "sleek", "abstract", "geometric" } , 0x4b8b5a, 0x60a5fa },
    };
    return t;
}

struct ColorWord { char const* word; float hue; int special; }; // special: 0 hue, 1 white, 2 black, 3 grey, 4 brown
std::vector<ColorWord> const& colorWords() {
    static std::vector<ColorWord> c = {
        { "red", 0, 0 }, { "crimson", 350, 0 }, { "scarlet", 5, 0 }, { "orange", 26, 0 },
        { "amber", 40, 0 }, { "yellow", 52, 0 }, { "lime", 88, 0 }, { "green", 130, 0 },
        { "emerald", 145, 0 }, { "mint", 155, 0 }, { "teal", 172, 0 }, { "turquoise", 178, 0 },
        { "cyan", 188, 0 }, { "aqua", 186, 0 }, { "blue", 218, 0 }, { "navy", 225, 0 },
        { "azure", 205, 0 }, { "indigo", 245, 0 }, { "purple", 275, 0 }, { "violet", 282, 0 },
        { "lavender", 270, 0 }, { "magenta", 305, 0 }, { "pink", 330, 0 }, { "rose", 340, 0 },
        { "white", 0, 1 }, { "black", 0, 2 }, { "grey", 0, 3 }, { "gray", 0, 3 },
        { "silver", 0, 3 }, { "brown", 25, 4 },
    };
    return c;
}

} // namespace

Palette themePalette(std::string const& theme) {
    for (auto const& t : themes()) {
        if (theme == t.name) {
            Palette p;
            p.name = t.name;
            p.c[BASE] = rgbHex(t.base);
            p.c[MID] = rgbHex(t.mid);
            p.c[LIGHT] = rgbHex(t.light);
            p.c[ACCENT] = rgbHex(t.accent);
            p.c[GLOW] = rgbHex(t.glow);
            p.c[FOLIAGE] = rgbHex(t.foliage);
            p.c[BLOOM] = rgbHex(t.bloom);
            p.c[WOOD] = rgbHex(t.wood);
            return p;
        }
    }
    return themePalette("modern");
}

// ------------------------------------------------------------------ words --

namespace {

std::vector<std::string> tokenize(std::string const& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (std::isalnum(static_cast<unsigned char>(ch))) {
            cur += char(std::tolower(static_cast<unsigned char>(ch)));
        }
        else if (!cur.empty()) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string singular(std::string const& w) {
    if (w.size() > 4 && w.ends_with("ies")) return w.substr(0, w.size() - 3) + "y";
    if (w.size() > 4 && (w.ends_with("ches") || w.ends_with("shes") || w.ends_with("xes"))) return w.substr(0, w.size() - 2);
    if (w.size() > 3 && w.ends_with("s") && !w.ends_with("ss") && !w.ends_with("us")) return w.substr(0, w.size() - 1);
    return w;
}

std::map<std::string, Kind> const& kindWords() {
    static std::map<std::string, Kind> m = {
        { "tree", Kind::Tree }, { "oak", Kind::Tree }, { "willow", Kind::Tree },
        { "pine", Kind::Pine }, { "spruce", Kind::Pine }, { "fir", Kind::Pine }, { "conifer", Kind::Pine },
        { "christmastree", Kind::Pine },
        { "palm", Kind::Palm }, { "coconut", Kind::Palm },
        { "sakura", Kind::Sakura }, { "blossom", Kind::Sakura },
        { "bush", Kind::Bush }, { "shrub", Kind::Bush }, { "hedge", Kind::Bush }, { "foliage", Kind::Bush },
        { "crystal", Kind::Crystal }, { "gem", Kind::Crystal }, { "shard", Kind::Crystal }, { "amethyst", Kind::Crystal }, { "quartz", Kind::Crystal },
        { "rock", Kind::Rock }, { "boulder", Kind::Rock }, { "stone", Kind::Rock }, { "pebble", Kind::Rock },
        { "mushroom", Kind::Mushroom }, { "shroom", Kind::Mushroom }, { "toadstool", Kind::Mushroom }, { "fungus", Kind::Mushroom },
        { "flower", Kind::Flower }, { "rose", Kind::Flower }, { "tulip", Kind::Flower }, { "daisy", Kind::Flower }, { "sunflower", Kind::Flower },
        { "grass", Kind::Grass }, { "weed", Kind::Grass }, { "reed", Kind::Grass },
        { "cactus", Kind::Cactus }, { "cacti", Kind::Cactus },
        { "island", Kind::Island }, { "floating", Kind::Island },
        { "mountain", Kind::Mountain }, { "hill", Kind::Mountain }, { "peak", Kind::Mountain }, { "volcano", Kind::Mountain },
        { "lamp", Kind::Lamp }, { "streetlight", Kind::Lamp }, { "streetlamp", Kind::Lamp }, { "lamppost", Kind::Lamp }, { "light", Kind::Lamp },
        { "torch", Kind::Torch }, { "brazier", Kind::Torch },
        { "campfire", Kind::Campfire }, { "bonfire", Kind::Campfire },
        { "candle", Kind::Candle },
        { "lantern", Kind::Lantern },
        { "tower", Kind::Tower }, { "turret", Kind::Tower }, { "lighthouse", Kind::Tower },
        { "castle", Kind::Castle }, { "fortress", Kind::Castle }, { "palace", Kind::Castle }, { "fort", Kind::Castle },
        { "house", Kind::House }, { "home", Kind::House }, { "cottage", Kind::House }, { "hut", Kind::House }, { "cabin", Kind::House },
        { "city", Kind::City }, { "skyline", Kind::City }, { "building", Kind::City }, { "skyscraper", Kind::City }, { "town", Kind::City },
        { "pillar", Kind::Pillar }, { "column", Kind::Pillar },
        { "arch", Kind::Arch }, { "gate", Kind::Arch }, { "doorway", Kind::Arch }, { "archway", Kind::Arch },
        { "portal", Kind::Portal }, { "vortex", Kind::Portal }, { "wormhole", Kind::Portal },
        { "pyramid", Kind::Pyramid },
        { "fence", Kind::Fence },
        { "sign", Kind::Sign }, { "signpost", Kind::Sign }, { "arrow", Kind::Sign },
        { "cloud", Kind::Cloud },
        { "sun", Kind::Sun },
        { "moon", Kind::Moon }, { "crescent", Kind::Moon },
        { "planet", Kind::Planet }, { "saturn", Kind::Planet }, { "earth", Kind::Planet }, { "world", Kind::Planet },
        { "star", Kind::Stars }, { "starfield", Kind::Stars }, { "sparkle", Kind::Stars }, { "twinkle", Kind::Stars },
        { "gear", Kind::Gear }, { "cog", Kind::Gear }, { "cogwheel", Kind::Gear },
        { "chain", Kind::Chain },
        { "spike", Kind::Spikes }, { "thorn", Kind::Spikes },
        { "waterfall", Kind::Waterfall }, { "cascade", Kind::Waterfall },
        { "wave", Kind::Waves },
        { "orb", Kind::Orb }, { "sphere", Kind::Orb }, { "energy", Kind::Orb }, { "core", Kind::Orb },
        { "heart", Kind::Heart },
        { "ghost", Kind::Ghost }, { "spirit", Kind::Ghost },
        { "skull", Kind::Skull }, { "skeleton", Kind::Skull },
        { "rocket", Kind::Rocket }, { "spaceship", Kind::Rocket }, { "ship", Kind::Rocket },
        { "ufo", Kind::Ufo }, { "saucer", Kind::Ufo },
        { "crown", Kind::Crown },
        { "snowman", Kind::Snowman },
        { "art", Kind::Abstract }, { "pattern", Kind::Abstract }, { "shape", Kind::Abstract }, { "design", Kind::Abstract },
    };
    return m;
}

std::map<std::string, Surf> const& surfWords() {
    static std::map<std::string, Surf> m = {
        { "grass", Surf::Grass }, { "grassy", Surf::Grass }, { "lawn", Surf::Grass },
        { "moss", Surf::Moss }, { "mossy", Surf::Moss }, { "overgrown", Surf::Moss },
        { "snow", Surf::Snow }, { "snowy", Surf::Snow }, { "frosted", Surf::Snow },
        { "icicle", Surf::Icicles },
        { "lava", Surf::Lava }, { "magma", Surf::Lava }, { "molten", Surf::Lava },
        { "crystal", Surf::Crystals }, { "crystals", Surf::Crystals }, { "gem", Surf::Crystals },
        { "spike", Surf::Spikes }, { "spiky", Surf::Spikes }, { "thorn", Surf::Spikes }, { "thorny", Surf::Spikes },
        { "vine", Surf::Vines }, { "ivy", Surf::Vines }, { "root", Surf::Vines },
        { "brick", Surf::Bricks }, { "bricks", Surf::Bricks }, { "masonry", Surf::Bricks }, { "cobblestone", Surf::Bricks },
        { "stripe", Surf::Stripes }, { "striped", Surf::Stripes },
        { "grid", Surf::Grid }, { "circuit", Surf::Grid },
        { "dot", Surf::Dots }, { "dotted", Surf::Dots }, { "polka", Surf::Dots }, { "bubble", Surf::Dots },
        { "panel", Surf::Panels }, { "plated", Surf::Panels }, { "plate", Surf::Panels },
        { "crack", Surf::Cracks }, { "cracked", Surf::Cracks }, { "broken", Surf::Cracks },
        { "drip", Surf::Drips }, { "dripping", Surf::Drips }, { "slime", Surf::Drips }, { "goo", Surf::Drips }, { "frosting", Surf::Drips },
        { "flower", Surf::Flowers }, { "flowery", Surf::Flowers }, { "floral", Surf::Flowers },
        { "outline", Surf::Outline }, { "outlined", Surf::Outline }, { "border", Surf::Outline }, { "edge", Surf::Outline },
        { "glow", Surf::Glow }, { "glowing", Surf::Glow }, { "neon", Surf::Glow },
        { "star", Surf::Stars }, { "starry", Surf::Stars },
        { "wave", Surf::Waves }, { "wavy", Surf::Waves },
        { "checker", Surf::Checker }, { "checkered", Surf::Checker }, { "checkerboard", Surf::Checker },
        { "rivet", Surf::Rivets }, { "bolt", Surf::Rivets }, { "riveted", Surf::Rivets },
        { "tile", Surf::Tiles }, { "tiled", Surf::Tiles },
    };
    return m;
}

int numberWord(std::string const& w) {
    static std::map<std::string, int> n = {
        { "a", 1 }, { "an", 1 }, { "one", 1 }, { "single", 1 }, { "two", 2 }, { "pair", 2 }, { "couple", 2 },
        { "three", 3 }, { "few", 3 }, { "some", 3 }, { "several", 4 }, { "four", 4 }, { "five", 5 },
        { "six", 6 }, { "seven", 7 }, { "eight", 8 }, { "many", 5 }, { "lots", 6 }, { "bunch", 5 },
        { "row", 5 }, { "group", 4 }, { "cluster", 4 },
    };
    if (auto it = n.find(w); it != n.end()) return it->second;
    if (!w.empty() && std::all_of(w.begin(), w.end(), ::isdigit) && w.size() < 3) return std::stoi(w);
    return 0;
}

bool contains(std::vector<std::string> const& toks, char const* w) {
    return std::find(toks.begin(), toks.end(), w) != toks.end();
}

} // namespace

bool Spec::has(Surf s) const {
    return std::find(surfaces.begin(), surfaces.end(), s) != surfaces.end();
}

bool isSkyKind(Kind k) {
    return k == Kind::Cloud || k == Kind::Sun || k == Kind::Moon || k == Kind::Planet || k == Kind::Stars || k == Kind::Ufo;
}

bool isSurfaceKind(Kind k) {
    return k == Kind::Grass || k == Kind::Spikes || k == Kind::Waves;
}

std::string defaultThemeFor(Kind k) {
    switch (k) {
        case Kind::Tree: case Kind::Bush: case Kind::Grass: case Kind::Island: case Kind::Mushroom: return "forest";
        case Kind::Pine: case Kind::Snowman: case Kind::Mountain: return "ice";
        case Kind::Palm: return "sunset";
        case Kind::DeadTree: case Kind::Ghost: case Kind::Skull: case Kind::Candle: return "spooky";
        case Kind::Sakura: return "sakura";
        case Kind::Flower: return "garden";
        case Kind::Crystal: case Kind::Portal: case Kind::Orb: return "crystal";
        case Kind::Rock: case Kind::Tower: case Kind::Castle: case Kind::Pillar: case Kind::Arch:
        case Kind::Torch: case Kind::Chain: case Kind::Fence: return "castle";
        case Kind::Lamp: case Kind::Moon: case Kind::City: return "night";
        case Kind::Campfire: case Kind::House: case Kind::Lantern: case Kind::Sign: return "cozy";
        case Kind::Cloud: return "sky";
        case Kind::Sun: return "sunset";
        case Kind::Planet: case Kind::Stars: case Kind::Rocket: case Kind::Ufo: return "space";
        case Kind::Gear: return "tech";
        case Kind::Spikes: return "hell";
        case Kind::Waterfall: case Kind::Waves: return "ocean";
        case Kind::Heart: return "candy";
        case Kind::Pyramid: case Kind::Cactus: return "desert";
        case Kind::Crown: return "gold";
        default: return "modern";
    }
}

std::string kindName(Kind k) {
    switch (k) {
        case Kind::Tree: return "tree"; case Kind::Pine: return "pine tree"; case Kind::Palm: return "palm tree";
        case Kind::DeadTree: return "dead tree"; case Kind::Sakura: return "cherry tree"; case Kind::Bush: return "bush";
        case Kind::Crystal: return "crystals"; case Kind::Rock: return "rocks"; case Kind::Mushroom: return "mushroom";
        case Kind::Flower: return "flowers"; case Kind::Grass: return "grass"; case Kind::Cactus: return "cactus";
        case Kind::Island: return "floating island"; case Kind::Mountain: return "mountains"; case Kind::Lamp: return "lamp";
        case Kind::Torch: return "torch"; case Kind::Campfire: return "campfire"; case Kind::Candle: return "candles";
        case Kind::Lantern: return "lantern"; case Kind::Tower: return "tower"; case Kind::Castle: return "castle";
        case Kind::House: return "house"; case Kind::City: return "city skyline"; case Kind::Pillar: return "pillar";
        case Kind::Arch: return "arch"; case Kind::Portal: return "portal"; case Kind::Pyramid: return "pyramid";
        case Kind::Fence: return "fence"; case Kind::Sign: return "sign"; case Kind::Cloud: return "cloud";
        case Kind::Sun: return "sun"; case Kind::Moon: return "moon"; case Kind::Planet: return "planet";
        case Kind::Stars: return "stars"; case Kind::Gear: return "gear"; case Kind::Chain: return "chain";
        case Kind::Spikes: return "spikes"; case Kind::Waterfall: return "waterfall"; case Kind::Waves: return "waves";
        case Kind::Orb: return "energy orb"; case Kind::Heart: return "heart"; case Kind::Ghost: return "ghost";
        case Kind::Skull: return "skull"; case Kind::Rocket: return "rocket"; case Kind::Ufo: return "UFO";
        case Kind::Crown: return "crown"; case Kind::Snowman: return "snowman"; case Kind::Abstract: return "abstract art";
        default: return "design";
    }
}

Spec parseSpec(std::string const& description) {
    Spec spec;
    auto toks = tokenize(description);

    // ---- theme: score every theme by keyword hits
    std::string bestTheme;
    int bestScore = 0;
    for (auto const& t : themes()) {
        int score = 0;
        for (auto const& w : toks) {
            for (auto const* kw : t.words) {
                if (w == kw || singular(w) == kw) score += 2;
            }
        }
        if (score > bestScore) { bestScore = score; bestTheme = t.name; }
    }

    // ---- flags
    auto any = [&](std::initializer_list<char const*> ws) {
        for (auto w : ws) if (contains(toks, w)) return true;
        return false;
    };
    if (any({ "glow", "glowing", "glows", "neon", "shiny", "shining", "luminous", "lit", "radiant", "lights", "emissive", "glowy" })) spec.glow = 1;
    if (any({ "noglow", "flat", "matte" }) || (contains(toks, "no") && contains(toks, "glow"))) spec.glow = 0;
    if (any({ "detailed", "intricate", "complex", "elaborate", "ornate", "fancy", "epic", "insane", "extreme", "beautiful", "amazing", "awesome", "good", "cool", "pro" })) spec.detail = 1.5f;
    if (any({ "simple", "minimal", "minimalist", "basic", "clean", "low" })) { spec.detail = 0.65f; spec.minimal = true; }
    if (any({ "big", "large", "huge", "giant", "massive", "tall", "enormous" })) spec.sizeMul = 1.5f;
    if (any({ "small", "tiny", "little", "mini", "short" })) spec.sizeMul = 0.65f;
    if (any({ "pixel", "pixelated", "8bit", "pixelart", "retro" })) spec.pixel = contains(toks, "pixel") || contains(toks, "pixelated") || contains(toks, "8bit") || contains(toks, "pixelart");
    if (any({ "background", "bg", "behind", "distant", "far" })) spec.background = true;
    if (any({ "foreground", "front" })) spec.foreground = true;
    if (any({ "night", "nighttime", "midnight", "dark" })) spec.night = true;
    if (any({ "roof", "roofs", "spire", "spires", "cone" })) spec.roof = true;
    if (any({ "broken", "ruined", "ruins", "ruin", "destroyed", "crumbling", "abandoned" })) spec.broken = true;
    if (any({ "hanging", "hang", "dangling", "ceiling" })) spec.hanging = true;
    if (any({ "ground", "platform", "hill", "floating" })) spec.ground = true;
    if (any({ "like", "into", "as", "shaped", "turn", "become", "transform" })) spec.fill = true;
    if (any({ "top", "above", "standing", "atop", "upon", "on" })) spec.onTop = true;
    if (any({ "window", "windows" }) && (spec.glow == 1 || spec.night)) spec.litWindows = true;

    // ---- colors
    std::vector<ColorWord> colors;
    for (auto const& w : toks) {
        for (auto const& cw : colorWords()) {
            if (w == cw.word) colors.push_back(cw);
        }
    }

    // ---- subjects & surfaces
    for (size_t i = 0; i < toks.size(); ++i) {
        auto const& raw = toks[i];
        auto w = singular(raw);
        bool plural = w != raw;

        // compound words
        if ((raw == "christmas" || raw == "xmas") && i + 1 < toks.size() && singular(toks[i + 1]) == "tree") {
            spec.subjects.push_back({ Kind::Pine, 1 });
            ++i;
            continue;
        }
        if ((raw == "dead" || raw == "spooky" || raw == "bare" || raw == "creepy") && i + 1 < toks.size() && singular(toks[i + 1]) == "tree") {
            spec.subjects.push_back({ Kind::DeadTree, 1 });
            ++i;
            continue;
        }
        if ((raw == "cherry" || raw == "sakura") && i + 1 < toks.size() && singular(toks[i + 1]) == "tree") {
            spec.subjects.push_back({ Kind::Sakura, 1 });
            ++i;
            continue;
        }
        if (raw == "street" && i + 1 < toks.size() && (singular(toks[i + 1]) == "light" || singular(toks[i + 1]) == "lamp")) {
            spec.subjects.push_back({ Kind::Lamp, 1 });
            ++i;
            continue;
        }

        // "<kind> tree(s)" -> one subject, plural taken from "trees"
        bool treeCompound = false;
        if (i + 1 < toks.size() && singular(toks[i + 1]) == "tree" && raw != "tree") {
            auto k2 = kindWords().find(w);
            if (k2 != kindWords().end() && (k2->second == Kind::Pine || k2->second == Kind::Palm || k2->second == Kind::Sakura || k2->second == Kind::DeadTree)) {
                treeCompound = true;
                plural = singular(toks[i + 1]) != toks[i + 1];
            }
        }

        auto sit = surfWords().find(w);
        if (sit == surfWords().end()) sit = surfWords().find(raw);
        if (sit != surfWords().end()) spec.surfaces.push_back(sit->second);

        auto kit = kindWords().find(w);
        if (kit == kindWords().end()) kit = kindWords().find(raw);
        if (kit == kindWords().end()) continue;
        Kind k = kit->second;

        // "light" is mostly an adjective unless clearly a noun ("a light", "lights")
        if (w == "light") {
            bool noun = (i > 0 && numberWord(toks[i - 1]) > 0) || plural;
            if (!noun) continue;
        }
        // "stone"/"rock" are usually materials ("stone wall"), only a subject when standalone
        if ((raw == "stone" || raw == "rock") && i + 1 < toks.size()) continue;
        // "fire" is a theme; "volcano" handled as mountain
        int count = 0;
        for (int back = 1; back <= 2 && int(i) - back >= 0; ++back) {
            int n = numberWord(toks[i - back]);
            if (n > 0) { count = n; break; }
        }
        if (count == 0) count = plural ? 3 : 1;
        if (treeCompound) ++i;
        if (k == Kind::Stars || k == Kind::Grass || k == Kind::Spikes || k == Kind::Waves || k == Kind::City || k == Kind::Mountain) count = 1;

        // merge duplicates
        bool merged = false;
        for (auto& s : spec.subjects) {
            if (s.kind == k) { s.count = std::max(s.count, count); merged = true; }
        }
        if (!merged) spec.subjects.push_back({ k, std::min(count, 8) });
    }

    // a "volcano" is a mountain in a lava theme
    if (contains(toks, "volcano") && bestTheme.empty()) bestTheme = "lava";

    if (!bestTheme.empty()) {
        spec.theme = bestTheme;
        spec.themeExplicit = true;
    }
    else if (!spec.subjects.empty()) {
        spec.theme = defaultThemeFor(spec.subjects.front().kind);
    }
    if (spec.night && !spec.themeExplicit) spec.theme = "night";

    spec.pal = themePalette(spec.theme);

    // explicit colors re-tint the palette while keeping the theme's tonal structure
    if (!colors.empty()) {
        spec.colorExplicit = true;
        auto const& c0 = colors[0];
        if (c0.special == 0 || c0.special == 4) {
            float h = c0.hue;
            float sat = c0.special == 4 ? 0.55f : 0.75f;
            spec.pal.c[BASE] = hsv(h, sat * 0.9f, 0.14f);
            spec.pal.c[MID] = hsv(h, sat, c0.special == 4 ? 0.45f : 0.62f);
            spec.pal.c[LIGHT] = hsv(h, sat * 0.4f, 1.f);
            spec.pal.c[GLOW] = hsv(h, 0.7f, 1.f);
            if (colors.size() < 2) spec.pal.c[ACCENT] = hsv(h + 180.f, 0.6f, 0.95f);
        }
        else if (c0.special == 1) {
            spec.pal.c[BASE] = rgbHex(0x8a919c);
            spec.pal.c[MID] = rgbHex(0xdfe4ea);
            spec.pal.c[LIGHT] = rgbHex(0xffffff);
        }
        else if (c0.special == 2) {
            spec.pal.c[BASE] = rgbHex(0x040404);
            spec.pal.c[MID] = rgbHex(0x17171c);
            spec.pal.c[LIGHT] = rgbHex(0x5c5f6b);
        }
        else if (c0.special == 3) {
            spec.pal.c[BASE] = rgbHex(0x2a2d33);
            spec.pal.c[MID] = rgbHex(0x7a7f88);
            spec.pal.c[LIGHT] = rgbHex(0xd7dbe0);
        }
        if (colors.size() >= 2) {
            auto const& c1 = colors[1];
            if (c1.special == 0 || c1.special == 4) {
                spec.pal.c[ACCENT] = hsv(c1.hue, c1.special == 4 ? 0.55f : 0.75f, 0.95f);
                if (c0.special != 0) spec.pal.c[GLOW] = hsv(c1.hue, 0.7f, 1.f);
            }
            else if (c1.special == 1) spec.pal.c[ACCENT] = rgbHex(0xffffff);
            else if (c1.special == 2) spec.pal.c[ACCENT] = rgbHex(0x101010);
        }
        spec.pal.name += "+color";
    }

    // theme implied surfaces
    if (spec.theme == "neon" && !spec.has(Surf::Glow)) spec.surfaces.push_back(Surf::Glow);
    return spec;
}

Palette paletteFor(std::string const& description) {
    return parseSpec(description).pal;
}

} // namespace deco
