//=============================================================================
// Author: Jared Simons
// Date Created: 1/3/2026
// 
// Purpose: A standard button.
//=============================================================================
#pragma once

#include "pch.h"
#include "BasePanel.h"

namespace AppFramework
{
	class Button : public BasePanel
	{
	public:
		typedef BasePanel BaseClass;

		Button(uint16_t in_XPos, uint16_t in_YPos, uint16_t in_Width, uint16_t in_Height);

		virtual void	Render() override;

		virtual void	OnPressed() override;
	};
} // namespace AppFramework
