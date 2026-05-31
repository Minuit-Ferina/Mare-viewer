/**
 * @file llrenderbackendmacosx-objc.mm
 * @brief macOS Objective-C bridge for backend-owned native render contexts.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * $/LicenseInfo$
 */

#include "llrenderbackendmacosx-objc.h"

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <objc/runtime.h>

#include "llnativeview-objc.h"

extern bool gHiDPISupport;

@interface LLRenderMacOSXContextAttachment : NSObject
{
    NSOpenGLContext* mContext;
}
- (id)initWithContext:(NSOpenGLContext*)context view:(NSView*)view;
- (NSOpenGLContext*)context;
- (void)detachFromView:(NSView*)view;
- (void)updateContext:(NSNotification*)notification;
@end

@implementation LLRenderMacOSXContextAttachment

- (id)initWithContext:(NSOpenGLContext*)context view:(NSView*)view
{
    self = [super init];
    if (!self)
    {
        return nil;
    }

    mContext = [context retain];

    [view setPostsFrameChangedNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateContext:)
                                                 name:NSViewFrameDidChangeNotification
                                               object:view];

    if ([view window])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(updateContext:)
                                                     name:NSWindowDidResizeNotification
                                                   object:[view window]];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(updateContext:)
                                                     name:NSWindowDidChangeScreenNotification
                                                   object:[view window]];
    }

    return self;
}

- (NSOpenGLContext*)context
{
    return mContext;
}

- (void)detachFromView:(NSView*)view
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [mContext clearDrawable];
    [mContext release];
    mContext = nil;
}

- (void)updateContext:(NSNotification*)notification
{
    [mContext update];
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [mContext release];
    [super dealloc];
}

@end

static char sRenderContextAttachmentKey;

static CGFloat get_effective_backing_scale(NSView* native_view)
{
    if (!gHiDPISupport)
    {
        return 1.0;
    }

    CGFloat backing_scale = [[native_view window] backingScaleFactor];
    if (backing_scale <= 0.0)
    {
        backing_scale = [[NSScreen mainScreen] backingScaleFactor];
    }
    if (backing_scale <= 0.0)
    {
        backing_scale = 1.0;
    }

    return backing_scale;
}

static LLRenderMacOSXContextAttachment* get_context_attachment(void* view)
{
    return (LLRenderMacOSXContextAttachment*)objc_getAssociatedObject(
        (id)view,
        &sRenderContextAttachmentKey);
}

