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
#include <iomanip>

static const char* XrResultToString(XrResult r) {
    static thread_local char buf[64];
    std::snprintf(buf, sizeof(buf), "%d (0x%08x)", (int)r, (unsigned int)r);
    return buf;
}

bool VR_CreateSwapchain(VRContext& vrContext) {
    std::cout << "[VRRen] VR_CreateSwapchain called. initialized=" << vrContext.initialized << "\n";
    if (!vrContext.initialized) {
        std::cout << "[VRRen] VR_CreateSwapchain: not initialized\n";
        return false;
    }

    vrContext.eyes.resize(2);
    for (int i = 0; i < 2; ++i) {
        XrSwapchainCreateInfo swapInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        swapInfo.arraySize = 1;

        swapInfo.format = 0;

        swapInfo.width = 1024;
        swapInfo.height = 1024;
        swapInfo.mipCount = 1;
        swapInfo.faceCount = 1;
        swapInfo.sampleCount = 1;
        swapInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        XrResult result = xrCreateSwapchain(vrContext.session, &swapInfo, &vrContext.eyes[i].swapchain);
        std::cout << "[VRRen] xrCreateSwapchain eye=" << i << " -> " << XrResultToString(result) << "\n";
        if (XR_FAILED(result)) {
            std::cerr << "[VRRen] Failed to create swapchain for eye " << i << "\n";
            return false;
        }
        vrContext.eyes[i].width = swapInfo.width;
        vrContext.eyes[i].height = swapInfo.height;
    }

    std::cout << "[VRRen] Swapchains created (count=" << vrContext.eyes.size() << ")\n";
    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer) {
    std::cout << "[VRRen] VR_Init called. renderer=" << static_cast<int>(renderer) << "\n";
    vrContext.window = window;
    vrContext.renderer = renderer;

    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    std::strncpy(createInfo.applicationInfo.applicationName, "LEGO Island VR", sizeof(createInfo.applicationInfo.applicationName) - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    std::strncpy(createInfo.applicationInfo.engineName, "OmniEngine", sizeof(createInfo.applicationInfo.engineName) - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult r = xrCreateInstance(&createInfo, &vrContext.instance);
    std::cout << "[VRRen] xrCreateInstance -> " << XrResultToString(r) << "\n";
    if (XR_FAILED(r)) {
        std::cerr << "[VRRen] xrCreateInstance failed\n";
        return false;
    }

    XrSystemGetInfo sysInfo{ XR_TYPE_SYSTEM_GET_INFO };
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    r = xrGetSystem(vrContext.instance, &sysInfo, &vrContext.systemId);
    std::cout << "[VRRen] xrGetSystem -> " << XrResultToString(r) << ", systemId=" << vrContext.systemId << "\n";
    if (XR_FAILED(r)) {
        std::cerr << "[VRRen] xrGetSystem failed\n";
        return false;
    }

#ifdef _WIN32
    std::cout << "[VRRen] Windows path: preparing graphics binding for renderer " << static_cast<int>(renderer) << "\n";

    switch (renderer) {
    case VRRenderer::OpenGL1:
    case VRRenderer::OpenGLES2:
    case VRRenderer::OpenGLES3: {
        XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
        graphicsBinding.hDC = nullptr;
        graphicsBinding.hGLRC = nullptr;

        XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
        sessionInfo.next = static_cast<void*>(&graphicsBinding);
        sessionInfo.systemId = vrContext.systemId;

        r = xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session);
        std::cout << "[VRRen] xrCreateSession (OpenGL path) -> " << XrResultToString(r) << "\n";
        if (XR_FAILED(r)) {
            std::cerr << "[VRRen] xrCreateSession (OpenGL path) failed\n";
            return false;
        }
    } break;

    case VRRenderer::SDL3GPU:
    case VRRenderer::Software: {
        XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
        sessionInfo.next = nullptr;
        sessionInfo.systemId = vrContext.systemId;

        r = xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session);
        std::cout << "[VRRen] xrCreateSession (software/SDL3GPU) -> " << XrResultToString(r) << "\n";
        if (XR_FAILED(r)) {
            std::cerr << "[VRRen] xrCreateSession (software/SDL3GPU) failed\n";
            return false;
        }
    } break;

    case VRRenderer::Directx9:
        std::cout << "[VRRen] DirectX9 renderer path selected - not implemented in this example\n";
        break;
    }
#else
    std::cout << "[VRRen] Non-Windows build; session creation path not implemented in this build.\n";
#endif

    vrContext.initialized = true;
    std::cout << "[VRRen] VR_Init succeeded; now creating swapchain(s)...\n";
    bool sc = VR_CreateSwapchain(vrContext);
    std::cout << "[VRRen] VR_CreateSwapchain returned " << (sc ? "success" : "failure") << "\n";
    return sc;
}

void VR_Shutdown(VRContext& vrContext) {
    std::cout << "[VRRen] VR_Shutdown called\n";
    if (!vrContext.initialized) {
        std::cout << "[VRRen] VR_Shutdown: already not initialized\n";
        return;
    }

    for (auto& eye : vrContext.eyes) {
        if (eye.swapchain != XR_NULL_HANDLE) {
            XrResult r = xrDestroySwapchain(eye.swapchain);
            std::cout << "[VRRen] xrDestroySwapchain -> " << XrResultToString(r) << "\n";
            eye.swapchain = XR_NULL_HANDLE;
        }
    }
    vrContext.eyes.clear();

    if (vrContext.session != XR_NULL_HANDLE) {
        XrResult r = xrDestroySession(vrContext.session);
        std::cout << "[VRRen] xrDestroySession -> " << XrResultToString(r) << "\n";
        vrContext.session = XR_NULL_HANDLE;
    }
    if (vrContext.instance != XR_NULL_HANDLE) {
        XrResult r = xrDestroyInstance(vrContext.instance);
        std::cout << "[VRRen] xrDestroyInstance -> " << XrResultToString(r) << "\n";
        vrContext.instance = XR_NULL_HANDLE;
    }

    vrContext.initialized = false;
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) {
    return vrContext.initialized && eyeIndex < static_cast<int>(vrContext.eyes.size());
}

bool VR_BeginFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return false;
    return true;
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
