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
#include <string.h>

#include "Display.h"
#include "Compositor.h"

namespace Athol {

// ----------------------------------------------------------------------------------------------------------
// CLASS: Display
// ----------------------------------------------------------------------------------------------------------

Display::Display()
    : m_display(wl_display_create())
    , m_width(1280)
    , m_height(720)
{
    initialize();
}

Display::~Display()
{
    wl_display_destroy(m_display);
    deinitialize();
}

#ifdef BROADCOM_NEXUS

void Display::initialize ()
{
    m_eglDisplay = NULL;
}

void Display::deinitialize( void )
{
}

#else

void Display::initialize()
{
    bcm_host_init();

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(m_eglDisplay, nullptr, nullptr);

    PFNEGLBINDWAYLANDDISPLAYWL bindDisplayFn = reinterpret_cast<PFNEGLBINDWAYLANDDISPLAYWL>(eglGetProcAddress("eglBindWaylandDisplayWL"));

    if (bindDisplayFn != nullptr)
    {
        bindDisplayFn (m_eglDisplay, m_display);

        m_displayHandle = vc_dispmanx_display_open(DISPMANX_ID_HDMI);
        graphics_get_display_size(DISPMANX_ID_HDMI, &m_width, &m_height);
    }
}

void Display::deinitialize()
{
    vc_dispmanx_display_close(m_displayHandle);
}

#endif

// ----------------------------------------------------------------------------------------------------------
// CLASS: Update
// ----------------------------------------------------------------------------------------------------------

#ifndef BROADCOM_NEXUS
static void complete(HandleUpdate, void* data)
{
    Compositor* compositor = static_cast<Compositor*>(data);

    assert(compositor != nullptr);

    if (compositor != nullptr)
    {
        compositor->updated();
    }
}
#endif

Update::Update(const uint32_t width, const uint32_t height)
    : m_width(width)
    , m_height(height)
{
#ifdef BROADCOM_NEXUS
    m_updateHandle = nullptr;
#else
    m_updateHandle = vc_dispmanx_update_start(10);
#endif
}

Update::~Update()
{
#ifdef BROADCOM_NEXUS
    Athol::Compositor::instance().updated();
#else
    vc_dispmanx_update_submit(m_updateHandle, complete, &(Athol::Compositor::instance()));
#endif
}

} // namespace Athol
