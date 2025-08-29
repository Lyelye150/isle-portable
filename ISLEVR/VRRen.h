#pragma once
#include <SDL3/SDL.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>

struct VRViewMatrix {
    float m[16];
};

struct VRProjMatrix {
    float m[16];
};

struct VREye {
    XrSwapchain swapchain = XR_NULL_HANDLE;
    int width = 0;
    int height = 0;
};

struct VRContext {
    XrInstance instance = XR_NULL_HANDLE;
    XrSession session = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSpace appSpace = XR_NULL_HANDLE;

    SDL_Window* window = nullptr;
    std::vector<VREye> eyes;
    bool initialized = false;
};

bool VR_Init(VRContext& vrContext, SDL_Window* window);
void VR_Shutdown(VRContext& vrContext);
bool VR_CreateSwapchain(VRContext& vrContext);

bool VR_BindEye(VRContext& vrContext, int eyeIndex);
bool VR_BeginFrame(VRContext& vrContext);
void VR_EndFrame(VRContext& vrContext);

VRViewMatrix VR_GetEyeViewMatrix(int eye);
VRProjMatrix VR_GetEyeProjMatrix(int eye);
