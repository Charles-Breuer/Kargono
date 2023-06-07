#include "../Includes.h"
#include "../Library.h"

void renderMainMenu()
{

	bool isDemoOpen = false;
	ImGui::Begin("Main Menu");
	if (ImGui::Button("Demo Button"))
	{
		UIWindow* window = &Resources::windowManager.demoWindow;
		if (!window->isRendering)
		{
			window->isRendering = true;
			Resources::currentApplication->currentWindow = window;
			for (int i = 0; i < Resources::windowManager.mainMenu.children.size(); i++)
			{
				if (window != Resources::windowManager.mainMenu.children.at(i))
				{
					Resources::windowManager.mainMenu.children.at(i)->closeChildren();
				}
			}
		}
		else
		{
			window->isRendering = false;
			Resources::currentApplication->currentWindow = window->parent;
		}
	}
	if (ImGui::Button("Dev Tools"))
	{
		UIWindow* window = &Resources::windowManager.devTools;
		if (!window->isRendering)
		{
			window->isRendering = true;
			Resources::currentApplication->currentWindow = window;
			for (int i = 0; i < Resources::windowManager.mainMenu.children.size(); i++)
			{
				if (window != Resources::windowManager.mainMenu.children.at(i))
				{
					Resources::windowManager.mainMenu.children.at(i)->closeChildren();
				}
			}
		}
		else
		{
			window->isRendering = false;
			Resources::currentApplication->currentWindow = window->parent;
		}
	}
	if (ImGui::Button("Exit Application"))
	{
		Resources::inputManager.accessClickFunction(EXIT_APPLICATION, nullptr);
	}
	ImGui::End();
}

void renderDemo()
{
	ImGui::ShowDemoWindow();
}

void renderDevTool()
{
	ImGui::Begin("Dev Tools");
	if (ImGui::CollapsingHeader("Application Resources"))
	{
		if (ImGui::CollapsingHeader("Objects"))
		{
			for (int i = 0; i < Resources::currentApplication->allSimpleObjects.size(); i++)
			{
				Object* object = &Resources::currentApplication->allSimpleObjects.at(i);
				if (ImGui::TreeNode((void*)(intptr_t)i, "Object %d", i + 1))
				{
					ImGui::Text("Translation: %fx, %fy, %fz", object->orientation.translation.x, object->orientation.translation.y, object->orientation.translation.z);
					ImGui::Text("Rotation: %fx, %fy, %fz, %fdeg", object->orientation.rotation.x, object->orientation.rotation.y, object->orientation.rotation.z, object->orientation.rotation.w);
					ImGui::Text("Scale: %fx, %fy, %fz", object->orientation.scale.x, object->orientation.scale.y, object->orientation.scale.z);
					ImGui::Separator();
					ImGui::TreePop();
				}
			}
		}
	}
	ImGui::End();
}
