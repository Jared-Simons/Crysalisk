//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: Implements the Vulkan Rendering Backend
//=============================================================================
#pragma once

#include "Renderer/IRendererBackend.h"
#include "EngineGlobals.h"

#include <Clock.h>
#include <Containers/CVector.h>
#include <Math/MatrixTypes.h>
#include <Math/VectorTypes.h>

#include <vulkan/vulkan.h>

// Forward Declarations
template <typename T> class CVector;

class	VulkanDevice;
struct	SwapchainSupportDetails;
class	VulkanSwapchain;
class	VulkanRenderpass;
class	VulkanCommandPool;
class	VulkanCommandBuffer;
class	VulkanFramebuffer;
class	VulkanFence;
class	VulkanSemaphore;
class	VulkanBuffer;
class	VulkanImage;
class	VulkanShader;
class	CVulkanRenderer;

// Utility Macro for checking success with VkResult
#define VK_CHECK(expr) expr == VK_SUCCESS

// Maximum frames that can be worked with at once
const char MAX_FRAMES_IN_FLIGHT{ 3 };

// TODO: These should be temporary
static unsigned int g_NewWidth{ 0 };
static unsigned int g_NewHeight{ 0 };
static bool g_FramebufferResized{ false };

//=================================
// 
// Vulkan Device
// 
//=================================
struct DeviceQueueFamilyInfo_t
{
	unsigned char GraphicsQueueIndex{ INVALID_ID_U8 };
	unsigned char ComputeQueueIndex{ INVALID_ID_U8 };
	unsigned char TransferQueueIndex{ INVALID_ID_U8 };
	unsigned char PresentQueueIndex{ INVALID_ID_U8 };

	bool IsComplete()
	{
		if (IsConceptOccupied(&GraphicsQueueIndex) && IsConceptOccupied(&PresentQueueIndex) 
			&& IsConceptOccupied(&TransferQueueIndex) && IsConceptOccupied(&ComputeQueueIndex))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsConceptOccupied(unsigned char* queueIndex) { return *queueIndex != INVALID_ID_U8; }
	bool IsIndexOccupied(unsigned char index) const
	{
		return (GraphicsQueueIndex == index) || (ComputeQueueIndex == index) || 
			(TransferQueueIndex == index) || (PresentQueueIndex == index);
	}

	size_t GetNumberOfValidQueues()
	{
		size_t numQueues{ 0 };

		if (IsConceptOccupied(&GraphicsQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&ComputeQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&TransferQueueIndex))
			numQueues++;

		if (IsConceptOccupied(&PresentQueueIndex))
			numQueues++;

		return numQueues;
	}
};

class VulkanDevice
{
public:
	static bool CreateDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pOutDevice);
	static void DestroyDevice(CVulkanRenderer* pRenderingContext, VulkanDevice* pDevice);

	unsigned int	FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags& properties) const;
	
	void		QuerySwapchainSupportDetails(CVulkanRenderer* pRenderingContext, SwapchainSupportDetails* pOutDetails) const;

private:
	bool		SelectPhysicalDevice(CVulkanRenderer* pRenderingContext);
	bool		QueryDeviceQueueFamilyProperties(CVulkanRenderer* pRenderingContext, VkPhysicalDevice& physicalDevice, DeviceQueueFamilyInfo_t* pOutQueueInfo);
	bool		QueryDeviceExtensionSupport(VkPhysicalDevice& physicalDevice);

	bool		CreateLogicalDevice(CVulkanRenderer* pRenderingContext);

public:
	VkDevice			m_LogicalDevice{ VK_NULL_HANDLE };

	VkQueue				m_GraphicsQueue{ VK_NULL_HANDLE };
	VkQueue				m_PresentQueue{ VK_NULL_HANDLE };
	VkQueue				m_TransferQueue{ VK_NULL_HANDLE };
	VkQueue				m_ComputeQueue{ VK_NULL_HANDLE };

	VkPhysicalDevice			m_PhysicalDevice{ VK_NULL_HANDLE };

	VkPhysicalDeviceProperties	m_PhysicalDeviceProperties{};
	VkPhysicalDeviceFeatures	m_PhysicalDeviceFeatures{};
	DeviceQueueFamilyInfo_t		m_PhysicalDeviceQueueFamilyInfo{};

	CVector<const char*>* m_DeviceExtensions;
};

//=================================
// 
// Vulkan Swapchain
// 
//=================================
struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR	surfaceCapabilities;
	CVector<VkSurfaceFormatKHR> formats;
	CVector<VkPresentModeKHR>	presentModes;
};

class VulkanSwapchain
{
public:
	static bool CreateSwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pOutSwapchain);
	static void DestroySwapchain(CVulkanRenderer* pRenderingContext, VulkanSwapchain* pSwapchain);

	bool		RecreateSwapchain(CVulkanRenderer* pRenderingContext, unsigned int newWidth, unsigned int newHeight);
	bool		Present(CVulkanRenderer* pRenderingContext, unsigned int imageIndex);

	bool		GetSwapchainImages(CVulkanRenderer* pRenderingContext);
	bool		CreateImageViews(CVulkanRenderer* pRenderingContext);

	VkSwapchainKHR			m_SwapchainHandle{ VK_NULL_HANDLE };
	SwapchainSupportDetails m_SwapchainSupport{};
	CVector<VkImage>		m_SwapchainImages;
	CVector<VkImageView>	m_SwapchainImageViews;

	// Framebuffers for rendering to the screen
	CVector<VulkanFramebuffer> m_Framebuffers;

	VkFormat			m_SwapchainImageFormat;
	VkExtent2D			m_SwapchainExtent;
};

