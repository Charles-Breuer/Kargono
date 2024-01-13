#include "kgpch.h"

#include "Kargono/UI/RuntimeUI.h"

#include "Kargono/Assets/AssetManager.h"
#include "Kargono/Core/Application.h"
#include "Kargono/Projects/Project.h"
#include "Kargono/Renderer/RenderCommand.h"
#include "Kargono/Renderer/Renderer.h"
#include "Kargono/Renderer/Shader.h"
#include "Kargono/Scene/Components.h"
#include "Kargono/Script/ScriptEngine.h"

namespace Kargono::UI
{
	RuntimeEngine RuntimeEngine::s_Engine{};
	Ref<Font> s_DefaultFont = nullptr;

	static RendererInputSpec s_BackgroundInputSpec{};

	// Editor Specific Fields
	static int32_t s_WindowToDelete { -1 };
	static int32_t s_WidgetToDelete { -1 };
	static int32_t s_WindowsToAddWidget { -1 };
	static UI::WidgetTypes s_WidgetTypeToAdd {UI::WidgetTypes::None };
	static uint32_t s_WindowToAdd {0};
	static int32_t s_SelectedWindow { -1 };
	static int32_t s_SelectedWidget { -1 };


	void RuntimeEngine::Init()
	{
		s_Engine.m_CurrentUI = nullptr;
		s_Engine.m_CurrentUIHandle = 0;
		s_DefaultFont = TextEngine::InstantiateEditorFont("resources/fonts/arial.ttf");
		s_Engine.m_CurrentFont = s_DefaultFont;
		s_Engine.m_FontHandle = 0;
		// Initialize Window Spec Data
		{
			ShaderSpecification shaderSpec {ColorInputType::FlatColor, TextureInputType::None, false, true, false, RenderingType::DrawIndex, false};
			auto [uuid, localShader] = Assets::AssetManager::GetShader(shaderSpec);
			Buffer localBuffer{ localShader->GetInputLayout().GetStride() };

			Shader::SetDataAtInputLocation<Math::vec4>({ 1.0f, 1.0f, 1.0f, 1.0f }, "a_Color", localBuffer, localShader);

			ShapeComponent* shapeComp = new ShapeComponent();
			shapeComp->CurrentShape = ShapeTypes::Quad;
			shapeComp->Vertices = CreateRef<std::vector<Math::vec3>>(Shape::s_Quad.GetIndexVertices());
			shapeComp->Indices = CreateRef<std::vector<uint32_t>>(Shape::s_Quad.GetIndices());

			s_BackgroundInputSpec.Shader = localShader;
			s_BackgroundInputSpec.Buffer = localBuffer;
			s_BackgroundInputSpec.ShapeComponent = shapeComp;
		}
	}

	void RuntimeEngine::Terminate()
	{
	}

	void RuntimeEngine::LoadUIObject(Ref<UIObject> uiObject, Assets::AssetHandle uiHandle)
	{
		ClearUIEngine();

		s_Engine.m_CurrentUI = uiObject;
		s_Engine.m_CurrentUIHandle = uiHandle;
		s_Engine.m_Windows = uiObject->Windows;
		s_Engine.m_SelectColor = uiObject->m_SelectColor;
		s_Engine.m_FunctionPointers.OnMove = uiObject->m_FunctionPointers.OnMove;
		RefreshDisplayedWindows();
		s_Engine.m_CurrentFont = uiObject->m_Font;
		s_Engine.m_FontHandle = uiObject->m_FontHandle;
		if (!s_Engine.m_CurrentFont)
		{
			s_Engine.m_CurrentFont = s_DefaultFont;
			s_Engine.m_FontHandle = 0;
		}

		if (s_Engine.m_Windows.size() > 0)
		{
			s_Engine.m_Windows.at(0).DisplayWindow();
			s_Engine.m_ActiveWindow = &s_Engine.m_Windows.at(0);
			if (s_Engine.m_ActiveWindow->DefaultActiveWidgetRef)
			{
				s_Engine.m_SelectedWidget = s_Engine.m_ActiveWindow->DefaultActiveWidgetRef.get();
				s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectColor;
			}
			else
			{
				s_Engine.m_SelectedWidget = nullptr;
			}

			for (auto& window : s_Engine.m_Windows)
			{
				if (window.WidgetCounts.TextWidgetCount < 1){ continue; }
				for (uint32_t iterator{0}; iterator < window.WidgetCounts.TextWidgetCount; iterator++)
				{
					auto textWidget = (TextWidget*)window.Widgets.at(window.WidgetCounts.TextWidgetLocation + iterator).get();
					textWidget->CalculateTextSize();
				}
			}

		}


	}


