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
	typedef u32 UIControlHandle_t;

	enum eControlType
	{
		CONTROL_TYPE_UNDEFINED = 0,
		CONTROL_TYPE_PANEL = 1,
		CONTROL_TYPE_BUTTON,

		CONTROL_TYPE_MAX_TYPES,
	};

	enum eUIControlFlags
	{
		UI_CONTROL_FLAG_GEOMETRY_UPDATED = 0x01,
	};

	struct UIControlRenderData_t
	{
		u64				vertexCount{ kInvalidID_U64 };
		u64				vertexDataOffset{ kInvalidID_U64 };
		u64				indexCount{ kInvalidID_U64 };
		u64				indexDataOffset{ kInvalidID_U64 };

		ShaderHandle_t	hShader{ kInvalidID_U32 };
		u32				iShaderInstance{ kInvalidID_U32 };

		GeometryInstance_t geo{};	// FIXME: This is temporary until we can render without it.
	};

	struct UIControlPanel_t
	{
		u16					m_eControlType{ CONTROL_TYPE_UNDEFINED };
		u32					m_iRenderableIdx{ kInvalidID_U32 };

		// Extents
		// FIXME: Make this work with window resizing.
		// Buttons break, for example, when the window is resized.
		u16					x{ 0 };
		u16					y{ 0 };
		u16					width{ 0 };
		u16					height{ 0 };

		Vector4D			color{};

		UIControlHandle_t	hParentControl{ kInvalidID_U32 };
		// An index into the appropriate array for our type-specific data.
		u32					typedIndex{ kInvalidID_U32 };
	};

	struct UIControlButton_t
	{
		UIControlPanel_t*	pPanel{ nullptr };

		// The color of the button when the mouse is hovered over it.
		// The color in the base control is used when the button does not have the cursor over it.
		Vector4D			hoveredColor{};

		bool				hovered{ false }; // FIXME: Make a flag?

		// FIXME: Better pfn signature here.
		void (*PFN_UI_Button_Control_Clicked)();
	};

} // namespace AppUI
