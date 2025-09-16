#pragma once

#include "d3drmrenderer.h"
#include "ddraw_impl.h"
#include "meshutils.h"
#include <SDL3/SDL.h>
#include <vector>

DEFINE_GUID(GX2_GUID, 0xA12B56F3, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x53);

struct GX2TextureCacheEntry {
	IDirect3DRMTexture* texture = nullptr;
	Uint32 version = 0;
	GX2Texture gx2Tex = {};
};

struct GX2MeshCacheEntry {
	const MeshGroup* meshGroup = nullptr;
	int version = 0;
	D3DRMVERTEX* vbo = nullptr;
	int vertexCount = 0;
	GX2VertexBuffer gx2VBO = {};
};

class GX2Renderer : public Direct3DRMRenderer {
public:
	GX2Renderer(DWORD width, DWORD height);
	~GX2Renderer() override;

	void PushLights(const SceneLight* lightsArray, size_t count) override;
	void SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back) override;
	Uint32 GetTextureId(IDirect3DRMTexture* texture, bool isUI, float scaleX, float scaleY) override;
	Uint32 GetMeshId(IDirect3DRMMesh* mesh, const MeshGroup* meshGroup) override;
	HRESULT BeginFrame() override;
	void EnableTransparency() override;
	void SubmitDraw(
		DWORD meshId,
		const D3DRMMATRIX4D& modelViewMatrix,
		const D3DRMMATRIX4D& worldMatrix,
		const D3DRMMATRIX4D& viewMatrix,
		const Matrix3x3& normalMatrix,
		const Appearance& appearance
	) override;
	HRESULT FinalizeFrame() override;
	void Resize(int width, int height, const ViewportTransform& viewportTransform) override;
	void Clear(float r, float g, float b) override;
	void Flip() override;
	void Draw2DImage(Uint32 textureId, const SDL_Rect& srcRect, const SDL_Rect& dstRect, FColor color) override;
	void Download(SDL_Surface* target) override;
	void SetDither(bool dither) override;

private:
	void AddTextureDestroyCallback(Uint32 id, IDirect3DRMTexture* texture);
	void AddMeshDestroyCallback(Uint32 id, IDirect3DRMMesh* mesh);
	void StartFrame();
	void UploadLights();

	D3DRMMATRIX4D m_projection = {};
	GX2RenderTarget m_renderTarget = {};
	GX2Shader m_shader = {};
	std::vector<GX2TextureCacheEntry> m_textures;
	std::vector<GX2MeshCacheEntry> m_meshs;
	ViewportTransform m_viewportTransform = {};
	std::vector<SceneLight> m_lights;
};

inline static void GX2Renderer_EnumDevice(LPD3DENUMDEVICESCALLBACK cb, void* ctx)
{
	D3DDEVICEDESC halDesc = {};
	halDesc.dcmColorModel = D3DCOLOR_RGB;
	halDesc.dwFlags = D3DDD_DEVICEZBUFFERBITDEPTH;
	halDesc.dwDeviceZBufferBitDepth = DDBD_24;
	halDesc.dwDeviceRenderBitDepth = DDBD_32;
	halDesc.dpcTriCaps.dwTextureCaps = D3DPTEXTURECAPS_PERSPECTIVE;
	halDesc.dpcTriCaps.dwShadeCaps = D3DPSHADECAPS_ALPHAFLATBLEND;
	halDesc.dpcTriCaps.dwTextureFilterCaps = D3DPTFILTERCAPS_LINEAR;

	D3DDEVICEDESC helDesc = {};

	EnumDevice(cb, ctx, "GX2", &halDesc, &helDesc, GX2_GUID);
}
