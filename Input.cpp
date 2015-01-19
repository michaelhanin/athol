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

#include "Input.h"

#include "Athol.h"
#include <cstdio>
#include <fcntl.h>

void Input::initialize(const Athol& athol, std::unique_ptr<API::InputClient> client)
{
    if (!client) {
        std::fprintf(stderr, "[Athol] No input client provided.\n");
        return;
    }

    m_udev = udev_new();
    if (!m_udev) {
        std::fprintf(stderr, "[Athol] Failed to create UDev context.\n");
        return;
    }

    m_libinput = libinput_udev_create_context(&m_interface, nullptr, m_udev);
    if (!m_libinput) {
        std::fprintf(stderr, "[Athol] Failed to create libinput context.\n");
        return;
    }

    if (libinput_udev_assign_seat(m_libinput, "seat0")) {
        std::fprintf(stderr, "[Athol] Failed to assign a seat for libinput.\n");
        return;
    }

    m_eventSource = wl_event_loop_add_fd(wl_display_get_event_loop(athol.display()),
        libinput_get_fd(m_libinput), WL_EVENT_READABLE, dispatch, this);
    m_client = std::move(client);

    std::fprintf(stderr, "[Athol] Input initialized.\n");
    processEvents();
}

struct libinput_interface Input::m_interface = {
    // open_restricted
    [](const char* path, int flags, void*)
    {
        return open(path, flags);
    },
    // close_restricted
    [](int fd, void*)
    {
        close(fd);
    }
};

int Input::dispatch(int, uint32_t, void* data)
{
    auto& input = *reinterpret_cast<Input*>(data);
    libinput_dispatch(input.m_libinput);
    input.processEvents();
    return 0;
}

void Input::processEvents()
{
    while (auto* event = libinput_get_event(m_libinput)) {
        switch (libinput_event_get_type(event)) {
        case LIBINPUT_EVENT_KEYBOARD_KEY:
        {
            auto* keyEvent = libinput_event_get_keyboard_event(event);

            m_client->handleKeyboardEvent(
                libinput_event_keyboard_get_time(keyEvent),
                libinput_event_keyboard_get_key(keyEvent),
                libinput_event_keyboard_get_key_state(keyEvent));
            break;
        }
        case LIBINPUT_EVENT_POINTER_MOTION:
        {
            auto* pointerEvent = libinput_event_get_pointer_event(event);
            m_client->handlePointerMotion(
                libinput_event_pointer_get_time(pointerEvent),
                libinput_event_pointer_get_dx(pointerEvent),
                libinput_event_pointer_get_dy(pointerEvent));
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON:
            auto* pointerEvent = libinput_event_get_pointer_event(event);
            m_client->handlePointerButton(
                libinput_event_pointer_get_time(pointerEvent),
                libinput_event_pointer_get_button(pointerEvent),
                libinput_event_pointer_get_button_state(pointerEvent));
            break;
        }

        libinput_event_destroy(event);
    }
}
