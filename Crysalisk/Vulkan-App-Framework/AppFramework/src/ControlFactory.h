//=============================================================================
// Author: Jared Simons
// Date Created: 12/25/2025
// 
// Purpose: Factory for creating/destroying UI controls.
//=============================================================================
#pragma once
#include "pch.h"

namespace AppFramework
{
	class BasePanel;

	enum eControlType
	{
		CONTROL_TYPE_UNDEFINED = 0,
		CONTROL_TYPE_PANEL,
		CONTROL_TYPE_BUTTON,

		CONTROL_TYPE_MAX
	};

	class ControlFactory
	{
	public:
		BasePanel*	CreateControl(eControlType eType, uint16_t xPos, uint16_t yPos, uint16_t iWidth, uint16_t iHeight);
		void		ReleaseControl(BasePanel* pControl);
	};

}  // namespace AppFramework
