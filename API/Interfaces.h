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

#ifndef Athol_API_Interfaces_h
#define Athol_API_Interfaces_h

#include <memory>
#include <wayland-server.h>

namespace API {

class InputClient {
public:
    virtual void handleKeyboardEvent(uint32_t time, uint32_t key, uint32_t state) = 0;

    virtual void handlePointerMotion(uint32_t time, double dx, double dy) = 0;
    virtual void handlePointerButton(uint32_t time, uint32_t button, uint32_t state) = 0;

    enum class PointerAxis {
        Wheel,
    };
    virtual void handlePointerAxis(uint32_t time, PointerAxis axis, double dx, double dy) = 0;
};

class Compositor {
public:
    virtual uint32_t width() = 0;
    virtual uint32_t height() = 0;
    virtual struct wl_display* display() const = 0;
    virtual void initializeInput(std::unique_ptr<InputClient>) = 0;
};

} // namespace API

#endif // Athol_API_Interfaces_h
