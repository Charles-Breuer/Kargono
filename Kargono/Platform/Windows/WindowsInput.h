#pragma once
#include "Kargono/Core/input.h"

namespace Kargono
{
	class WindowsInput : public Input
	{
	protected:
		virtual bool IsKeyPressedImpl(KeyCode keycode) override;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) override;
		virtual std::pair<float, float> GetMousePositionImpl() override;
		virtual bool GetMouseXImpl() override;
		virtual bool GetMouseYImpl() override;


	};

}