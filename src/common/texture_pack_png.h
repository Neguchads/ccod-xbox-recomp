#pragma once
// Decoder exclusivo do Windows; WIC entrega BGRA sem pre-multiplicar alpha.
#include "texture_pack.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <array>
#include <limits>
#include <new>

namespace dev {
inline constexpr uint64_t kMaxPackPngBytes = 70ull * 1024 * 1024;
inline constexpr uint64_t kMaxPackPngPixelsBytes = 256ull * 1024 * 1024;
inline bool IsPackPng(std::span<const uint8_t> bytes) {
    constexpr std::array<uint8_t,8> signature{137,80,78,71,13,10,26,10};
    return bytes.size() >= signature.size() &&
           std::equal(signature.begin(), signature.end(), bytes.begin());
}
namespace png_detail {
inline constexpr auto kCrcTable = [] {
    std::array<uint32_t,256> table{};
    for (uint32_t n=0; n<256; ++n) {
        uint32_t crc=n;
        for (unsigned bit=0; bit<8; ++bit) crc=(crc>>1) ^ (0xedb88320u & (0u-(crc&1u)));
        table[n]=crc;
    }
    return table;
}();
struct ComApartment {
    HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ~ComApartment() { if (SUCCEEDED(result)) CoUninitialize(); }
};
inline uint32_t BigEndian(const uint8_t* data) {
    return (uint32_t(data[0]) << 24) | (uint32_t(data[1]) << 16) |
           (uint32_t(data[2]) << 8) | uint32_t(data[3]);
}
}
inline std::optional<TextureImage> DecodePackPng(std::span<const uint8_t> bytes) {
    if (!IsPackPng(bytes) || bytes.size() < 33 || bytes.size() > kMaxPackPngBytes)
        return std::nullopt;
    if (png_detail::BigEndian(bytes.data()+8) != 13 ||
        std::memcmp(bytes.data()+12,"IHDR",4) != 0) return std::nullopt;
    const uint32_t width = png_detail::BigEndian(bytes.data()+16);
    const uint32_t height = png_detail::BigEndian(bytes.data()+20);
    const uint64_t size = uint64_t(width)*height*4;
    if (!width || !height || width > kMaxPackSide || height > kMaxPackSide ||
        size > kMaxPackPngPixelsBytes || size > std::numeric_limits<UINT>::max())
        return std::nullopt;
    // WIC pode tolerar arquivo incompleto: exija todos os chunks ate IEND.
    bool ended = false;
    std::array<uint8_t,2> zlib{};
    size_t zlib_bytes=0;
    for (size_t offset=8; offset < bytes.size();) {
        if (bytes.size()-offset < 12) return std::nullopt;
        const uint32_t length = png_detail::BigEndian(bytes.data()+offset);
        if (uint64_t(length)+12 > bytes.size()-offset) return std::nullopt;
        uint32_t crc=0xffffffffu;
        for (size_t i=offset+4; i<offset+8+size_t(length); ++i)
            crc=(crc>>8) ^ png_detail::kCrcTable[(crc^bytes[i])&255u];
        if ((crc^0xffffffffu)!=png_detail::BigEndian(bytes.data()+offset+8+length))
            return std::nullopt;
        if (std::memcmp(bytes.data()+offset+4,"IDAT",4)==0) {
            // O cabecalho zlib pode estar dividido entre chunks IDAT, inclusive chunks vazios.
            for (size_t i=0; i<length && zlib_bytes<zlib.size(); ++i)
                zlib[zlib_bytes++]=bytes[offset+8+i];
        }
        if (std::memcmp(bytes.data()+offset+4,"IEND",4)==0) {
            ended = length==0 && offset+12==bytes.size();
            break;
        }
        offset += size_t(length)+12;
    }
    // PNG exige deflate, janela de ate 32 KiB, FCHECK valido e nenhum dicionario predefinido.
    if (!ended || zlib_bytes!=2 || (zlib[0]&15u)!=8 || (zlib[0]>>4)>7 ||
        (zlib[1]&32u) || ((uint32_t(zlib[0])<<8)|zlib[1])%31u) return std::nullopt;
    png_detail::ComApartment apartment;
    if (FAILED(apartment.result) && apartment.result != RPC_E_CHANGED_MODE)
        return std::nullopt;
    // O buffer precisa sobreviver ao stream WIC que o referencia.
    std::vector<uint8_t> encoded;
    try { encoded.assign(bytes.begin(),bytes.end()); }
    catch (const std::bad_alloc&) { return std::nullopt; }
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(factory.GetAddressOf()))) ||
        FAILED(factory->CreateStream(stream.GetAddressOf()))) return std::nullopt;
    try {
        // InitializeFromMemory exige memoria mutavel, mas o contrato recebe const.
        if (FAILED(stream->InitializeFromMemory(encoded.data(),static_cast<DWORD>(encoded.size()))) ||
            FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,
                                                     decoder.GetAddressOf())) ||
            FAILED(decoder->GetFrame(0,frame.GetAddressOf()))) return std::nullopt;
        UINT actualWidth=0,actualHeight=0;
        if (FAILED(frame->GetSize(&actualWidth,&actualHeight)) || actualWidth!=width || actualHeight!=height ||
            FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
            FAILED(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppBGRA,
                     WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))) return std::nullopt;
        TextureImage image{width,height,std::vector<uint8_t>(static_cast<size_t>(size))};
        if (FAILED(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(size),image.bgra.data())))
            return std::nullopt;
        return image;
    } catch (const std::bad_alloc&) {
        return std::nullopt;
    }
}
}