//=================================
// 
// Vulkan Renderpass
// 
//=================================
class VulkanRenderpass
{
public:
	static bool		CreateRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pOutRenderpass);
	static void		DestroyRenderpass(CVulkanRenderer* pRenderingContext, VulkanRenderpass* pRenderpass);

	bool			Begin(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer, unsigned int imageIndex);
	void			End(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);

	VkRenderPass m_Handle{ VK_NULL_HANDLE };
};

//=================================
// 
// Vulkan Command Buffer
// 
//=================================
enum class eVulkanCommandBufferState
{
	/* The command buffer has just been allocated or has been reset to the initial state. Can only be set to record or freed. */
	COMMAND_BUFFER_STATE_INITIAL,
	/* The command buffer can be written to with vkCmd calls. */
	COMMAND_BUFFER_STATE_RECORDING,
	/* The command buffer has been written to and can be freed or submitted for execution. */
	COMMAND_BUFFER_EXECUTABLE,
	/* The command buffer has been submitted for queue submission and CANNOT be modified at this point. */
	COMMAND_BUFFER_STATE_PENDING,
	/* The command buffer has been invalidated through an operation that modified it's resources, can be freed or rest. */
	COMMAND_BUFFER_STATE_INVALID
};

class VulkanCommandPool
{
public:
	static bool		CreateCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pOutPool);
	static void		DestroyCommandPool(CVulkanRenderer* pRenderingContext, VulkanCommandPool* pPool);

public:
	VkCommandPool				m_Handle{ VK_NULL_HANDLE };
	VkCommandPoolCreateFlags	m_PoolFlags;
};

class VulkanCommandBuffer
{
public:
	static bool		CreateCommandBuffer(CVulkanRenderer* pRenderingContext, bool isPrimary, VulkanCommandBuffer* pOutCommandBuffer);
	static void		DestroyCommandBuffer(CVulkanRenderer* pRenderingContext, VulkanCommandBuffer* pCommandBuffer);

	bool			Begin();
	void			End();
	void			Reset();
	bool			Submit(CVulkanRenderer* pRenderingContext);

	bool			BeginSingleUse(CVulkanRenderer* pRenderingContext, VkCommandPool* pCommandPool);
	void			EndSingleUse(CVulkanRenderer* pRenderingContext, VkCommandPool* pCommandPool);

public:
	VkCommandBuffer				m_Handle{ VK_NULL_HANDLE };
	eVulkanCommandBufferState	m_State{ eVulkanCommandBufferState::COMMAND_BUFFER_STATE_INITIAL };

	// Is this a primary or secondary level command buffer?
	bool						m_bIsPrimary{ false };
};


//=================================
// 
// Vulkan Framebuffer
// 
//=================================
class VulkanFramebuffer
{
public:
	static bool		CreateFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pOutFramebuffer, unsigned int framebufferIndex);
	static void		DestroyFramebuffer(CVulkanRenderer* pRenderingContext, VulkanFramebuffer* pFramebuffer);

public:
	VkFramebuffer			m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Fence
