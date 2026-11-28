#pragma once
// Adaptado do cache LoadShader/SaveShader do patch MIT de jmafort/CastlevaniaCOD-PC,
// revisao 00408f145232c2593fef47499979adac5024ba98. Avisos em LICENSES/graphics-cache-MIT.txt.
#include <windows.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <cstdio>
#include <cstdint>
#include <string>

namespace cod_graphics {
inline constexpr uint64_t kShaderCacheVersion = 1;
inline constexpr uint64_t kMaxShaderBytes = 16u << 20;
inline uint64_t ShaderBytesHash(const void* data, size_t size) {
    uint64_t hash = 14695981039346656037ull;
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; }
    return hash;
}
inline std::filesystem::path ShaderFile(const std::filesystem::path& dir, uint64_t key) {
    char name[32];
    std::snprintf(name, sizeof(name), "%016llx.cso", static_cast<unsigned long long>(key));
    return dir / name;
}
inline Microsoft::WRL::ComPtr<ID3DBlob> LoadShader(const std::filesystem::path& dir, uint64_t key) {
    if (dir.empty()) return nullptr;
    FILE* file = nullptr;
    if (_wfopen_s(&file, ShaderFile(dir, key).c_str(), L"rb") != 0 || !file) return nullptr;
    // Versao, chave, tamanho e hash: arquivo errado, cortado ou alterado vira cache miss.
    uint64_t head[4]{};
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    bool ok = std::fread(head, sizeof(head), 1, file) == 1 && head[0] == kShaderCacheVersion &&
        head[1] == key && head[2] && head[2] <= kMaxShaderBytes &&
        SUCCEEDED(D3DCreateBlob(SIZE_T(head[2]), &blob)) &&
        std::fread(blob->GetBufferPointer(), 1, size_t(head[2]), file) == head[2] &&
        ShaderBytesHash(blob->GetBufferPointer(), size_t(head[2])) == head[3] && std::fgetc(file) == EOF;
    ok = std::ferror(file) == 0 && ok;
    std::fclose(file);
    return ok ? blob : nullptr;
}
inline bool SaveShader(const std::filesystem::path& dir, uint64_t key, ID3DBlob* blob) {
    if (dir.empty() || !blob || !blob->GetBufferSize() || blob->GetBufferSize() > kMaxShaderBytes) return false;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) return false;
    const auto file = ShaderFile(dir, key);
    auto temp = file;
    temp += "." + std::to_string(GetCurrentProcessId()) + "." + std::to_string(GetCurrentThreadId()) + ".tmp";
    FILE* stream = nullptr;
    if (_wfopen_s(&stream, temp.c_str(), L"wb") != 0 || !stream) return false;
    const uint64_t head[4]{kShaderCacheVersion, key, blob->GetBufferSize(),
        ShaderBytesHash(blob->GetBufferPointer(), blob->GetBufferSize())};
    bool ok = std::fwrite(head, sizeof(head), 1, stream) == 1 &&
        std::fwrite(blob->GetBufferPointer(), 1, blob->GetBufferSize(), stream) == blob->GetBufferSize();
    ok = std::fclose(stream) == 0 && ok;
    // Windows: substitui inclusive um cache corrompido, sem deixar arquivo parcial visivel.
    if (ok) ok = MoveFileExW(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!ok) std::filesystem::remove(temp, ec);
    return ok;
}
inline uint64_t ShaderCacheKey(const char* source, size_t size, const char* entry, const char* profile, uint32_t flags) {
    // Separadores e tamanhos evitam ambiguidade entre fonte, entrada e perfil.
    std::string input(source, size);
    input.push_back('\0'); input.append(entry); input.push_back('\0'); input.append(profile);
    input.append(reinterpret_cast<const char*>(&flags), sizeof(flags));
    return ShaderBytesHash(input.data(), input.size());
}
inline HRESULT CompileShaderCached(const std::filesystem::path& dir, const char* source, size_t size,
    const char* entry, const char* profile, uint32_t flags, ID3DBlob** code, ID3DBlob** errors, bool& hit) {
    hit = false;
    const auto key = ShaderCacheKey(source, size, entry, profile, flags);
    if (auto cached = LoadShader(dir, key)) {
        *code = cached.Detach();
        if (errors) *errors = nullptr;
        hit = true;
        return S_OK;
    }
    const HRESULT result = D3DCompile(source, size, nullptr, nullptr, nullptr, entry, profile, flags, 0, code, errors);
    if (SUCCEEDED(result)) SaveShader(dir, key, *code);
    return result;
}
}
