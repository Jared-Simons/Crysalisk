//=============================================================================
// Author: Jared Simons
// Date Created: 4/4/2025
// 
// Purpose: Dynamic memory allocator, managed with a freelist.
//=============================================================================
#include "DynamicAllocator.h"

namespace Utl
{
	DynamicAllocator::DynamicAllocator()
	{
		
	}

	void DynamicAllocator::InitAllocatorInternals(size_t pSizeInBytes, void* pMemoryBlock, bool bStoreFreelistInMemoryBlock)
	{
		CAssert(pSizeInBytes > 0 && pMemoryBlock != nullptr);
		m_SizeInBytes = pSizeInBytes;
		m_pMemoryBlock = pMemoryBlock;
		m_FreeList.SetNumBytesToTrack(pSizeInBytes);

		if (bStoreFreelistInMemoryBlock)
		{
			// Initialize the freelist now.
			m_FreeList.InitFreelist(pMemoryBlock);
			size_t freelistArraySize = m_FreeList.GetInternalMemoryRequirement();
			m_pMemoryBlock = reinterpret_cast<uint8_t*>(m_pMemoryBlock) + freelistArraySize;
			m_FreeList.SetNumBytesToTrack(pSizeInBytes - freelistArraySize);
		}
	}

	void DynamicAllocator::InitFreelist(void* pMemoryBlock)
	{
		CAssert(pMemoryBlock != nullptr);
		m_FreeList.InitFreelist(pMemoryBlock);
	}

	void* DynamicAllocator::Allocate(size_t size)
	{
		void* pNewBlock = nullptr;

		size_t offset = m_FreeList.Allocate(size + kAllocHeaderSize);
		AllocHeader_t* newHeader = reinterpret_cast<AllocHeader_t*>(reinterpret_cast<uint8_t*>(m_pMemoryBlock) + offset);
		newHeader->Offset = offset;
		newHeader->Size = size;
		pNewBlock = reinterpret_cast<uint8_t*>(newHeader) + kAllocHeaderSize;

		return pNewBlock;
	}

	int DynamicAllocator::Free(void* pMemoryBlock)
	{
		CAssert(pMemoryBlock != nullptr);

		AllocHeader_t* pHeader = reinterpret_cast<AllocHeader_t*>(reinterpret_cast<uint8_t*>(pMemoryBlock) - kAllocHeaderSize);

		/*CAssert(pHeader->Offset != 2);
		if (pHeader->Offset == 2)
		{
			pHeader = (AllocHeader_t*)((uint8_t*)pMemoryBlock - 24);
		}*/
		// Mark the memory as being free in the freelist.
		m_FreeList.Free(pHeader->Size + kAllocHeaderSize, pHeader->Offset);

		return (int)pHeader->Size;
	}
}