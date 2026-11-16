#pragma once
#include "Canvas.hpp"

namespace deco {

// Draws a subject with its base at the canvas origin, ~S units tall.
void drawProp(Canvas& c, Kind k, float S);
float propWidth(Kind k, float S);
void drawMoonCrescent(Canvas& c, float S);
void drawFlame(Canvas& c, float x, float y, float s);

// Decorates a block structure (cells on the 30-unit grid).
void decorateStructure(Canvas& c, std::vector<Rect> const& blocks, std::string& styleName);

struct StructureInfo {
    Rect bbox;
    std::vector<Rect> fills;          // merged rectangles covering all cells
    struct Seg { float x0, x1, y; };  // horizontal edge
    struct VSeg { float y0, y1, x; }; // vertical edge
    std::vector<Seg> tops, bottoms;
    std::vector<VSeg> lefts, rights;
    int cells = 0;
};
StructureInfo analyzeStructure(std::vector<Rect> const& blocks);

} // namespace deco