	bool RuntimeEngine::SaveCurrentUIIntoUIObject()
	{
		if (!static_cast<bool>(s_Engine.m_CurrentUI) || s_Engine.m_CurrentUIHandle == 0)
		{
			KG_CORE_ERROR("Attempt to save user interface with invalid UIObject or Assets::AssetHandle in s_Engine");
			return false;
		}

		for (auto& window : s_Engine.m_Windows)
		{
			if (window.DefaultActiveWidgetRef)
			{
				auto iterator = std::find(window.Widgets.begin(), window.Widgets.end(), window.DefaultActiveWidgetRef);
				window.DefaultActiveWidget = static_cast<int32_t>(iterator - window.Widgets.begin());
			}
			else
			{
				window.DefaultActiveWidget = -1;
			}
		}

		CalculateDirections();
		auto currentUI = s_Engine.m_CurrentUI;
		currentUI->m_FunctionPointers.OnMove = s_Engine.m_FunctionPointers.OnMove;
		currentUI->m_SelectColor = s_Engine.m_SelectColor;
		currentUI->m_Font = s_Engine.m_CurrentFont;
		currentUI->m_FontHandle = s_Engine.m_FontHandle;
		currentUI->Windows = s_Engine.m_Windows;
		return true;
	}

	void RuntimeEngine::DeleteWindow(uint32_t windowLocation)
	{
		auto& windows = s_Engine.GetAllWindows();
		std::vector<Window>::iterator windowPointer = windows.begin() + static_cast<uint32_t>(windowLocation);
		windows.erase(windowPointer);

		RuntimeEngine::RefreshDisplayedWindows();
	}

	void RuntimeEngine::PushRenderData(const Math::mat4& cameraViewMatrix, uint32_t viewportWidth, uint32_t viewportHeight)
	{
		RenderCommand::ClearDepthBuffer();
		// Iterate through all characters
		Math::mat4 orthographicProjection = glm::ortho((float)0, static_cast<float>(viewportWidth),
			(float)0, static_cast<float>(viewportHeight), (float)-1, (float)1);
		Math::mat4 outputMatrix = orthographicProjection;

		/*Math::vec2 windowLocation = Math::vec2(0.3f, 0.2f);
		Math::vec2 windowSize = Math::vec2(0.2f, 0.2f);*/

		Renderer::BeginScene(outputMatrix);

		// Submit all windows
		for (auto window : s_Engine.m_DisplayedWindows)
		{
			Math::vec3 scale = Math::vec3(viewportWidth * window->Size.x, viewportHeight * window->Size.y, 1.0f);
			Math::vec3 initialTranslation = Math::vec3((viewportWidth * window->ScreenPosition.x), (viewportHeight * window->ScreenPosition.y), window->ScreenPosition.z);
			Math::vec3 translation = Math::vec3( initialTranslation.x + (scale.x / 2),  initialTranslation.y + (scale.y / 2), initialTranslation.z);

			s_BackgroundInputSpec.TransformMatrix = glm::translate(Math::mat4(1.0f), translation)
				* glm::scale(Math::mat4(1.0f), scale);
			Shader::SetDataAtInputLocation<Math::vec4>(window->BackgroundColor, "a_Color", s_BackgroundInputSpec.Buffer, s_BackgroundInputSpec.Shader);

			Renderer::SubmitDataToRenderer(s_BackgroundInputSpec);

			initialTranslation.z += 0.001f;
			for (auto& widget : window->Widgets)
			{
				widget->PushRenderData(initialTranslation, scale, static_cast<float>(viewportWidth));
			}
		}

		Renderer::EndScene();

	}

