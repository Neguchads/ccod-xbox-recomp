#include "../src/common/texture_pack_mips.h"
#include <cstdio>

int main() {
    using namespace dev;
    TextureImage opaque{2, 2, {0,0,0,255, 100,100,100,255, 200,200,200,255, 100,100,100,255}};
    const auto base = opaque.bgra;
    if (!BuildPackMipmaps(opaque) || opaque.levels != 2 || opaque.bgra.size() != 20 ||
        !std::equal(base.begin(), base.end(), opaque.bgra.begin()) ||
        opaque.bgra[16] != 100 || opaque.bgra[19] != 255) return 1;
    // Canais independentes: o jogo pode ler RGB sem usar o alpha.
    TextureImage alpha{2, 2, {0,0,255,255, 255,0,0,0, 255,0,0,0, 255,0,0,0}};
    if (!BuildPackMipmaps(alpha) || alpha.bgra[16] != 191 || alpha.bgra[18] != 64 || alpha.bgra[19] != 64) return 2;
    TextureImage odd{3, 3, std::vector<uint8_t>(36, 255)};
    // O ultimo pixel de dimensao impar precisa participar da media.
    for (size_t i = 0; i < 36; i += 4) odd.bgra[i] = 0;
    odd.bgra[32] = 252;
    if (!BuildPackMipmaps(odd) || odd.levels != 2 || odd.bgra.size() != 40 || odd.bgra[36] != 28) return 3;
    TextureImage line{1, 8, std::vector<uint8_t>(32, 180)};
    if (!BuildPackMipmaps(line) || line.levels != 4 || line.bgra.size() != 60) return 4;
    const auto complete = line.bgra;
    if (!BuildPackMipmaps(line) || line.bgra != complete) return 5;
    TextureImage transparent{2, 2, std::vector<uint8_t>(16, 0)};
    if (!BuildPackMipmaps(transparent) || transparent.bgra[16] != 0 || transparent.bgra[19] != 0) return 6;
    TextureImage bc7{4, 4, std::vector<uint8_t>(16, 17), PackFormat::Bc7};
    if (!BuildPackMipmaps(bc7) || bc7.bgra.size() != 16 || bc7.levels != 1) return 7;
    TextureImage invalid{2, 2, {1,2,3,4}};
    if (BuildPackMipmaps(invalid) || invalid.bgra.size() != 4 || BuildPackMipmaps(invalid = {})) return 8;
    // A base ja ocupa128MiB: a cadeia completa ultrapassaria o teto, entao nao aloca nem altera.
    TextureImage large{8192,4096,std::vector<uint8_t>(128u<<20,77)};
    const auto* pixels=large.bgra.data();
    if (!BuildPackMipmaps(large) || large.levels!=1 || large.bgra.size()!=(128u<<20) ||
        large.bgra.data()!=pixels || large.bgra.front()!=77 || large.bgra.back()!=77) return 9;
    std::puts("Pack mipmaps: base unchanged, independent channels, odd dimensions, 1D, idempotence and BC7 preservation passed");
}
