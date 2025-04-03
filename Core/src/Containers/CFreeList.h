//=============================================================================
// Author: Jared Simons
// Date Created: 4/2/2025
// 
// Purpose: Freelist used to track ranges of memory.
//=============================================================================
#pragma once

namespace Utl
{
	struct FreeListNode_t
	{
		size_t			Size{ 0 };
		size_t			Offset{ 0 };
		FreeListNode_t* pNextNode{ nullptr };
	};

	class CFreeList
	{
	public:
		CFreeList(size_t bytesToTrack)
		{
			m_HeadNode.Size = bytesToTrack;
			m_HeadNode.Offset = 0;
			m_HeadNode.pNextNode = nullptr;
		}

	private:
		FreeListNode_t m_HeadNode{};
		FreeListNode_t* m_pNextFreeNode{ &m_HeadNode };
	};
}
