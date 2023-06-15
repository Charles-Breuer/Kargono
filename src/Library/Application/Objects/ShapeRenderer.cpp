#include "Objects.h"
#include "../../Library.h"

/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * ShapeRenderer Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/

 /*============================================================================================================================================================================================
  * Overloaded Constructors
  *============================================================================================================================================================================================*/

ShapeRenderer::ShapeRenderer(Orientation& orientation,
	GLMesh* mesh,
	GLShader* shader)

{
	this->orientation = &orientation;
	this->mesh = mesh;
	this->shader = shader;

}


ShapeRenderer::ShapeRenderer(Orientation& orientation)
{
	this->orientation = &orientation;
	this->mesh = Resources::currentApplication->renderer->defaultMesh;
	this->shader = Resources::currentApplication->renderer->defaultShader;

}

ShapeRenderer::~ShapeRenderer()
{
	orientation = nullptr;
	mesh = nullptr;
	shader = nullptr;
}

/*============================================================================================================================================================================================
 * Getter/Setter
 *============================================================================================================================================================================================*/


/*============================================================================================================================================================================================
 * External Functionality
 *============================================================================================================================================================================================*/

void ShapeRenderer::addLightSource(glm::vec3 color)
{
	this->lightSource = LightSource(this->orientation->translation, color);
	this->shader = Resources::shaderManager.lightSourceShader;
	lightSource.parentObject = LightSource::OBJECT;
	Resources::currentApplication->renderer->lightSourceRenderBuffer.push_back(&lightSource);
}