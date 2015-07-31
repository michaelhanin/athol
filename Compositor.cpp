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

#include <cstdio>
#include <cstdlib>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <unistd.h>

#include "Compositor.h"

namespace Athol {

// ----------------------------------------------------------------------------------------------------
// Create a loader module for WPE 
// ----------------------------------------------------------------------------------------------------
class Loader : public API::Compositor
{
private:
    Loader (const Loader&);
    Loader& operator= (const Loader&);

public:
    Loader() = default;
    ~Loader() = default;
    
    virtual uint32_t width()
    {
        return (Athol::Compositor::instance().display().width());
    }
    virtual uint32_t height()
    {
        return (Athol::Compositor::instance().display().height());
    }
    virtual struct wl_display* display() const
    {
        return (Athol::Compositor::instance().display().display());
    }
    virtual void initializeInput(std::unique_ptr<API::InputClient> client)
    {
        Athol::Compositor::instance().m_input.initialize(std::move(client));
    }
};

static Loader g_loader;

// ----------------------------------------------------------------------------------------------------
// Wayland interface implementation for compositor
// ----------------------------------------------------------------------------------------------------
const struct wl_compositor_interface g_compositorInterface = {

    // create_surface
    [](struct wl_client* client, struct wl_resource* resource, uint32_t id)
    {
        new Surface(Athol::Compositor::instance().display(), client, resource, id);
    },

    // create_region
    [](struct wl_client*, struct wl_resource*, uint32_t)
    {
        std::fprintf(stderr, "g_compositorInterface::create_region not implemented\n");
    }
};

// ----------------------------------------------------------------------------------------------------
// Static links for Wayland callbacks.
// ----------------------------------------------------------------------------------------------------

static void bindCompositorInterface(struct wl_client* client, void* data, uint32_t version, uint32_t id)
{
    struct wl_resource* resource = wl_resource_create(client, &wl_compositor_interface, 3, id);
    if (!resource) {
	wl_client_post_no_memory(client);
    } 
    else {
        wl_resource_set_implementation(resource, &g_compositorInterface, data, nullptr);
    }
}

static void repaint(void* data)
{
    auto* compositor = static_cast<Compositor*>(data);

    assert (compositor != nullptr);

    if (compositor != nullptr) {
        compositor->repaint();
    }
}

static int completed (int fd, uint32_t mask, void* data)
{
    if (mask == WL_EVENT_READABLE) {

        auto* compositor= static_cast<Compositor*>(data);

        assert (compositor != nullptr);

        if (compositor != nullptr) {
            compositor->completed(fd);
        }
    }

    return 1;
}

// ----------------------------------------------------------------------------------------------------------
// Object section. From here we implement the class declarations.
// ----------------------------------------------------------------------------------------------------------
// CLASS: Compositor
// ----------------------------------------------------------------------------------------------------------
/* static */ Compositor* Compositor::g_instance = nullptr;

Compositor::Compositor(const char* socketName)
    : m_repaintSequence(0)
    , m_eventfd(-1)
    , m_display()
    , m_input()
{
    assert (g_instance == nullptr);

    pthread_mutexattr_t  structAttributes;

    // Create a recursive mutex for this process (no named version, use semaphore)
    if ( (pthread_mutexattr_init(&structAttributes) != 0) ||
         (pthread_mutexattr_settype(&structAttributes, PTHREAD_MUTEX_RECURSIVE) != 0) ||
         (pthread_mutex_init(&m_syncMutex, &structAttributes) != 0) ) {

        // That will be the day, if this fails...
        assert (false);
    }

    if (m_display.isAvailable() == true) {
        fprintf (stdout, "[Athol] Display is available.\n");
        wl_display_add_socket(m_display.display(), socketName);
        setenv("WAYLAND_DISPLAY", socketName, 1);

        if (wl_global_create(m_display.display(), &wl_compositor_interface, 3, this, bindCompositorInterface)) {
            fprintf (stdout, "[Athol] Attached compositor interface to the display.\n");
            wl_list_init(&m_surfaceList);
            wl_list_init(&m_pointerList);
            wl_list_init(&m_callbackList);

            //m_eventfd = eventfd(0ULL, EFD_CLOEXEC | EFD_NONBLOCK);
            m_eventfd = eventfd(0ULL, EFD_NONBLOCK);
            if (m_eventfd != -1) {
                fprintf (stdout, "[Athol] Communication file descriptor created.\n");
                m_vsyncSource = wl_event_loop_add_fd(wl_display_get_event_loop(m_display.display()),
                                                     m_eventfd, WL_EVENT_READABLE, Athol::completed, this);
            } else {
                fprintf (stderr, "[Athol] Could not create EventFileDescriptor. Error [%d]\n", errno);
            }
        }
    }

    g_instance = this;
}

Compositor::~Compositor()
{
    if (m_eventfd != -1) 
        close (m_eventfd);
    pthread_mutex_destroy(&m_syncMutex);
    g_instance = nullptr;
}

void Compositor::run()
{
    wl_display_run(m_display.display());
}

API::Compositor* Compositor::loader ()
{
    return (&g_loader);
}

// Wno-invalid-offsetof (C++ and Objective-C++ only)
// 
// Suppress warnings from applying the ‘offsetof’ macro to a non-POD type.
//
// According to the 1998 ISO C++ standard, applying ‘offsetof’ to a non-POD type is undefined. In 
// existing C++ implementations, however, ‘offsetof’ typically gives meaningful results even when 
// applied to certain kinds of non-POD types. (Such as a simple ‘struct’ that fails to be a POD 
// type only by virtue of having a constructor.) This flag is for users who are aware that they 
// are writing nonportable code and who have deliberately chosen to ignore the warning about it.
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

void Compositor::scheduleRepaint(Surface& surface)
{
    pthread_mutex_lock(&m_syncMutex);    

    // See if it is already in the list, if so, we do not need to add it.
    Surface* listedSurface;
    bool found = false;

    wl_list_for_each(listedSurface, &m_surfaceList, link) {
        if (&surface == listedSurface) {
            found = true;
            break;
        }
    }

    if (found == false) {
        // It is not pending an update, lets schedule it for repaint.
        wl_list_insert(m_surfaceList.prev, &surface.link);

        if ( wl_list_empty (&m_callbackList) && ((wl_list_length (&m_surfaceList) + wl_list_length (&m_pointerList)) == 1) ) {
            // It is even the forst entry, make wayland aware of a required repaint.
            wl_event_loop_add_idle( wl_display_get_event_loop(m_display.display()), Athol::repaint, this);
        }
    }
   
    pthread_mutex_unlock(&m_syncMutex);    
}

void Compositor::scheduleReposition(Pointer& pointer)
{
    pthread_mutex_lock(&m_syncMutex);    

    // See if it is already in the list, if so, we do not need to add it.
    Pointer* listedPointer;
    bool found = false;

    wl_list_for_each(listedPointer, &m_pointerList, link) {
        if (&pointer == listedPointer) {
            found = true;
            break;
        }
    }

    if (found == false) {
        // It is not pending an update, lets schedule it for repaint.
        wl_list_insert(m_pointerList.prev, &pointer.link);

        if ( wl_list_empty (&m_callbackList) && ((wl_list_length (&m_surfaceList) + wl_list_length (&m_pointerList)) == 1) ) {
            // It is even the first entry, make wayland aware of a required repaint.
            wl_event_loop_add_idle( wl_display_get_event_loop(m_display.display()), Athol::repaint, this);
        }
    }
   
    pthread_mutex_unlock(&m_syncMutex);  
}

void Compositor::repaint()
{
    pthread_mutex_lock(&m_syncMutex);    

    // only start a new one if we are not pending on some callbacks !!
    if (wl_list_empty (&m_callbackList)) {

        Update update(m_display.width(), m_display.height());

        Surface* surface;
        Surface* nextSurface;
        wl_list_for_each_safe(surface, nextSurface, &m_surfaceList, link) {
            surface->repaint(update);
            wl_list_insert(m_callbackList.prev, &(surface->link));
        }
  
        Pointer* pointer;
        wl_list_for_each(pointer, &m_pointerList, link) {
            pointer->reposition(update);
        }
  
        wl_list_init(&m_surfaceList);
        wl_list_init(&m_pointerList);
    }

    pthread_mutex_unlock(&m_syncMutex);    
}

void Compositor::completed (int fd)
{
    pthread_mutex_lock(&m_syncMutex);    

    assert (fd == m_eventfd);

    uint64_t time;
    ssize_t ret = read(m_eventfd, &time, sizeof(time));

    if (ret == sizeof(time)) {

        // And again we completed a sequence.
        m_repaintSequence++;

        Surface* surface;
        wl_list_for_each(surface, &m_callbackList, link)
            surface->dispatchFrameCallbacks(time);

        // If there were callbacks pending, no other repaint should have been started.
        if (!wl_list_empty (&m_callbackList)) {

            wl_list_init(&m_callbackList);

            if ( !wl_list_empty (&m_surfaceList) || !wl_list_empty (&m_pointerList) )  {
                // Seems there is a redraw pending
                wl_event_loop_add_idle( wl_display_get_event_loop(m_display.display()), Athol::repaint, this);
            }
        }
    }

    pthread_mutex_unlock(&m_syncMutex);    
}

#pragma GCC diagnostic pop ignored "-Winvalid-offsetof"

void Compositor::updated()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    ssize_t ret = write(m_eventfd, &time, sizeof(time));
    if (ret != sizeof(time))
        return; // FIXME: At least log this.
}

} // namespace Athol
