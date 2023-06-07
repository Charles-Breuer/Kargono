#include "Objects.h"
#include "../Library.h"

/*============================================================================================================================================================================================
 *============================================================================================================================================================================================
 * ModelRenderer Class
 *============================================================================================================================================================================================
 *============================================================================================================================================================================================*/

 /*============================================================================================================================================================================================
  * Overloaded Constructors
  *============================================================================================================================================================================================*/

ModelRenderer::ModelRenderer(Orientation& orientation,
	Model* model,
	GLShader* shader)

{
	this->orientation = &orientation;
	this->model = model;
	this->shader = shader;
	
}


ModelRenderer::ModelRenderer(Orientation& orientation)
{
	this->orientation = &orientation;
	this->model = Resources::currentApplication->defaultModel;
	this->shader = Resources::currentApplication->defaultShader;
	
}

ModelRenderer::~ModelRenderer()
{
	
}

/*============================================================================================================================================================================================
 * Getter/Setter
 *============================================================================================================================================================================================*/


/*============================================================================================================================================================================================
 * External Functionality
 *============================================================================================================================================================================================*/

void ModelRenderer::addLightSource(glm::vec3 color)
{
	this->lightSource = LightSource(this->orientation->translation, color);
	this->shader = &Resources::shaderManager.lightSourceShader;
	lightSource.parentObject = LightSource::OBJECT;
	Resources::currentApplication->allLightSources.push_back(&lightSource);
}