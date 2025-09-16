#include "d3drmrenderer.h"
#include "d3drmrenderer_gx2.h"
#include "d3drmtexture_impl.h"
#include "ddraw_impl.h"
#include "meshutils.h"
#include "miniwin.h"

#include <algorithm>
#include <vector>
#include <cstring>

static bool g_rendering = false;

static void GX2InitTexture(GX2Texture* tex, int width, int height, GX2SurfaceFormat format)
{
	GX2InitTexture(tex, width, height, format);
}

static void GX2UploadTexture(GX2Texture* tex, void* data, size_t size)
{
	GX2UploadTexture(tex, data, size);
}

static void GX2SetTextureFilter(GX2Texture* tex, GX2TexFilter filter)
{
	GX2SetTextureFilter(tex, filter);
}

static void GX2SetTextureWrap(GX2Texture* tex, GX2TexWrap wrap)
{
	GX2SetTextureWrap(tex, wrap);
}

static void GX2GenerateMipmaps(GX2Texture* tex)
{
	GX2GenerateMipmaps(tex);
}

static void GX2CreateVertexBuffer(GX2VertexBuffer* vbo, void* data, size_t size)
{
	GX2CreateVertexBuffer(vbo, data, size);
}

static void GX2DeleteVertexBuffer(GX2VertexBuffer* vbo)
{
	GX2DeleteVertexBuffer(vbo);
}

static void GX2DeleteTexture(GX2Texture* tex)
{
	GX2DeleteTexture(tex);
}

GX2Renderer::GX2Renderer(DWORD width, DWORD height)
{
	m_width = width;
	m_height = height;
	GX2Init();
	GX2InitShaderEx(&m_shader, vshader_gx2_bin, vshader_gx2_bin_size);
	GX2CreateRenderTarget(&m_renderTarget, m_width, m_height, GX2_SURFACE_FORMAT_R8G8B8A8_UNORM);
}

GX2Renderer::~GX2Renderer()
{
	for (auto& entry : m_textures) {
		if (entry.texture) {
			GX2DeleteTexture(&entry.gx2Tex);
		}
	}
	for (auto& mesh : m_meshs) {
		if (mesh.meshGroup) {
			delete[] mesh.vbo;
		}
	}
	GX2DestroyRenderTarget(&m_renderTarget);
}

void GX2Renderer::PushLights(const SceneLight* lights, size_t count)
{
	m_lights.assign(lights, lights + count);
}

void GX2Renderer::SetProjection(const D3DRMMATRIX4D& projection, D3DVALUE front, D3DVALUE back)
{
	memcpy(&m_projection, projection, sizeof(D3DRMMATRIX4D));
}

struct GX2CacheDestroyContext {
	GX2Renderer* renderer;
	Uint32 id;
};

void GX2Renderer::AddTextureDestroyCallback(Uint32 id, IDirect3DRMTexture* texture)
{
	auto* ctx = new GX2CacheDestroyContext{this, id};
	texture->AddDestroyCallback(
		[](IDirect3DRMObject* obj, void* arg) {
			auto* ctx = static_cast<GX2CacheDestroyContext*>(arg);
			auto& entry = ctx->renderer->m_textures[ctx->id];
			if (entry.texture) {
				GX2DeleteTexture(&entry.gx2Tex);
				entry.texture = nullptr;
			}
			delete ctx;
		},
		ctx
	);
}

static bool ConvertAndUploadTexture(GX2Texture* tex, SDL_Surface* surface, bool isUI, float scaleX, float scaleY)
{
	int width = surface->w;
	int height = surface->h;
	SDL_Surface* resized = surface;
	if (scaleX != 1.0f || scaleY != 1.0f) {
		width = static_cast<int>(width * scaleX);
		height = static_cast<int>(height * scaleY);
		resized = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
		SDL_Rect dstRect = {0, 0, width, height};
		SDL_BlitScaled(surface, nullptr, resized, &dstRect);
	}
	GX2InitTexture(tex, width, height, GX2_SURFACE_FORMAT_R8G8B8A8_UNORM);
	GX2UploadTexture(tex, resized->pixels, width * height * 4);
	if (isUI) {
		GX2SetTextureFilter(tex, GX2_TEX_FILTER_NEAREST);
		GX2SetTextureWrap(tex, GX2_TEX_WRAP_CLAMP);
	}
	else {
		GX2SetTextureFilter(tex, GX2_TEX_FILTER_LINEAR);
		GX2SetTextureWrap(tex, GX2_TEX_WRAP_REPEAT);
		GX2GenerateMipmaps(tex);
	}
	if (resized != surface) {
		SDL_FreeSurface(resized);
	}
	return true;
}

