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

#ifdef BROADCOM_NEXUS

#include <refsw/nexus_config.h>
#include <refsw/nexus_platform.h>
#include <refsw/nexus_display.h>
#include <refsw/nexus_core_utils.h>
#include <refsw/default_nexus.h>

static NXPL_PlatformHandle  nxpl_handle = 0;
static NEXUS_VideoFormat gs_requested_video_format  = static_cast<NEXUS_VideoFormat>(~0);

typedef struct {
  NEXUS_VideoFormat format;
  const char* name;
} formatTable;

static formatTable gs_possible_formats[] = {

  { NEXUS_VideoFormat_e1080i,     "1080i" },
  { NEXUS_VideoFormat_e720p,      "720p" },
  { NEXUS_VideoFormat_e480p,      "480p" },
  { NEXUS_VideoFormat_eNtsc,      "480i" },
  { NEXUS_VideoFormat_e720p50hz,  "720p50Hz" },
  { NEXUS_VideoFormat_e1080p24hz, "1080p24Hz" },
  { NEXUS_VideoFormat_e1080i50hz, "1080i50Hz" },
  { NEXUS_VideoFormat_e1080p50hz, "1080p50Hz" },
  { NEXUS_VideoFormat_e1080p60hz, "1080p60Hz" },
  { NEXUS_VideoFormat_ePal,       "576i" },
  { NEXUS_VideoFormat_e576p,      "576p" },

  /* END of ARRAY */
  { static_cast<NEXUS_VideoFormat>(~0), NULL }
};

#if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM

static void hotplug_callback(void *pParam, int iParam)
{
   NEXUS_HdmiOutputStatus status;
   NEXUS_HdmiOutputHandle hdmi = (NEXUS_HdmiOutputHandle)pParam;
   NEXUS_DisplayHandle display = (NEXUS_DisplayHandle)iParam;

   NEXUS_HdmiOutput_GetStatus(hdmi, &status);

   fprintf (stdout, "[Athol] HDMI hotplug event: %s", status.connected?"connected":"not connected");

   /* the app can choose to switch to the preferred format, but it's not required. */
   if (status.connected)
   {
      NEXUS_DisplaySettings displaySettings;
      NEXUS_Display_GetSettings(display, &displaySettings);

      fprintf (stdout, "[Athol] Switching to preferred format %d", status.preferredVideoFormat);

      /* See if the format is supprted */
      if ( (gs_requested_video_format != static_cast<NEXUS_VideoFormat>(~0)) && (status.videoFormatSupported[gs_requested_video_format]) )
      {
        displaySettings.format = gs_requested_video_format;
      }
      else
      {
        displaySettings.format = status.preferredVideoFormat;
      }

      NEXUS_Display_SetSettings(display, &displaySettings);
   }
}

static void initHDMIOutput(NEXUS_DisplayHandle display)
{
  unsigned int index = 0;
  const char* environment = getenv ( "ATHOL_VIDEO_FORMAT" );

  if ( (environment != NULL) && (environment[0] != '\0') )
  {
    fprintf (stdout, "[Athol] Defined QT_VIDEO_FORMAT: <%s>\n", environment);

    // See if we can find it.
    while ( (gs_possible_formats[index].name != NULL) && (strcmp(gs_possible_formats[index].name, environment) != 0) )
    {
      index++;
    }
    if (gs_possible_formats[index].name != NULL)
    {
      gs_requested_video_format = gs_possible_formats[index].format;
    }
  }

  NEXUS_HdmiOutputSettings      hdmiSettings;
  NEXUS_PlatformConfiguration   platform_config;

  NEXUS_Platform_GetConfiguration(&platform_config);

  if (platform_config.outputs.hdmi[0])
  {
    NEXUS_Display_AddOutput(display, NEXUS_HdmiOutput_GetVideoConnector(platform_config.outputs.hdmi[0]));

    /* Install hotplug callback -- video only for now */
    NEXUS_HdmiOutput_GetSettings(platform_config.outputs.hdmi[0], &hdmiSettings);

    hdmiSettings.hotplugCallback.callback = hotplug_callback;
    hdmiSettings.hotplugCallback.context = platform_config.outputs.hdmi[0];
    hdmiSettings.hotplugCallback.param = (int)display;

    NEXUS_HdmiOutput_SetSettings(platform_config.outputs.hdmi[0], &hdmiSettings);

    /* Force a hotplug to switch to a supported format if necessary */
    hotplug_callback(platform_config.outputs.hdmi[0], (int)display);
  }
}

#endif

