#include "header.h"
#include <cstring>
#ifdef _WIN32
#include <SDL3/SDL.h>
#endif
#include <openxr/openxr.h>

bool isVRHeadsetConnected() {
    uint32_t extCount = 0;
    XrResult result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);

    if (XR_FAILED(result) || extCount == 0) {
        SDL_Log("[OpenXR] No OpenXR runtime/extensions found.");
        return false;
    }

    XrInstance instance = XR_NULL_HANDLE;
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    memset(&createInfo, 0, sizeof(createInfo));
    strcpy(createInfo.applicationInfo.applicationName, "LEGO Island (VR MODE)");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "OmniEngine");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    result = xrCreateInstance(&createInfo, &instance);
    if (XR_FAILED(result)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[OpenXR] Failed to create instance: %d", result);
        return false;
    }

    XrSystemGetInfo sysInfo{XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrSystemId sysId;
    result = xrGetSystem(instance, &sysInfo, &sysId);

    xrDestroyInstance(instance);

    if (XR_FAILED(result)) {
        SDL_Log("[OpenXR] Runtime present, but no HMD detected.");
        return false;
    }

    SDL_Log("[OpenXR] VR headset detected with systemId=%llu.", (unsigned long long)sysId);
    return true;
}

void checkVR() {
    if (!isVRHeadsetConnected()) {
        SDL_Log("[OpenXR] No VR headset detected.");
    } else {
        SDL_Log("[OpenXR] VR headset detected and ready.");
    }
}
