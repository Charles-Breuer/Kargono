#include "Kargono/kgpch.h"
#include "Kargono/LayerStack.h"

namespace Kargono 
{
	LayerStack::LayerStack() 
	{
		m_LayerInsert = m_Layers.begin();
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
		{
			if (layer) 
			{
				delete layer;
				layer = nullptr;
			}
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_LayerInsert = m_Layers.emplace(m_LayerInsert, layer);
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto location = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (location != m_Layers.end())
		{
			m_Layers.erase(location);
			m_LayerInsert--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto location = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (location != m_Layers.end())
		{
			m_Layers.erase(location);
		}
	}
}