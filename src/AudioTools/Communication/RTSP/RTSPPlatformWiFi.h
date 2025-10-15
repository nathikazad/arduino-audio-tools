/*
 * Author: Phil Schatzmann
 *
 * Based on Micro-RTSP library:
 * https://github.com/geeksville/Micro-RTSP
 * https://github.com/Tomp0801/Micro-RTSP-Audio
 *
 */
#pragma once
#include "WiFi.h"
#include "RTSPPlatform.h"

namespace audio_tools {

/**
 * @brief RTSP platform binding for Arduino WiFi
 *
 * Convenience type alias that binds the generic `RTSPPlatform` to the
 * Arduino WiFi networking stack (`WiFiServer`, `WiFiClient`, `WiFiUDP`).
 * Use this when running the RTSP server over WiFi-capable boards.
 *
 * @ingroup rtsp
 */
using RTSPPlatformWiFi = RTSPPlatform<WiFiServer, WiFiClient, WiFiUDP>;

}  // namespace audio_tools