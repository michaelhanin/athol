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

#ifndef Compositor_h
#define Compositor_h

#include <pthread.h>
#include <assert.h>
#include <wayland-server.h>

#include "Input.h"
#include "Pointer.h"
#include "Surface.h"
#include "Display.h"
#include "API/Interfaces.h"

namespace Athol {

class Compositor { 
private:
    Compositor() = delete;
    Compositor(const Compositor&) = delete;
    Compositor& operator= (const Compositor&) = delete;

    Compositor(const char name[]);
    ~Compositor();

public:
    // Static methods, to access the singelton compositor.
    inline static Compositor& create (const char name[])
    {
        Compositor* newInstance = new Compositor(name);

        assert ( (newInstance != NULL) && (newInstance == g_instance) );

        return (*newInstance);
    }
    inline void destroy ()
    {
        // Time to commit harakiri :-)
        delete this; 
    }
    inline static Compositor& instance()
    {
        assert (g_instance != NULL);

        return (*g_instance);
    }

    inline bool IsOperational () const 
    {
        return (m_eventfd != -1);
    }

    void run();
    void scheduleRepaint(Surface&);
    void scheduleReposition(Pointer&);

    inline Display& display() { return (m_display); }
    API::Compositor* loader ();

    void repaint();
    void completed(int fd);
    void updated();

private:
    pthread_mutex_t m_syncMutex;

    unsigned long long m_repaintSequence;
    struct wl_event_source* m_vsyncSource;
    struct wl_list m_surfaceList;
    struct wl_list m_pointerList;
    struct wl_list m_callbackList;
    int m_eventfd;

    friend class Loader;
    Display m_display;
    Input m_input;

    static Compositor* g_instance;
};

} // namespace Athol

#endif // Compositor_h
