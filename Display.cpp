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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <bcm_host.h>

#include "Display.h"
#include "Compositor.h"

namespace Athol {

using BindDisplayType = PFNEGLBINDWAYLANDDISPLAYWL;

// ----------------------------------------------------------------------------------------------------------
// CLASS: Display
// ----------------------------------------------------------------------------------------------------------

Display::Display()
    : m_display(wl_display_create())
    , m_width(0)
    , m_height(0)
{
    bcm_host_init();

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(m_eglDisplay, nullptr, nullptr);

    BindDisplayType bindDisplayFn = reinterpret_cast<BindDisplayType>(eglGetProcAddress("eglBindWaylandDisplayWL"));

    if (bindDisplayFn != nullptr)
    {
        bindDisplayFn (m_eglDisplay, m_display);

        m_displayHandle = vc_dispmanx_display_open(DISPMANX_ID_HDMI);
        graphics_get_display_size(DISPMANX_ID_HDMI, &m_width, &m_height);
    }
}

Display::~Display()
{
    wl_display_destroy(m_display);
    vc_dispmanx_display_close(m_displayHandle);
}

// ----------------------------------------------------------------------------------------------------------
// CLASS: Update
// ----------------------------------------------------------------------------------------------------------

static void complete(HandleUpdate, void* data)
{
    Compositor* compositor = static_cast<Compositor*>(data);

    assert(compositor != nullptr);

    if (compositor != nullptr)
    {
        compositor->updated();
    }
}

Update::Update(const uint32_t width, const uint32_t height)
    : m_width(width)
    , m_height(height)
{
    m_updateHandle = vc_dispmanx_update_start(10);
}

Update::~Update()
{
    vc_dispmanx_update_submit(m_updateHandle, complete, &(Athol::Compositor::instance()));
}

} // namespace Athol
