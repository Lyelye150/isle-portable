#pragma once
#include <openxr/openxr.h>
#include <SDL3/SDL.h>
#include <vector>

struct VREyeFramebuffer {
    int eyeIndex = -1;
};

struct VRContext {
    XrInstance* instance = nullptr;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    bool initialized = false;

    SDL_Window* window = nullptr;

    XrSwapchain swapchain = XR_NULL_HANDLE;
    int32_t width = 0;
    int32_t height = 0;

    std::vector<VREyeFramebuffer> eyes;
};

bool VR_Init(VRContext& vrContext, SDL_Window* window);
void VR_Shutdown(VRContext& vrContext);

bool VR_BeginFrame(VRContext& vrContext);
void VR_EndFrame(VRContext& vrContext);

bool VR_CreateSwapchain(VRContext& vrContext);
bool VR_BindEye(VRContext& vrContext, int eyeIndex);

struct VRViewMatrix { float m[16]; };
struct VRProjMatrix { float m[16]; };

VRViewMatrix VR_GetEyeViewMatrix(int eye);
VRProjMatrix VR_GetEyeProjMatrix(int eye);
