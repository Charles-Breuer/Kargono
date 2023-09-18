#include "kgpch.h"

#include "Kargono/Renderer/RenderCommand.h"
#include "API/OpenGL/OpenGLRendererAPI.h"

namespace Kargono
{
	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}