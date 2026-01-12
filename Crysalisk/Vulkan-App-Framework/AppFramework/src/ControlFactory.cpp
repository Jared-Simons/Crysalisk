//=============================================================================
// Author: Jared Simons
// Date Created: 12/25/2025
// 
// Purpose: Factory for creating/destroying UI controls.
//=============================================================================
#include "ControlFactory.h"

#include "Renderer/IRendererBackend.h"

// Known controls.
#include "Controls/BasePanel.h"
#include "Controls/Button.h"

namespace AppFramework
{
	BasePanel* ControlFactory::CreateControl(IRendererBackend* pRenderer, eControlType eType, uint16_t xPos, uint16_t yPos, uint16_t iWidth, uint16_t iHeight)
	{
		BasePanel* pNewControl = nullptr;

		switch (eType)
		{
		case CONTROL_TYPE_PANEL:
			pNewControl = new BasePanel(xPos, yPos, iWidth, iHeight);
			pNewControl->Initialize(pRenderer);
			break;

		case CONTROL_TYPE_BUTTON:
			pNewControl = new Button(xPos, yPos, iWidth, iHeight);
			pNewControl->Initialize(pRenderer);
			break;

		case CONTROL_TYPE_UNDEFINED:
		default:
			// CONTROLTODO: Assert here!
			return nullptr;
		}

		return pNewControl;
	}

	void ControlFactory::ReleaseControl(BasePanel* pControl)
	{
		if (!pControl)
		{
			// TODO: Logging.
			return;
		}

		delete pControl;
		pControl = nullptr;
	}

}  // namespace AppFramework
