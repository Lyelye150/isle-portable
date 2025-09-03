#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <vector>
#include <cstdint>

class VRRen {
public:
    VRRen();
    ~VRRen();

    bool Initialize(SDL_Window* window);
    void Shutdown();
    void RenderFrame();

private:
    XrInstance m_instance;
    XrSystemId m_systemId;
    XrSession m_session;
    XrSpace m_appSpace;
    SDL_Window* m_window;
    SDL_GLContext m_glContext;
};

// --- Added this to fix a error during the building process ---

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    } u;
    long long QuadPart;
} LARGE_INTEGER;

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// ------------------------------------------------ - big line Lyelye 2025 (you can remove this (MESSAGE NOT THE CODE) or not)

struct VRViewMatrix { float m[16]; };
struct VRProjMatrix { float m[16]; };

struct VREye {
    XrSwapchain swapchain = XR_NULL_HANDLE;
    int width = 0;
    int height = 0;
    std::vector<XrSwapchainImageOpenGLKHR> xrImages;
    std::vector<GLuint> glImages;
};

enum class VRRenderer {
    OpenGL1,
    OpenGLES2,
    OpenGLES3,
    Directx9,
    SDL3GPU,
    Software
};

struct VRContext {
    XrInstance instance = XR_NULL_HANDLE;
    XrSession session = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSpace appSpace = XR_NULL_HANDLE;
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    std::vector<VREye> eyes;
    bool initialized = false;
    VRRenderer renderer = VRRenderer::OpenGL1;
    XrFrameState frameState{ XR_TYPE_FRAME_STATE };
    std::vector<uint32_t> acquiredImageIndices;
    std::vector<bool> imageAcquired;
    std::vector<XrView> lastViews;
};

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer = VRRenderer::OpenGL1);
void VR_Shutdown(VRContext& vrContext);
bool VR_CreateSwapchain(VRContext& vrContext);
bool VR_BeginFrame(VRContext& vrContext);
void VR_EndFrame(VRContext& vrContext);
GLuint VR_AcquireEye(VRContext& vrContext, int eyeIndex);
bool VR_ReleaseEye(VRContext& vrContext, int eyeIndex);
bool VR_BindEye(VRContext& vrContext, int eyeIndex);
void VR_GetEyeExtent(VRContext& vrContext, int eyeIndex, int& outWidth, int& outHeight);
VRViewMatrix VR_GetEyeViewMatrix(const VRContext& vrContext, int eyeIndex);
VRProjMatrix VR_GetEyeProjMatrix(const VRContext& vrContext, int eyeIndex);