Uint32 GX2Renderer::GetTextureId(IDirect3DRMTexture* iTexture, bool isUI, float scaleX, float scaleY)
{
	auto texture = static_cast<Direct3DRMTextureImpl*>(iTexture);
	auto surface = static_cast<DirectDrawSurfaceImpl*>(texture->m_surface);
	SDL_Surface* originalSurface = surface->m_surface;
	for (Uint32 i = 0; i < m_textures.size(); ++i) {
		auto& tex = m_textures[i];
		if (tex.texture == texture) {
			if (tex.version != texture->m_version) {
				GX2DeleteTexture(&tex.gx2Tex);
				ConvertAndUploadTexture(&tex.gx2Tex, originalSurface, isUI, scaleX, scaleY);
				tex.version = texture->m_version;
			}
			return i;
		}
	}
	GX2TextureCacheEntry entry;
	entry.texture = texture;
	entry.version = texture->m_version;
	if (!ConvertAndUploadTexture(&entry.gx2Tex, originalSurface, isUI, scaleX, scaleY)) {
		return NO_TEXTURE_ID;
	}
	for (Uint32 i = 0; i < m_textures.size(); ++i) {
		if (!m_textures[i].texture) {
			m_textures[i] = std::move(entry);
			AddTextureDestroyCallback(i, texture);
			return i;
		}
	}
	m_textures.push_back(std::move(entry));
	AddTextureDestroyCallback((Uint32) (m_textures.size() - 1), texture);
	return (Uint32) (m_textures.size() - 1);
}

GX2MeshCacheEntry GX2UploadMesh(const MeshGroup& meshGroup)
{
	GX2MeshCacheEntry cache{&meshGroup, meshGroup.version};
	std::vector<D3DRMVERTEX> vertexBuffer(meshGroup.vertices.begin(), meshGroup.vertices.end());
	std::vector<uint16_t> indexBuffer(meshGroup.indices.begin(), meshGroup.indices.end());
	std::vector<D3DRMVERTEX> uploadBuffer;
	uploadBuffer.reserve(indexBuffer.size());
	for (auto idx : indexBuffer) {
		uploadBuffer.push_back(vertexBuffer[idx]);
	}
	cache.vertexCount = uploadBuffer.size();
	cache.vbo = new D3DRMVERTEX[cache.vertexCount];
	memcpy(cache.vbo, uploadBuffer.data(), cache.vertexCount * sizeof(D3DRMVERTEX));
	GX2CreateVertexBuffer(&cache.gx2VBO, cache.vbo, cache.vertexCount * sizeof(D3DRMVERTEX));
	return cache;
}

void GX2Renderer::AddMeshDestroyCallback(Uint32 id, IDirect3DRMMesh* mesh)
{
	auto* ctx = new GX2CacheDestroyContext{this, id};
	mesh->AddDestroyCallback(
		[](IDirect3DRMObject* obj, void* arg) {
			auto* ctx = static_cast<GX2CacheDestroyContext*>(arg);
			auto& cacheEntry = ctx->renderer->m_meshs[ctx->id];
			if (cacheEntry.meshGroup) {
				cacheEntry.meshGroup = nullptr;
				delete[] cacheEntry.vbo;
				GX2DeleteVertexBuffer(&cacheEntry.gx2VBO);
				cacheEntry.vertexCount = 0;
			}
			delete ctx;
		},
		ctx
	);
}

