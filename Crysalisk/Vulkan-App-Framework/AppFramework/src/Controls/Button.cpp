//=============================================================================
// Author: Jared Simons
// Date Created: 1/3/2026
// 
// Purpose: A standard button.
//=============================================================================
#include "Button.h"

#include "UIContext.h"

namespace AppFramework
{
	Button::Button(uint16_t in_XPos, uint16_t in_YPos, uint16_t in_Width, uint16_t in_Height)
		: BasePanel(in_XPos, in_YPos, in_Width, in_Height)
	{
	}

	void Button::Render()
	{
		int xPos, yPos;
		GetMousePosition(xPos, yPos);

		bool bHoveredOver = IsMouseOverlapping(xPos, yPos);

		uint8_t iRenderFlags = 0;
		/*if (bHoveredOver)
			iRenderFlags = GEOMETRY_RENDER_FLAG_CONTROL_HOVERED_OVER;*/
			
		//g_pRenderer->RenderGeometry(m_GeometryData, iRenderFlags);
	}

	void Button::OnPressed()
	{
		SetUIContextQuitting(true);
	}

} // namespace AppFramework
