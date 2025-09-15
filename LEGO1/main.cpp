#ifdef MINIWIN
#include "miniwin/windows.h"
#else
#include <windows.h>
#endif

#ifdef __WIIU__
extern "C" void rpl_entry(void) {
    LegoApp app;
    app.Run();
}
#endif

// FUNCTION: LEGO1 0x10091ee0
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
