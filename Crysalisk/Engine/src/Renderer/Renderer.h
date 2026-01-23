//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include <Containers/CVector.h>
#include "Core/Clock.h"
#include "Core/Logging.h"
#include "Defines.h"
#include "Renderer/Camera.h"
#include "Resources/ResourceTypes.h"

// Forward Declarations
class IRendererBackend;
class Platform::IPlatformWindow;
struct RenderPacket_t;

//-------------------------------------------------------------------------
// The renderer frontend.
//-------------------------------------------------------------------------
class IRenderer
{
public:
	static void		AllocateRenderSystemState(IRenderer** ppSystemState);

	// Initialize, shutdown.
	virtual bool			Initialize(RendererConfig_t& pAppConfig) = 0;
	virtual void			Shutdown() = 0;
	virtual bool			Update(Platform::IPlatformWindow* pTargetWindow, struct RenderPacket_t& pRenderData, class CMap* pCurrentMap) = 0;

	virtual bool			RegisterWindow(Platform::IPlatformWindow& pWindow) = 0;
	virtual void			ReleaseWindowResources(Platform::IPlatformWindow& pWindow) = 0;
	
	virtual void			ConfigureRenderViews(Utl::CVector<class IRenderView*>& pRenderViews) = 0;
	virtual void			RegisterRenderView(class IRenderView* pRenderView) = 0;

	virtual bool			AcquireTextureResources(struct Texture* pTexture, int internalIndex, size_t nPixelDataSize, byte* pPixelData) = 0;
	virtual void			ReleaseTextureResources(struct Texture* pTexture) = 0;

	virtual bool			CreateShader(class Shader* pShader, struct ShaderConfig_t& pAppConfig) = 0;
	virtual void			DestroyShader(class Shader* pShader) = 0;

	virtual void			AcquireShaderInstanceResources(class Shader* pShader, unsigned int* pOutInstanceID, const char* pszResourceName) = 0;
	virtual void			ReleaseShaderInstanceResources(class Shader* pShader, unsigned int pInstanceID) = 0;

	virtual void			BindShader(class Shader* pShader) = 0;
	virtual void			BindShaderPerFrameUniforms(class Shader* pShader) = 0;
	virtual void			BindShaderPerInstanceUniforms(class Shader* pShader, unsigned int hInstanceID) = 0;

	virtual void			SetShaderUniform(class Shader* pShader, struct ShaderUniform_t* pUniform, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) = 0;

	virtual bool			CreateRendererView(RendererViewConfig_t& pViewConfig, RendererView& pOutView) = 0;
	virtual void			DestroyRendererView(RendererView& pView) = 0;

	virtual bool			BeginRendererView(RendererView& pView) = 0;
	virtual void			EndRendererView() = 0;

	virtual bool			CreateRenderBuffer(size_t bufferSize, eRenderBufferUsage bufferUsage, bool bUseFreelist, const char* pszBufferName, RenderBuffer& pOutBuffer) = 0;
	virtual void			DestroyRenderBuffer(RenderBuffer& pBuffer) = 0;

	virtual void			AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, GeometryConfig_t& pGeometryConfig, GeometryInstance_t& pOutInstance) = 0;
	virtual void			AcquireGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryConfig2D_t& pGeometryConfig, GeometryInstance_t& pOutInstance) = 0;
	virtual void			AcquireGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) = 0;
	virtual void			ReleaseGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry) = 0;

	virtual void			DrawGeometry(GeometryInstance_t& pGeometry, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer) = 0;

	virtual void			UpdateGeometryResources(RenderBuffer& pVertexBuffer, RenderBuffer& pIndexBuffer, GeometryInstance_t& pGeometry, GeometryConfig_t& pNewGeometryConfig) = 0;
	virtual void			UpdateGeometryResources(GeometryInstance_t& pGeometry, GeometryConfig2D_t& pNewGeometryConfig) = 0;
	virtual void			UpdateGeometryResources(RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer, uint64_t iVertexCount, uint64_t iVertexStride, void* pVertices, uint64_t iIndexCount, void* pIndices, GeometryInstance_t& pOutInstance) = 0;

	virtual const Camera_t& GetMainCamera() const = 0;

	// Temp geometry stuff
	virtual bool	CreateTempTextObjects() = 0;
	virtual void	DestroyTempGeometry() = 0;

	virtual void	SetMinimized(bool bMinimized) = 0;
	virtual void	SetWindowDimensions(unsigned int iWidth, unsigned int iHeight) = 0;

	virtual RenderBuffer&	GetWorldVertexBuffer() = 0;
	virtual RenderBuffer&	GetWorldIndexBuffer() = 0;

	virtual RenderBuffer&	GetUIVertexBuffer() = 0;
	virtual RenderBuffer&	GetUIIndexBuffer() = 0;

	virtual RendererView&	GetSceneView() const = 0;
	virtual RendererView&	GetUIView() const = 0;

	virtual inline size_t	GetCurrentRenderFrame() const = 0;
};