	void RuntimeEngine::AddWindow(Window& window)
	{
		window.DisplayWindow();
		s_Engine.m_Windows.push_back(window);
		RuntimeEngine::RefreshDisplayedWindows();
	}

	void RuntimeEngine::SetFont(Ref<Font> newFont, Assets::AssetHandle fontHandle)
	{
		s_Engine.m_CurrentFont = newFont;
		s_Engine.m_FontHandle = fontHandle;

		for (auto& window : s_Engine.m_Windows)
		{
			if (window.WidgetCounts.TextWidgetCount < 1) { continue; }
			for (uint32_t iterator{ 0 }; iterator < window.WidgetCounts.TextWidgetCount; iterator++)
			{
				auto textWidget = (TextWidget*)window.Widgets.at(window.WidgetCounts.TextWidgetLocation + iterator).get();
				textWidget->CalculateTextSize();
			}
		}
	}

	std::vector<Window>& RuntimeEngine::GetAllWindows()
	{
		return s_Engine.m_Windows;
	}

	void Window::IncrementIterators(uint16_t iterator)
	{
		// Note that comparisons with itself does not cause increment
		//		Ex: TextWidgetLocation is not greater than TextWidgetLocation
		if (WidgetCounts.TextWidgetLocation > iterator) { WidgetCounts.TextWidgetLocation++; }
		if (WidgetCounts.ButtonWidgetLocation > iterator) { WidgetCounts.ButtonWidgetLocation++; }
		if (WidgetCounts.CheckboxWidgetLocation > iterator) { WidgetCounts.CheckboxWidgetLocation++; }
		if (WidgetCounts.ComboWidgetLocation > iterator) { WidgetCounts.ComboWidgetLocation++; }
		if (WidgetCounts.PopupWidgetLocation > iterator) { WidgetCounts.PopupWidgetLocation++; }
	}

	void Window::DecrementIterators(uint16_t iterator)
	{
		// Note that comparisons with itself does not cause increment
		//		Ex: TextWidgetLocation is not greater than TextWidgetLocation
		if (WidgetCounts.TextWidgetLocation > iterator) { WidgetCounts.TextWidgetLocation--; }
		if (WidgetCounts.ButtonWidgetLocation > iterator) { WidgetCounts.ButtonWidgetLocation--; }
		if (WidgetCounts.CheckboxWidgetLocation > iterator) { WidgetCounts.CheckboxWidgetLocation--; }
		if (WidgetCounts.ComboWidgetLocation > iterator) { WidgetCounts.ComboWidgetLocation--; }
		if (WidgetCounts.PopupWidgetLocation > iterator) { WidgetCounts.PopupWidgetLocation--; }
	}

	void RuntimeEngine::RefreshDisplayedWindows()
	{
		s_Engine.m_DisplayedWindows.clear();
		for (auto& window : s_Engine.GetAllWindows())
		{
			if (window.GetWindowDisplayed()) { s_Engine.m_DisplayedWindows.push_back(&window); }
		}
	}

	void RuntimeEngine::ClearUIEngine()
	{
		s_Engine.m_CurrentUI = nullptr;
		s_Engine.m_CurrentUIHandle = 0;
		s_Engine.m_Windows.clear();
		s_Engine.m_DisplayedWindows.clear();
		s_Engine.m_SelectedWidget = nullptr;
		s_Engine.m_HoveredWidget = nullptr;
		s_Engine.m_ActiveWindow = nullptr;
		s_Engine.m_CurrentFont = s_DefaultFont;
		s_Engine.m_FontHandle = 0;

		// Clear Editor Values
		s_WindowToDelete = -1;
		s_WidgetToDelete = -1;
		s_WindowsToAddWidget = -1;
		s_WidgetTypeToAdd = UI::WidgetTypes::None;
		s_WindowToAdd = 0;
		s_SelectedWindow = -1;
		s_SelectedWidget = -1;
	}

