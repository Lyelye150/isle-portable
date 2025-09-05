#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL

#include "VRRen.h"
#include "../miniwin/src/d3drm/backends/opengl1/actual.h"

#include <iostream>
#include <vector>
#include <cstring>
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

static const char* XrResultToString(XrResult r) {
    static thread_local char buf[64];
    std::snprintf(buf, sizeof(buf), "%d (0x%08x)", (int)r, (unsigned int)r);
    return buf;
}

static bool CheckXr(XrResult r, const char* what) {
    if (XR_SUCCEEDED(r)) return true;
    std::cerr << "[VRRen] " << what << " failed: " << XrResultToString(r) << "\n";
    return false;
}

static int64_t ChooseSwapchainFormatGL(XrSession session) {
    uint32_t count = 0;
    XrResult r = xrEnumerateSwapchainFormats(session, 0, &count, nullptr);
    if (XR_FAILED(r) || count == 0) return 0;
    std::vector<int64_t> fmts(count);
    r = xrEnumerateSwapchainFormats(session, count, &count, fmts.data());
    if (XR_FAILED(r)) return 0;
    const int64_t prefer = 0x8058;
    for (auto f : fmts) if (f == prefer) return f;
    return fmts[0];
}

static bool CreateSessionOpenGL(VRContext& ctx) {
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = nullptr;
    graphicsBinding.hGLRC = nullptr;
    if (ctx.window) {
        SDL_PropertiesID props = SDL_GetWindowProperties(ctx.window);
        HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (hwnd) {
            graphicsBinding.hDC = GetDC(hwnd);
            graphicsBinding.hGLRC = reinterpret_cast<HGLRC>(ctx.glContext);
        }
    }
    XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
    sessionInfo.next = &graphicsBinding;
    sessionInfo.systemId = ctx.systemId;
    XrResult r = xrCreateSession(ctx.instance, &sessionInfo, &ctx.session);
    if (XR_FAILED(r)) return false;

    XrReferenceSpaceCreateInfo rsci{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    rsci.poseInReferenceSpace.orientation.w = 1.0f;
    r = xrCreateReferenceSpace(ctx.session, &rsci, &ctx.appSpace);
    if (XR_FAILED(r)) return false;

    return true;
}

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized || vrContext.session == XR_NULL_HANDLE) return false;

    uint32_t viewCount = 0;
    XrResult r = xrEnumerateViewConfigurationViews(
        vrContext.instance, vrContext.systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        0, &viewCount, nullptr);
    if (XR_FAILED(r) || viewCount == 0) return false;

    std::vector<XrViewConfigurationView> views(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    r = xrEnumerateViewConfigurationViews(
        vrContext.instance, vrContext.systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
        viewCount, &viewCount, views.data());
    if (XR_FAILED(r)) return false;

    int64_t chosenFormat = ChooseSwapchainFormatGL(vrContext.session);
    if (chosenFormat == 0) return false;

    const uint32_t eyeCount = viewCount;
    vrContext.eyes.clear();
    vrContext.eyes.resize(eyeCount);
    vrContext.acquiredImageIndices.assign(eyeCount, 0u);
    vrContext.imageAcquired.assign(eyeCount, false);
    vrContext.lastViews.resize(eyeCount);

    for (uint32_t eye = 0; eye < eyeCount; ++eye) {
        XrSwapchainCreateInfo sci{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        sci.arraySize = 1;
        sci.format = chosenFormat;
        sci.width = views[eye].recommendedImageRectWidth;
        sci.height = views[eye].recommendedImageRectHeight;
        sci.mipCount = 1;
        sci.faceCount = 1;
        sci.sampleCount = 1;
        sci.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

        XrSwapchain swapchain = XR_NULL_HANDLE;
        r = xrCreateSwapchain(vrContext.session, &sci, &swapchain);
        if (XR_FAILED(r)) {
            for (auto& e : vrContext.eyes) {
                if (e.swapchain != XR_NULL_HANDLE) xrDestroySwapchain(e.swapchain);
                e.swapchain = XR_NULL_HANDLE;
            }
            return false;
        }

        vrContext.eyes[eye].swapchain = swapchain;
        vrContext.eyes[eye].width = (int)sci.width;
        vrContext.eyes[eye].height = (int)sci.height;

        uint32_t imgCount = 0;
        r = xrEnumerateSwapchainImages(swapchain, 0, &imgCount, nullptr);
        if (XR_FAILED(r) || imgCount == 0) return false;

        vrContext.eyes[eye].xrImages.assign(imgCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
        r = xrEnumerateSwapchainImages(
            swapchain, imgCount, &imgCount,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(vrContext.eyes[eye].xrImages.data()));
        if (XR_FAILED(r)) return false;

        vrContext.eyes[eye].glImages.resize(imgCount);
        for (uint32_t i = 0; i < imgCount; ++i) {
            vrContext.eyes[eye].glImages[i] = vrContext.eyes[eye].xrImages[i].image;
        }
    }
    return true;
}

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer) {
    vrContext.window = window;
    vrContext.renderer = renderer;
    if (!vrContext.glContext) {
        vrContext.glContext = SDL_GL_CreateContext(window);
        if (!vrContext.glContext) return false;
    }

    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    std::strncpy(createInfo.applicationInfo.applicationName, "LEGO Island VR", sizeof(createInfo.applicationInfo.applicationName) - 1);
    createInfo.applicationInfo.applicationVersion = 1;
    std::strncpy(createInfo.applicationInfo.engineName, "OmniEngine", sizeof(createInfo.applicationInfo.engineName) - 1);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    static const char* kExts[] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    createInfo.enabledExtensionCount = (uint32_t)(sizeof(kExts) / sizeof(kExts[0]));
    createInfo.enabledExtensionNames = kExts;

    XrResult r = xrCreateInstance(&createInfo, &vrContext.instance);
    if (XR_FAILED(r)) return false;

    XrSystemGetInfo sysInfo{ XR_TYPE_SYSTEM_GET_INFO };
    sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    r = xrGetSystem(vrContext.instance, &sysInfo, &vrContext.systemId);
    if (XR_FAILED(r)) return false;

    if (!CreateSessionOpenGL(vrContext)) return false;

    vrContext.initialized = true;
    if (!VR_CreateSwapchain(vrContext)) {
        VR_Shutdown(vrContext);
        return false;
    }
    return true;
}

void VR_Shutdown(VRContext& vrContext) {
    for (auto& eye : vrContext.eyes) {
        if (eye.swapchain != XR_NULL_HANDLE) {
            xrDestroySwapchain(eye.swapchain);
            eye.swapchain = XR_NULL_HANDLE;
        }
    }
    vrContext.eyes.clear();

    if (vrContext.appSpace != XR_NULL_HANDLE) {
        xrDestroySpace(vrContext.appSpace);
        vrContext.appSpace = XR_NULL_HANDLE;
    }
    if (vrContext.session != XR_NULL_HANDLE) {
        xrDestroySession(vrContext.session);
        vrContext.session = XR_NULL_HANDLE;
    }
    if (vrContext.instance != XR_NULL_HANDLE) {
        xrDestroyInstance(vrContext.instance);
        vrContext.instance = XR_NULL_HANDLE;
    }
    if (vrContext.glContext) {
        SDL_GL_DestroyContext(vrContext.glContext);
        vrContext.glContext = nullptr;
    }
    vrContext.initialized = false;
}

bool VR_BeginFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return false;
    XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
    XrFrameState frameState{ XR_TYPE_FRAME_STATE };
    XrResult r = xrWaitFrame(vrContext.session, &waitInfo, &frameState);
    if (XR_FAILED(r)) return false;
    vrContext.frameState = frameState;

    XrFrameBeginInfo bi{ XR_TYPE_FRAME_BEGIN_INFO };
    r = xrBeginFrame(vrContext.session, &bi);
    if (XR_FAILED(r)) return false;

    return true;
}

