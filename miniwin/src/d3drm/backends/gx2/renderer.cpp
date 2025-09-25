#include <cstdio>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <gfd.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <whb/gfx.h>
#include <whb/log_udp.h>
#include <whb/proc.h>
#include <whb/sdcard.h>

static const float sPositionData[] = {1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f};
static const float sColourData[] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f};

int main(int argc, char** argv)
{
	GX2RBuffer positionBuffer = {};
	GX2RBuffer colourBuffer = {};
	WHBGfxShaderGroup group = {};
	void* buffer = nullptr;
	char* gshFileData = nullptr;
	char path[256];
	int result = 0;

	WHBLogUdpInit();
	WHBProcInit();
	WHBGfxInit();

	if (!WHBMountSdCard()) {
		result = -1;
		goto exit;
	}

	sprintf(path, "%s/wiiu/isle-U/content/renderer.gsh", WHBGetSdCardMountPath());
	gshFileData = WHBReadWholeFile(path, nullptr);
	if (!gshFileData) {
		result = -1;
		goto exit;
	}

	if (!WHBGfxLoadGFDShaderGroup(&group, 0, gshFileData)) {
		result = -1;
		goto exit;
	}

	WHBGfxInitShaderAttribute(&group, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
	WHBGfxInitShaderAttribute(&group, "aColour", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
	WHBGfxInitFetchShader(&group);

	WHBFreeWholeFile(gshFileData);
	gshFileData = nullptr;

	positionBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_READ |
						   GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
	positionBuffer.elemSize = sizeof(float) * 4;
	positionBuffer.elemCount = 3;
	GX2RCreateBuffer(&positionBuffer);
	buffer = GX2RLockBufferEx(&positionBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);
	memcpy(buffer, sPositionData, positionBuffer.elemSize * positionBuffer.elemCount);
	GX2RUnlockBufferEx(&positionBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);

	colourBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_READ |
						 GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
	colourBuffer.elemSize = sizeof(float) * 4;
	colourBuffer.elemCount = 3;
	GX2RCreateBuffer(&colourBuffer);
	buffer = GX2RLockBufferEx(&colourBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);
	memcpy(buffer, sColourData, colourBuffer.elemSize * colourBuffer.elemCount);
	GX2RUnlockBufferEx(&colourBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);

	while (WHBProcIsRunning()) {
		float* colours = (float*) GX2RLockBufferEx(&colourBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);
		for (int i = 0; i < 12; i++) {
			colours[i] = colours[i] >= 1.0f ? 0.0f : (colours[i] + 0.01f);
		}
		GX2RUnlockBufferEx(&colourBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);

		WHBGfxBeginRender();

		WHBGfxBeginRenderTV();
		WHBGfxClearColor(0.0f, 0.0f, 0.1f, 1.0f);
		GX2SetFetchShader(&group.fetchShader);
		GX2SetVertexShader(group.vertexShader);
		GX2SetPixelShader(group.pixelShader);
		GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
		GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
		GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
		WHBGfxFinishRenderTV();

		WHBGfxBeginRenderDRC();
		WHBGfxClearColor(0.1f, 0.0f, 0.1f, 1.0f);
		GX2SetFetchShader(&group.fetchShader);
		GX2SetVertexShader(group.vertexShader);
		GX2SetPixelShader(group.pixelShader);
		GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
		GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
		GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
		WHBGfxFinishRenderDRC();

		WHBGfxFinishRender();
	}

exit:
	GX2RDestroyBufferEx(&positionBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);
	GX2RDestroyBufferEx(&colourBuffer, GX2R_RESOURCE_USAGE_CPU_WRITE);
	WHBUnmountSdCard();
	WHBGfxShutdown();
	WHBProcShutdown();
	WHBLogUdpDeinit();
	return result;
}
