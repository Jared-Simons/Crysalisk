//=============================================================================
// Author: Jared Simons
// Date Created: 6/4/2026
// 
// Purpose: A linear allocator.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Memory/IMemoryAllocator.h>

namespace Utl
{
	class CORE_API LinearAllocator : public IBaseMemoryAllocator
	{
	public:
		bool			Initialize(u64 iAllocatorSize);
		void			Destroy();

		virtual void*	Allocate(u64 numBytes) override;

		virtual int		Free(void* pMemBlock) override;
		virtual void	FreeAll() override;

	private:
		u64		m_iTotalAllocatorSize{ 0 };
		u64		m_iMemoryHeadPtr{ 0 };
		bool	m_bAllocatorOwnsMemory{ false };
		void*	m_pMemory{ nullptr };
	};
} // namespace Utl