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

#ifndef Display_h
#define Display_h

#include <wayland-server.h>

#include "Types.h"

namespace Athol {

class Display {
private:
    Display(const Display&) = delete;
    Display& operator= (const Display&) = delete;

public:
    Display();
    ~Display();

    inline bool isAvailable() const { return ((m_height != 0) && (m_width != 0)); }
    inline uint32_t height() const { return (m_height); }
    inline uint32_t width() const { return (m_width); }

    struct wl_display* display() { return m_display; }
    inline HandleDisplay handle() { return m_displayHandle; }
    inline EGLDisplay eglHandle() { return m_eglDisplay; }

private:
    void initialize();
    void deinitialize();

private:
    uint32_t m_height;
    uint32_t m_width;
    struct wl_display* m_display;
    EGLDisplay m_eglDisplay;
    HandleDisplay m_displayHandle;
};

class Update {
private:
    Update();
    Update(const Update&);
    Update& operator=(const Update&);

public:
    Update(const uint32_t width, const uint32_t height);
    ~Update();

    inline uint32_t width() const { return m_width; }
    inline uint32_t height() const { return m_height; }
    HandleUpdate handle() { return m_updateHandle; }

private:
    uint32_t m_width;
    uint32_t m_height;
    HandleUpdate m_updateHandle;
};

} // namespace Athol

#endif // Dislay_h