	Ref<UIObject> RuntimeEngine::GetCurrentUIObject()
	{
		return s_Engine.m_CurrentUI;
	}

	Assets::AssetHandle RuntimeEngine::GetCurrentUIHandle()
	{
		return s_Engine.m_CurrentUIHandle;
	}

	void RuntimeEngine::SetCurrentUIObject(Ref<UIObject> newUI)
	{
		s_Engine.m_CurrentUI = newUI;
	}

	void RuntimeEngine::SetCurrentUIHandle(Assets::AssetHandle newHandle)
	{
		s_Engine.m_CurrentUIHandle = newHandle;
	}

	void RuntimeEngine::SetSelectedWidgetColor(const Math::vec4& color)
	{
		s_Engine.m_SelectedWidget->ActiveBackgroundColor = color;
	}

	void RuntimeEngine::SetWidgetText(const std::string& windowTag, const std::string& widgetTag, const std::string& newText)
	{
		for (auto& window : s_Engine.m_Windows)
		{
			if (window.Tag == windowTag)
			{
				for (auto& widget : window.Widgets)
				{
					if (widget->Tag == widgetTag)
					{
						if (widget->WidgetType == WidgetTypes::TextWidget)
						{
							TextWidget* textWidget = (TextWidget*)widget.get();
							textWidget->Text = newText;
							return;
						}
					}
				}
			}
		}
	}

	void RuntimeEngine::SetFunctionOnMove(const std::string& function)
	{
		s_Engine.m_FunctionPointers.OnMove = function;
	}

	std::string RuntimeEngine::GetFunctionOnMove()
	{
		return s_Engine.m_FunctionPointers.OnMove;
	}

	void RuntimeEngine::SetDisplayWindow(const std::string& windowTag, bool display)
	{
		for (auto& window : s_Engine.m_Windows)
		{
			if (window.Tag == windowTag)
			{
				if (display)
				{
					window.DisplayWindow();
				}
				else
				{
					window.HideWindow();
				}
				return;
			}
		}
	}

	void RuntimeEngine::MoveRight()
	{
		if (s_Engine.m_CurrentUI && s_Engine.m_SelectedWidget && 
			s_Engine.m_ActiveWindow && s_Engine.m_SelectedWidget->DirectionPointer.Right != -1)
		{
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectedWidget->DefaultBackgroundColor;
			s_Engine.m_SelectedWidget = s_Engine.m_ActiveWindow->Widgets.at(s_Engine.m_SelectedWidget->DirectionPointer.Right).get();
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectColor;
			Script::ScriptEngine::RunCustomCallsFunction(s_Engine.m_FunctionPointers.OnMove);
		}
	}

	void RuntimeEngine::MoveLeft()
	{
		if (s_Engine.m_CurrentUI && s_Engine.m_SelectedWidget &&
			s_Engine.m_ActiveWindow && s_Engine.m_SelectedWidget->DirectionPointer.Left != -1)
		{
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectedWidget->DefaultBackgroundColor;
			s_Engine.m_SelectedWidget = s_Engine.m_ActiveWindow->Widgets.at(s_Engine.m_SelectedWidget->DirectionPointer.Left).get();
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectColor;
			Script::ScriptEngine::RunCustomCallsFunction(s_Engine.m_FunctionPointers.OnMove);
		}
	}

