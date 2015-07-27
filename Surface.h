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

#ifndef Surface_h
#define Surface_h

#include <pthread.h>
#include <wayland-server.h>

#include "Types.h"
#include "Display.h"

namespace Athol {

class Surface {
private:
    Surface() = delete;
    Surface(const Surface&) = delete;
    Surface& operator= (const Surface&) = delete;

private:
    // Needed to add this class into a single linked list in the compositor.
    friend class Compositor;
    struct wl_list link;

public:
    Surface(Display& display, struct wl_client*, struct wl_resource*, uint32_t);
    ~Surface();

    void repaint(Update&);
    void dispatchFrameCallbacks(uint64_t sequence);
    void attach(struct wl_resource*);
    void add(struct wl_list*);

private:
    void initialize();
    void deinitialize();

private:
    pthread_mutex_t m_syncMutex;

    Display& m_display;

    struct wl_resource* m_resource;
    struct wl_resource* m_current;

    struct wl_list m_frameCallbacks;

    HandleElement  m_elementHandle;
    HandleResource m_background;
};

} // namespace Athol

#endif // Surface_h
