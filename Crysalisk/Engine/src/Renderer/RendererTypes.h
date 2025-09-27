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
#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Purpose: Frame-specific data to be used in rendering operations.
//-------------------------------------------------------------------------
struct RenderPacket_t
{
	// The time elapsed between frames.
	double DeltaTime;
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
	int		NumDrawCalls{ 0 };
	// The number of meshes frustum culled in the previous frame
	int		NumGeometriesFrustumCulled{ 0 };
	// Number of frames rendered in the previous frame.
	int		PreviousFrameFPS{ 0 };
	// Number of frames rendered in the current frame.
	int		CurrentFrameFPS{ 0 };
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
	Matrix4x4,
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

//-------------------------------------------------------------------------
// Purpose: A generic buffer used for storing data.
//-------------------------------------------------------------------------
class RenderBuffer
{
public:
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
	// A pointer to the internal data of the buffer.
	void*	m_pInternalData{ nullptr };
};

//-------------------------------------------------------------------------
// Purpose: A geometric plane, used to build a frustum.
//-------------------------------------------------------------------------
struct Plane
{
	Vector3D	vecNormal{};
	float		flDistance{};

	static inline Plane PlaneCreate(Vector3D pVecPoint, Vector3D pVecNormal)
	{
		Plane outPlane{};
		outPlane.vecNormal = pVecNormal.Normalize();
		outPlane.flDistance = Vector3D::DotProduct(outPlane.vecNormal, pVecPoint);
		return outPlane;
	}

	inline float GetSignedDistanceToPlane(Vector3D& pPoint)
	{
		return Vector3D::DotProduct(vecNormal, pPoint) - flDistance;
	}
};

//-------------------------------------------------------------------------
// Purpose: The different faces of a frustum.
//-------------------------------------------------------------------------
enum eFrustumPlanes
{
	TOP_FACE = 0,
	BOTTOM_FACE,
	RIGHT_FACE,
	LEFT_FACE,
	FAR_FACE,
	NEAR_FACE,

	MAX_FRUSTUM_PLANES
};

//-------------------------------------------------------------------------
// Purpose: A frustum, typically created by a camera and used for frustum-culling.
//-------------------------------------------------------------------------
struct Frustum
{
	// Can be indexed using the eFrustumPlanes enum.
	Plane	m_Faces[MAX_FRUSTUM_PLANES];

	inline bool	FrustumIntersectsAABB(Vector3D& pVecCenter, Vector3D& pVecExtents)
	{
		for (int iFrustumFace = 0; iFrustumFace < MAX_FRUSTUM_PLANES; iFrustumFace++)
		{
			float r = pVecExtents.x * M_ABS_F(m_Faces[iFrustumFace].vecNormal.x) +
				pVecExtents.y * M_ABS_F(m_Faces[iFrustumFace].vecNormal.y) +
				pVecExtents.z * M_ABS_F(m_Faces[iFrustumFace].vecNormal.z);
			
			if (!(-r <= m_Faces[iFrustumFace].GetSignedDistanceToPlane(pVecCenter)))
				return false;
		}

		return true;
	}
};
