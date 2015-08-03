/*
 * Copyright (c) 2015, Igalia S.L.
 * Copyright (c) 2015, Metrological
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <utility>
#include <vector>
#include <assert.h>

#include "Surface.h"
#include "Compositor.h"

namespace Athol {

struct FrameCallback {
    struct wl_resource* resource;
    struct wl_list link;
};

// ----------------------------------------------------------------------------------------------------
// Wayland interface implementation for resource
// ----------------------------------------------------------------------------------------------------
const struct wl_surface_interface g_surfaceInterface {

    // destroy
    [](struct wl_client*, struct wl_resource* resource)
    {
        wl_resource_destroy(resource);
    },

    // attach
    [](struct wl_client*, struct wl_resource* resource, struct wl_resource* bufferResource, int32_t, int32_t)
    {
        auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));

        assert (surface != nullptr);

        surface->attach(bufferResource);
    },

    // damage
    [](struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t) 
    { 
    },

    // frame
    [](struct wl_client* client, struct wl_resource* resource, uint32_t callbackID)
    {
        auto* callback = new FrameCallback;
        callback->resource = wl_resource_create(client, &wl_callback_interface, 1, callbackID);
        wl_resource_set_implementation(callback->resource, nullptr, callback,
            [](struct wl_resource* resource) {
                auto* callback = static_cast<FrameCallback*>(wl_resource_get_user_data(resource));
                wl_list_remove(&callback->link);
                delete callback;
            });

        auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));
        
        assert (surface != nullptr);

        surface->add (&(callback->link));

    },

    // set_opaque_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) 
    { 
    },

    // set_input_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) 
    { 
    },

    // commit
    [](struct wl_client*, struct wl_resource* resource)
    {
        Athol::Compositor::instance().scheduleRepaint(*static_cast<Surface*>(wl_resource_get_user_data(resource)));
    },

    // set_buffer_transform
    [](struct wl_client*, struct wl_resource*, int) 
    { 
    },

    // set_buffer_scale
    [](struct wl_client*, struct wl_resource*, int32_t) 
    { 
    }
};

// ----------------------------------------------------------------------------------------------------
// Static links for Wayland callbacks.
// ----------------------------------------------------------------------------------------------------
/* static */ void destroySurface(struct wl_resource* resource)
{
    auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));

    if (surface != nullptr) {
        delete surface;
    }
}

#ifndef BROADCOM_NEXUS

static PFNEGLQUERYWAYLANDBUFFERWL g_queryWaylandBufferFn = nullptr;

static HandleElement createElement(HandleUpdate update, HandleResource resource, Display& display)
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, display.width() << 16, display.height() << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, display.width(), display.height());

    return vc_dispmanx_element_add(update, display.handle(), 0,
        &destRect, resource, &srcRect, DISPMANX_PROTECTION_NONE, &alpha,
        nullptr, DISPMANX_NO_ROTATE);
}

#endif

// ----------------------------------------------------------------------------------------------------------
// Object section. From here we implement the class declarations.
// ----------------------------------------------------------------------------------------------------------
// CLASS: Surface
// ----------------------------------------------------------------------------------------------------------
Surface::Surface(Display& display, struct wl_client* client, struct wl_resource* resource, uint32_t id)
    : m_display(display)
    , m_current(nullptr)
{
    pthread_mutexattr_t  structAttributes;

    // Create a recursive mutex for this process (no named version, use semaphore)
    if ( (pthread_mutexattr_init(&structAttributes) != 0) ||
         (pthread_mutexattr_settype(&structAttributes, PTHREAD_MUTEX_RECURSIVE) != 0) ||
         (pthread_mutex_init(&m_syncMutex, &structAttributes) != 0) ) {

        // That will be the day, if this fails...
        assert (false);
    }

    wl_list_init(&m_frameCallbacks);
    wl_list_init(&link);

    m_resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
    wl_resource_set_implementation(m_resource, &g_surfaceInterface, this, destroySurface);

    initialize();

    fprintf (stdout, "[Athol] Created surface.\n");
}

Surface::~Surface()
{
    FrameCallback* callback;
    FrameCallback* nextCallback;
    wl_list_for_each_safe(callback, nextCallback, &m_frameCallbacks, link)
        wl_resource_destroy(callback->resource);

    wl_list_init(&m_frameCallbacks);

    deinitialize();

    m_resource = nullptr;
}

