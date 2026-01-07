//=============================================================================
// Author: Jared Simons
// Date Created: 12/25/2025
// 
// Purpose: Base class that ALL UI controls inherit from.
//=============================================================================
#pragma once

#include "pch.h"
#include "Renderer/VulkanRenderer.h"

namespace AppFramework
{
	class BasePanel
	{
	public:
		BasePanel(uint16_t in_XPos, uint16_t in_YPos, uint16_t in_Width, uint16_t in_Height);

		virtual bool	Initialize();

		virtual void	Render();

		virtual bool	IsMouseOverlapping(const uint16_t xPos, const uint16_t yPos);

		virtual void	OnPressed() {};

	private:
		void			GenerateGeometry();

	protected:
		uint16_t	m_XPos{ 0 };
		uint16_t	m_YPos{ 0 };
		uint16_t	m_Width{ 0 };
		uint16_t	m_Height{ 0 };

		GeometryInstanceData_t	m_GeometryData{};
	};
} // namespace AppFramework
