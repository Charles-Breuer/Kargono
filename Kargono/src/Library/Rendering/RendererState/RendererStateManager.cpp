#include "Kargono/kgpch.h"
#include "RendererState.h"
#include "../../Library.h"

/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * RendererState Manager Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/

 /*============================================================================================================================================================================================
  * Instantiate/Delete Application objects
  *============================================================================================================================================================================================*/
void RendererStateManager::CreateDefaultRenderers()
{
	

	CreateSample3DRenderer();
	CreateSample2DRenderer();

}

void RendererStateManager::DestroyDefaultRenderers()
{
	delete Sample3DRenderer;
	delete Sample2DRenderer;
	Sample3DRenderer = nullptr;
	Sample2DRenderer = nullptr;

}

void RendererStateManager::DestroyRenderer(RendererState& application)
{

}

/*============================================================================================================================================================================================
 * Initializes RendererState Objects with specified values
 *============================================================================================================================================================================================*/

void RendererStateManager::CreateSample3DRenderer()
{
	int Version[2]{ 4, 6 };
	Sample3DRenderer = new RendererState("3D Renderer", Version, glm::vec2{1920.0f, 1080.0f}, glm::vec3{0.0f, 0.0f, 0.0f});
}

void RendererStateManager::CreateSample2DRenderer()
{
	int Version[2]{ 4, 6 };
	Sample2DRenderer = new RendererState("2D Renderer", Version, glm::vec2{1920.0f, 1080.0f}, glm::vec3{0.0f, 0.11f, 0.1f});

}