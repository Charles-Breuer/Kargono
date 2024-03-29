#pragma once

#include "Kargono/Core/Layer.h"
#include "Kargono/Events/ApplicationEvent.h"

namespace Kargono 
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Events::Event& e) override;

		void Begin();
		void End();
		
		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetDarkThemeColors();

		uint32_t GetActiveWidgetID() const;
	private:
		bool m_BlockEvents = true;

	};
}
