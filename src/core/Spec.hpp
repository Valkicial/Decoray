// Description parsing: turns free text into a structured design spec.
#pragma once
#include "DecoCore.hpp"

namespace deco {

enum class Kind {
    None,
    Tree, Pine, Palm, DeadTree, Sakura, Bush, Crystal, Rock, Mushroom, Flower,
    Grass, Cactus, Island, Mountain,
    Lamp, Torch, Campfire, Candle, Lantern,
    Tower, Castle, House, City, Pillar, Arch, Portal, Pyramid, Fence, Sign,
    Cloud, Sun, Moon, Planet, Stars,
    Gear, Chain, Spikes, Waterfall, Waves, Orb,
    Heart, Ghost, Skull, Rocket, Ufo, Crown, Snowman,
    Abstract,
};

enum class Surf {
    Grass, Moss, Snow, Icicles, Lava, Crystals, Spikes, Vines, Bricks, Stripes,
    Grid, Dots, Panels, Cracks, Drips, Flowers, Outline, Glow, Stars, Waves,
    Checker, Rivets, Tiles,
};

struct SubjectReq {
    Kind kind = Kind::None;
    int count = 1;
};

struct Spec {
    std::string theme = "modern";
    Palette pal;
    std::vector<SubjectReq> subjects;
    std::vector<Surf> surfaces;
    int glow = -1;            // -1 auto, 0 off, 1 on
    float detail = 1.f;       // words like "detailed" / "simple"
    float sizeMul = 1.f;
    bool pixel = false;
    bool background = false;
    bool foreground = false;
    bool fill = false;        // "make it look like X" -> X fills the structure
    bool onTop = false;       // "with X on top"
    bool night = false;
    bool litWindows = false;
    bool roof = false;
    bool broken = false;
    bool minimal = false;
    bool hanging = false;
    bool ground = false;      // add a ground strip/island under free props
    bool themeExplicit = false;
    bool colorExplicit = false;

    bool has(Surf s) const;
    bool wantsGlow(bool fallback) const { return glow < 0 ? fallback : glow == 1; }
};

Spec parseSpec(std::string const& description);
std::string kindName(Kind k);
// Theme to use when the description only names a subject.
std::string defaultThemeFor(Kind k);
Palette themePalette(std::string const& theme);
bool isSkyKind(Kind k);
bool isSurfaceKind(Kind k);

} // namespace deco
