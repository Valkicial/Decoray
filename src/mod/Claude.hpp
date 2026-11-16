// Optional Claude API backend. The model returns primitives in the same
// format the procedural engine uses, so everything downstream is shared.
#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include "../core/DecoCore.hpp"

namespace aideco {

struct AIRequest {
    std::string description;
    bool structure = false;
    std::vector<deco::Rect> blocks;  // world rects when structure == true
    deco::Vec2 anchor;               // world cursor position for free placement
    float sizeBlocks = 4;
    int limit = 80;
    deco::Design draft;              // procedural draft used as a starting point
    std::string examples;            // user examples that match the request
};

geode::utils::web::WebRequest buildClaudeRequest(AIRequest const& req);

// Converts the API response into a design in world coordinates.
geode::Result<deco::Design> parseClaudeResponse(geode::utils::web::WebResponse const& resp, AIRequest const& req);

bool claudeConfigured();

} // namespace aideco
