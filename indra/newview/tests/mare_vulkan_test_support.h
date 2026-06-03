#ifndef MARE_VULKAN_TEST_SUPPORT_H
#define MARE_VULKAN_TEST_SUPPORT_H

#include <cstdint>
#include <string>
#include <vector>

namespace mare_vulkan_test
{
void* create_window(int width, int height, const char* title);
bool pump_events(void* window);
void get_view_size(void* view, std::uint32_t* width, std::uint32_t* height);
void destroy_window(void* window);

struct RGBPPM
{
    std::uint32_t mWidth = 0;
    std::uint32_t mHeight = 0;
    std::vector<std::uint8_t> mPixels;
};

bool write_rgb_ppm_file(
    const std::string& path,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& rgb_pixels);

bool read_rgb_ppm_file(
    const std::string& path,
    RGBPPM& image);

bool compare_rgb_ppm_files(
    const std::string& reference_path,
    const std::string& candidate_path,
    double mean_tolerance,
    std::uint32_t max_tolerance);

bool write_rgba_readback_as_rgb_ppm(
    const std::string& path,
    std::uint32_t width,
    std::uint32_t height,
    const std::vector<std::uint8_t>& rgba_pixels);
}

#endif