	void RuntimeEngine::MoveUp()
	{
		if (s_Engine.m_CurrentUI && s_Engine.m_SelectedWidget &&
			s_Engine.m_ActiveWindow && s_Engine.m_SelectedWidget->DirectionPointer.Up != -1)
		{
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectedWidget->DefaultBackgroundColor;
			s_Engine.m_SelectedWidget = s_Engine.m_ActiveWindow->Widgets.at(s_Engine.m_SelectedWidget->DirectionPointer.Up).get();
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectColor;
			Script::ScriptEngine::RunCustomCallsFunction(s_Engine.m_FunctionPointers.OnMove);
		}
	}

	void RuntimeEngine::MoveDown()
	{
		if (s_Engine.m_CurrentUI && s_Engine.m_SelectedWidget &&
			s_Engine.m_ActiveWindow && s_Engine.m_SelectedWidget->DirectionPointer.Down != -1)
		{
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectedWidget->DefaultBackgroundColor;
			s_Engine.m_SelectedWidget = s_Engine.m_ActiveWindow->Widgets.at(s_Engine.m_SelectedWidget->DirectionPointer.Down).get();
			s_Engine.m_SelectedWidget->ActiveBackgroundColor = s_Engine.m_SelectColor;
			Script::ScriptEngine::RunCustomCallsFunction(s_Engine.m_FunctionPointers.OnMove);
		}
	}

	void RuntimeEngine::OnPress()
	{
		Script::ScriptEngine::RunCustomCallsFunction(s_Engine.m_SelectedWidget->FunctionPointers.OnPress);
	}

	void RuntimeEngine::CalculateDirections()
	{
		for (auto& window : s_Engine.m_Windows)
		{
			for (auto& currentWidget : window.Widgets)
			{
				Ref<Widget> currentBestChoice;
				uint32_t currentChoiceLocation;
				float currentBestDistance;
				uint32_t iteration;


				// Calculate Right
				currentBestChoice = nullptr;
				currentChoiceLocation = 0;
				currentBestDistance = std::numeric_limits<float>::max();
				iteration = 0;

				for (auto& potentialChoice : window.Widgets)
				{
					if (potentialChoice == currentWidget || !potentialChoice->Selectable)
					{
						iteration++;
						continue;
					}

					float currentDistance = glm::abs(glm::distance(potentialChoice->WindowPosition, currentWidget->WindowPosition));

					// Constraints ----------------
					float currentWidgetRightExtent = currentWidget->WindowPosition.x + (currentWidget->Size.x / 2);
					float potentialWidgetLeftExtent = potentialChoice->WindowPosition.x - (potentialChoice->Size.x / 2);
					if (currentWidgetRightExtent >= potentialWidgetLeftExtent)
					{
						iteration++;
						continue;
					}

					// Preferences ----------------
					if (currentBestChoice == nullptr)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}

					
					// Prefer Lowest Distance Widget
					if (currentDistance < currentBestDistance)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}

					iteration++;
				}
				if (currentBestChoice)
				{
					currentWidget->DirectionPointer.Right = currentChoiceLocation;
				}
				else
				{
					currentWidget->DirectionPointer.Right = -1;
				}

				// Calculate Left
				currentBestChoice = nullptr;
				currentChoiceLocation = 0;
				currentBestDistance = std::numeric_limits<float>::max();
				iteration = 0;

				for (auto& potentialChoice : window.Widgets)
				{
					if (potentialChoice == currentWidget || !potentialChoice->Selectable)
					{
						iteration++;
						continue;
					}

					float currentDistance = glm::abs(glm::distance(potentialChoice->WindowPosition, currentWidget->WindowPosition));

					// Constraints ----------------
					float currentWidgetLeftExtent = currentWidget->WindowPosition.x - (currentWidget->Size.x / 2);
					float potentialWidgetRightExtent = potentialChoice->WindowPosition.x + (potentialChoice->Size.x / 2);
					if (currentWidgetLeftExtent <= potentialWidgetRightExtent)
					{
						iteration++;
						continue;
					}

					// Preferences ----------------
					if (currentBestChoice == nullptr)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}


					// Prefer Lowest Distance Widget
					if (currentDistance < currentBestDistance)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}

