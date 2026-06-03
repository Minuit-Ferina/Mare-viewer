#include "mare_vulkan_test_support.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>

namespace mare_vulkan_test
{
using U8 = std::uint8_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;
using F64 = double;

bool write_rgb_ppm_file(
    const std::string& path,
    U32 width,
    U32 height,
    const std::vector<U8>& rgb_pixels)
{
    if (path.empty())
    {
        return true;
    }

    const size_t expected_size =
        static_cast<size_t>(width) * static_cast<size_t>(height) * 3U;
    if (rgb_pixels.size() != expected_size)
    {
        std::cerr
            << "Invalid PPM pixel buffer for "
            << path
            << ": expected "
            << expected_size
            << " bytes, got "
            << rgb_pixels.size()
            << ".\n";
        return false;
    }

    std::ofstream output(path, std::ios::binary);
    if (!output.is_open())
    {
        std::cerr << "Unable to open reference PPM file: " << path << ".\n";
        return false;
    }

    output
        << "P6\n"
        << width
        << " "
        << height
        << "\n255\n";
    output.write(
        reinterpret_cast<const char*>(rgb_pixels.data()),
        static_cast<std::streamsize>(rgb_pixels.size()));
    if (!output.good())
    {
        std::cerr << "Failed while writing reference PPM file: " << path << ".\n";
        return false;
    }

    std::cout << "Wrote Mare smoke RGB PPM to " << path << ".\n";
    return true;
}

bool read_rgb_ppm_file(
    const std::string& path,
    RGBPPM& image)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        std::cerr << "Unable to open PPM file: " << path << ".\n";
        return false;
    }

    std::vector<U8> bytes{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    size_t position = 0;
    const auto read_token = [&]() -> std::string
    {
        for (;;)
        {
            while (position < bytes.size() &&
                std::isspace(static_cast<unsigned char>(bytes[position])))
            {
                ++position;
            }
            if (position < bytes.size() && bytes[position] == '#')
            {
                while (position < bytes.size() &&
                    bytes[position] != '\n' &&
                    bytes[position] != '\r')
                {
                    ++position;
                }
                continue;
            }
            break;
        }

        const size_t token_begin = position;
        while (position < bytes.size() &&
            !std::isspace(static_cast<unsigned char>(bytes[position])))
        {
            ++position;
        }
        return std::string(
            reinterpret_cast<const char*>(bytes.data() + token_begin),
            position - token_begin);
    };

    const std::string magic = read_token();
    const std::string width_token = read_token();
    const std::string height_token = read_token();
    const std::string max_token = read_token();
    if (magic != "P6" ||
        width_token.empty() ||
        height_token.empty() ||
        max_token != "255")
    {
        std::cerr << "Unsupported PPM header in " << path << ".\n";
        return false;
    }

    char* end = nullptr;
    const unsigned long width = std::strtoul(width_token.c_str(), &end, 10);
    if (*end != '\0' || width == 0 || width > std::numeric_limits<U32>::max())
    {
        std::cerr << "Invalid PPM width in " << path << ".\n";
        return false;
    }
    const unsigned long height = std::strtoul(height_token.c_str(), &end, 10);
    if (*end != '\0' || height == 0 || height > std::numeric_limits<U32>::max())
    {
        std::cerr << "Invalid PPM height in " << path << ".\n";
        return false;
    }

    if (position < bytes.size() &&
        std::isspace(static_cast<unsigned char>(bytes[position])))
    {
        ++position;
    }

    const size_t expected_size =
        static_cast<size_t>(width) * static_cast<size_t>(height) * 3U;
    if (bytes.size() - position != expected_size)
    {
        std::cerr
            << "Invalid PPM pixel payload in "
            << path
            << ": expected "
            << expected_size
            << " bytes, got "
            << (bytes.size() - position)
            << ".\n";
        return false;
    }

    image.mWidth = static_cast<U32>(width);
    image.mHeight = static_cast<U32>(height);
    image.mPixels.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(position),
        bytes.end());
    return true;
}

