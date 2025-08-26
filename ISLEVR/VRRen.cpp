#include "VRRen.h"
#include <SDL3/SDL.h>
#include <iostream>

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    vrContext.eyes.resize(2);
    for (int i = 0; i < 2; ++i) {
        vrContext.eyes[i].eyeIndex = i;
    }

    SDL_Log("[VRRen] Dummy per-eye swapchains created (no GL). Size: %dx%d",
            vrContext.width, vrContext.height);
    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window) {
    if (!vrContext.instance) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[VRRen] No OpenXR instance provided.");
        return false;
    }

    vrContext.window = window;

    XrSystemGetInfo sysInfo{XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    XrResult result = xrGetSystem(*vrContext.instance, &sysInfo, &vrContext.systemId);
    if (XR_FAILED(result)) {
        SDL_Log("[VRRen] Runtime present but no HMD detected.");
        return false;
    }

    vrContext.initialized = true;
    vrContext.width = 1024;
    vrContext.height = 1024;

    if (!VR_CreateSwapchain(vrContext)) return false;

    SDL_Log("[VRRen] VR initialized (dummy, no GL).");
    return true;
}

void VR_Shutdown(VRContext& vrContext) {
    if (vrContext.initialized) {
        vrContext.eyes.clear();

        if (vrContext.session != XR_NULL_HANDLE) {
            vrContext.session = XR_NULL_HANDLE;
        }

        vrContext.initialized = false;
        SDL_Log("[VRRen] VR session shut down.");
    }
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) {
    if (!vrContext.initialized || eyeIndex >= (int)vrContext.eyes.size()) return false;
    SDL_Log("[VRRen] Binding eye %d (dummy, no GL)", eyeIndex);
    return true;
}

bool VR_BeginFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    VR_BindEye(vrContext, 0);
    SDL_Log("[VRRen] Begin frame.");
    return true;
}

void VR_EndFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return;

    if (vrContext.window) {
        SDL_RenderPresent(SDL_GetRenderer(vrContext.window));
    }
    SDL_Log("[VRRen] End frame.");
}

VRViewMatrix VR_GetEyeViewMatrix(int eye) {
    VRViewMatrix mat{};
    for (int i = 0; i < 16; i++) mat.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    return mat;
}

VRProjMatrix VR_GetEyeProjMatrix(int eye) {
    VRProjMatrix mat{};
    for (int i = 0; i < 16; i++) mat.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    return mat;
}
