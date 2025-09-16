#ifndef LEGODXINFO_H
#define LEGODXINFO_H

#include "mxdirectxinfo.h"

#ifdef __WIIU__
class LegoDeviceEnumerate : public MxDeviceEnumerate {
public:
    int ParseDeviceName(const char* p_deviceId) { return -1; }
    int ProcessDeviceBytes(int p_deviceNum, GUID& p_guid) { return 0; }
    int GetDevice(int p_deviceNum, MxDriver*& p_driver, Direct3DDeviceInfo*& p_device) { return 0; }
    int FormatDeviceName(char* p_buffer, const MxDriver* p_ddInfo, const Direct3DDeviceInfo* p_d3dInfo) const { return 0; }
    int BETA_1011cc65(int p_idx, char* p_buffer) { return 0; }
    int GetBestDevice() { return 0; }
    int FUN_1009d210() { return 0; }
    unsigned char FUN_1009d3d0(Direct3DDeviceInfo& p_device) { return 0; }

    LegoDeviceEnumerate() {}
    ~LegoDeviceEnumerate() {}
};
#else
class LegoDeviceEnumerate : public MxDeviceEnumerate {
public:
	int ParseDeviceName(const char* p_deviceId);
	int ProcessDeviceBytes(int p_deviceNum, GUID& p_guid);
	int GetDevice(int p_deviceNum, MxDriver*& p_driver, Direct3DDeviceInfo*& p_device);
	int FormatDeviceName(char* p_buffer, const MxDriver* p_ddInfo, const Direct3DDeviceInfo* p_d3dInfo) const;
	int BETA_1011cc65(int p_idx, char* p_buffer);
	int GetBestDevice();
	int FUN_1009d210();
	unsigned char FUN_1009d3d0(Direct3DDeviceInfo& p_device);
};
#endif

#endif // LEGODXINFO_H
