#pragma once

#include "Kargono/Core/Timestep.h"
#include "Kargono/Events/Event.h"

namespace Kargono 
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnEditorUIRender() {}
		virtual void OnEvent(Events::Event& event) {}

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;

	};
}
