#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#include "miniwin/windows.h"
#include "VRRen.h"
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <cstring>
#include <iostream>

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    vrContext.eyes.resize(2);
    for (int i = 0; i < 2; ++i) {
        XrSwapchainCreateInfo swapInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapInfo.arraySize = 1;

        switch (vrContext.renderer) {
            case VRRenderer::OpenGL1:
            case VRRenderer::OpenGLES2:
            case VRRenderer::OpenGLES3:
            case VRRenderer::Directx9:
            case VRRenderer::SDL3GPU:
            case VRRenderer::Software:
                swapInfo.format = 0;
                break;
        }

        swapInfo.width = 1024;
        swapInfo.height = 1024;
        swapInfo.mipCount = 1;
        swapInfo.faceCount = 1;
        swapInfo.sampleCount = 1;
        swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        XrResult result = xrCreateSwapchain(vrContext.session, &swapInfo, &vrContext.eyes[i].swapchain);
        if (XR_FAILED(result)) {
            std::cerr << "[VRRen] Failed to create swapchain for eye " << i << std::endl;
            return false;
        }
        vrContext.eyes[i].width = swapInfo.width;
        vrContext.eyes[i].height = swapInfo.height;
    }

    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer) {
    vrContext.window = window;
    vrContext.renderer = renderer;

    if (XR_FAILED(xrCreateInstance(&createInfo, &vrContext.instance))) return false;

    XrSystemGetInfo sysInfo{XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (XR_FAILED(xrGetSystem(vrContext.instance, &sysInfo, &vrContext.systemId))) return false;

#ifdef _WIN32
    switch (renderer) {
        case VRRenderer::OpenGL1:
        case VRRenderer::OpenGLES2:
        case VRRenderer::OpenGLES3: {
            XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
            graphicsBinding.hDC   = nullptr;
            graphicsBinding.hGLRC = nullptr;

            XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
            sessionInfo.next     = static_cast<void*>(&graphicsBinding);
            sessionInfo.systemId = vrContext.systemId;
            if (XR_FAILED(xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session))) return false;
        } break;

        case VRRenderer::SDL3GPU:
        case VRRenderer::Software: {
            XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
            sessionInfo.next     = nullptr;
            sessionInfo.systemId = vrContext.systemId;
            if (XR_FAILED(xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session))) return false;
        } break;

        case VRRenderer::Directx9:
            break;
    }
#endif

    vrContext.initialized = true;
    return VR_CreateSwapchain(vrContext);
}

void VR_Shutdown(VRContext& vrContext) {
    if (!vrContext.initialized) return;

    for (auto& eye : vrContext.eyes) {
        if (eye.swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.swapchain);
            eye.swapchain = XR_NULL_HANDLE;
        }
    }
    vrContext.eyes.clear();

    if (vrContext.session != XR_NULL_HANDLE) {
        xrDestroySession(vrContext.session);
        vrContext.session = XR_NULL_HANDLE;
    }
    if (vrContext.instance != XR_NULL_HANDLE) {
        xrDestroyInstance(vrContext.instance);
        vrContext.instance = XR_NULL_HANDLE;
    }

    vrContext.initialized = false;
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) {
    return vrContext.initialized && eyeIndex < static_cast<int>(vrContext.eyes.size());
}

bool VR_BeginFrame(VRContext& vrContext) {
    return vrContext.initialized;
}

void VR_EndFrame(VRContext& vrContext) {}

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
