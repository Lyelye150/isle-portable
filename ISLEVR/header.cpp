#include "header.h"
#include <iostream>

#ifdef MINIWIN
#include "miniwin/src/internal/miniwin.h"
#endif

bool isVRHeadsetConnected() { return false; }

void showVRNotDetectedMessage() {
#ifdef MINIWIN
    mwInit();
    mwMessageBox("VR Headset Not Detected",
                 "Please connect a VR headset before running this application.");
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
