#import <Cocoa/Cocoa.h>

#include "mare_vulkan_smoke_macosx.h"
#include "mare_vulkan_test_support.h"

namespace mare_vulkan_test
{
void* create_window(int width, int height, const char* title)
{
    @autoreleasepool
    {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp finishLaunching];

        NSRect screen_frame = [[NSScreen mainScreen] frame];
        NSRect window_frame = NSMakeRect(
            NSMidX(screen_frame) - width / 2.0,
            NSMidY(screen_frame) - height / 2.0,
            width,
            height);

        NSWindow* window = [[NSWindow alloc]
            initWithContentRect:window_frame
            styleMask:NSWindowStyleMaskTitled |
                NSWindowStyleMaskClosable |
                NSWindowStyleMaskResizable |
                NSWindowStyleMaskMiniaturizable
            backing:NSBackingStoreBuffered
            defer:NO];
        if (!window)
        {
            return nullptr;
        }

        [window setTitle:[NSString stringWithUTF8String:title ? title : "Mare Vulkan Smoke"]];
        [window setReleasedWhenClosed:NO];
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        return window;
    }
}

bool pump_events(void* window)
{
    @autoreleasepool
    {
        NSEvent* event = nil;
        do
        {
            event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                      untilDate:[NSDate dateWithTimeIntervalSinceNow:0.0]
                                         inMode:NSDefaultRunLoopMode
                                        dequeue:YES];
            if (event)
            {
                [NSApp sendEvent:event];
            }
        }
        while (event);

        [NSApp updateWindows];
        return window && [(NSWindow*)window isVisible];
    }
}

void get_view_size(void* view, std::uint32_t* width, std::uint32_t* height)
{
    @autoreleasepool
    {
        if (!view || !width || !height)
        {
            return;
        }

        NSView* native_view = (NSView*)view;
        NSRect bounds = [native_view bounds];
        CGFloat backing_scale = [[native_view window] backingScaleFactor];
        if (backing_scale <= 0.0)
        {
            backing_scale = [[NSScreen mainScreen] backingScaleFactor];
        }
        if (backing_scale <= 0.0)
        {
            backing_scale = 1.0;
        }

        *width = bounds.size.width > 1.0 ?
            (unsigned int)(bounds.size.width * backing_scale) :
            1U;
        *height = bounds.size.height > 1.0 ?
            static_cast<std::uint32_t>(bounds.size.height * backing_scale) :
            1U;
    }
}

void destroy_window(void* window)
{
    @autoreleasepool
    {
        if (window)
        {
            [(NSWindow*)window close];
            [(NSWindow*)window release];
        }
    }
}
}

void* mare_vulkan_smoke_create_window(int width, int height, const char* title)
{
    return mare_vulkan_test::create_window(width, height, title);
}

bool mare_vulkan_smoke_pump_events(void* window)
{
    return mare_vulkan_test::pump_events(window);
}

void mare_vulkan_smoke_get_view_size(void* view, unsigned int* width, unsigned int* height)
{
    if (!width || !height)
    {
        return;
    }

    std::uint32_t view_width = 1;
    std::uint32_t view_height = 1;
    mare_vulkan_test::get_view_size(view, &view_width, &view_height);
    *width = view_width;
    *height = view_height;
}

void mare_vulkan_smoke_destroy_window(void* window)
{
    mare_vulkan_test::destroy_window(window);
}
