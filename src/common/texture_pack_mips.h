#pragma once
#include "texture_pack.h"

namespace dev {
// Gera niveis menores uma vez ao carregar BMP/PNG; DDS ja leva os niveis preparados pelo autor.
// Filtra canais independentemente: o combiner Xbox pode usar RGB mesmo quando alpha e zero.
inline bool BuildPackMipmaps(TextureImage& image) {
    if (image.format != PackFormat::Bgra8 || image.levels != 1) return true;
    if (!image.width || !image.height || image.width > kMaxPackSide || image.height > kMaxPackSide ||
        image.bgra.size() != PackLevelSize(PackFormat::Bgra8, image.width, image.height)) return false;
    constexpr size_t kMaxMipBytes = 128u << 20;
    uint64_t total = image.bgra.size();
    uint32_t w = image.width, h = image.height;
    while (w > 1 || h > 1) {
        w = std::max(1u, w / 2); h = std::max(1u, h / 2);
        total += PackLevelSize(PackFormat::Bgra8, w, h);
    }
    // Texturas gigantes continuam validas sem gerar mais memoria no host.
    if (total > kMaxMipBytes) return true;
    image.bgra.reserve(size_t(total));
    w = image.width; h = image.height;
    size_t previous = 0;
    while (w > 1 || h > 1) {
        const uint32_t nw = std::max(1u, w / 2), nh = std::max(1u, h / 2);
        const size_t next = image.bgra.size();
        image.bgra.resize(next + size_t(nw) * nh * 4);
        for (uint32_t y = 0; y < nh; ++y) for (uint32_t x = 0; x < nw; ++x) {
            const uint32_t x0 = x * w / nw, x1 = (x + 1) * w / nw;
            const uint32_t y0 = y * h / nh, y1 = (y + 1) * h / nh;
            uint32_t channels[4]{};
            for (uint32_t sy = y0; sy < y1; ++sy) for (uint32_t sx = x0; sx < x1; ++sx) {
                const size_t pixel = previous + (size_t(sy) * w + sx) * 4;
                for (size_t c = 0; c < 4; ++c) channels[c] += image.bgra[pixel + c];
            }
            const uint32_t count = (x1 - x0) * (y1 - y0);
            const size_t pixel = next + (size_t(y) * nw + x) * 4;
            for (size_t c = 0; c < 4; ++c)
                image.bgra[pixel + c] = uint8_t((channels[c] + count / 2) / count);
        }
        previous = next; w = nw; h = nh; ++image.levels;
    }
    return true;
}
}