bool compare_rgb_ppm_files(
    const std::string& reference_path,
    const std::string& candidate_path,
    F64 mean_tolerance,
    U32 max_tolerance)
{
    RGBPPM reference;
    RGBPPM candidate;
    if (!read_rgb_ppm_file(reference_path, reference) ||
        !read_rgb_ppm_file(candidate_path, candidate))
    {
        return false;
    }

    if (reference.mWidth != candidate.mWidth ||
        reference.mHeight != candidate.mHeight ||
        reference.mPixels.size() != candidate.mPixels.size())
    {
        std::cerr
            << "Mare smoke PPM compare FAIL: size mismatch, reference "
            << reference.mWidth
            << "x"
            << reference.mHeight
            << ", candidate "
            << candidate.mWidth
            << "x"
            << candidate.mHeight
            << ".\n";
        return false;
    }

    struct PPMDiffStats
    {
        U64 mDiffSum = 0;
        U32 mMaxDiff = 0;
        U64 mNonzeroCount = 0;
        F64 mMeanDiff = 0.0;
        bool mFlipY = false;
    };

    const auto compute_stats = [&](bool flip_y)
    {
        PPMDiffStats stats;
        stats.mFlipY = flip_y;
        for (U32 y = 0; y < reference.mHeight; ++y)
        {
            const U32 candidate_y =
                flip_y ? reference.mHeight - 1U - y : y;
            for (U32 x = 0; x < reference.mWidth; ++x)
            {
                const size_t reference_pixel =
                    (static_cast<size_t>(y) *
                        static_cast<size_t>(reference.mWidth) +
                        static_cast<size_t>(x)) * 3U;
                const size_t candidate_pixel =
                    (static_cast<size_t>(candidate_y) *
                        static_cast<size_t>(candidate.mWidth) +
                        static_cast<size_t>(x)) * 3U;
                for (U32 channel = 0; channel < 3U; ++channel)
                {
                    const U32 diff =
                        static_cast<U32>(
                            std::abs(
                                static_cast<int>(
                                    reference.mPixels[reference_pixel + channel]) -
                                static_cast<int>(
                                    candidate.mPixels[candidate_pixel + channel])));
                    stats.mDiffSum += diff;
                    stats.mMaxDiff = std::max(stats.mMaxDiff, diff);
                    if (diff > 0)
                    {
                        ++stats.mNonzeroCount;
                    }
                }
            }
        }

        stats.mMeanDiff =
            reference.mPixels.empty() ?
                0.0 :
                static_cast<F64>(stats.mDiffSum) /
                    static_cast<F64>(reference.mPixels.size());
        return stats;
    };

    const PPMDiffStats direct_stats = compute_stats(false);
    const PPMDiffStats flip_y_stats = compute_stats(true);
    const PPMDiffStats& stats =
        (direct_stats.mMeanDiff <= flip_y_stats.mMeanDiff) ?
            direct_stats :
            flip_y_stats;
    const bool passed =
        stats.mMeanDiff <= mean_tolerance &&
        stats.mMaxDiff <= max_tolerance;
    std::cout
        << "Mare smoke PPM compare "
        << (passed ? "PASS" : "FAIL")
        << ": candidate "
        << candidate_path
        << " vs reference "
        << reference_path
        << ", mean channel diff "
        << std::fixed
        << std::setprecision(4)
        << stats.mMeanDiff
        << ", max channel diff "
        << stats.mMaxDiff
        << ", nonzero channels "
        << stats.mNonzeroCount
        << "/"
        << reference.mPixels.size()
        << ", orientation "
        << (stats.mFlipY ? "candidate-flip-y" : "direct")
        << ", tolerance mean <= "
        << mean_tolerance
        << ", max <= "
        << max_tolerance
        << ".\n";
    return passed;
}

bool write_rgba_readback_as_rgb_ppm(
    const std::string& path,
    U32 width,
    U32 height,
    const std::vector<U8>& rgba_pixels)
{
    const size_t expected_size =
        static_cast<size_t>(width) * static_cast<size_t>(height) * 4U;
    if (rgba_pixels.size() != expected_size)
    {
        std::cerr
            << "Invalid RGBA readback size for "
            << path
            << ": expected "
            << expected_size
            << " bytes, got "
            << rgba_pixels.size()
            << ".\n";
        return false;
    }

    std::vector<U8> rgb_pixels;
    rgb_pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3U);
    for (size_t rgba = 0, rgb = 0; rgba < rgba_pixels.size(); rgba += 4, rgb += 3)
    {
        rgb_pixels[rgb + 0] = rgba_pixels[rgba + 0];
        rgb_pixels[rgb + 1] = rgba_pixels[rgba + 1];
        rgb_pixels[rgb + 2] = rgba_pixels[rgba + 2];
    }
    return write_rgb_ppm_file(path, width, height, rgb_pixels);
}
}