void* ll_render_macosx_create_native_view(void* window)
{
    NSRect native_frame = [[(LLNSWindow*)window contentView] bounds];
    LLNativeView* native_view = [[LLNativeView alloc] initWithFrame:native_frame
                                                   withSamples:0
                                                     andVsync:false];
    [(LLNSWindow*)window setContentView:native_view];
    return native_view;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

bool ll_render_macosx_attach_native_context(void* view, void* pixel_format, bool vsync)
{
    NSOpenGLPixelFormat* format = [[[NSOpenGLPixelFormat alloc]
        initWithCGLPixelFormatObj:(CGLPixelFormatObj)pixel_format] autorelease];
    if (format == nil)
    {
        NSLog(@"Failed to create pixel format!", nil);
        return false;
    }

    NSOpenGLContext* context = [[[NSOpenGLContext alloc] initWithFormat:format
                                                           shareContext:nil] autorelease];
    if (context == nil)
    {
        NSLog(@"Failed to create OpenGL context!", nil);
        return false;
    }

    LLNativeView* native_view = (LLNativeView*)view;
    [context setView:native_view];
    [context makeCurrentContext];

    int swap_interval = vsync ? 1 : 0;
    [context setValues:&swap_interval forParameter:NSOpenGLContextParameterSwapInterval];

    LLRenderMacOSXContextAttachment* attachment =
        [[LLRenderMacOSXContextAttachment alloc] initWithContext:context view:native_view];
    objc_setAssociatedObject(
        native_view,
        &sRenderContextAttachmentKey,
        attachment,
        OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [attachment release];

    return true;
}

#pragma clang diagnostic pop

void* ll_render_macosx_get_native_context(void* view)
{
    LLRenderMacOSXContextAttachment* attachment = get_context_attachment(view);
    return attachment ? [[attachment context] CGLContextObj] : nullptr;
}

void ll_render_macosx_destroy_native_view(void* view)
{
    LLRenderMacOSXContextAttachment* attachment = get_context_attachment(view);
    if (attachment)
    {
        if ([NSOpenGLContext currentContext] == [attachment context])
        {
            [NSOpenGLContext clearCurrentContext];
        }
        [attachment detachFromView:(NSView*)view];
        objc_setAssociatedObject(
            (id)view,
            &sRenderContextAttachmentKey,
            nil,
            OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    [(NSView*)view removeFromSuperview];
}

void* ll_render_macosx_create_metal_native_view(void* window)
{
    NSRect native_frame = [[(LLNSWindow*)window contentView] bounds];
    LLNativeView* native_view = [[LLNativeView alloc] initWithFrame:native_frame
                                                       withSamples:0
                                                         andVsync:false];
    if (native_view == nil)
    {
        NSLog(@"Failed to create native Metal view!", nil);
        return nullptr;
    }

    [[NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"] load];
    Class metal_layer_class = NSClassFromString(@"CAMetalLayer");
    if (metal_layer_class == Nil)
    {
        NSLog(@"Failed to load CAMetalLayer!", nil);
        [native_view release];
        return nullptr;
    }

    id metal_layer = [metal_layer_class layer];
    if (metal_layer == nil)
    {
        NSLog(@"Failed to create CAMetalLayer!", nil);
        [native_view release];
        return nullptr;
    }

    CGFloat backing_scale = get_effective_backing_scale(native_view);

    NSRect native_bounds = [native_view bounds];
    NSSize drawable_size = NSMakeSize(
        native_bounds.size.width * backing_scale,
        native_bounds.size.height * backing_scale);

    [metal_layer setValue:[NSNumber numberWithBool:YES] forKey:@"opaque"];
    [metal_layer setValue:[NSNumber numberWithDouble:backing_scale]
                   forKey:@"contentsScale"];
    [metal_layer setValue:[NSValue valueWithSize:drawable_size] forKey:@"drawableSize"];
    [native_view setWantsLayer:YES];
    [native_view setLayer:metal_layer];
    [(LLNSWindow*)window setContentView:native_view];
    return native_view;
}

void* ll_render_macosx_get_metal_layer(void* view)
{
    return [(NSView*)view layer];
}

bool ll_render_macosx_get_metal_layer_drawable_size(void* view, unsigned int* width, unsigned int* height)
{
    if (!view || !width || !height)
    {
        return false;
    }

    NSView* native_view = (NSView*)view;
    id metal_layer = [native_view layer];
    if (metal_layer == nil)
    {
        return false;
    }

    CGFloat backing_scale = get_effective_backing_scale(native_view);

    NSRect native_bounds = [native_view bounds];
    NSSize drawable_size = NSMakeSize(
        native_bounds.size.width * backing_scale,
        native_bounds.size.height * backing_scale);
    [metal_layer setValue:[NSNumber numberWithDouble:backing_scale]
                   forKey:@"contentsScale"];
    [metal_layer setValue:[NSValue valueWithSize:drawable_size] forKey:@"drawableSize"];

    *width = drawable_size.width > 1.0 ? (unsigned int)drawable_size.width : 1U;
    *height = drawable_size.height > 1.0 ? (unsigned int)drawable_size.height : 1U;
    return true;
}

bool ll_render_macosx_get_native_view_size(void* view, unsigned int* width, unsigned int* height)
{
    if (!view || !width || !height)
    {
        return false;
    }

    NSView* native_view = (NSView*)view;
    NSRect native_bounds = [native_view bounds];
    *width = native_bounds.size.width > 1.0 ? (unsigned int)native_bounds.size.width : 1U;
    *height = native_bounds.size.height > 1.0 ? (unsigned int)native_bounds.size.height : 1U;
    return true;
}
