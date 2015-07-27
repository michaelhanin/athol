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

#ifndef Pointer_h
#define Pointer_h

#include <utility>
#include <wayland-util.h>

#include "Types.h"
#include "Display.h"

namespace Athol {

class Pointer {
private:
    Pointer() = delete;
    Pointer(const Pointer&) = delete;
    Pointer& operator= (const Pointer&) = delete;

private:
    // Required to place it in a linked list by the compositor
    friend class Compositor;
    struct wl_list link;

public:
    Pointer(Display&);
    ~Pointer();

    void move(double dx, double dy);
    void reposition(Update&);

    struct CursorPointerData {
        static const uint32_t width = 48;
        static const uint32_t height = 48;
        static uint8_t data[width * height * 4 + 1];
    };

private:
    Display& m_display;
    HandleElement m_elementHandle;

    static const uint32_t pointerWidth = 16;
    static const uint32_t pointerHeight = 16;

    std::pair<uint32_t, uint32_t> m_position;
};

} // namespace Athol

#endif