static bool InitPlatform ( void )
{
  NEXUS_Error err;

  NEXUS_PlatformSettings platform_settings;

  fprintf (stdout, "[Athol] Initialise Nexus Platform. GetDDefaultSettings().\n");

  /* Initialise the Nexus platform */
  NEXUS_Platform_GetDefaultSettings(&platform_settings);
  platform_settings.openFrontend = false;

  fprintf (stdout, "[Athol] Initialise Nexus Platform. Init().\n");
  /* Initialise the Nexus platform */
  err = NEXUS_Platform_Init(&platform_settings);

  return (err ? false : true);
}

static NEXUS_DisplayHandle OpenDisplay ( const unsigned int ID, unsigned int& width, unsigned int& height )
{
  NEXUS_DisplayHandle    display = NULL;
  NEXUS_DisplaySettings  display_settings;

  NEXUS_Display_GetDefaultSettings(&display_settings);

  display = NEXUS_Display_Open(ID, &display_settings);

  if (display != NULL)
  {
    NEXUS_VideoFormatInfo   video_format_info;
    NEXUS_GraphicsSettings  graphics_settings;
    NEXUS_Display_GetGraphicsSettings(display, &graphics_settings);

    graphics_settings.horizontalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;
    graphics_settings.verticalFilter = NEXUS_GraphicsFilterCoeffs_eBilinear;

    NEXUS_Display_SetGraphicsSettings(display, &graphics_settings);

    #if NEXUS_NUM_HDMI_OUTPUTS && !NEXUS_DTV_PLATFORM
    fprintf (stdout, "[Athol] Init HDMI.\n");
    InitHDMIOutput(display);
    #endif

    NEXUS_Display_GetSettings(display, &display_settings);
    NEXUS_VideoFormat_GetInfo(display_settings.format, &video_format_info);

    fprintf (stdout, "[Athol] Register display.\n");
    NXPL_RegisterNexusDisplayPlatform ( &nxpl_handle, display );
  }

  return display;
}

#else

#include <bcm_host.h>

#endif

// ----------------------------------------------------------------------------------------------------------
// END PLATFORM DEPENDEND INITIALIZATION/DEINITIALIZATION
// ----------------------------------------------------------------------------------------------------------

#include "Display.h"
#include "Compositor.h"

namespace Athol {

// ----------------------------------------------------------------------------------------------------------
// CLASS: Display
// ----------------------------------------------------------------------------------------------------------

Display::Display()
    : m_display(wl_display_create())
    , m_width(0)
    , m_height(0)
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
  if (InitPlatform() == false)
  {
    fprintf (stderr, "[Athol] Could not initilize the platform!!!\n");
  }
  else
  {
    m_width  = ~0;
    m_height = ~0;

    environment = getenv( "ATHOL_SCREEN_WIDTH" );

    if ( (environment != NULL) && (environment[0] != '\0') ) {
      m_width = ::atoi(environment);

      if (m_width == 0) {
        m_width = 1280;
        fprintf (stdout, "[Athol] Screen width defined as 0, using hard coded width: %d\n", m_width);
      } else {
        fprintf (stdout, "[Athol] Screen width defined as: %d\n", m_width);
      }
    } else {
      fprintf (stdout, "[Athol] No override defined in ATHOL_SCREEN_WIDTH\n");
    }

    environment = qgetenv( "ATHOL_SCREEN_HEIGHT" );

    if ( (environment != NULL) && (environment[0] != '\0') ) {
      m_height = ::atoi(environment);
    
      if (m_height == 0) {
        m_height = 720;
        fprintf (stdout, "[Athol] Screen height defined as 0, using hard coded height: %d\n", m_height);
      } else {
        fprintf (stdout, "[Athol] Screen height defined as: %d\n", m_height);
      }
    } else {
      fprintf (stdout, "[Athol] No override defined in ATHOL_SCREEN_HEIGHT\n");
    }

    m_displayHandle = OpenDisplay (0, width, height); 

    if (m_displayHandle == NULL) {
      m_width = 0;
      m_height = 0;
      fprintf (stdout, "[Athol] Failed to open the display\n");
    } else {
      m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      eglInitialize(m_eglDisplay, nullptr, nullptr);

      fprintf (stdout, "[Athol] Surface dimensions: %d x %d\n", m_width, m_height);
    }
  }
}

void Display::deinitialize( void )
{
   if ( m_displayHandle != 0 )
   {
       NXPL_UnregisterNexusDisplayPlatform ( nxpl_handle );
   }

   NEXUS_Platform_Uninit ();
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
#ifdef BROADCOM_NEXUS
    m_updateHandle = nullptr;
#else
    m_updateHandle = vc_dispmanx_update_start(10);
#endif
}

Update::~Update()
{
#ifndef BROADCOM_NEXUS
    vc_dispmanx_update_submit(m_updateHandle, complete, &(Athol::Compositor::instance()));
#endif
}

} // namespace Athol