#ifdef BROADCOM_NEXUS

void Surface::initialize ()
{
   NXPL_NativeWindowInfo win_info;

   win_info.x        = 0;
   win_info.y        = 0;
   win_info.width    = m_display.width();
   win_info.height   = m_display.height();
   win_info.stretch  = true;
   win_info.clientID = 0; //FIXME hardcoding

   fprintf (stdout, "[Athol] Creating native window. Width (%d) x Height (%d)\n", win_info.height, win_info.width);
   m_elementHandle = static_cast<NEXUS_SurfaceClient*> (NXPL_CreateNativeWindow ( &win_info ));
   m_background = nullptr;
}

void Surface::deinitialize ()
{
}

#else

void Surface::initialize()
{
    if (g_queryWaylandBufferFn == nullptr) {
        g_queryWaylandBufferFn = reinterpret_cast<PFNEGLQUERYWAYLANDBUFFERWL>(eglGetProcAddress("eglQueryWaylandBufferWL"));
    }

    assert (g_queryWaylandBufferFn != nullptr);

    {
        Update update(m_display.width(), m_display.height());

        uint32_t imagePtr;
        VC_RECT_T rect;
        vc_dispmanx_rect_set(&rect, 0, 0, m_display.width(), m_display.height());
        std::vector<uint8_t> pixels(m_display.width() * m_display.height() * 4, 0);
        m_background = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, m_display.width(), m_display.height(), &imagePtr);

        // TODO: No need for a display.height here??
        vc_dispmanx_resource_write_data(m_background, VC_IMAGE_ARGB8888, m_display.width() * 4, pixels.data(), &rect);
        m_elementHandle = createElement(update.handle(), m_background, m_display);
    }
}

void Surface::deinitialize ()
{
    if ( (m_background != DISPMANX_NO_HANDLE) || (m_elementHandle != DISPMANX_NO_HANDLE) )
    {
        Update update(m_display.width(), m_display.height());

        if (m_background != DISPMANX_NO_HANDLE)
            vc_dispmanx_resource_delete(m_background);
        if (m_elementHandle != DISPMANX_NO_HANDLE)
            vc_dispmanx_element_remove(update.handle(), m_elementHandle);
    }
}

#endif


void Surface::repaint(Update& update)
{
    pthread_mutex_lock (&m_syncMutex);

    if (m_current != nullptr) {

    fprintf (stdout, "[Athol] Repaint.\n");
#ifdef BROADCOM_NEXUS
#else    
        EGLint width, height;
        g_queryWaylandBufferFn(m_display.eglHandle(), m_current, EGL_WIDTH, &width);
        g_queryWaylandBufferFn(m_display.eglHandle(), m_current, EGL_HEIGHT, &height);

        if ( (width == update.width()) && (height == update.height()) ) {

            if (m_background != DISPMANX_NO_HANDLE) {

                vc_dispmanx_resource_delete(m_background);
                m_background = DISPMANX_NO_HANDLE;
  
                if (m_elementHandle != DISPMANX_NO_HANDLE)
                vc_dispmanx_element_remove(update.handle(), m_elementHandle);
                m_elementHandle = createElement(update.handle(), DISPMANX_NO_HANDLE, m_display);
            }

            vc_dispmanx_element_change_source(update.handle(), m_elementHandle,
            vc_dispmanx_get_handle_from_wl_buffer(m_current));
        }
#endif
    }

    pthread_mutex_unlock (&m_syncMutex);
}

void Surface::dispatchFrameCallbacks(uint64_t sequence)
{
    FrameCallback* callback;
    FrameCallback* nextCallback;
    wl_list_for_each_safe(callback, nextCallback, &m_frameCallbacks, link) {
        wl_callback_send_done(callback->resource, sequence);
        wl_resource_destroy(callback->resource);
    }

    wl_list_init(&m_frameCallbacks);
}

void Surface::attach (struct wl_resource* resource)
{
    if (m_current != resource) {
        struct wl_resource* previousResource = m_current;

        pthread_mutex_lock (&m_syncMutex);
        m_current = resource;
        pthread_mutex_unlock (&m_syncMutex);

        if (previousResource != nullptr)
            wl_resource_queue_event(previousResource, WL_BUFFER_RELEASE);
    }
}

void Surface::add (struct wl_list* resource)
{
    wl_list_insert(m_frameCallbacks.prev, resource);
}


} // namespace Athol