					iteration++;
				}
				if (currentBestChoice)
				{
					currentWidget->DirectionPointer.Left = currentChoiceLocation;
				}
				else
				{
					currentWidget->DirectionPointer.Left = -1;
				}

				// Calculate Up
				currentBestChoice = nullptr;
				currentChoiceLocation = 0;
				currentBestDistance = std::numeric_limits<float>::max();
				iteration = 0;

				for (auto& potentialChoice : window.Widgets)
				{
					if (potentialChoice == currentWidget || !potentialChoice->Selectable)
					{
						iteration++;
						continue;
					}

					float currentDistance = glm::abs(glm::distance(potentialChoice->WindowPosition, currentWidget->WindowPosition));

					// Constraints ----------------
					float currentWidgetUpExtent = currentWidget->WindowPosition.y + (currentWidget->Size.y / 2);
					float potentialWidgetDownExtent = potentialChoice->WindowPosition.y - (potentialChoice->Size.y / 2);
					if (currentWidgetUpExtent >= potentialWidgetDownExtent)
					{
						iteration++;
						continue;
					}

					// Preferences ----------------
					if (currentBestChoice == nullptr)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}


					// Prefer Lowest Distance Widget
					if (currentDistance < currentBestDistance)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}

					iteration++;
				}
				if (currentBestChoice)
				{
					currentWidget->DirectionPointer.Up = currentChoiceLocation;
				}
				else
				{
					currentWidget->DirectionPointer.Up = -1;
				}

				// Calculate Down
				currentBestChoice = nullptr;
				currentChoiceLocation = 0;
				currentBestDistance = std::numeric_limits<float>::max();
				iteration = 0;

				for (auto& potentialChoice : window.Widgets)
				{
					if (potentialChoice == currentWidget || !potentialChoice->Selectable)
					{
						iteration++;
						continue;
					}

					float currentDistance = glm::abs(glm::distance(potentialChoice->WindowPosition, currentWidget->WindowPosition));

					// Constraints ----------------
					float currentWidgetDownExtent = currentWidget->WindowPosition.y - (currentWidget->Size.y / 2);
					float potentialWidgetUpExtent = potentialChoice->WindowPosition.y + (potentialChoice->Size.y / 2);
					if (currentWidgetDownExtent <= potentialWidgetUpExtent)
					{
						iteration++;
						continue;
					}

					// Preferences ----------------
					if (currentBestChoice == nullptr)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}


					// Prefer Lowest Distance Widget
					if (currentDistance < currentBestDistance)
					{
						currentBestChoice = potentialChoice;
						currentChoiceLocation = iteration;
						currentBestDistance = currentDistance;
						iteration++;
						continue;
					}

					iteration++;
				}
				if (currentBestChoice)
				{
					currentWidget->DirectionPointer.Down = currentChoiceLocation;
				}
				else
				{
					currentWidget->DirectionPointer.Down = -1;
				}

			}
		}
	}

	int32_t& RuntimeEngine::GetWindowToDelete()
	{
		return s_WindowToDelete;
	}

	int32_t& RuntimeEngine::GetWidgetToDelete()
	{
		return s_WidgetToDelete;
	}

	int32_t& RuntimeEngine::GetWindowsToAddWidget()
	{
		return s_WindowsToAddWidget;
	}

	UI::WidgetTypes& RuntimeEngine::GetWidgetTypeToAdd()
	{
		return s_WidgetTypeToAdd;
	}

	uint32_t& RuntimeEngine::GetWindowToAdd()
	{
		return s_WindowToAdd;
	}

	int32_t& RuntimeEngine::GetSelectedWindow()
	{
		return s_SelectedWindow;
	}

	int32_t& RuntimeEngine::GetSelectedWidget()
	{
		return s_SelectedWidget;
	}

	Math::vec4& RuntimeEngine::GetSelectColor()
	{
		return s_Engine.m_SelectColor;
	}

	void Window::DisplayWindow()
	{
		if (WindowDisplayed) { return; }

		WindowDisplayed = true;

		RuntimeEngine::RefreshDisplayedWindows();

	}

	void Window::HideWindow()
	{
		if (!WindowDisplayed) { return; }

		WindowDisplayed = false;

		RuntimeEngine::RefreshDisplayedWindows();
	}

	bool Window::GetWindowDisplayed()
	{
		return WindowDisplayed;
	}

	void Window::AddTextWidget(Ref<TextWidget> newWidget)
	{
		if (WidgetCounts.TextWidgetCount == 0)
		{
			WidgetCounts.TextWidgetLocation = static_cast<uint16_t>(Widgets.size());
			Widgets.push_back(newWidget);
			WidgetCounts.TextWidgetCount++;
			return;
		}

		Widgets.insert(Widgets.begin() + WidgetCounts.TextWidgetLocation + WidgetCounts.TextWidgetCount, newWidget);
		Window::IncrementIterators(WidgetCounts.TextWidgetLocation);
		WidgetCounts.TextWidgetCount++;

	}
	void Window::AddButtonWidget(Ref<ButtonWidget> newWidget)
	{
		if (WidgetCounts.ButtonWidgetCount == 0)
		{
			WidgetCounts.ButtonWidgetLocation = static_cast<uint16_t>(Widgets.size());
			Widgets.push_back(newWidget);
			WidgetCounts.ButtonWidgetCount++;
			return;
		}

		Widgets.insert(Widgets.begin() + WidgetCounts.ButtonWidgetLocation + WidgetCounts.ButtonWidgetCount, newWidget);
		Window::IncrementIterators(WidgetCounts.ButtonWidgetLocation);
		WidgetCounts.ButtonWidgetCount++;
	}
	void Window::AddCheckboxWidget(Ref<CheckboxWidget> newWidget)
	{
		if (WidgetCounts.CheckboxWidgetCount == 0)
		{
			WidgetCounts.CheckboxWidgetLocation = static_cast<uint16_t>(Widgets.size());
			Widgets.push_back(newWidget);
			WidgetCounts.CheckboxWidgetCount++;
			return;
		}

		Widgets.insert(Widgets.begin() + WidgetCounts.CheckboxWidgetLocation + WidgetCounts.CheckboxWidgetCount, newWidget);
		Window::IncrementIterators(WidgetCounts.CheckboxWidgetLocation);
		WidgetCounts.CheckboxWidgetCount++;
	}
	void Window::AddComboWidget(Ref<ComboWidget> newWidget)
	{
		if (WidgetCounts.ComboWidgetCount == 0)
		{
			WidgetCounts.ComboWidgetLocation = static_cast<uint16_t>(Widgets.size());
			Widgets.push_back(newWidget);
			WidgetCounts.ComboWidgetCount++;
			return;
		}

		Widgets.insert(Widgets.begin() + WidgetCounts.ComboWidgetLocation + WidgetCounts.ComboWidgetCount, newWidget);
		Window::IncrementIterators(WidgetCounts.ComboWidgetLocation);
		WidgetCounts.ComboWidgetCount++;
	}
	void Window::AddPopupWidget(Ref<PopupWidget> newWidget)
	{
		if (WidgetCounts.PopupWidgetCount == 0)
		{
			WidgetCounts.PopupWidgetLocation = static_cast<uint16_t>(Widgets.size());
			Widgets.push_back(newWidget);
			WidgetCounts.PopupWidgetCount++;
			return;
		}

		Widgets.insert(Widgets.begin() + WidgetCounts.PopupWidgetLocation + WidgetCounts.PopupWidgetCount, newWidget);
		Window::IncrementIterators(WidgetCounts.PopupWidgetLocation);
		WidgetCounts.PopupWidgetCount++;
	}

	void Window::DeleteWidget(int32_t widgetLocation)
	{
		KG_CORE_ASSERT(widgetLocation >= 0, "Invalid Location provided to DeleteWidget!");
		switch (Widgets.at(widgetLocation)->WidgetType)
		{
		case WidgetTypes::TextWidget: {WidgetCounts.TextWidgetCount--;  break; }
		case WidgetTypes::ButtonWidget: {WidgetCounts.ButtonWidgetCount--;  break; }
		case WidgetTypes::CheckboxWidget: {WidgetCounts.CheckboxWidgetCount--;  break; }
		case WidgetTypes::ComboWidget: {WidgetCounts.ComboWidgetCount--;  break; }
		case WidgetTypes::PopupWidget: {WidgetCounts.PopupWidgetCount--;  break; }
		}

		DecrementIterators(static_cast<int16_t>(widgetLocation));

		Widgets.erase(Widgets.begin() + widgetLocation);
	}

	void TextWidget::PushRenderData(Math::vec3 windowTranslation, const Math::vec3& windowSize, float viewportWidth)
	{
		Math::vec3 widgetSize = Math::vec3(windowSize.x * Size.x, windowSize.y * Size.y, 1.0f);
		Math::vec3 widgetTranslation = Math::vec3(windowTranslation.x + (windowSize.x * WindowPosition.x),
							windowTranslation.y + (windowSize.y * WindowPosition.y),
							windowTranslation.z);
		// Draw Widget Background
		s_BackgroundInputSpec.TransformMatrix = glm::translate(Math::mat4(1.0f), Math::vec3(widgetTranslation.x + (widgetSize.x / 2), widgetTranslation.y + (widgetSize.y / 2), widgetTranslation.z))
			* glm::scale(Math::mat4(1.0f), widgetSize);

		Shader::SetDataAtInputLocation<Math::vec4>(ActiveBackgroundColor, "a_Color", s_BackgroundInputSpec.Buffer, s_BackgroundInputSpec.Shader);
		Renderer::SubmitDataToRenderer(s_BackgroundInputSpec);

		widgetTranslation.z += 0.001f;

		// Render Text
		Math::vec2 resolution = Utility::ScreenResolutionToAspectRatio(Projects::Project::GetTargetResolution());
		float textSize = (viewportWidth * 0.15f * TextSize) * (resolution.y / resolution.x);
		//textSize /= RuntimeEngine::s_Engine.m_CurrentFont->GetAverageWidth();


		if (TextCentered)
		{
			//widgetTranslation = Math::vec3(widgetTranslation.x + (widgetSize.x * 0.5f) - ((TextAbsoluteDimensions.x * 0.5f) * textSize), widgetTranslation.y + (widgetSize.y * 0.5f), widgetTranslation.z);
			widgetTranslation = Math::vec3(widgetTranslation.x + (widgetSize.x * 0.5f) - ((TextAbsoluteDimensions.x * 0.5f) * textSize), widgetTranslation.y + (widgetSize.y * 0.5f) - ((TextAbsoluteDimensions.y * 0.5f) * textSize), widgetTranslation.z);
		}

		//KG_CORE_INFO("Hey, the widget size for this text is {} wide and the text is {} wide", widgetSize.x, TextAbsoluteDimensions.x * textSize);
		//KG_CORE_INFO("Hey, the widget height is {} and the text height is {}", widgetSize.y, TextAbsoluteDimensions.y * textSize);

		//KG_CORE_INFO(textSize);
		//KG_CORE_INFO("Text Absolute Size is {} {}", TextAbsoluteDimensions.x, TextAbsoluteDimensions.y);
		//textSize += 50.0f;

		RuntimeEngine::s_Engine.m_CurrentFont->PushTextData(Text, widgetTranslation, TextColor, textSize);
	}

	void TextWidget::CalculateTextSize()
	{
		TextAbsoluteDimensions = RuntimeEngine::s_Engine.m_CurrentFont->CalculateTextSize(Text);
	}

	void TextWidget::SetText(const std::string& newText)
	{
		Text = newText;
		CalculateTextSize();
	}

}
