#import <Cocoa/Cocoa.h>

#include "mare_vulkan_smoke_macosx.h"

void* mare_vulkan_smoke_create_window(int width, int height, const char* title)
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

bool mare_vulkan_smoke_pump_events(void* window)
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

void mare_vulkan_smoke_get_view_size(void* view, unsigned int* width, unsigned int* height)
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
            (unsigned int)(bounds.size.height * backing_scale) :
            1U;
    }
}

void mare_vulkan_smoke_destroy_window(void* window)
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
