// Offline preview: runs the same generator as the mod and dumps emitted
// objects (with resolved colors) as JSON for render.py.
//   g++ -std=c++20 -O2 -I../../src tools/preview/preview.cpp ../../src/core/*.cpp -o preview
//   ./preview "mossy castle wall" --limit 120 --blocks 6x2 > out.json
#include "core/DecoCore.hpp"
#include <cstdio>
#include <cstring>
#include <string>

using namespace deco;

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "usage: preview \"description\" [--limit N] [--seed N] [--size B] [--blocks WxH] [--pixel]\n"); return 1; }
    std::string desc = argv[1];
    Options opts;
    Target target;
    target.anchor = { 0, 0 };
    for (int i = 2; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--limit") && i + 1 < argc) opts.limit = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc) opts.seed = uint32_t(std::atoi(argv[++i]));
        else if (!std::strcmp(argv[i], "--size") && i + 1 < argc) opts.sizeBlocks = float(std::atof(argv[++i]));
        else if (!std::strcmp(argv[i], "--pixel")) opts.pixel = true;
        else if (!std::strcmp(argv[i], "--blocks") && i + 1 < argc) {
            int w = 1, h = 1;
            std::sscanf(argv[++i], "%dx%d", &w, &h);
            target.structure = true;
            for (int x = 0; x < w; ++x) for (int y = 0; y < h; ++y) target.blocks.push_back({ x * 30.f, y * 30.f, 30, 30 });
        }
        else if (!std::strcmp(argv[i], "--stairs")) {
            target.structure = true;
            for (int x = 0; x < 6; ++x) for (int y = 0; y <= x / 2; ++y) target.blocks.push_back({ x * 30.f, y * 30.f, 30, 30 });
        }
    }
    auto design = generate(desc, target, opts);
    auto objs = emit(design, defaultObjectSize, opts.limit, opts.pixel || design.pixel);
    std::printf("{\"summary\":\"%s\",\"count\":%zu,\"detail\":%.2f,\"palette\":[", design.summary.c_str(), objs.size(), design.detail);
    for (int s = 0; s < SLOT_COUNT; ++s) std::printf("%s\"%s\"", s ? "," : "", hex(design.palette.c[s]).c_str());
    std::printf("],\"blocks\":[");
    for (size_t i = 0; i < target.blocks.size(); ++i) {
        auto& b = target.blocks[i];
        std::printf("%s[%g,%g,%g,%g]", i ? "," : "", b.x, b.y, b.w, b.h);
    }
    std::printf("],\"objects\":[");
    for (size_t i = 0; i < objs.size(); ++i) {
        auto const& o = objs[i];
        std::printf("%s{\"id\":%d,\"x\":%.2f,\"y\":%.2f,\"sx\":%.4f,\"sy\":%.4f,\"r\":%.2f,\"fx\":%d,\"fy\":%d,\"slot\":%d,\"h\":%.2f,\"s\":%.3f,\"v\":%.3f,\"zl\":%d,\"zo\":%d}",
            i ? "," : "", o.id, o.x, o.y, o.scaleX, o.scaleY, o.rot, o.flipX, o.flipY, o.slot, o.hsvH, o.hsvS, o.hsvV, o.zLayer, o.zOrder);
    }
    std::printf("]}\n");
    return 0;
}
