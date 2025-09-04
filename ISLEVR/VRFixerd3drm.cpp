// this file exists to fix a error when building

#include "VRRen.h"

bool VR_Init(VRContext& vrContext, SDL_Window* window, VRRenderer renderer) {
    vrContext.window = window;
    vrContext.renderer = renderer;
    vrContext.initialized = true;
    return true;
}

void VR_Shutdown(VRContext& vrContext) {
    vrContext.initialized = false;
}

bool VR_BeginFrame(VRContext& vrContext) { return true; }
void VR_EndFrame(VRContext& vrContext) {}
bool VR_BindEye(VRContext& vrContext, int eyeIndex) { return true; }
