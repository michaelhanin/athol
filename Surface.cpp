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

#include "Surface.h"

#include "Athol.h"
#include <utility>
#include <vector>

#define BUILD_WAYLAND
#include <bcm_host.h>

struct FrameCallback {
    struct wl_resource* resource;
    struct wl_list link;
};

Surface::Surface(Athol& athol, struct wl_client* client, struct wl_resource* resource, uint32_t id)
    : m_athol(athol)
    , m_background(DISPMANX_NO_HANDLE)
{
    m_resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
    wl_resource_set_implementation(m_resource, &m_surfaceInterface, this, destroySurface);

    wl_list_init(&m_frameCallbacks);

    {
        Athol::Update update(athol);

        uint32_t imagePtr;
        VC_RECT_T rect;
        vc_dispmanx_rect_set(&rect, 0, 0, 720, 1280);
        std::vector<uint8_t> pixels(720 * 1280 * 4, 0);
        m_background = vc_dispmanx_resource_create(VC_IMAGE_ARGB8888, 720, 1280, &imagePtr);
        vc_dispmanx_resource_write_data(m_background, VC_IMAGE_ARGB8888, 720 * 4, pixels.data(), &rect);

        m_elementHandle = createElement(update, m_background);
    }
}

void Surface::repaint(Athol::Update& update)
{
    std::swap(m_buffers.current, m_buffers.pending);
    if (!m_buffers.current)
        return;

    EGLint width, height;
    Athol::f_queryWaylandBuffer(update.eglDisplay(), m_buffers.current.resource(), EGL_WIDTH, &width);
    Athol::f_queryWaylandBuffer(update.eglDisplay(), m_buffers.current.resource(), EGL_HEIGHT, &height);

    if (width != 720 || height != 1280)
        return;

    if (m_background != DISPMANX_NO_HANDLE) {
        vc_dispmanx_resource_delete(m_background);
        m_background = DISPMANX_NO_HANDLE;

        if (m_elementHandle != DISPMANX_NO_HANDLE)
            vc_dispmanx_element_remove(update.handle(), m_elementHandle);
        m_elementHandle = createElement(update, DISPMANX_NO_HANDLE);
    }

    vc_dispmanx_element_change_source(update.handle(), m_elementHandle,
        vc_dispmanx_get_handle_from_wl_buffer(m_buffers.current.resource()));
}

void Surface::dispatchFrameCallbacks(uint64_t time)
{
    FrameCallback* callback;
    FrameCallback* nextCallback;
    wl_list_for_each_safe(callback, nextCallback, &m_frameCallbacks, link) {
        wl_callback_send_done(callback->resource, time);
        wl_resource_destroy(callback->resource);
    }

    wl_list_init(&m_frameCallbacks);
}

void Surface::destroySurface(struct wl_resource* resource)
{
    auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));
    surface->m_resource = nullptr;
    delete surface;
}

const struct wl_surface_interface Surface::m_surfaceInterface {
    // destroy
    [](struct wl_client*, struct wl_resource* resource)
    {
        wl_resource_destroy(resource);
    },
    // attach
    [](struct wl_client*, struct wl_resource* resource, struct wl_resource* bufferResource, int32_t, int32_t)
    {
        auto& surface = *static_cast<Surface*>(wl_resource_get_user_data(resource));

        if (surface.m_buffers.pending.resource() != bufferResource)
            surface.m_buffers.pending = Buffer(bufferResource);
    },
    // damage
    [](struct wl_client*, struct wl_resource*, int32_t, int32_t, int32_t, int32_t) { },
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
        wl_list_insert(surface->m_frameCallbacks.prev, &callback->link);
    },
    // set_opaque_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) { },
    // set_input_region
    [](struct wl_client*, struct wl_resource*, struct wl_resource*) { },
    // commit
    [](struct wl_client*, struct wl_resource* resource)
    {
        auto& surface = *static_cast<Surface*>(wl_resource_get_user_data(resource));
        surface.m_athol.scheduleRepaint(surface);
    },
    // set_buffer_transform
    [](struct wl_client*, struct wl_resource*, int) { },
    // set_buffer_scale
    [](struct wl_client*, struct wl_resource*, int32_t) { }
};

DISPMANX_ELEMENT_HANDLE_T Surface::createElement(Athol::Update& update, DISPMANX_RESOURCE_HANDLE_T resource)
{
    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS),
        255, 0
    };

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, 720 << 16, 1280 << 16);
    vc_dispmanx_rect_set(&destRect, 0, 0, 720, 1280);

    return vc_dispmanx_element_add(update.handle(), update.displayHandle(), 0,
        &destRect, resource, &srcRect, DISPMANX_PROTECTION_NONE, &alpha,
        nullptr, DISPMANX_ROTATE_90);
}
