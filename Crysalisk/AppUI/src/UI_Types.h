//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: Type definitions for the UI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

// Engine
#include <Resources/ResourceTypes.h>

namespace AppUI
{
	enum eControlType
	{
		CONTROL_TYPE_UNDEFINED = 0,
		CONTROL_TYPE_PANEL = 1,

		CONTROL_TYPE_MAX_TYPES,
	};

	struct BaseUIControl
	{
		u16					m_eControlType{ kInvalidID_U16 };
		GeometryInstance_t	m_Geometry{};
		// FIXME: Make this an internal handle?
		void*				m_pControlData{ nullptr };
	};

} // namespace AppUI
