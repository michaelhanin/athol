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

#include <vector>
#include <cstdio>

#include "Pointer.h"

namespace Athol {

Pointer::Pointer(Display& display)
    : m_display(display)
    , m_position(0, 0)
{
    initialize();
}

Pointer::~Pointer()
{
    deinitialize();
}

#ifdef BROADCOM_NEXUS

void Pointer::initialize()
{
    m_elementHandle = nullptr;
}

void Pointer::deinitialize()
{
}

#else

void Pointer::initialize()
{
    Update update(m_display.width(),m_display.height());

    static VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FROM_SOURCE),
        255, 0
    };

    uint32_t imagePtr;
    VC_RECT_T rect;
    vc_dispmanx_rect_set(&rect, 0, 0, CursorPointerData::width, CursorPointerData::height);
    DISPMANX_RESOURCE_HANDLE_T pointerResource = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, CursorPointerData::width, CursorPointerData::height, &imagePtr);
    vc_dispmanx_resource_write_data(pointerResource, VC_IMAGE_RGBA32, CursorPointerData::width * 4, CursorPointerData::data, &rect);

    VC_RECT_T srcRect, destRect;
    vc_dispmanx_rect_set(&srcRect, 0, 0, CursorPointerData::width << 16, CursorPointerData::height << 16);
    vc_dispmanx_rect_set(&destRect, m_position.first, m_position.second, pointerWidth, pointerHeight);

    m_elementHandle = vc_dispmanx_element_add(update.handle(), m_display.handle(), 10,
        &destRect, pointerResource, &srcRect, DISPMANX_PROTECTION_NONE, &alpha,
        nullptr, DISPMANX_NO_ROTATE);
    
    vc_dispmanx_resource_delete(pointerResource);
}

void Pointer::deinitialize()
{
    if (m_elementHandle != DISPMANX_NO_HANDLE)
    {
        Update update(m_display.width(), m_display.height());
        vc_dispmanx_element_remove(update.handle(), m_elementHandle);
    }
}

#endif

void Pointer::move(double dx, double dy)
{
    m_position.first = std::min<uint32_t>(std::max<int32_t>(0, m_position.first + dx), m_display.width() - 1);
    m_position.second = std::min<uint32_t>(std::max<int32_t>(0, m_position.second + dy), m_display.height() - 1);
}

void Pointer::reposition(Update& update)
{
#ifndef BROADCOM_NEXUS
    VC_RECT_T destRect;
    vc_dispmanx_rect_set(&destRect, m_position.first, m_position.second, pointerWidth, pointerHeight);

    vc_dispmanx_element_change_attributes(update.handle(), m_elementHandle, 1 << 2,
        0, 0, &destRect, nullptr, DISPMANX_NO_HANDLE, DISPMANX_NO_ROTATE);
#endif
}

} // namespace Athol
