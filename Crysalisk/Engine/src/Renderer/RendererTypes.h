//=============================================================================
// Author: Jared Simons
// Date Created: 10/8/2024
// 
// Purpose: Generic types used by the render system.
//=============================================================================
#pragma once

#include "Defines.h"

#include <Containers/CFreeList.h>
#include <Containers/CVector.h>
#include <Math/MathLib.h>
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Renderer configuration, to be set by the application.
//-------------------------------------------------------------------------
struct RendererConfig_t
{
	// The application's name.
	const char* szApplicationName;
	// A pointer to the main engine state.
	class IEngine* pEngine;
	// An opaque pointer to the window we'll be rendering to.
	Platform::IPlatformWindow* pMainWindow;
	// A pointer to the loaded backend module.
	Platform::ISysModule* pRendererBackendDLL;

	size_t	MaxTextures; // The amount of textures that can be loaded at once. Must match the texture system config's amount.
	size_t	MaxPipelines; // The amount of graphics pipelines that can be loaded at once. Must be greater than the maximum materials.
};

enum eRenderViewType
{
	RENDER_VIEW_TYPE_UNDEFINED = -1,
	RENDER_VIEW_TYPE_3D_SCENE = 0,
	RENDER_VIEW_TYPE_DEBUG,
	RENDER_VIEW_TYPE_UI,

	RENDER_VIEW_MAX_TYPES,
};

//-------------------------------------------------------------------------
// Purpose: Frame-specific data to be used in rendering operations.
//-------------------------------------------------------------------------
struct RenderPacket_t
{
	// TODO: Clean this up.
	// The time elapsed between frames.
	// FIXME: DRY
	double	DeltaTime;
	size_t	iNumDrawCalls{ 0 };

	unsigned int								iCulledGeometries{ 0 };

	Matrix4x4									matProjection{};
	struct Camera_t*							pCamera{ nullptr };

	// FIXME: Should these be here?
	struct RenderMetrics_t* pMetrics{ nullptr };

	void*	pRenderViewExtendedData[RENDER_VIEW_MAX_TYPES];
};

//-------------------------------------------------------------------------
// Purpose: Stats collected about renderer metrics.
//-------------------------------------------------------------------------
struct RenderMetrics_t
{
	// The frametime of the previous frame.
	double	FrameTime{ 0.0 };
	// The time in seconds since the last metrics update.
	double	TimeSinceLastMetricsUpdate{ 0.0 };
	// The number of individual draw calls made this frame.
	unsigned int		NumDrawCalls{ 0 };
	// The number of meshes frustum culled in the previous frame
	unsigned int		NumGeometriesFrustumCulled{ 0 };
	// Number of frames rendered in the previous frame.
	unsigned int		PreviousFrameFPS{ 0 };
	// Number of frames rendered in the current frame.
	unsigned int		CurrentFrameFPS{ 0 };
};

//-------------------------------------------------------------------------
// Purpose: Types that can be used in shaders.
//-------------------------------------------------------------------------
enum class ShaderAttribute_t
{
	Undefined,
	Int32,
	Float32,
	Vector2,
	Vector3,
	Vector4,
	Matrix4,
	GenericStructure,
	Sampler,
};

//-------------------------------------------------------------------------
// Purpose: A vertex attribute used in a shader.
//-------------------------------------------------------------------------
struct VertexAttribute_t
{
	// The name of the attribute.
	char*				szName{ nullptr };
	// The attribute's type.
	ShaderAttribute_t	type{ ShaderAttribute_t::Undefined };
	// The length of the attribute's data.
	uint32_t			stride{ 0 };

	VertexAttribute_t()
		: szName{ nullptr }
		, type{ ShaderAttribute_t::Undefined } 
		, stride{ 0 } {}

	VertexAttribute_t(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;
	}

	VertexAttribute_t& operator=(const VertexAttribute_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		type = pOther.type;
		stride = pOther.stride;

		return *this;
	}

	~VertexAttribute_t()
	{
		if (szName)
			Utl::StringFree(szName);
	}
};

//-------------------------------------------------------------------------
// Purpose: The group a shader's uniform belongs to.
//-------------------------------------------------------------------------
enum class eShaderUniformGroup
{
	// These uniforms get updated once per-frame, typically global data like translation matrices.
	PER_FRAME,
	// These uniforms get updated once per-instance, per-frame, typically for material-specific data.
	PER_MATERIAL,
	// The uniforms get updated once per-draw, typically used for the model matrix.
	PER_DRAW
};

//-------------------------------------------------------------------------
// Purpose: The configuration data for a shader uniform.
//-------------------------------------------------------------------------
struct ShaderUniformConfig_t
{
	// The name of the uniform.
	char*							szName;
	// The uniform's type.
	ShaderAttribute_t				eType;
	// The uniform's update group.
	eShaderUniformGroup				eFrequency;
	// The length of a single element in bytes (does not account for a whole array).
	uint32_t						iStride;	
	// The size of the uniform's array.
	int								iArraySize;

	ShaderUniformConfig_t()
		: szName{ nullptr }
		, eType{ ShaderAttribute_t::Undefined }
		, eFrequency{ eShaderUniformGroup::PER_MATERIAL }
		, iStride{ 0 }
		, iArraySize{ 1 } { }

	ShaderUniformConfig_t(const ShaderUniformConfig_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		else
			szName = nullptr;
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;
	}

	ShaderUniformConfig_t& operator=(const ShaderUniformConfig_t& pOther)
	{
		if (pOther.szName)
			szName = Utl::StringDuplicate(pOther.szName);
		eType = pOther.eType;
		eFrequency = pOther.eFrequency;
		iStride = pOther.iStride;
		iArraySize = pOther.iArraySize;

		return *this;
	}

