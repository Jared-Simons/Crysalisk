//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: Type definitions for the UI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

// Core
#include <Resources/Loaders/FontLoader.h>

// Engine
#include <Resources/ResourceTypes.h>
#include <Systems/FontSystem.h>	// FIXME: Get rid of TextObject

namespace AppUI
{
	typedef u32 UIControlHandle_t;

	enum eControlType
	{
		CONTROL_TYPE_UNDEFINED = 0,
		CONTROL_TYPE_PANEL = 1,
		CONTROL_TYPE_FRAME,
		CONTROL_TYPE_LABEL,
		CONTROL_TYPE_BUTTON,
		CONTROL_TYPE_IMAGE_PANEL,

		CONTROL_TYPE_MAX_TYPES,
	};

	enum eUIControlFlagBits
	{
		// Should this control be rendered?
		// TODO: Implement.
		UI_CONTROL_FLAG_GEOMETRY_VISIBLE = 0x00000001,

		// Should the text in the entry be cleared upon being selected?
		// This gets unset upon clearing the text.
		UI_CONTROL_FLAG_TEXT_AUTO_CLEAR = 0x00000100,
	};

	typedef u16 UIControlFlags;

	enum eControlJustification
	{
		UI_CONTROL_JUSTIFICATION_RIGHT = 0x0,
		UI_CONTROL_JUSTIFICATION_LEFT = 0x1,
		UI_CONTROL_JUSTIFICATION_TOP = 0x2,
		UI_CONTROL_JUSTIFICATION_BOTTOM = 0x3,
	};

	struct UIControlRenderData_t
	{
		u64				vertexCount{ kInvalidID_U64 };
		u64				vertexDataOffset{ kInvalidID_U64 };
		u64				indexCount{ kInvalidID_U64 };
		u64				indexDataOffset{ kInvalidID_U64 };

		u16				extendedRenderDataIdx{ kInvalidID_U16 };

		ShaderHandle_t	hShader{ kInvalidID_U32 };
		u32				iShaderInstance{ kInvalidID_U32 };

		// Should this control be rendered?
		bool visible{ true };

		GeometryInstance_t geo{};	// FIXME: This is temporary until we can render without it.
	};

	struct UIImageRenderData_t	// Extended data for rendering images.
	{
		TextureHandle_t	hTexture{ kInvalidID_U32 };
	};

	struct UIBaseControl_t
	{
		u16 m_eControlType{ CONTROL_TYPE_UNDEFINED };
		u32 m_iRenderableIdx{ kInvalidID_U32 };
		u32 m_iControlDataIdx{ kInvalidID_U32 };

		UIControlHandle_t				m_hParentControl{ kInvalidID_U32 };
		bool							m_bInteractable{ false };	// Can this control be interacted with?

		UIControlFlags	flags{ 0 };
	};

	struct UIControlPanel_t
	{
		// Extents
		u16					x{ 0 };
		u16					y{ 0 };
		u16					width{ 0 };
		u16					height{ 0 };

		Vector4D			color{};
	};

	struct UIControlFrame_t
	{
		UIControlPanel_t*	panel{ nullptr };

		// FIXME: Have the option of a textbox/label here.
	};

	struct UIControlLabel_t
	{
		FontHandle_t	hFont{ kInvalidID_U32 };
		char*			pszText{ nullptr };

		// Extents
		u16					x{ 0 };
		u16					y{ 0 };
		u16					width{ 0 };
		u16					height{ 0 };

		// FIXME: Deprecate this.
		TextObject	text{};
	};

	// Callback for button presses.
	typedef void (*PFN_UI_Button_Control_Clicked)(void* pListenerInst);

	struct UIControlButton_t
	{
		UIControlPanel_t	panel{};

		// The color of the button when the mouse is hovered over it.
		// The color in the base control is used when the button does not have the cursor over it.
		Vector4D			hoveredColor{};

		bool				hovered{ false }; // FIXME: Make a flag?

		// FIXME: Better pfn signature here.
		PFN_UI_Button_Control_Clicked onPressedCallback{ nullptr };
		void* callbackListenerInst{ nullptr };
	};

	struct UIControlImagePanel_t
	{
		UIControlPanel_t	panel{};
		TextureHandle_t		hImage{ kInvalidID_U32 };
		FontLoader::GlyphLayoutData_t* pCurGlyph{ nullptr };
	};

} // namespace AppUI