Uint32 GX2Renderer::GetMeshId(IDirect3DRMMesh* mesh, const MeshGroup* meshGroup)
{
	for (Uint32 i = 0; i < m_meshs.size(); ++i) {
		auto& cache = m_meshs[i];
		if (cache.meshGroup == meshGroup) {
			if (cache.version != meshGroup->version) {
				cache = std::move(GX2UploadMesh(*meshGroup));
			}
			return i;
		}
	}
	auto newCache = GX2UploadMesh(*meshGroup);
	for (Uint32 i = 0; i < m_meshs.size(); ++i) {
		if (!m_meshs[i].meshGroup) {
			m_meshs[i] = std::move(newCache);
			AddMeshDestroyCallback(i, mesh);
			return i;
		}
	}
	m_meshs.push_back(std::move(newCache));
	AddMeshDestroyCallback((Uint32) (m_meshs.size() - 1), mesh);
	return (Uint32) (m_meshs.size() - 1);
}

void GX2Renderer::StartFrame()
{
	if (g_rendering) {
		return;
	}
	GX2BeginFrame(&m_renderTarget);
	g_rendering = true;
}

void GX2Renderer::UploadLights()
{
	for (const auto& light : m_lights) {
		if (light.positional == 0.0f && light.directional == 0.0f) {
			GX2SetAmbientLight(light.color);
		}
		else if (light.directional == 1.0f) {
			GX2SetDirectionalLight(light.direction, light.color);
		}
		else if (light.positional == 1.0f) {
			GX2SetPointLight(light.position, light.color);
		}
	}
}

HRESULT GX2Renderer::BeginFrame()
{
	StartFrame();
	GX2EnableDepthTest(true);
	UploadLights();
	GX2UploadProjectionMatrix(&m_projection);
	return S_OK;
}

void GX2Renderer::EnableTransparency()
{
	GX2EnableBlend(true);
	GX2SetBlendFunc(GX2_BLEND_SRC_ALPHA, GX2_BLEND_ONE_MINUS_SRC_ALPHA);
}

HRESULT GX2Renderer::FinalizeFrame()
{
	GX2FlushCommands();
	return S_OK;
}

void GX2Renderer::Clear(float r, float g, float b)
{
	StartFrame();
	GX2ClearRenderTarget(&m_renderTarget, r, g, b, 1.0f);
}

void GX2Renderer::Flip()
{
	GX2EndFrame();
	g_rendering = false;
}

void GX2Renderer::SubmitDraw(
	DWORD meshId,
	const D3DRMMATRIX4D& modelViewMatrix,
	const D3DRMMATRIX4D& worldMatrix,
	const D3DRMMATRIX4D& viewMatrix,
	const Matrix3x3& normalMatrix,
	const Appearance& appearance
)
{
	auto& mesh = m_meshs[meshId];
	GX2BindShader(&m_shader);
	GX2BindVertexBuffer(&mesh.gx2VBO);
	GX2SetModelViewMatrix(modelViewMatrix);
	GX2SetMaterial(appearance.color, appearance.shininess);
	GX2Texture* tex = (appearance.textureId != NO_TEXTURE_ID) ? &m_textures[appearance.textureId].gx2Tex : nullptr;
	if (tex) {
		GX2BindTexture(tex);
	}
	GX2DrawTriangles(mesh.vertexCount);
}

void GX2Renderer::Draw2DImage(Uint32 textureId, const SDL_Rect& srcRect, const SDL_Rect& dstRect, FColor color)
{
	StartFrame();
	GX2SetOrthoProjection(0, m_width, 0, m_height);
	GX2Texture* tex = (textureId != NO_TEXTURE_ID) ? &m_textures[textureId].gx2Tex : nullptr;
	if (tex) {
		GX2BindTexture(tex);
	}
	GX2Draw2DQuad(dstRect.x, dstRect.y, dstRect.w, dstRect.h, srcRect.x, srcRect.y, srcRect.w, srcRect.h, color);
}

void GX2Renderer::Resize(int width, int height, const ViewportTransform& viewportTransform)
{
	m_width = width;
	m_height = height;
	m_viewportTransform = viewportTransform;
	GX2ResizeRenderTarget(&m_renderTarget, width, height);
}

void GX2Renderer::SetDither(bool dither)
{
	GX2SetDither(dither);
}

void GX2Renderer::Download(SDL_Surface* target)
{
	GX2ReadFramebuffer(&m_renderTarget, target->pixels, target->w, target->h);
}