// 
//=================================
class VulkanFence
{
public:
	static bool		CreateFence(CVulkanRenderer* pRenderingContext, bool bCreateSignaled, VulkanFence* pOutFence);
	static void		DestroyFence(CVulkanRenderer* pRenderingContext, VulkanFence* pFence);

public:
		VkFence	m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Semaphore
// 
//=================================
class VulkanSemaphore
{
public:
	// NOTE: This needs to be named 'CreateVulkanSemaphore' as there is an existing 'CreateSemaphore' macro in the Win32 API that I don't want to disturb.
	static bool		CreateVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pOutSemaphore);
	static void		DestroyVulkanSemaphore(CVulkanRenderer* pRenderingContext, VulkanSemaphore* pSemaphore);

public:
	VkSemaphore m_Handle{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Buffer
// 
//=================================
class VulkanBuffer
{
public:
	static bool		CreateBuffer(CVulkanRenderer* pRenderingContext, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VulkanBuffer* pOutBuffer);
	static void		DestroyBuffer(CVulkanRenderer* pRenderingContext, VulkanBuffer* pBuffer);

	// TODO: Handle offsets and other data here
	void			UploadDataToBuffer(CVulkanRenderer* pRenderingContext, size_t dataSize, void* data, size_t offset);
	bool			CopyBufferToImage(CVulkanRenderer* pRenderingContext, unsigned int width, unsigned int height, VulkanImage* pImage);

	VkBuffer		m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory	m_Memory{ VK_NULL_HANDLE };
};


//=================================
// 
// Vulkan Image
// 
//=================================
class VulkanImage
{
public:
	static bool		CreateImage(CVulkanRenderer* pRenderingContext, int width, int height, int numChannels, void* pPixelData, bool bCreateView, VulkanImage* pOutImage);
	static void		DestroyImage(CVulkanRenderer* pRenderingContext, VulkanImage* pImage);

	bool			TransitionImageLayout(CVulkanRenderer* pRenderingContext, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkImage			m_Handle{ VK_NULL_HANDLE };
	VkDeviceMemory	m_ImageMemory{ VK_NULL_HANDLE };
	VkImageView		m_ImageView{ VK_NULL_HANDLE };
	VkSampler		m_ImageSampler{ VK_NULL_HANDLE };

	VkFormat		m_ImageFormat{ VK_FORMAT_UNDEFINED };
};


//=================================
// 
// Uniform Buffer
// 
//=================================
// TODO: Move to the frontend
struct UniformBufferObject
{
	UniformBufferObject() {}

	Matrix4x4 model;
	Matrix4x4 view;
	Matrix4x4 projection;
};


//=================================
// 
// Vulkan Shader
// 
//=================================
class VulkanShader
{
public:
	VulkanShader()
		: m_VertModule{ VK_NULL_HANDLE }
		, m_FragModule{ VK_NULL_HANDLE }
		, m_Pipeline{ VK_NULL_HANDLE }
		, m_PipelineLayout{ VK_NULL_HANDLE }
		, m_DescriptorPool{ VK_NULL_HANDLE }
		, m_DescriptorSetLayout{ VK_NULL_HANDLE }
		, m_DescriptorSets{ VK_NULL_HANDLE } {}

	static bool		CreateVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);
	static void		DestroyVulkanShader(CVulkanRenderer* pRenderingContext, VulkanShader* pShader);

	static bool		CreatePipeline(CVulkanRenderer* pRenderingContext, VulkanShader* pOutShader);

	bool			Bind(CVulkanRenderer* pRenderingContext, VkCommandBuffer* pCommandBuffer, uint32_t imageIndex) const;
	void			UpdateUniforms(CVulkanRenderer* pRenderingContext, uint32_t imageIndex, struct Camera s_Camera, float deltaTime);
	bool			AcquireInstanceResources(CVulkanRenderer* pRenderingContext, Texture& pTexture, void* pPixelData);
	bool			AcquireTextureResources(CVulkanRenderer* pRenderingContext, Texture& pTexture, void* pPixelData);
	bool			CreateDescriptorResources(CVulkanRenderer* pRenderingContext);

	void			UpdateDescriptorSets(CVulkanRenderer* pRenderingContext);

private:

public:
	VkShaderModule		m_VertModule;
	VkShaderModule		m_FragModule;

	VkPipeline			m_Pipeline;
	VkPipelineLayout	m_PipelineLayout;

	VkDescriptorSetLayout		m_DescriptorSetLayout;
	VkDescriptorPool			m_DescriptorPool;
	VkDescriptorSet				m_DescriptorSets[MAX_FRAMES_IN_FLIGHT]; // NOTE: Will need to be dynamic to support hot-swapping of double and triple buffering

	// TODO: Instance level resources!
	CVector<VulkanBuffer>		m_UniformBuffers;
	CVector<void*>				m_UniformBuffersMappedMemory;

	VulkanImage m_TextureImage;
	bool descriptorUpdateNeeded{ false };
};

// TODO TEMP
struct Camera
{
	Vector3D Position;
	Vector3D Front;
	Vector3D Up;
	Vector3D Right;
	Vector3D WorldUp;

	float Yaw;
	float Pitch;

	float MovementSpeed;

	Camera(Vector3D inPosition, Vector3D inUp, float inYaw, float inPitch)
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
		front.x = static_cast<float>(M_COS(M_DEG2RAD(Yaw)) * M_COS(M_DEG2RAD(Pitch)));
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

	bool		CreateTextureResources(Texture& pTexture, unsigned char* pPixelData) override;
	void		DestroyTextureResources(Texture& pTexture) override;

	bool		CreateGeometryResources(StaticMesh& pMesh, CVector<Vertex3D>& vertices, CVector<unsigned short>& indices) override;
	void		DestroyGeometryResources(StaticMesh& pMesh) override;
	void		DrawGeometry(StaticMesh& pMesh, VulkanBuffer& pVertexBuffer, VulkanBuffer& pIndexBuffer, VulkanCommandBuffer& pCommandBuffer, uint32_t imageIndex);

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
	VulkanCommandPool	m_GraphicsCommandPool{};
	CVector<VulkanCommandBuffer> m_CommandBuffers;

	// Sync Objects
	CVector<VulkanFence> m_FrameInFlightFences;
	CVector<VulkanSemaphore> m_ImageAvailableSemaphores;
	CVector<VulkanSemaphore> m_RenderFinishedSemaphores;

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

	unsigned int m_CurrentFrame{ 0 };

	unsigned int m_FramebufferWidth{ 0 };
	unsigned int m_FramebufferHeight{ 0 };

	Clock m_RendererClock;
};

IRendererBackend* GetVulkanBackend();

