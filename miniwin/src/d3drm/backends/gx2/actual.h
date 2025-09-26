#pragma once
#include <gx2/mem.h>

inline uint32_t GX2RGetGpuAddr(const GX2RBuffer* buffer)
{
	return (uint32_t) buffer->gpuAddr;
}
