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

#ifndef AtholTypes_h
#define AtholTypes_h

// The #define enables the additional function declarations in the
// header files e.g. vc_dispmanx_get_handle_from_wl_buffer
// BUILD_WAYLAND flag is used in eglext.h
#define BUILD_WAYLAND

#ifdef BROADCOM_NEXUS
#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/default_nexus.h>
#else
#include <bcm_host.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace Athol {

#ifdef BROADCOM_NEXUS

typedef void*                      HandleUpdate;
typedef NEXUS_SurfaceClient*       HandleElement;
typedef NEXUS_DisplayHandle        HandleDisplay;
typedef void*                      HandleResource;
#else
typedef DISPMANX_UPDATE_HANDLE_T   HandleUpdate;
typedef DISPMANX_ELEMENT_HANDLE_T  HandleElement;
typedef DISPMANX_DISPLAY_HANDLE_T  HandleDisplay;
typedef DISPMANX_RESOURCE_HANDLE_T HandleResource;
#endif

} //namespace Athol

#endif // AtholTypes_h
