#include "../src/common/texture_pack_png.h"
#include "../src/common/texture_pack_mips.h"
#include "../src/common/texture_lookup_rules.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <source_location>
using Microsoft::WRL::ComPtr;
static void Check(bool condition, const std::source_location where=std::source_location::current()) {
    if (!condition) { std::fprintf(stderr,"PNG test failed at line %u\n",where.line()); std::abort(); }
}
static std::vector<uint8_t> Encode(UINT width, UINT height, std::vector<uint8_t> pixels) {
    ComPtr<IWICImagingFactory> factory;
    Check(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(factory.GetAddressOf()))));
    ComPtr<IStream> stream;
    Check(SUCCEEDED(CreateStreamOnHGlobal(nullptr,TRUE,stream.GetAddressOf())));
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    Check(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.GetAddressOf())));
    Check(SUCCEEDED(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache)));
    Check(SUCCEEDED(encoder->CreateNewFrame(frame.GetAddressOf(),nullptr)));
    Check(SUCCEEDED(frame->Initialize(nullptr)));
    Check(SUCCEEDED(frame->SetSize(width,height)));
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
    Check(SUCCEEDED(frame->SetPixelFormat(&format)));
    Check(IsEqualGUID(format,GUID_WICPixelFormat32bppBGRA));
    Check(SUCCEEDED(frame->WritePixels(height,width*4,static_cast<UINT>(pixels.size()),pixels.data())));
    Check(SUCCEEDED(frame->Commit()));
    Check(SUCCEEDED(encoder->Commit()));
    STATSTG stat{};
    Check(SUCCEEDED(stream->Stat(&stat,STATFLAG_NONAME)));
    LARGE_INTEGER zero{};
    Check(SUCCEEDED(stream->Seek(zero,STREAM_SEEK_SET,nullptr)));
    std::vector<uint8_t> result(static_cast<size_t>(stat.cbSize.QuadPart));
    ULONG read=0;
    Check(SUCCEEDED(stream->Read(result.data(),static_cast<ULONG>(result.size()),&read)));
    Check(read==result.size());
    return result;
}
static void Put(std::vector<uint8_t>& bytes,size_t offset,uint32_t value) {
    for(unsigned i=0;i<4;++i) bytes[offset+i]=uint8_t(value>>(24-i*8));
}
static void AddChunk(std::vector<uint8_t>& output, const char* type, std::span<const uint8_t> data) {
    const size_t start=output.size();
    output.resize(start+12+data.size());
    Put(output,start,uint32_t(data.size()));
    std::memcpy(output.data()+start+4,type,4);
    std::copy(data.begin(),data.end(),output.begin()+ptrdiff_t(start+8));
    uint32_t crc=0xffffffffu;
    for (size_t i=start+4;i<start+8+data.size();++i) {
        crc^=output[i];
        for (unsigned bit=0;bit<8;++bit) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1u)));
    }
    Put(output,start+8+data.size(),crc^0xffffffffu);
}
int main(int argc, char** argv) {
    Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)));
    const std::vector<uint8_t> pixels{17,33,201,128, 91,72,13,255, 0,0,0,0, 222,111,44,64};
    auto png=Encode(2,2,pixels);
    auto decoded=dev::DecodePackPng(png);
    Check(decoded && decoded->width==2 && decoded->height==2 && decoded->levels==1 &&
          decoded->format==dev::PackFormat::Bgra8 && decoded->bgra==pixels);
    Check(dev::BuildPackMipmaps(*decoded));
    Check(decoded->levels==2 && decoded->bgra.size()==20);
    Check(std::equal(pixels.begin(),pixels.end(),decoded->bgra.begin()));
    const auto mipBytes=decoded->bgra;
    Check(dev::BuildPackMipmaps(*decoded) && decoded->bgra==mipBytes);
    auto tiny=Encode(1,1,{5,10,20,30});
    Check(dev::DecodePackPng(tiny).has_value());
    for(size_t length=0;length<png.size();++length)
        Check(!dev::DecodePackPng(std::span<const uint8_t>(png.data(),length)));
    auto invalid=png; invalid[0]=0; Check(!dev::DecodePackPng(invalid));
    invalid=png; invalid[12]='X'; Check(!dev::DecodePackPng(invalid));
    invalid=png; Put(invalid,8,0xffffffff); Check(!dev::DecodePackPng(invalid));
    for(uint32_t width : {0u,dev::kMaxPackSide+1,0xffffffffu}) {
        invalid=png; Put(invalid,16,width); Check(!dev::DecodePackPng(invalid));
        invalid=png; Put(invalid,20,width); Check(!dev::DecodePackPng(invalid));
    }
    invalid=png; Put(invalid,16,dev::kMaxPackSide); Put(invalid,20,dev::kMaxPackSide);
    Check(!dev::DecodePackPng(invalid));
    invalid=png; invalid.back()^=255; Check(!dev::DecodePackPng(invalid));
    // Fixture PNG indexado 2x1: paleta vermelho/verde, tRNS opaco/transparente.
    const std::vector<uint8_t> indexed{137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,2,0,0,0,1,
        8,3,0,0,0,195,252,143,184,0,0,0,6,80,76,84,69,255,0,0,0,255,0,210,135,239,113,
        0,0,0,2,116,82,78,83,255,0,229,183,48,74,0,0,0,11,73,68,65,84,120,156,99,96,96,4,
        0,0,4,0,2,191,122,63,74,0,0,0,0,73,69,78,68,174,66,96,130};
    auto palette = dev::DecodePackPng(indexed);
    Check(palette && palette->width == 2 && palette->height == 1 &&
        palette->bgra == std::vector<uint8_t>({0,0,255,255, 0,255,0,0}));
    // O zlib pode comecar num IDAT de um byte, seguido por IDAT vazio e pelo restante.
    std::vector<uint8_t> split(indexed.begin(),indexed.begin()+65);
    AddChunk(split,"IDAT",std::span(indexed).subspan(73,1));
    AddChunk(split,"IDAT",{});
    AddChunk(split,"IDAT",std::span(indexed).subspan(74,10));
    AddChunk(split,"IEND",{});
    auto split_image=dev::DecodePackPng(split);
    Check(split_image && split_image->bgra==palette->bgra);
    // IDAT com CRC incorreto e, depois, zlib corrompido mas CRC valido.
    invalid = indexed; invalid[73] = 0;
    Check(!dev::DecodePackPng(invalid));
    uint32_t crc=0xffffffffu;
    for (size_t i=69; i<84; ++i) {
        crc ^= invalid[i];
        for (unsigned bit=0; bit<8; ++bit) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1u)));
    }
    Put(invalid, 84, crc ^ 0xffffffffu);
    Check(!dev::DecodePackPng(invalid));
    // O decoder nao pode retirar a inicializacao COM que pertence ao chamador.
    Check(CoInitializeEx(nullptr,COINIT_MULTITHREADED)==S_FALSE);
    CoUninitialize();
    CoUninitialize();
    // Tambem inicializa COM sozinho e aceita o apartment STA preexistente.
    Check(dev::DecodePackPng(png).has_value());
    Check(SUCCEEDED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED)));
    decoded=dev::DecodePackPng(png);
    Check(decoded && decoded->bgra==pixels);
    Check(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED)==S_FALSE);
    CoUninitialize();
    CoUninitialize();
    std::puts("PNG: pixels/alpha, small image, truncation, invalid input, limits, COM MTA/STA passed");
    // Opcional: conferir os packs reais sem abrir o jogo nem alterar arquivos.
    if (argc > 1) {
        unsigned images = 0, generated = 0;
        for (const auto& file : std::filesystem::recursive_directory_iterator(argv[1])) {
            if (!file.is_regular_file() || !texture_lookup::ParseTextureFile(file.path().filename().string())) continue;
            std::ifstream stream(file.path(), std::ios::binary);
            Check(bool(stream));
            const auto length=file.file_size();
            Check(length<=dev::kMaxPackPngBytes);
            std::vector<uint8_t> bytes(static_cast<size_t>(length));
            Check(bool(stream.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))));
            auto image = dev::IsPackPng(bytes) ? dev::DecodePackPng(bytes) : dev::DecodePackTexture(bytes);
            if (!image) { std::fprintf(stderr, "Invalid pack texture: %s\n", file.path().filename().string().c_str()); return 1; }
            const auto levels = image->levels;
            Check(dev::BuildPackMipmaps(*image));
            generated += image->levels > levels;
            ++images;
        }
        Check(images > 0);
        std::printf("Real packs: %u textures decoded, %u mip chains generated; source files unchanged\n", images, generated);
    }
}
