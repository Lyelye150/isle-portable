#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#include "VRRen.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include "miniwin/windows.h"

static const char* XrResultToString(XrResult r) {
    static thread_local char buf[64];
    std::snprintf(buf, sizeof(buf), "%d (0x%08x)", (int)r, (unsigned int)r);
    return buf;
}

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

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
        if (XR_FAILED(result)) return false;

        vrContext.eyes[i].width = swapInfo.width;
        vrContext.eyes[i].height = swapInfo.height;
    }

    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer) {
    vrContext.window = window;
    vrContext.renderer = renderer;

    vrContext.glContext = SDL_GL_CreateContext(window);
    if (!vrContext.glContext) {
        std::cerr << "[VRRen] SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return false;
    }

    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    std::strncpy(createInfo.applicationInfo.applicationName, "LEGO Island VR", sizeof(createInfo.applicationInfo.applicationName) - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    std::strncpy(createInfo.applicationInfo.engineName, "OmniEngine", sizeof(createInfo.applicationInfo.engineName) - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrResult r = xrCreateInstance(&createInfo, &vrContext.instance);
    if (XR_FAILED(r)) {
        std::cerr << "[VRRen] xrCreateInstance failed: " << XrResultToString(r) << "\n";
        return false;
    }

    XrSystemGetInfo sysInfo{ XR_TYPE_SYSTEM_GET_INFO };
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    r = xrGetSystem(vrContext.instance, &sysInfo, &vrContext.systemId);
    if (XR_FAILED(r)) {
        std::cerr << "[VRRen] xrGetSystem failed: " << XrResultToString(r) << "\n";
        return false;
    }

    SDL_WindowProperties* props = SDL_GetWindowProperties(window);
    HDC hdc = GetDC(static_cast<HWND>(props->win.window));

    XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = hdc;
    graphicsBinding.hGLRC = vrContext.glContext;

    XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = vrContext.systemId;

    r = xrCreateSession(vrContext.instance, &sessionInfo, &vrContext.session);
    if (XR_FAILED(r)) {
        std::cerr << "[VRRen] xrCreateSession failed: " << XrResultToString(r) << "\n";
        return false;
    }

    vrContext.initialized = true;
    return VR_CreateSwapchain(vrContext);
}

void VR_Shutdown(VRContext& vrContext) {
    if (!vrContext.initialized) return;

    for (auto& eye : vrContext.eyes) {
        if (eye.swapchain != XR_NULL_HANDLE) xrDestroySwapchain(eye.swapchain);
        eye.swapchain = XR_NULL_HANDLE;
    }
    vrContext.eyes.clear();

    if (vrContext.session != XR_NULL_HANDLE) xrDestroySession(vrContext.session);
    vrContext.session = XR_NULL_HANDLE;

    if (vrContext.instance != XR_NULL_HANDLE) xrDestroyInstance(vrContext.instance);
    vrContext.instance = XR_NULL_HANDLE;

    if (vrContext.glContext) SDL_GL_DeleteContext(vrContext.glContext);
    vrContext.glContext = nullptr;

    vrContext.initialized = false;
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) { return vrContext.initialized && eyeIndex < (int)vrContext.eyes.size(); }
bool VR_BeginFrame(VRContext&) { return true; }
void VR_EndFrame(VRContext&) {}

VRViewMatrix VR_GetEyeViewMatrix(int) {
    VRViewMatrix mat{};
    for (int i = 0; i < 16; i++) mat.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    return mat;
}
VRProjMatrix VR_GetEyeProjMatrix(int) {
    VRProjMatrix mat{};
    for (int i = 0; i < 16; i++) mat.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    return mat;
}