	~ShaderUniformConfig_t()
	{
		if (szName)
			Utl::StringFree(szName);
	}
};

//-------------------------------------------------------------------------
// Purpose: The different types of renderbuffers.
//-------------------------------------------------------------------------
enum class eSamplerTypes
{
	INVALID = -1,
	LINEAR_REPEAT,

	NEAREST_REPEAT,

	MAX_TYPES,
};

//-------------------------------------------------------------------------
// Purpose: The different types of renderbuffers.
//-------------------------------------------------------------------------
enum class eRenderBufferUsage
{
	// This buffer is used to store vertex data for drawing.
	VERTEX,
	// This buffer is used to store index data for drawing.
	INDEX,
	// This buffer is used to store uniform data for use in shaders.
	UNIFORM,
	// This buffer is used for other storage purposes.
	STORAGE
};

typedef uint8_t BufferVertexFlags_t;

enum eVertexBufferDataFlags
{
	VERTEX_BUFFER_DATA_UNDEFINED = 0x00,
	VERTEX_BUFFER_DATA_POSITION = 0x01,
	VERTEX_BUFFER_DATA_TEXCOORD = 0x02,
	VERTEX_BUFFER_DATA_NORMAL = 0x04,
	VERTEX_BUFFER_DATA_TANGENT = 0x08,
};

//-------------------------------------------------------------------------
// Purpose: A generic buffer used for storing data.
//-------------------------------------------------------------------------
class RenderBuffer
{
public:
	char*			pszBufferName{ nullptr };

	// The types of vertex data stored in this buffer.
	BufferVertexFlags_t	uBufferDataFlags{ 0 };

	size_t				m_CurrentBoundOffset{ kInvalidID_U64 };

	// A pointer to the internal data of the buffer.
	void* m_pInternalData{ nullptr };
	// TODO: Store the type off here as well?
};

//-------------------------------------------------------------------------
// Purpose: The different types of renderer view attachments.
//-------------------------------------------------------------------------
enum class eRendererViewAttachmentType
{
	UNDEFINED = -1,

	// This attachment will be used to store color information.
	COLOR,
	// This attachment will be used to store depth information.
	DEPTH
};

//-------------------------------------------------------------------------
// Purpose: Represents configuration for a renderer view attachment.
//-------------------------------------------------------------------------
struct RendererViewAttachmentConfig_t
{
	// The type of attachment.
	eRendererViewAttachmentType	eAttachmentType{ eRendererViewAttachmentType::UNDEFINED };
	// Whether or not this attachment should be cleared prior to rendering.
	// DEPRECATED!
	bool						bShouldClear{ false };
};

//-------------------------------------------------------------------------
// Purpose: Represents configuration for a renderer view.
//-------------------------------------------------------------------------
struct RendererViewConfig_t
{
	// The name of this render view.
	char*											m_pszViewName{ nullptr };
	// A list of attachments associated with this view.
	Utl::CVector<RendererViewAttachmentConfig_t>	attachmentConfigs{};
	// A color used for debugging tools. Otherwise does nothing.
	Vector3D										m_debuggingColor{};

	// TODO: This needs to work!
	~RendererViewConfig_t()
	{
		if (m_pszViewName)
		{
			/*Utl::StringFree(m_pszViewName);
			m_pszViewName = nullptr;*/
		}
	}
};

//-------------------------------------------------------------------------
// Purpose: A generic renderer view (internally a renderpass).
//-------------------------------------------------------------------------
class RendererView
{
public:
	// The name of this render view.
	char* m_pszViewName{ nullptr };
	// Indicates if the renderer view should clear it's render target before rendering.
	bool	m_bShouldClear{ false };
	// A pointer to the internal data of the pass.
	void*	m_pInternalData{ nullptr };
	// A color used for debugging tools. Otherwise does nothing.
	Vector3D m_debuggingColor{};
};

//-------------------------------------------------------------------------
// Purpose: TODO Documentation!
//-------------------------------------------------------------------------
typedef unsigned char RenderpassClearFlags_t;

enum eRenderpassClearFlags
{
	RENDERPASS_CLEAR_NONE = 0x00,
	RENDERPASS_CLEAR_COLOR = 0x01,
	RENDERPASS_CLEAR_DEPTH = 0x02,
	RENDERPASS_CLEAR_STENCIL = 0x04,
};

struct RenderpassConfig_t
{
	char*											pszName{ nullptr };
	Utl::CVector<RendererViewAttachmentConfig_t>	attachmentConfigs{};

	RenderpassClearFlags_t							clearFlags{ 0 };
	bool											bPresentsAfter{ false };
};

//-------------------------------------------------------------------------
// Purpose: TODO Documentation!
//-------------------------------------------------------------------------
struct Renderpass
{
	char* pszName{ nullptr };
	void* pInternalData{ nullptr };
};

//-------------------------------------------------------------------------
// Purpose: TODO Documentation!
// TODO: Replace the above RendererView class with this!
//-------------------------------------------------------------------------
class IRenderView
{
public:
	virtual bool	Initialize(const struct EngineInterfaces_t* pEngineSystems) = 0;
	virtual void	Shutdown(class IRenderer* pRenderer) = 0;

	virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) = 0;
	virtual void	Render(RenderPacket_t* pRenderData) = 0;

	virtual const char*			GetViewName() const = 0;
	virtual const Renderpass*	GetRenderpass() const = 0;
};
