#include "header.h"
#include <iostream>
#ifdef _WIN32
#include <SDL3/SDL.h>
#endif

bool isVRHeadsetConnected() {
    XrInstance instance{XR_NULL_HANDLE};
    XrSystemId systemId{XR_NULL_SYSTEM_ID};

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(createInfo.applicationInfo.applicationName, "LegoIsland");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "OmniEngine");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult result = xrCreateInstance(&createInfo, &instance);
    if (XR_FAILED(result)) {
        return false;
    }

    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    result = xrGetSystem(instance, &systemInfo, &systemId);
    xrDestroyInstance(instance);

    return XR_SUCCEEDED(result);
}

void showVRNotDetectedMessage() {
#ifdef _WIN32
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Failed to initialize SDL3: " << SDL_GetError() << "\n";
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
    }
}
