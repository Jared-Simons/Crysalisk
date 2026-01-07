//=============================================================================
// Author: Jared Simons
// Date Created: 12/25/2025
// 
// Purpose: Base class that ALL UI controls inherit from.
//=============================================================================
#include "BasePanel.h"

#include "UIContext.h" // TODO: TEMP

#include "Math/MathLib.h"
#include <Platform/Platform.h>
#include "Renderer/IRendererBackend.h"

namespace AppFramework
{
	BasePanel::BasePanel(uint16_t in_XPos, uint16_t in_YPos, uint16_t in_Width, uint16_t in_Height)
	{
		m_XPos = in_XPos;
		m_YPos = in_YPos;
		m_Width = in_Width;
		m_Height = in_Height;
	}

	bool BasePanel::Initialize(IRendererBackend* pRenderer)
	{
		// CONTROLTODO: Verify that the control has been properly setup.
		GenerateGeometry();

		return true;
	}

	// TEMP until we have a true input system.
	//extern bool bLeftMouseButtonPressed;
	bool BasePanel::IsMouseOverlapping(const uint16_t xPos, const uint16_t yPos)
	{
		if (xPos > m_XPos + m_Width)
			return false;
		if (yPos > m_YPos + m_Height)
			return false;
		if (xPos < m_XPos)
			return false;
		if (yPos < m_YPos)
			return false;

		return true;
	}

	void BasePanel::Render()
	{			
		//g_pRenderer->RenderGeometry(m_GeometryData, 0);
	}

	void BasePanel::GenerateGeometry()
	{
		std::vector<Vector2D> vertices(6);

		// CONTROLTODO: Get rid of these narrowing conversions.
		vertices[0] = { (float)m_XPos, (float)m_YPos };
		vertices[1] = { (float)m_XPos, (float)m_YPos + m_Height };
		vertices[2] = { (float)m_XPos + m_Width, (float)m_YPos };
		vertices[3] = { (float)m_XPos + m_Width, (float)m_YPos };
		vertices[4] = { (float)m_XPos, (float)m_YPos + m_Height };
		vertices[5] = { (float)m_XPos + m_Width, (float)m_YPos + m_Height };

		//uint32_t screenWidth, screenHeight;
		//g_pRenderer->GetRenderSurfaceDimensions(screenWidth, screenHeight);

		// Scale to screen dimensions and move to NDC.
		/*for (int idx = 0; idx < 6; idx++)
		{
			vertices[idx].x = ((vertices[idx].x / screenWidth) * 2.0f) - 1.0f;
			vertices[idx].y = ((vertices[idx].y / screenHeight) * 2.0f) - 1.0f;
		}*/

		// Acquire GPU resources.
		//m_GeometryData = g_pRenderer->AcquireGeometryResources(6, vertices.data());
	}

} // namespace AppFramework
