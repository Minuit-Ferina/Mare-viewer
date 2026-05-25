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

static LLRenderMacOSXContextAttachment* get_context_attachment(void* view)
{
    return (LLRenderMacOSXContextAttachment*)objc_getAssociatedObject(
        (id)view,
        &sRenderContextAttachmentKey);
}

void* ll_render_macosx_create_native_view(void* window)
{
    LLNativeView* native_view = [[LLNativeView alloc] initWithFrame:[(LLNSWindow*)window frame]
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
