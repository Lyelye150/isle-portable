#include "header.h"
#include <iostream>
#include <cstring>
#ifdef _WIN32
#include <SDL3/SDL.h>
#endif

#include <openxr/openxr.h>

bool isVRHeadsetConnected() {
    uint32_t extCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    if (XR_FAILED(result)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[OpenXR] Failed to enumerate extensions: %d", result);
        return false;
    }

    if (extCount == 0) {
        SDL_Log("[OpenXR] No OpenXR runtime/extensions found.");
        return false;
    }

    SDL_Log("[OpenXR] %u OpenXR extensions found. Runtime likely available.", extCount);
    return true;
}

void showVRNotDetectedMessage() {
#ifdef _WIN32
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize SDL3: %s", SDL_GetError());
        return;
    }

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                             "VR Headset Not Detected",
                             "No compatible VR headset detected. Please connect a PCVR or Meta Quest headset.",
                             nullptr);
#else
    SDL_Log("[OpenXR] No compatible VR headset detected. Please connect a PCVR or Meta Quest headset.");
#endif
}

void checkVR() {
    if (!isVRHeadsetConnected()) {
        showVRNotDetectedMessage();
        exit(EXIT_FAILURE);
    } else {
        SDL_Log("[OpenXR] OpenXR runtime detected.");
    }
}
