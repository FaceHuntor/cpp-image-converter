#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    char signature[2];
    uint32_t full_size;
    char reserved[4] {0};
    uint32_t data_indent;

    // поля заголовка Bitmap File Header
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t info_header_size;
    int32_t image_width;
    int32_t image_height;
    uint16_t plane_count;
    uint16_t bits_per_pixel;
    uint32_t compression_type;
    uint32_t data_size;
    int32_t hor_resolution;
    int32_t vert_resolution;
    int32_t colors_count;
    int32_t meaningful_color_count;
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool ReadHeaders(ifstream& ifs, BitmapFileHeader& file_header, BitmapInfoHeader& info_header) {
    ifs.readsome(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if(ifs.bad()
       || string_view(file_header.signature, 2) != "BM"sv
       || file_header.data_indent != sizeof(file_header) + sizeof(info_header))
    {
        return false;
    }
    ifs.readsome(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if(ifs.bad()
       || info_header.info_header_size != sizeof(info_header)
       || info_header.plane_count != 1
       || info_header.bits_per_pixel != 24
       || info_header.compression_type != 0
       || file_header.full_size - file_header.data_indent != info_header.data_size)
    {
        return false;
    }

    return true;
}

void ReadHeaders(const Image& image, BitmapFileHeader& file_header, BitmapInfoHeader& info_header) {
    auto stride = GetBMPStride(image.GetWidth());

    info_header.image_width = image.GetWidth();
    info_header.image_height = image.GetHeight();
    info_header.info_header_size = sizeof(info_header);
    info_header.plane_count = 1;
    info_header.bits_per_pixel = 24;
    info_header.compression_type = 0;
    info_header.data_size = stride * info_header.image_height;
    info_header.hor_resolution = 11811;
    info_header.vert_resolution = 11811;
    info_header.colors_count = 0;
    info_header.meaningful_color_count = 0x1000000;

    file_header.signature[0] = 'B';
    file_header.signature[1] = 'M';
    file_header.data_indent = sizeof(file_header) + sizeof(info_header);
    file_header.full_size = file_header.data_indent + info_header.data_size;
}

bool WriteHeaders(ofstream& ofs, BitmapFileHeader& file_header, BitmapInfoHeader& info_header) {
    ofs.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    ofs.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    return ofs.good();
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image)
{
    ofstream ofs(file, ios::binary);
    BitmapFileHeader file_header{};
    BitmapInfoHeader info_header{};

    ReadHeaders(image, file_header, info_header);
    if(!WriteHeaders(ofs, file_header, info_header)) {
        return false;
    }
    auto stride = GetBMPStride(info_header.image_width);

    std::vector<char> buff(stride);

    for (int y = info_header.image_height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < info_header.image_width; ++x) {
            buff[x * 3    ] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buff.data(), stride);
    }

    return ofs.good();
}

// напишите эту функцию
Image LoadBMP(const Path& file)
{
    ifstream ifs(file, ios::binary);
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;
    if(!ReadHeaders(ifs, file_header, info_header)) {
        return {};
    }

    Image result(info_header.image_width, info_header.image_height, img_lib::Color::Black());
    auto stride = GetBMPStride(info_header.image_width);
    std::vector<char> buff(stride);

    for (int y = info_header.image_height - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), stride);

        for (int x = 0; x < info_header.image_width; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3    ]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib