//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Defines.h"
#include "VulkanTypes.h"
#include "Renderer/IRendererBackend.h"

#include <Clock.h>

// Utility macro for checking success with VkResult
#define VK_CHECK(expr) expr == VK_SUCCESS

// Maximum frames that can be worked with at once
const u8 MAX_FRAMES_IN_FLIGHT{ 3 };

// TODO: These should be temporary
static u32 g_NewWidth{ 0 };
static u32 g_NewHeight{ 0 };
static bool g_FramebufferResized{ false };

// TODO: TEMP METRICS
struct RendererMetrics
{
	u64 numTriangles{ 0 };
	u64 numVertices{ 0 };
	u64 numMeshes{ 0 };
	u64 numDrawCalls{ 0 };
	f32 deltaTime{ 0 };

	void ResetMetrics()
	{
		numTriangles = 0;
		numVertices = 0;
		numMeshes = 0;
		numDrawCalls = 0;
		deltaTime = 0.0f;
	}
};
// TODO: END TEMP METRICS




// TODO TEMP
struct Camera
{
	Vector3D Position;
	Vector3D Front;
	Vector3D Up;
	Vector3D Right;
	Vector3D WorldUp;

	f32 Yaw;
	f32 Pitch;

	f32 MovementSpeed;

	Camera(Vector3D inPosition, Vector3D inUp, f32 inYaw, f32 inPitch)
		: Front(Vector3D(0.0f, 0.0f, -1.0f))
		, MovementSpeed(1.0f)
	{
		Position = inPosition;
		WorldUp = inUp;
		Pitch = inPitch;
		Yaw = inYaw;
		UpdateCameraVectors();
	}

	Matrix4x4 GetViewMatrix() { return Matrix4x4::LookAt(Position, Position + Front, Up); }

	void UpdateCameraVectors()
	{
		Vector3D front;
		front.x = M_COS(M_DEG2RAD(Yaw) * M_COS(M_DEG2RAD(Pitch)));
		front.y = M_SIN(M_DEG2RAD(Pitch));
		front.z = M_SIN(M_DEG2RAD(Yaw)) * M_COS(M_DEG2RAD(Pitch));
		Front = front.Normalize();

		Vector3D newRight = Vector3D::CrossProduct(Front, WorldUp);
		Right = newRight.Normalize();

		Vector3D newUp = Vector3D::CrossProduct(Right, Front);
		Up = newUp.Normalize();
	}
};
// TODO END TEMP

//=================================
// 
// Vulkan Resource Data
// 
//=================================
struct VulkanImageData_t
{
	VulkanImage internalImage;
	u32 offset;
};

struct VulkanGeometryData_t
{
	u64 m_VertexDataOffset{ 0 };
	u64 m_VertexDataSize{ 0 };

	u64 m_IndexDataOffset{ 0 };
	u64 m_IndexDataSize{ 0 };
};


//=================================
// 
// Vulkan Rendering Context
// 
//=================================
class CVulkanRenderer : public IRendererBackend
{
public:
	CVulkanRenderer() {}

	bool		RendererBackend_Initialize() override;
	void		RendererBackend_Shutdown() override;
	bool		RendererBackend_Update(struct RenderPacket_t* pRenderData) override;

private:
	bool		CreateVulkanInstance();
	bool		QueryInstanceExtensionSupport(CVector<const char*>& requiredExtensions);
	bool		QueryValidationLayerSupport(CVector<const char*>& requiredValidationLayers);

	bool		CreateDebugMessenger();
	void		DestroyDebugMessenger();

	bool		CreateTextureResources(Texture& pTexture, byte* pPixelData) override;
	void		DestroyTextureResources(Texture& pTexture) override;

	bool		CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<u16>& indices) override;
	void		DestroyGeometryResources(StaticMesh& pMesh) override;
	void		DrawGeometry(StaticMesh& pMesh, VulkanBuffer& pVertexBuffer, VulkanBuffer& pIndexBuffer, VulkanCommandBuffer& pCommandBuffer, u32 imageIndex);

public:
	// Instance
	VkInstance	m_Instance{ VK_NULL_HANDLE };

	// TODO: Implement
	VkAllocationCallbacks* m_Allocator{ nullptr };

	// Debug Messenger
	VkDebugUtilsMessengerEXT	m_DebugMessenger{ VK_NULL_HANDLE };

	// Surface
	VkSurfaceKHR	m_Surface{ VK_NULL_HANDLE };

	// Device
	VulkanDevice	m_Device{};

	// Swapchain
	VulkanSwapchain	m_Swapchain{};

	// Main Renderpass
	VulkanRenderpass	m_MainRenderpass{};

	// Command Buffers
	VkCommandPool	m_GraphicsCommandPool{ VK_NULL_HANDLE };
	CVector<VulkanCommandBuffer> m_CommandBuffers;

	// Sync Objects
	CVector<VkFence>		m_FrameInFlightFences;
	CVector<VkSemaphore> m_ImageAvailableSemaphores;
	CVector<VkSemaphore> m_RenderFinishedSemaphores;

	// Buffers
	VulkanBuffer m_VertexBuffer;
	VulkanBuffer m_IndexBuffer;

	size_t		m_VertexBufferOffset{ 0 };
	size_t		m_IndexBufferOffset{ 0 };

	// Shaders
	VulkanShader		m_StandardShader;

	// Validation Layers
	CVector<const char*>* m_ValidationLayers{};
	bool		EnableValidationLayers()
	{
#ifdef _DEBUG
		return true;
#else
		return false;
#endif
	}

	u32 m_CurrentFrame{ 0 };

	u32 m_FramebufferWidth{ 0 };
	u32 m_FramebufferHeight{ 0 };

	Clock m_RendererClock;
	RendererMetrics m_Metrics;
};

IRendererBackend* GetVulkanBackend();

