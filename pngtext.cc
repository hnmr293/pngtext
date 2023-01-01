#include <iostream>
#include <fstream>
#include <format>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <io.h>
#include <fcntl.h>
#endif

#include "crc32.hpp"

void show_usage(std::ostream &out, const std::string &progname)
{
    out << std::format(
               "usage: {0} -x\n"
               "       {0} -x <input>\n"
               "           extract tEXt chunk \n"
               "           (read image from stdin or file <input>, write text to stdout)\n"
               "       {0} -o <input> <output>\n"
               "           overwrite tEXt chunk \n"
               "           (read text from stdin, read image from file <input>, write image to file <output>)\n"
               "       {0} -c\n"
               "           compute input text's CRC32 value assuming it is in tEXt chunk \n"
               "           (read text from stdin, write hex string to stdout)\n"
               "example:\n"
               "    <copy>\n"
               "       $ cat source.png | ./pngtext -x | ./pngtext -o source.png dest.png\n"
               "    <delete>\n"
               "       $ printf '' | ./pngtext -o input.png output.png\n"
               "    <replace>\n"
               "       $ ./pngtext -x original.png | sed -e 's/xxx/yyy/g' | ./pngtext -o original.png output.png",
               progname)
        << std::endl;
}

#pragma pack(push, 0)
struct png_chunk_header
{
    int32_t size;
    char name[4];
};
#pragma pack(pop)

bool check_png(std::istream &in)
{
    char png_header[8];
    in.read(png_header, 8);
    if (!in || std::memcmp(png_header, "\x89PNG\x0d\x0a\x1a\x0a", 8) != 0)
    {
        std::cerr << "not PNG file" << std::endl;
        return false;
    }
    return true;
}

bool read_chunk_header(std::istream &in, size_t *chunk_size, char chunk_name[4])
{
    png_chunk_header chunk_header{0};
    in.read(reinterpret_cast<char *>(&chunk_header), sizeof(chunk_header));
    if (in.gcount() != sizeof(chunk_header))
    {
        return false;
    }

    int32_t size = __builtin_bswap32(chunk_header.size);
    // std::cerr << std::string{chunk_header.name, 4} << " " << size << std::endl;
    if (chunk_size)
        *chunk_size = size;
    if (chunk_name)
        std::memcpy(chunk_name, chunk_header.name, 4);

    return true;
}

bool extract_process(std::istream &in, std::ostream &out)
{
    if (!check_png(in))
    {
        return false;
    }

    while (in)
    {
        size_t size;
        char name[4];
        if (!read_chunk_header(in, &size, name))
        {
            return false;
        }

        if (std::memcmp(name, "tEXt", 4) != 0)
        {
            // std::cin can't be seeked
            // in.seekg(size + 4 /* crc32 */, std::ios::cur);
            char dummy;
            for (int32_t i = 0; i < size + 4 /* crc32 */; ++i)
            {
                in.read(&dummy, 1);
            }
            continue;
        }

        std::vector<char> buffer{};
        buffer.resize(size);
        in.read(buffer.data(), buffer.size());
        out.write(buffer.data(), buffer.size());
        return true;
    }

    return false;
}

std::vector<uint8_t>
read_text_from(std::istream &in)
{
    std::vector<uint8_t> text{'t', 'E', 'X', 't'};
    text.reserve(1 * 1024 * 1024); // 1 MiB
    while (in)
    {
        char v;
        in.read(&v, 1);
        if (in.gcount() == 0)
        {
            break;
        }
        text.push_back(v);
    }
    return text;
}

bool overwrite_process(std::istream &in_text, std::istream &in, std::ostream &out)
{
    auto text = read_text_from(in_text);
    uint32_t crc_new = {crc32(text)};
    // std::cout << text.size() << std::endl;
    // std::cout << std::string{reinterpret_cast<const char *>(text.data()), text.size()} << std::endl;

    if (!check_png(in))
    {
        return false;
    }

    const char PNG_HEADER[] = "\x89PNG\x0d\x0a\x1a\x0a";
    out.write(PNG_HEADER, 8);

    while (in)
    {
        size_t size;
        char name[4];
        if (!read_chunk_header(in, &size, name))
        {
            return false;
        }

        // std::cout << std::string{name, 4} << " (" << size << ")" << std::endl;
        if (std::memcmp(name, "tEXt", 4) == 0)
        {
            // treat empty input as `erase command`
            if (text.size() > 4)
            {
                uint32_t s = __builtin_bswap32(text.size() - 4);
                out.write(reinterpret_cast<const char *>(&s), 4);
                out.write(reinterpret_cast<const char *>(text.data()), text.size());
                uint32_t c = __builtin_bswap32(crc_new);
                out.write(reinterpret_cast<const char *>(&c), 4);
            }
            // skip old text chunk
            char *buf = new char[size + 4 /* crc32 */];
            in.read(buf, size + 4);
            delete[] buf;
        }
        else
        {
            uint32_t s = __builtin_bswap32(size);
            out.write(reinterpret_cast<const char *>(&s), 4);
            out.write(name, 4);

            for (size_t i = 0; i < size + 4 /* crc32 */; ++i)
            {
                char c;
                in.read(&c, 1);
                out.write(&c, 1);
            }

            if (std::memcmp(name, "IEND", 4) == 0)
            {
                break;
            }
        }
    }

    return true;
}

bool extract(const std::string &input)
{
    if (input == "-")
    {
        return extract_process(std::cin, std::cout);
    }
    else
    {
        std::ifstream in{input, std::ios::in | std::ios::binary};
        if (!in)
        {
            std::cerr << "input file '" << input << "' not found" << std::endl;
            return 1;
        }
        return extract_process(in, std::cout);
    }
}

bool overwrite(const std::string &input, const std::string &output)
{
    std::ifstream in{input, std::ios::in | std::ios::binary};
    std::ofstream out{output, std::ios::out | std::ios::binary};
    if (!in)
    {
        std::cerr << "input file '" << input << "' not found" << std::endl;
        return 1;
    }
    if (!out)
    {
        std::cerr << "can not open '" << output << "' to write" << std::endl;
        return 1;
    }
    return overwrite_process(std::cin, in, out);
}

bool compute_crc32()
{
    auto text = read_text_from(std::cin);

    union
    {
        uint32_t u32;
        uint8_t u8[4];
    } crc = {crc32(text)};

    std::cout
        << std::format("{3:02X}{2:02X}{1:02X}{0:02X}", crc.u8[0], crc.u8[1], crc.u8[2], crc.u8[3])
        << std::endl;
    return true;
}

int main(int argc, char *argv[])
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    std::string path = argc < 1 ? "" : argv[0];
    auto progname = std::filesystem::path{path}.filename().string();

    if (argc < 2)
    {
        show_usage(std::cerr, progname);
        return 1;
    }

    if (std::strcmp(argv[1], "-x") == 0)
    {
        if (argc != 2 && argc != 3)
        {
            show_usage(std::cerr, progname);
            return 1;
        }
        return extract(argc < 3 ? "-" : argv[2]) ? 0 : 1;
    }
    else if (std::strcmp(argv[1], "-o") == 0)
    {
        if (argc != 4)
        {
            show_usage(std::cerr, progname);
            return 1;
        }
        return overwrite(argv[2], argv[3]) ? 0 : 1;
    }
    else if (std::strcmp(argv[1], "-c") == 0)
    {
        if (argc != 2)
        {
            show_usage(std::cerr, progname);
            return 1;
        }
        return compute_crc32() ? 0 : 1;
    }
    else
    {
        show_usage(std::cerr, progname);
        return 1;
    }
}
