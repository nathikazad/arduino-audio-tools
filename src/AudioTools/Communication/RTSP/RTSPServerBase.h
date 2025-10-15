/*
 * Author: Phil Schatzmann
 *
 * Based on Micro-RTSP library:
 * https://github.com/geeksville/Micro-RTSP
 * https://github.com/Tomp0801/Micro-RTSP-Audio
 */
#pragma once
#include "RTSPSession.h"
#include "RTSPAudioStreamer.h"
#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#endif

namespace audio_tools {

/**
 * @brief RTSPServerBase - Shared logic for RTSPServer and RTSPServerTaskless
 *
 * This class contains all protocol, session, and connection logic, but no task/timer code.
 * Derived classes implement scheduling: either with tasks (RTSPServer) or manual loop (RTSPServerTaskless).
 */
template <typename Platform>
class RTSPServerBase {
 public:
  RTSPServerBase(RTSPAudioStreamerBase<Platform>& streamer, int port = 8554)
      : streamer(&streamer), port(port) {
    server = nullptr;
  }

  ~RTSPServerBase() { stop(); }

  void setOnSessionPath(bool (*cb)(const char* path, void* ref), void* ref = nullptr) {
    onSessionPathCb = cb;
    onSessionPathRef = ref;
  }

#ifdef ESP32
  bool begin(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    WiFi.setSleep(WIFI_PS_NONE);
    Serial.println();
    Serial.print("connect to rtsp://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(port);
    Serial.println();
    return begin();
  }
#endif

  virtual bool begin() {
    streamer->initAudioSource();
    if (server == nullptr) {
      server = Platform::createServer(port);
      LOGI("RTSP server started on port %d", port);
    }
    return server != nullptr;
  }

  void end() {
    if (server) {
      delete server;
      server = nullptr;
    }
    client_count = 0;
    if (rtspSession) {
      delete rtspSession;
      rtspSession = nullptr;
    }
  }

  int clientCount() { return client_count; }
  operator bool() { return client_count > 0; }
  void setSessionTimeoutMs(unsigned long ms) { sessionTimeoutMs = ms; }

 protected:
  int port = 554;
  typename Platform::TcpServerType* server = nullptr;
  typename Platform::TcpClientType client;
  RTSPAudioStreamerBase<Platform>* streamer = nullptr;
  int client_count = 0;
  RtspSession<Platform>* rtspSession = nullptr;
  bool (*onSessionPathCb)(const char*, void*) = nullptr;
  void* onSessionPathRef = nullptr;
  unsigned long sessionTimeoutMs = 60000; // 60 seconds
  unsigned long lastRequestTime = 0; 

  // Accept new client if none is active
  void acceptClient() {
    if (client_count == 0 && server) {
      auto newClient = Platform::getAvailableClient(server);
      if (newClient.connected()) {
        client = newClient;
        client_count++;
        // Create session
        rtspSession = new RtspSession<Platform>(client, *streamer);
        if (onSessionPathCb) {
          rtspSession->setOnSessionPath(onSessionPathCb, onSessionPathRef);
        }
        lastRequestTime = millis();
      }
    }
  }

  // Handle session if active
  void handleSession() {
    if (client_count > 0 && rtspSession) {
      uint32_t timeout = 30;
      bool gotRequest = rtspSession->handleRequests(timeout);
      if (gotRequest) {
        lastRequestTime = millis();
      }
      // If streaming, check for session timeout
      if (sessionTimeoutMs > 0 && rtspSession->isStreaming()) {
        if ((millis() - lastRequestTime) > sessionTimeoutMs) {
          // Timeout, mark session closed
          rtspSession->closeSession();
         /* If you want to trigger cleanup, you could also call:
           rtspSession->handleRtspTeardown();
           (if public, or expose a public method for teardown)
         */
        }
      }
      // Clean up if session closed
      if (!rtspSession->isSessionOpen()) {
        delete rtspSession;
        rtspSession = nullptr;
        if (client.connected()) {
          Platform::closeSocket(&client);
        }
        client_count--;
      }
    }
  }
};

} // namespace audio_tools
