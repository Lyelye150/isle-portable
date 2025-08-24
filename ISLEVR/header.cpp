#include "header.h"
#include <SDL3/SDL.h>
#include <iostream>

bool isVRHeadsetConnected() {
    return false;
}

void showVRNotDetectedMessage() {
#ifdef VRDEC
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Failed to initialize SDL3: " << SDL_GetError() << "\n";
    }

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                             "VR Headset Not Detected",
                             "Please connect a VR headset before running this application.",
                             nullptr);
#else
    std::cerr << "ERROR: VR Headset not detected. Please connect a VR headset.\n";
#endif
}

void checkVR() {
    if (!isVRHeadsetConnected()) {
        showVRNotDetectedMessage();
        exit(EXIT_FAILURE);
    }
}
