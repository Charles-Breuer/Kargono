#pragma once
#include "Shaders/Shaders.h"
#include "Mesh/Meshes.h"
#include "Textures/Textures.h"
#include "../Application/Objects/Objects.h"
#include "Cameras/Cameras.h"
#include "../Application/Input/Input.h"
#include "LightSource/LightSource.h"
#include "RendererState/RendererState.h"
#include "Model/Model.h"

namespace Resources 
{
	extern InputManager inputManager;
	extern TextureManager textureManager;
	extern MeshManager meshManager;
	extern ShaderManager shaderManager;
	extern CameraManager cameraManager;
	extern RendererStateManager applicationManager;
	extern WindowManager windowManager;
	extern ModelManager modelManager;

	extern RendererState* currentRenderer;

	extern float currentFrame;
	extern float deltaTime;
	extern float lastFrame;
	extern int framesPerSecond;
	extern float framesCounter;
}