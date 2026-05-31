#ifndef MARE_VULKAN_SMOKE_MACOSX_H
#define MARE_VULKAN_SMOKE_MACOSX_H

void* mare_vulkan_smoke_create_window(int width, int height, const char* title);
bool mare_vulkan_smoke_pump_events(void* window);
void mare_vulkan_smoke_get_view_size(void* view, unsigned int* width, unsigned int* height);
void mare_vulkan_smoke_destroy_window(void* window);

#endif
