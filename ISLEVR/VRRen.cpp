#include "VRRen.h"
#include <SDL3/SDL_opengl.h>
#include <iostream>
#include <cstring>

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    vrContext.eyes.resize(2);
    for (int i = 0; i < 2; ++i) {
        XrSwapchainCreateInfo swapInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        swapInfo.arraySize = 1;
        swapInfo.format = GL_SRGB8_ALPHA8;
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

    std::cout << "[VRRen] Swapchains created." << std::endl;
    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window) {
    vrContext.window = window;

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy(createInfo.applicationInfo.applicationName, "MiniwinVR");
    createInfo.applicationInfo.applicationVersion = 1;
    strcpy(createInfo.applicationInfo.engineName, "Miniwin");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    if (XR_FAILED(xrCreateInstance(&createInfo, &vrContext.instance))) {
        std::cerr << "[VRRen] Failed to create XR instance." << std::endl;
        return false;
    }

    XrSystemGetInfo sysInfo{XR_TYPE_SYSTEM_GET_INFO};
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (XR_FAILED(xrGetSystem(vrContext.instance, &sysInfo, &vrContext.systemId))) {
        std::cerr << "[VRRen] No VR system detected." << std::endl;
        return false;
    }

#ifdef _WIN32
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
    graphicsBinding.hDC = GetDC(SDL_GetWindowWMInfo(window)->info.win.window);
    graphicsBinding.hGLRC = wglGetCurrentContext();
#else
    XrGraphicsBindingOpenGLXlibKHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR};
    graphicsBinding.xDisplay = SDL_GetWindowWMInfo(window)->info.x11.display;
    graphicsBinding.glxDrawable = SDL_GetWindowWMInfo(window)->info.x11.window;
    graphicsBinding.glxContext = glXGetCurrentContext();
#endif

    XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = vrContext.systemId;

    if (XR_FAILED(xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session))) {
        std::cerr << "[VRRen] Failed to create XR session." << std::endl;
        return false;
    }

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
    std::cout << "[VRRen] VR session shut down." << std::endl;
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) {
    return vrContext.initialized && eyeIndex < (int)vrContext.eyes.size();
}

bool VR_BeginFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return false;
    return true;
}

void VR_EndFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return;
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