void VR_EndFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return;

    std::vector<XrCompositionLayerProjectionView> projViews;
    projViews.reserve(vrContext.eyes.size());

    for (uint32_t eye = 0; eye < vrContext.eyes.size(); ++eye) {
        if (!vrContext.imageAcquired[eye]) continue;

        XrCompositionLayerSwapchainSubImage sub{ XR_TYPE_COMPOSITION_LAYER_SWAPCHAIN_SUB_IMAGE };
        sub.swapchain = vrContext.eyes[eye].swapchain;
        sub.imageArrayIndex = vrContext.acquiredImageIndices[eye];
        sub.imageRect.offset.x = 0;
        sub.imageRect.offset.y = 0;
        sub.imageRect.extent.width = vrContext.eyes[eye].width;
        sub.imageRect.extent.height = vrContext.eyes[eye].height;

        XrPosef pose{}; pose.orientation.w = 1.0f;
        XrFovf fov{};
        if (eye < (int)vrContext.lastViews.size()) {
            pose = vrContext.lastViews[eye].pose;
            fov = vrContext.lastViews[eye].fov;
        }

        XrCompositionLayerProjectionView pv{ XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
        pv.pose = pose;
        pv.fov = fov;
        pv.subImage = sub;
        projViews.push_back(pv);
    }

    XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    layer.space = vrContext.appSpace;
    layer.viewCount = (uint32_t)projViews.size();
    layer.views = projViews.empty() ? nullptr : projViews.data();

    XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = vrContext.frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = projViews.empty() ? 0u : 1u;

    const XrCompositionLayerBaseHeader* layers[] = {
        reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)
    };
    endInfo.layers = projViews.empty() ? nullptr : layers;

    xrEndFrame(vrContext.session, &endInfo);

    for (size_t i = 0; i < vrContext.imageAcquired.size(); ++i) {
        vrContext.imageAcquired[i] = false;
    }
}
