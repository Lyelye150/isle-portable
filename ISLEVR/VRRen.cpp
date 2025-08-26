#include "VRRen.h"
#include <SDL3/SDL.h>
#include <iostream>

bool VR_CreateSwapchain(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    vrContext.eyes.resize(2);
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &vrContext.eyes[i].fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, vrContext.eyes[i].fbo);

        glGenTextures(1, &vrContext.eyes[i].texture);
        glBindTexture(GL_TEXTURE_2D, vrContext.eyes[i].texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrContext.width, vrContext.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vrContext.eyes[i].texture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[VRRen] Failed to create FBO for eye %d", i);
            return false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    SDL_Log("[VRRen] OpenGL1 per-eye FBOs created, %dx%d", vrContext.width, vrContext.height);
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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    vrContext.glContext = SDL_GL_CreateContext(window);
    if (!vrContext.glContext) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[VRRen] Failed to create OpenGL1 context: %s", SDL_GetError());
        return false;
    }

    vrContext.initialized = true;
    vrContext.width = 1024;
    vrContext.height = 1024;

    if (!VR_CreateSwapchain(vrContext)) return false;

    SDL_Log("[VRRen] VR initialized: HMD detected with OpenGL1 context ready.");
    return true;
}

void VR_Shutdown(VRContext& vrContext) {
    if (vrContext.initialized) {
        for (auto& eye : vrContext.eyes) {
            if (eye.texture) glDeleteTextures(1, &eye.texture);
            if (eye.fbo) glDeleteFramebuffers(1, &eye.fbo);
        }
        vrContext.eyes.clear();

        if (vrContext.glContext) {
            SDL_GL_DeleteContext(vrContext.glContext);
            vrContext.glContext = nullptr;
        }

        if (vrContext.session != XR_NULL_HANDLE) {
            vrContext.session = XR_NULL_HANDLE;
        }

        vrContext.initialized = false;
        SDL_Log("[VRRen] VR session and OpenGL context shut down.");
    }
}

bool VR_BindEye(VRContext& vrContext, int eyeIndex) {
    if (!vrContext.initialized || eyeIndex >= (int)vrContext.eyes.size()) return false;
    glBindFramebuffer(GL_FRAMEBUFFER, vrContext.eyes[eyeIndex].fbo);
    glViewport(0, 0, vrContext.width, vrContext.height);
    return true;
}

bool VR_BeginFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return false;

    VR_BindEye(vrContext, 0);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

void VR_EndFrame(VRContext& vrContext) {
    if (!vrContext.initialized) return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(vrContext.window);
}
