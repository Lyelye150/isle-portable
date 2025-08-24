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
    if (XR_FAILED(result) || extCount == 0) {
#ifdef _WIN32
        SDL_Log("[OpenXR] No runtime/extensions available.");
#else
        std::cerr << "[OpenXR] No runtime/extensions available.\n";
#endif
        return false;
    }

    XrInstance instance{XR_NULL_HANDLE};
    XrInstanceCreateInfo createInfo{};
    createInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;

    strncpy(createInfo.applicationInfo.applicationName, "LegoIsland", XR_MAX_APPLICATION_NAME_SIZE - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    strncpy(createInfo.applicationInfo.engineName, "OmniEngine", XR_MAX_ENGINE_NAME_SIZE - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    result = xrCreateInstance(&createInfo, &instance);
    if (XR_FAILED(result)) {
#ifdef _WIN32
        SDL_Log("[OpenXR] xrCreateInstance failed: %d", result);
#else
        std::cerr << "[OpenXR] xrCreateInstance failed: " << result << "\n";
#endif
        return false;
    }

#ifdef _WIN32
    SDL_Log("[OpenXR] xrCreateInstance succeeded.");
#endif

    XrSystemId systemId{XR_NULL_SYSTEM_ID};
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    result = xrGetSystem(instance, &systemInfo, &systemId);
    xrDestroyInstance(instance);

    if (XR_FAILED(result)) {
#ifdef _WIN32
        SDL_Log("[OpenXR] xrGetSystem failed: %d", result);
#else
        std::cerr << "[OpenXR] xrGetSystem failed: " << result << "\n";
#endif
        return false;
    }

#ifdef _WIN32
    SDL_Log("[OpenXR] xrGetSystem succeeded. systemId = %llu", (unsigned long long)systemId);
#endif

    return true;
}

void showVRNotDetectedMessage() {
#ifdef _WIN32
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL3: %s", SDL_GetError());
        return;
    }

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                             "VR Headset Not Detected",
                             "No compatible VR headset detected. Please connect a PCVR or Meta Quest headset.",
                             nullptr);
#else
    std::cerr << "No compatible VR headset detected. Please connect a PCVR or Meta Quest headset.\n";
#endif
}

void checkVR() {
    if (!isVRHeadsetConnected()) {
        showVRNotDetectedMessage();
        exit(EXIT_FAILURE);
    } else {
#ifdef _WIN32
        SDL_Log("[OpenXR] VR headset detected and OpenXR runtime available.");
#else
        std::cout << "[OpenXR] VR headset detected and OpenXR runtime available.\n";
#endif
    }
}
