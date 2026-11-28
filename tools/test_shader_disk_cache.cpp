#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../src/common/shader_disk_cache.h"
#include <fstream>
#include <cstring>
#include <vector>

int main(int argc, char** argv) {
    using namespace cod_graphics;
    // O segundo processo le o bytecode do primeiro, sem recompilar.
    if (argc == 3 && std::strcmp(argv[1], "--read") == 0) {
        const auto blob = LoadShader(std::filesystem::path(argv[2]), 42);
        return blob && blob->GetBufferSize() > 32 ? 0 : 10;
    }
    const auto dir = std::filesystem::temp_directory_path() /
        ("cod-shader-cache-test-" + std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(dir);
    Microsoft::WRL::ComPtr<ID3DBlob> code, errors;
    const char source[] = "float4 main() : SV_Target { return float4(1,0,0,1); }";
    if (FAILED(D3DCompile(source, sizeof(source)-1, nullptr, nullptr, nullptr, "main", "ps_5_0",
        D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code, &errors))) return 1;
    if (!SaveShader(dir, 42, code.Get())) return 2;
    auto loaded = LoadShader(dir, 42);
    if (!loaded || loaded->GetBufferSize() != code->GetBufferSize() ||
        std::memcmp(loaded->GetBufferPointer(), code->GetBufferPointer(), code->GetBufferSize())) return 3;
    std::string command = "\"" + std::filesystem::absolute(argv[0]).string() + "\" --read \"" + dir.string() + "\"";
    STARTUPINFOA startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
        &startup, &process)) return 4;
    if (WaitForSingleObject(process.hProcess, 10000) != WAIT_OBJECT_0) return 5;
    DWORD exit_code = 1; GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess); CloseHandle(process.hThread);
    if (exit_code) return 6;
    auto file = ShaderFile(dir, 42);
    { std::ofstream out(file, std::ios::binary | std::ios::app); out.put('x'); }
    if (LoadShader(dir, 42)) return 7;
    if (!SaveShader(dir, 42, code.Get()) || !LoadShader(dir, 42)) return 8;
    std::filesystem::resize_file(file, 9);
    if (LoadShader(dir, 42) || LoadShader(dir, 43)) return 9;
    if (!SaveShader(dir, 42, code.Get())) return 11;
    { std::fstream out(file, std::ios::binary | std::ios::in | std::ios::out); out.seekp(8); out.put('x'); }
    if (LoadShader(dir, 42)) return 14;
    if (!SaveShader(dir, 42, code.Get())) return 15;
    { std::fstream out(file, std::ios::binary | std::ios::in | std::ios::out);
        const uint64_t excessive = kMaxShaderBytes + 1; out.seekp(16);
        out.write(reinterpret_cast<const char*>(&excessive), sizeof(excessive)); }
    if (LoadShader(dir, 42)) return 16;
    if (!SaveShader(dir, 42, code.Get())) return 17;
    { std::fstream out(file, std::ios::binary | std::ios::in | std::ios::out); out.seekp(32); out.put('x'); }
    if (LoadShader(dir, 42)) return 12;
    { std::ofstream out(dir / "not-a-directory"); out << 'x'; }
    if (SaveShader(dir / "not-a-directory", 1, code.Get()) || SaveShader({}, 1, code.Get())) return 13;
    const auto source_size = sizeof(source)-1;
    const auto key = ShaderCacheKey(source, source_size, "main", "ps_5_0", 0);
    if (key == ShaderCacheKey(source, source_size-1, "main", "ps_5_0", 0) ||
        key == ShaderCacheKey(source, source_size, "other", "ps_5_0", 0) ||
        key == ShaderCacheKey(source, source_size, "main", "ps_4_0", 0) ||
        key == ShaderCacheKey(source, source_size, "main", "ps_5_0", D3DCOMPILE_DEBUG)) return 18;
    bool hit = true;
    code.Reset(); errors.Reset();
    if (FAILED(CompileShaderCached(dir, source, source_size, "main", "ps_5_0", 0, &code, &errors, hit)) || hit) return 19;
    code.Reset(); errors.Reset();
    if (FAILED(CompileShaderCached(dir, source, source_size, "main", "ps_5_0", 0, &code, &errors, hit)) || !hit) return 20;
    std::filesystem::resize_file(ShaderFile(dir, key), 4);
    code.Reset(); errors.Reset();
    if (FAILED(CompileShaderCached(dir, source, source_size, "main", "ps_5_0", 0, &code, &errors, hit)) || hit) return 21;
    // Negacao real de acesso, restrita a pasta temporaria exclusiva deste teste.
    const auto denied = dir / "denied";
    std::filesystem::create_directory(denied);
    DWORD needed = 0;
    GetFileSecurityW(denied.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &needed);
    std::vector<uint8_t> saved_security(needed);
    if (!needed || !GetFileSecurityW(denied.c_str(), DACL_SECURITY_INFORMATION,
        reinterpret_cast<PSECURITY_DESCRIPTOR>(saved_security.data()), needed, &needed)) return 22;
    SECURITY_DESCRIPTOR descriptor{};
    ACL acl{};
    if (!InitializeSecurityDescriptor(&descriptor, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(&acl, sizeof(acl), ACL_REVISION) || !SetSecurityDescriptorDacl(&descriptor, TRUE, &acl, FALSE) ||
        !SetFileSecurityW(denied.c_str(), DACL_SECURITY_INFORMATION, &descriptor)) return 23;
    code.Reset(); errors.Reset();
    const HRESULT compiled = CompileShaderCached(denied, source, source_size, "main", "ps_5_0", 0, &code, &errors, hit);
    const bool save_failed = !SaveShader(denied, 1, code.Get());
    // Restaurar ACL antes de avaliar o teste para nao deixar pasta bloqueada.
    const bool restored = SetFileSecurityW(denied.c_str(), DACL_SECURITY_INFORMATION,
        reinterpret_cast<PSECURITY_DESCRIPTOR>(saved_security.data())) != 0;
    if (!restored) return 24;
    if (FAILED(compiled) || hit || !code || !save_failed) return 25;
    std::filesystem::remove_all(dir);
    std::puts("Shader disk cache: real HLSL, second process, corruption, repair and unwritable folder passed");
}
