#include "Kargono/kgpch.h"

#include "EditorLayer.h"

#include "imgui.h"
#include "ImGuizmo.h"

namespace Kargono
{

	EditorLayer* EditorLayer::s_EditorLayer = nullptr;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
		Application::GetCurrentApp().AddImGuiLayer();
		s_EditorLayer = this;
	}

	void EditorLayer::OnAttach()
	{
		auto& currentWindow = Application::GetCurrentApp().GetWindow();

		UI::EditorEngine::Init();
		m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>();

		FramebufferSpecification fbSpec;
		fbSpec.Attachments = { FramebufferDataFormat::RGBA8, FramebufferDataFormat::RED_INTEGER,  FramebufferDataFormat::Depth };
		fbSpec.Width = 1600;
		fbSpec.Height = 900;
		m_ViewportFramebuffer = Framebuffer::Create(fbSpec);

		m_EditorScene = CreateRef<Scene>();
		Scene::SetActiveScene(m_EditorScene);

		m_SceneState = SceneState::Edit;
		m_LogPanel = CreateScope<LogPanel>();

		auto commandLineArgs = Application::GetCurrentApp().GetSpecification().CommandLineArgs;
		if (commandLineArgs.Count > 1)
		{
			auto projectFilePath = commandLineArgs[1];
			OpenProject(projectFilePath);
		}
		else
		{
			// TODO: prompt the user to select a directory
			// NewProject();

			// If no project is opened, close Editor
			// TODO: this is while we don't have a new project path
			if (!OpenProject())
			{
				Application::GetCurrentApp().Close();
				return;
			}

		}

		Renderer::Init();
		Renderer::SetLineWidth(4.0f);
		UI::TextEngine::Init();
		UI::RuntimeEngine::Init();

		m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
		
		InitializeOverlayData();

		m_ViewportSize = { currentWindow.GetWidth(), currentWindow.GetHeight() };
		Scene::GetActiveScene()->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
	}

	void EditorLayer::OnDetach()
	{
		auto allAudioComponents = m_EditorScene->GetAllEntitiesWith<AudioComponent>();
		for (auto& entity : allAudioComponents)
		{
			Entity e = { entity, m_EditorScene.get()};
			auto& audioComponent = e.GetComponent<AudioComponent>();
			audioComponent.Audio.reset();
		}
		auto allMultiAudioComponents = m_EditorScene->GetAllEntitiesWith<MultiAudioComponent>();
		for (auto& entity : allMultiAudioComponents)
		{
			Entity e = { entity, m_EditorScene.get() };
			auto& multiAudioComponent = e.GetComponent<MultiAudioComponent>();
			for (auto& [key, component] : multiAudioComponent.AudioComponents)
			{
				component.Audio.reset();
			}
		}
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		if (FramebufferSpecification spec = m_ViewportFramebuffer->GetSpecification();
			static_cast<float>(m_ViewportSize.x) > 0.0f && static_cast<float>(m_ViewportSize.y) > 0.0f &&
			(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
		{
			m_ViewportFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_EditorCamera.SetViewportSize(static_cast<float>(m_ViewportSize.x), static_cast<float>(m_ViewportSize.y));
		}
		
		// Render
		Renderer::ResetStats();
		m_ViewportFramebuffer->Bind();
		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();

		// Clear our entity ID attachment to -1
		m_ViewportFramebuffer->ClearAttachment(1, -1);

		// Update Scene
		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				m_EditorCamera.OnUpdate(ts);

				OnUpdateEditor(ts, m_EditorCamera);
				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera.OnUpdate(ts);

				OnUpdateSimulation(ts, m_EditorCamera);
				break;
			}
			case SceneState::Play:
			{
				OnUpdateRuntime(ts);
				break;
			}
		}

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		Math::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;

		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX <(int)viewportSize.x && mouseY < (int)viewportSize.y)
		{
			int pixelData = m_ViewportFramebuffer->ReadPixel(1, mouseX, mouseY);
			*Scene::GetActiveScene()->GetHoveredEntity() = Scene::GetActiveScene()->CheckEntityExists((entt::entity)pixelData) ? Entity((entt::entity)pixelData, Scene::GetActiveScene().get()) : Entity();
		}

		OnOverlayRender();

		if (m_ShowUserInterface)
		{
			UI::RuntimeEngine::PushRenderData(m_EditorCamera, m_ViewportSize.x, m_ViewportSize.y);
		}


		m_ViewportFramebuffer->Unbind();

	}

	void EditorLayer::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		Scene::GetActiveScene()->RenderScene(camera, camera.GetViewMatrix());
	}

	void EditorLayer::OnUpdateRuntime(Timestep ts)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			// Update Scripts

			Script::ScriptEngine::OnUpdate(ts);

			Scene::GetActiveScene()->OnUpdatePhysics(ts);
		}

		// Render 2D
		Entity cameraEntity = Scene::GetActiveScene()->GetPrimaryCameraEntity();
		Camera* mainCamera = &cameraEntity.GetComponent<CameraComponent>().Camera;
		Math::mat4 cameraTransform = cameraEntity.GetComponent<TransformComponent>().GetTransform();

		if (mainCamera)
		{
			// Transform Matrix needs to be inversed so that final view is from the perspective of the camera
			Scene::GetActiveScene()->RenderScene(*mainCamera, glm::inverse(cameraTransform));
		}

	}

	void EditorLayer::OnUpdateSimulation(Timestep ts, EditorCamera& camera)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			Scene::GetActiveScene()->OnUpdatePhysics(ts);
		}

		// Render
		Scene::GetActiveScene()->RenderScene(camera, camera.GetViewMatrix());
	}

	void EditorLayer::Step(int frames)
	{
		m_StepFrames = frames;
	}


	void EditorLayer::OnImGuiRender()
	{
		static bool dockspaceOpen = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
		// because it would be confusing to have two docking targets within each others.
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
		// and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		if (m_RuntimeFullscreen && (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate) && !m_IsPaused)
		{
			if (m_ShowViewport) { UI_Viewport(); }
			if (m_ShowToolbar) { UI_Toolbar(); }
			ImGui::End();
			return;
		}

		// Set up Menu Toolbar
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Project ...", "Ctrl+O")) { OpenProject(); }

				ImGui::Separator();

				if (ImGui::MenuItem("New Scene", "Ctrl+N")){ NewScene();}

				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					SaveScene();
					SaveProject();
				}

				if (ImGui::MenuItem("Exit")) { Application::GetCurrentApp().Close(); }
				ImGui::EndMenu();

			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload Assembly", "Ctrl+R"))
				{
					if (m_SceneState != SceneState::Edit) { OnSceneStop(); }
					Script::ScriptEngine::ReloadAssembly();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Panels"))
			{
				ImGui::MenuItem("Content Browser", NULL, &m_ShowContentBrowser);
				ImGui::MenuItem("Scene Hierarchy", NULL, &m_ShowSceneHierarchy);
				ImGui::MenuItem("Viewport", NULL, &m_ShowViewport);
				ImGui::MenuItem("Toolbar", NULL, &m_ShowToolbar);
				ImGui::Separator();
				ImGui::MenuItem("User Interface Editor", NULL, &m_ShowUserInterfaceEditor);
				ImGui::MenuItem("Input Mode Editor", NULL, &m_ShowInputEditor);
				ImGui::Separator();
				ImGui::MenuItem("Settings", NULL, &m_ShowSettings);
				ImGui::MenuItem("Project", NULL, &m_ShowProject);
				ImGui::Separator();
				ImGui::MenuItem("Stats", NULL, &m_ShowStats);
				ImGui::MenuItem("Log", NULL, &m_ShowLog);
				ImGui::MenuItem("ImGui Demo", NULL, &m_ShowDemoWindow);
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		if (m_ShowSceneHierarchy) { m_SceneHierarchyPanel->OnImGuiRender(); }
		if (m_ShowContentBrowser) { m_ContentBrowserPanel->OnImGuiRender(); }
		if (m_ShowLog) { m_LogPanel->OnImGuiRender(); }
		if (m_ShowStats) { UI_Stats(); }
		if (m_ShowSettings) { UI_Settings(); }
		if (m_ShowViewport) { UI_Viewport(); }
		if (m_ShowUserInterfaceEditor) { UI_UserInterface(); }
		if (m_ShowInputEditor) { m_InputEditorPanel->OnImGuiRender(); }
		if (m_ShowToolbar) { UI_Toolbar(); }
		if (m_ShowProject) { UI_Project(); }
		if (m_ShowDemoWindow) { ImGui::ShowDemoWindow(); }

		ImGui::End();
	}

	void EditorLayer::UI_Toolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		auto& colors = ImGui::GetStyle().Colors;
		const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
		const auto& buttonActive = colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

		ImGui::Begin("##toolbar", nullptr, 0);

		bool toolbarEnabled = (bool)Scene::GetActiveScene();

		ImVec4 tintColor = ImVec4(1, 1, 1, 1);

		if (!toolbarEnabled) { tintColor.w = 0.5f; }

		float size = ImGui::GetWindowHeight() - 4.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

		bool hasPlayButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play;
		bool hasSimulateButton = m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate;
		bool hasPauseButton = m_SceneState != SceneState::Edit;

		if (hasPlayButton)
		{
			Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate) ? UI::EditorEngine::s_IconPlay : UI::EditorEngine::s_IconStop;
			if (ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor )
				&& toolbarEnabled)
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate) { OnScenePlay(); }
				else if (m_SceneState == SceneState::Play) { OnSceneStop(); }
			}
		}
		if (hasSimulateButton)
		{
			if (hasPlayButton) { ImGui::SameLine(); }
			
			Ref<Texture2D> icon = (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play) ? UI::EditorEngine::s_IconSimulate : UI::EditorEngine::s_IconStop;
			//ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
			if (ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor)
				&& toolbarEnabled)
			{
				if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play) { OnSceneSimulate(); }
				else if (m_SceneState == SceneState::Simulate) { OnSceneStop(); }
			}
			
		}

		if (hasPauseButton)
		{
			ImGui::SameLine();
			{
				Ref<Texture2D> icon = UI::EditorEngine::s_IconPause;
				if (ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor)
					&& toolbarEnabled)
				{
					m_IsPaused = !m_IsPaused;
				}
			}
			if (m_IsPaused)
			{
				ImGui::SameLine();
				{
					Ref<Texture2D> icon = UI::EditorEngine::s_IconStep;
					if (ImGui::ImageButton((ImTextureID)(uint64_t)icon->GetRendererID(), ImVec2(size, size), ImVec2{ 0, 1 }, ImVec2{ 1, 0 }, 0, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor)
						&& toolbarEnabled)
					{
						Step(1);
					}
				}
			}
		}

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
		ImGui::End();
	}

	void EditorLayer::UI_Settings()
	{
		ImGui::Begin("Settings");
		ImGui::Checkbox("Show Physics Colliders", &m_ShowPhysicsColliders);
		ImGui::Checkbox("Show Camera Frustrums", &m_ShowCameraFrustrums);
		ImGui::Checkbox("Show Runtime User Interface", &m_ShowUserInterface);
		ImGui::Checkbox("Fullscreen While Running or Simulating", &m_RuntimeFullscreen);
		static float musicVolume = 10.0f;
		ImGui::Separator();
		static int32_t* choice = (int32_t*)&m_EditorCamera.GetMovementType();
		ImGui::Text("Editor Camera Movement:");
		if (ImGui::RadioButton("Model Viewer", choice, (int32_t)EditorCamera::MovementType::ModelView))
		{
			m_EditorCamera.SetMovementType(EditorCamera::MovementType::ModelView);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("FreeFly", choice, (int32_t)EditorCamera::MovementType::FreeFly))
		{
			m_EditorCamera.SetMovementType(EditorCamera::MovementType::FreeFly);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(Tab)");

		if (*choice == (int32_t)EditorCamera::MovementType::FreeFly)
		{
			ImGui::DragFloat("Speed", &m_EditorCamera.GetMovementSpeed(), 0.5f, 
				m_EditorCamera.GetMinMovementSpeed(), m_EditorCamera.GetMaxMovementSpeed());
		}

		ImGui::Text("Physics Settings:");
		if (ImGui::DragFloat2("Gravity", glm::value_ptr(m_EditorScene->GetPhysicsSpecification().Gravity), 0.05f))
		{
			if (Scene::GetActiveScene()->GetPhysicsWorld())
			{
				Scene::GetActiveScene()->GetPhysicsSpecification().Gravity = m_EditorScene->GetPhysicsSpecification().Gravity;
				Scene::GetActiveScene()->GetPhysicsWorld()->SetGravity(m_EditorScene->GetPhysicsSpecification().Gravity);
			}
		}
		ImGui::End();
	}

	void EditorLayer::UI_Project()
	{
		ImGui::Begin("Project");
		ImGui::TextUnformatted("Project Name:");
		ImGui::Separator();
		ImGui::Text(Projects::Project::GetProjectName().c_str());
		ImGui::NewLine();

		ImGui::TextUnformatted("Project Directory:");
		ImGui::Separator();
		ImGui::Text(Projects::Project::GetProjectDirectory().string().c_str());
		ImGui::NewLine();

		ImGui::TextUnformatted("Starting Scene:");
		ImGui::Separator();
		if (ImGui::BeginCombo("##Select Starting Scene", Projects::Project::GetStartScenePath(false).string().c_str()))
		{

			for (auto& [uuid, asset] : Assets::AssetManager::GetSceneRegistry())
			{
				if (ImGui::Selectable(asset.Data.IntermediateLocation.string().c_str()))
				{
					Projects::Project::SetStartingScene(uuid, asset.Data.IntermediateLocation);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::NewLine();

		ImGui::TextUnformatted("Default Game Fullscreen:");
		ImGui::Separator();
		bool projectFullscreen = Projects::Project::GetIsFullscreen();
		if (ImGui::Checkbox("Default Game Fullscreen", &projectFullscreen))
		{
			Projects::Project::SetIsFullscreen(projectFullscreen);
		}
		ImGui::NewLine();

		ImGui::TextUnformatted("Target Application Resolution:");
		ImGui::Separator();
		if (ImGui::BeginCombo("##", Utility::ScreenResolutionToString(Projects::Project::GetTargetResolution()).c_str()))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.2f, 0.6f, 1.0f));
			ImGui::Text("Aspect Ratio: 1:1 (Box)");
			ImGui::PopStyleColor();
			ImGui::Separator();
			if (ImGui::Selectable("800x800"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R800x800);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("400x400"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R400x400);
				ImGui::SetItemDefaultFocus();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.2f, 0.6f, 1.0f));
			ImGui::Text("Aspect Ratio: 16:9 (Widescreen)");
			ImGui::PopStyleColor();
			ImGui::Separator();
			if (ImGui::Selectable("1920x1080"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1920x1080);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1600x900"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1600x900);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1366x768"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1366x768);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1280x720"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1280x720);
				ImGui::SetItemDefaultFocus();
			}

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.2f, 0.6f, 1.0f));
			ImGui::Text("Aspect Ratio: 4:3 (Fullscreen)");
			ImGui::PopStyleColor();
			ImGui::Separator();
			if (ImGui::Selectable("1600x1200"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1600x1200);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1280x960"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1280x960);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1152x864"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1152x864);
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable("1024x768"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::R1024x768);
				ImGui::SetItemDefaultFocus();
			}

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.97f, 0.2f, 0.6f, 1.0f));
			ImGui::Text("Aspect Ratio: Automatic (Based on Device Used)");
			ImGui::PopStyleColor();
			ImGui::Separator();
			if (ImGui::Selectable("Match Device"))
			{
				Projects::Project::SetTargetResolution(Projects::ScreenResolutionOptions::MatchDevice);
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::NewLine();


		ImGui::End();
	}

	void EditorLayer::UI_Stats()
	{
		ImGui::Begin("Stats");

		std::string name = "None";
		if (*Scene::GetActiveScene()->GetHoveredEntity())
		{
			name = Scene::GetActiveScene()->GetHoveredEntity()->GetComponent<TagComponent>().Tag;
		}
		ImGui::Text("Hovered Entity: %s", name.c_str());

		auto stats = Renderer::GetStats();
		ImGui::Text("Renderer Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		/*ImGui::Text("Quads: %d", stats.VertexCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());*/

		ImGui::End();
	}

	void EditorLayer::UI_Viewport()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoDecoration;
		ImGui::Begin("Viewport", 0, window_flags);
		auto viewportOffset = ImGui::GetWindowPos();
		static Math::uvec2 oldViewportSize = m_ViewportSize;
		Math::vec2 localViewportBounds[2];

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		Application::GetCurrentApp().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		Math::uvec2 aspectRatio = Utility::ScreenResolutionToAspectRatio(Projects::Project::GetTargetResolution());
		if (aspectRatio.x > aspectRatio.y && ((viewportPanelSize.x / aspectRatio.x) * aspectRatio.y) < viewportPanelSize.y)
		{
			m_ViewportSize = { viewportPanelSize.x, (viewportPanelSize.x / aspectRatio.x) * aspectRatio.y };
		}
		else
		{
			m_ViewportSize = { (viewportPanelSize.y / aspectRatio.y) * aspectRatio.x, viewportPanelSize.y };
		}

		localViewportBounds[0] = {  (viewportPanelSize.x - static_cast<float>(m_ViewportSize.x)) * 0.5f, (viewportPanelSize.y - static_cast<float>(m_ViewportSize.y)) * 0.5f};
		localViewportBounds[1] = { m_ViewportBounds[0].x + static_cast<float>(m_ViewportSize.x),  m_ViewportBounds[0].y + static_cast<float>(m_ViewportSize.y) };
		m_ViewportBounds[0] = {  localViewportBounds[0].x + viewportOffset.x, localViewportBounds[0].y + viewportOffset.y};
		m_ViewportBounds[1] = { m_ViewportBounds[0].x + static_cast<float>(m_ViewportSize.x), m_ViewportBounds[0].y + static_cast<float>(m_ViewportSize.y)};
		ImGui::SetCursorPos(ImVec2(localViewportBounds[0].x, localViewportBounds[0].y));
		uint64_t textureID = m_ViewportFramebuffer->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ static_cast<float>(m_ViewportSize.x), static_cast<float>(m_ViewportSize.y) }, ImVec2{ 0, 1 },
			ImVec2{ 1, 0 });

		if (oldViewportSize != m_ViewportSize)
		{
			Scene::GetActiveScene()->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_SCENE"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				OpenScene(path);
			}
			ImGui::EndDragDropTarget();
		}

		if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)

		{
			// Gizmos
			Entity selectedEntity = *Scene::GetActiveScene()->GetSelectedEntity();
			if (selectedEntity && m_GizmoType != -1)
			{
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist();
				ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
					m_ViewportBounds[1].x - m_ViewportBounds[0].x,
					m_ViewportBounds[1].y - m_ViewportBounds[0].y);

				// Editor Camera
				const Math::mat4& cameraProjection = m_EditorCamera.GetProjection();
				Math::mat4 cameraView = m_EditorCamera.GetViewMatrix();

				// Entity Transform
				auto& tc = selectedEntity.GetComponent<TransformComponent>();
				Math::mat4 transform = tc.GetTransform();

				// Snapping
				bool snap = InputPolling::IsKeyPressed(Key::LeftControl);
				float snapValue = 0.5f;
				if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) { snapValue = 45.0f; }

				float snapValues[3] = { snapValue, snapValue, snapValue };

				ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
					(ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
					nullptr, snap ? snapValues : nullptr);
				if (ImGuizmo::IsUsing())
				{
					Math::vec3 translation, rotation, scale;
					Math::DecomposeTransform(transform, translation, rotation, scale);

					Math::vec3 deltaRotation = rotation - tc.Rotation;
					tc.Translation = translation;
					tc.Rotation += deltaRotation;
					tc.Scale = scale;
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}


	static void DisplayWidgetSpecificInfo(Ref<UI::Widget> widget, int32_t selectedWidget)
	{
		switch (widget->WidgetType)
		{
		case UI::WidgetTypes::TextWidget:
			{
				UI::TextWidget* textWidget = (UI::TextWidget*)widget.get();

			char buffer[256] = {};
			strncpy_s(buffer, textWidget->Text.c_str(), sizeof(buffer));
			if (ImGui::Button(textWidget->Text.c_str()))
			{
				ImGui::OpenPopup((std::string("##Input Text") + std::to_string(selectedWidget)).c_str());
			}
			ImGui::SameLine();
			ImGui::Text("Widget Text");

			if (ImGui::BeginPopup((std::string("##Input Text") + std::to_string(selectedWidget)).c_str()))
			{
				ImGui::InputText((std::string("##Input Text") + std::to_string(selectedWidget)).c_str(), buffer, sizeof(buffer));
				if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
				{
					textWidget->SetText(std::string(buffer));
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::DragFloat((std::string("Text Size##") + std::to_string(selectedWidget)).c_str(), &textWidget->TextSize,
				0.01f, 0.0f, 5.0f);
			break;
			}
		default:
			KG_CORE_ASSERT(false, "Invalid Widget Type Presented!");
			break;
		}
	}

	void EditorLayer::UI_UserInterface()
	{
		static int32_t windowToDelete { -1 };
		static int32_t widgetToDelete { -1 };
		static int32_t windowsToAddWidget { -1 };
		static UI::WidgetTypes widgetTypeToAdd {UI::WidgetTypes::None };
		static uint32_t windowToAdd {0};
		int32_t windowIteration{ 1 };
		static int32_t selectedWindow { -1 };
		static int32_t selectedWidget { -1 };

		ImGui::Begin("User Interface Editor");

		Assets::AssetHandle currentUIHandle = UI::RuntimeEngine::GetCurrentUIHandle();
		if (ImGui::BeginCombo("##Select User Interface", static_cast<bool>(currentUIHandle) ? Assets::AssetManager::GetUIObjectLocation(currentUIHandle).string().c_str() : "None"))
		{
			if (ImGui::Selectable("None"))
			{
				UI::RuntimeEngine::ClearUIEngine();
				windowToDelete = -1;
				widgetToDelete = -1;
				windowsToAddWidget = -1;
				widgetTypeToAdd = UI::WidgetTypes::None;
				windowToAdd = 0;
				windowIteration = 1;
				selectedWindow = -1;
				selectedWidget = -1;
			}
			for (auto& [uuid, asset] : Assets::AssetManager::GetUIObjectRegistry())
			{
				if (ImGui::Selectable(asset.Data.IntermediateLocation.string().c_str()))
				{
					UI::RuntimeEngine::ClearUIEngine();
					windowToDelete = -1;
					widgetToDelete = -1;
					windowsToAddWidget = -1;
					widgetTypeToAdd = UI::WidgetTypes::None;
					windowToAdd = 0;
					windowIteration = 1;
					selectedWindow = -1;
					selectedWidget = -1;

					UI::RuntimeEngine::LoadUIObject(Assets::AssetManager::GetUIObject(uuid), uuid);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SameLine();

		if (ImGui::Button("Save Current User Interface"))
		{
			if (UI::RuntimeEngine::SaveCurrentUIIntoUIObject())
			{
				Assets::AssetManager::SaveUIObject(UI::RuntimeEngine::GetCurrentUIHandle(), UI::RuntimeEngine::GetCurrentUIObject());
			}
		}
		ImGui::SameLine();

		if (ImGui::Button("Create New User Interface"))
		{
			ImGui::OpenPopup("Create New User Interface");
		}

		if (ImGui::BeginPopup("Create New User Interface"))
		{
			static char buffer[256];
			memset(buffer, 0, 256);
			ImGui::InputText("New User Interface Name", buffer, sizeof(buffer));
			if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))
			{
				UI::RuntimeEngine::ClearUIEngine();
				windowToDelete = -1;
				widgetToDelete = -1;
				windowsToAddWidget = -1;
				widgetTypeToAdd = UI::WidgetTypes::None;
				windowToAdd = 0;
				windowIteration = 1;
				selectedWindow = -1;
				selectedWidget = -1;

				Assets::AssetHandle newHandle = Assets::AssetManager::CreateNewUIObject(std::string(buffer));
				UI::RuntimeEngine::LoadUIObject(Assets::AssetManager::GetUIObject(newHandle), newHandle);

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::BeginTable("All Windows", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			for (auto& window : UI::RuntimeEngine::GetAllWindows())
			{
				ImGui::AlignTextToFramePadding();
				ImGuiTreeNodeFlags windowFlags = ((selectedWindow == windowIteration && selectedWidget == -1) ? ImGuiTreeNodeFlags_Selected : 0) |
					ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				bool node_open = ImGui::TreeNodeEx(("Window " + std::to_string(windowIteration)).c_str(), windowFlags);
				if (ImGui::IsItemClicked())
				{
					selectedWindow = windowIteration;
					selectedWidget = -1;
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup((std::string("RightClickOptions##UIWindow") + std::to_string(windowIteration)).c_str());
				}

				if (ImGui::BeginPopup((std::string("RightClickOptions##UIWindow") + std::to_string(windowIteration)).c_str()))
				{
					if (ImGui::Selectable((std::string("Add Text Widget##") + std::to_string(windowIteration)).c_str()))
					{
						windowsToAddWidget = windowIteration;
						widgetTypeToAdd = UI::WidgetTypes::TextWidget;
					}

					if (ImGui::Selectable((std::string("Delete Window##") + std::to_string(windowIteration)).c_str()))
					{
						windowToDelete = windowIteration;
					}
					ImGui::EndPopup();
				}

				if (node_open)
				{
					uint32_t widgetIteration{ 1 };
					
					for (auto& widget : window.Widgets)
					{
						ImGuiTreeNodeFlags widgetFlags = ((selectedWidget == widgetIteration) ? ImGuiTreeNodeFlags_Selected : 0) |
							ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
						bool widgetOpened = ImGui::TreeNodeEx(("Widget " + std::to_string(widgetIteration) + std::string(" ##") + std::to_string(windowIteration)).c_str(), widgetFlags);
						if (ImGui::IsItemClicked())
						{
							selectedWindow = windowIteration;
							selectedWidget = widgetIteration;
						}

						if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
						{
							ImGui::OpenPopup((std::string("RightClickOptions##UIWidget") + std::to_string(widgetIteration)).c_str());
						}

						if (ImGui::BeginPopup((std::string("RightClickOptions##UIWidget") + std::to_string(widgetIteration)).c_str()))
						{
							if (ImGui::Selectable((std::string("Delete Widget##") + std::to_string(widgetIteration)).c_str()))
							{
								selectedWindow = windowIteration;
								widgetToDelete = widgetIteration;
							}
							ImGui::EndPopup();
						}

						if (widgetOpened) { ImGui::TreePop(); }
						widgetIteration++;
					}

					ImGui::TreePop();
				}

				windowIteration++;
			}

			if (ImGui::Button("Add Window"))
			{
				windowToAdd++;
			}

			ImGui::TableSetColumnIndex(1);
			if (selectedWindow != -1)
			{
				if (selectedWidget != -1)
				{
					auto& widget = UI::RuntimeEngine::GetAllWindows().at(selectedWindow - 1).Widgets.at(selectedWidget - 1);
					ImGui::Text(("Widget " + std::to_string(selectedWidget)).c_str());
					ImGui::Separator();
					ImGui::DragFloat2((std::string("Widget Location##") + std::to_string(selectedWidget)).c_str(),
					                  glm::value_ptr(widget->WindowPosition), 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat2((std::string("Widget Size##") + std::to_string(selectedWidget)).c_str(), glm::value_ptr(widget->Size), 0.01f, 0.0f, 1.0f);
					ImGui::ColorEdit4("Background Color", glm::value_ptr(widget->BackgroundColor));
					DisplayWidgetSpecificInfo(widget, selectedWidget);
				}
				else
				{
					auto& window = UI::RuntimeEngine::GetAllWindows().at(selectedWindow - 1);
					ImGui::Text(("Window " + std::to_string(selectedWindow)).c_str());
					ImGui::Separator();
					bool windowDisplayed = window.GetWindowDisplayed();
					if (ImGui::Checkbox((std::string("Display Window##") + std::to_string(selectedWindow)).c_str(), &windowDisplayed))
					{
						if (windowDisplayed)
						{
							window.DisplayWindow();
						}
						if (!windowDisplayed)
						{
							window.HideWindow();
						}
					}
					ImGui::DragFloat3((std::string("Window Location##") + std::to_string(selectedWindow)).c_str(), glm::value_ptr(window.ScreenPosition), 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat2((std::string("Window Size##") + std::to_string(selectedWindow)).c_str(), glm::value_ptr(window.Size), 0.01f, 0.0f, 1.0f);
					ImGui::ColorEdit4("Background Color", glm::value_ptr(window.BackgroundColor));
				}
				
			}

			ImGui::EndTable();
		}


		ImGui::End();

		if (windowToDelete != -1)
		{
			UI::RuntimeEngine::DeleteWindow(static_cast<uint32_t>(windowToDelete - 1));
			windowToDelete = -1;

			selectedWindow = -1;
			selectedWidget = -1;
		}

		if (widgetToDelete != -1)
		{
			auto& window = UI::RuntimeEngine::GetAllWindows().at(selectedWindow - 1);
			window.DeleteWidget(widgetToDelete - 1);

			widgetToDelete = -1;
			selectedWidget = -1;

		}

		if (windowsToAddWidget != -1)
		{
			auto& windows = UI::RuntimeEngine::GetAllWindows();
			switch (widgetTypeToAdd)
			{
				case UI::WidgetTypes::TextWidget:
				{
					UI::RuntimeEngine::GetAllWindows().at(windowsToAddWidget - 1).AddTextWidget(CreateRef<UI::TextWidget>());
					break;
				}
				default:
				{
					KG_CORE_ASSERT(false, "Invalid widgetTypeToAdd value");
					break;
				}
			}
			
			windowsToAddWidget = -1;
		}

		if (windowToAdd != 0)
		{
			UI::Window window1 {};
			window1.Size = Math::vec2(0.4f, 0.4f);
			window1.ScreenPosition = Math::vec3(0.3f, 0.3f, 0.0f);
			UI::RuntimeEngine::AddWindow(window1);
			windowToAdd = 0;
		}
	}
	

	void EditorLayer::OnEvent(Events::Event& event)
	{
		Events::EventDispatcher dispatcher(event);
		if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
		{
			m_EditorCamera.OnEvent(event);
			dispatcher.Dispatch<Events::KeyPressedEvent>(KG_BIND_EVENT_FN(EditorLayer::OnKeyPressedEditor));
			dispatcher.Dispatch<Events::MouseButtonPressedEvent>(KG_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
		}
		if (m_SceneState == SceneState::Play && m_IsPaused) { dispatcher.Dispatch<Events::MouseButtonPressedEvent>(KG_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed)); }
		dispatcher.Dispatch<Events::PhysicsCollisionEvent>(KG_BIND_EVENT_FN(EditorLayer::OnPhysicsCollision));
		if (m_SceneState == SceneState::Play) { dispatcher.Dispatch<Events::KeyPressedEvent>(KG_BIND_EVENT_FN(EditorLayer::OnKeyPressedRuntime)); }
	}

	bool EditorLayer::OnKeyPressedRuntime(Events::KeyPressedEvent event)
	{
		Script::ScriptEngine::OnKeyPressed(event);
		return false;
	}

	bool EditorLayer::OnKeyPressedEditor(Events::KeyPressedEvent event)
	{
		if (event.IsRepeat()) { return false; }

		bool control = InputPolling::IsKeyPressed(Key::LeftControl) || InputPolling::IsKeyPressed(Key::RightControl);
		bool shift = InputPolling::IsKeyPressed(Key::LeftShift) || InputPolling::IsKeyPressed(Key::RightShift);
		bool alt = InputPolling::IsKeyPressed(Key::LeftAlt) || InputPolling::IsKeyPressed(Key::RightAlt);

		switch (event.GetKeyCode())
		{
		case Key::Escape:
		{
			m_SceneHierarchyPanel->SetSelectedEntity({});
			break;
		}
		case Key::Tab:
		{
			m_EditorCamera.ToggleMovementType();
			break;
		}

		case Key::N:
		{
			if (control) { NewScene(); }
			break;
		}
		case Key::O:
		{
			if (control) { OpenProject(); }
			break;
		}
		case Key::S:
		{
			if (control)
			{
				SaveScene();
				SaveProject();
			}
			break;
		}

		// Scene Commands

		case Key::D:
		{
			if (control) { OnDuplicateEntity(); }
			break;
		}

		// Gizmos
		case Key::Q:
			{
			if (!ImGuizmo::IsUsing() && !alt) { m_GizmoType = -1; }
				break;
			}
		case Key::W:
		{
			if (!ImGuizmo::IsUsing() && !alt) { m_GizmoType = ImGuizmo::OPERATION::TRANSLATE; }
			break;
		}
		case Key::E:
		{
			if (!ImGuizmo::IsUsing() && !alt) { m_GizmoType = ImGuizmo::OPERATION::ROTATE; }
			break;
		}
		case Key::R:
		{
			if (control)
			{
				if (m_SceneState != SceneState::Edit) { OnSceneStop(); }
				Script::ScriptEngine::ReloadAssembly();
			}

			if (!ImGuizmo::IsUsing()) { m_GizmoType = ImGuizmo::OPERATION::SCALE; }
			break;
		}
		case Key::Delete:
			{
			if (Application::GetCurrentApp().GetImGuiLayer()->GetActiveWidgetID() == 0)
			{
				Entity selectedEntity = *Scene::GetActiveScene()->GetSelectedEntity();
				if (selectedEntity)
				{
					m_EditorScene->DestroyEntity(selectedEntity);
					m_SceneHierarchyPanel->SetSelectedEntity({});
				}
			}
				break;

			}
			default:
			{
				break;
			}
		}
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(Events::MouseButtonPressedEvent event)
	{
		if (event.GetMouseButton() == Mouse::ButtonLeft)
		{
			if (m_ViewportHovered && !ImGuizmo::IsOver() && !InputPolling::IsKeyPressed(Key::LeftAlt) && *Scene::GetActiveScene()->GetHoveredEntity())
			{
				m_SceneHierarchyPanel->SetSelectedEntity(*Scene::GetActiveScene()->GetHoveredEntity());
				// Algorithm to enable double clicking for an entity!
				static float previousTime{ 0.0f };
				static Entity previousEntity {};
				float currentTime = Utility::Time::GetTime();
				if (std::fabs(currentTime - previousTime) < 0.2f && *Scene::GetActiveScene()->GetHoveredEntity() == previousEntity)
				{
					auto& transformComponent = Scene::GetActiveScene()->GetHoveredEntity()->GetComponent<TransformComponent>();
					m_EditorCamera.SetFocalPoint(transformComponent.Translation);
					m_EditorCamera.SetDistance(std::max({ transformComponent.Scale.x, transformComponent.Scale.y, transformComponent.Scale.z }) * 2.5f);
					m_EditorCamera.SetMovementType(EditorCamera::MovementType::ModelView);
				}
				previousTime = currentTime;
				previousEntity = *Scene::GetActiveScene()->GetHoveredEntity();

			}
		}
		return false;
	}


	bool EditorLayer::OnPhysicsCollision(Events::PhysicsCollisionEvent event)
	{
		Script::ScriptEngine::OnPhysicsCollision(event);
		return false;
	}

	static RendererInputSpec s_CircleInputSpec{};
	static RendererInputSpec s_LineInputSpec{};
	static Math::vec4 s_RectangleVertexPositions[4]
	{
		{ -0.5f, -0.5f, 0.0f, 1.0f },
			{ 0.5f, -0.5f, 0.0f, 1.0f },
			{ 0.5f, 0.5f, 0.0f, 1.0f },
			{ -0.5f, 0.5f, 0.0f, 1.0f }
	};
	static Math::vec4 s_CubeVertexPositions[8]
	{
		{ -0.5f, -0.5f, 0.5f, 1.0f },		// 0
			{ 0.5f, -0.5f, 0.5f, 1.0f },		// 1
			{ 0.5f, 0.5f, 0.5f, 1.0f },		// 2
			{ -0.5f, 0.5f, 0.5f, 1.0f },		// 3
		{ -0.5f, -0.5f, -0.5f, 1.0f },		// 4
			{ 0.5f, -0.5f, -0.5f, 1.0f },	// 5
			{ 0.5f, 0.5f, -0.5f, 1.0f },		// 6
		{ -0.5f, 0.5f, -0.5f, 1.0f },		// 7
	};

	static Math::uvec2 s_CubeIndices[12]
	{
		{0,1},
		{1,2},
		{2,3},
		{3,0},
		{4,5},
		{5,6},
		{6,7},
		{7,4},
		{0,4},
		{1,5},
		{2,6},
		{3,7}
	};

	static Math::vec4 s_FrustrumVertexPositions[8]
	{
		{ -1.0f, -1.0f, 1.0f, 1.0f },		// 0
			{ 1.0f, -1.0f, 1.0f, 1.0f },		// 1
			{ 1.0f, 1.0f, 1.0f, 1.0f },		// 2
			{ -1.0f, 1.0f, 1.0f, 1.0f },		// 3
		{ -1.0f, -1.0f, -1.0f, 1.0f },		// 4
			{ 1.0f, -1.0f, -1.0f, 1.0f },	// 5
			{ 1.0f, 1.0f, -1.0f, 1.0f },		// 6
		{ -1.0f, 1.0f, -1.0f, 1.0f },		// 7
	};

	static Math::uvec2 s_FrustrumIndices[16]
	{
		{0,1},
		{1,2},
		{2,3},
		{3,0},
		{4,5},
		{5,6},
		{6,7},
		{7,4},
		{0,4},
		{1,5},
		{2,6},
		{3,7},
		{4, 8},
		{5, 8},
		{6, 8},
		{7,8}
	};

	static Ref<std::vector<Math::vec3>> s_OutputVector = CreateRef<std::vector<Math::vec3>>();

	void EditorLayer::InitializeOverlayData()
	{

		// Set up Line Input Specifications for Overlay Calls
		{
			ShaderSpecification lineShaderSpec {ColorInputType::FlatColor, TextureInputType::None, false, true, false, RenderingType::DrawLine, false};
			auto [uuid, localShader] = Assets::AssetManager::GetShader(lineShaderSpec);
			Buffer localBuffer{ localShader->GetInputLayout().GetStride() };

			Shader::SetDataAtInputLocation<Math::vec4>({ 0.0f, 1.0f, 0.0f, 1.0f }, "a_Color", localBuffer, localShader);

			ShapeComponent* lineShapeComponent = new ShapeComponent();
			lineShapeComponent->CurrentShape = ShapeTypes::None;
			lineShapeComponent->Vertices = nullptr;

			s_LineInputSpec.Shader = localShader;
			s_LineInputSpec.Buffer = localBuffer;
			s_LineInputSpec.ShapeComponent = lineShapeComponent;
		}


		// Set up Circle Input Specification for Overlay Calls

		{
			ShaderSpecification shaderSpec {ColorInputType::FlatColor,  TextureInputType::None, true, true, false, RenderingType::DrawIndex, false};
			auto [uuid, localShader] = Assets::AssetManager::GetShader(shaderSpec);
			Buffer localBuffer{ localShader->GetInputLayout().GetStride() };

			Shader::SetDataAtInputLocation<Math::vec4>({ 0.0f, 1.0f, 0.0f, 1.0f }, "a_Color", localBuffer, localShader);
			Shader::SetDataAtInputLocation<float>(0.05f, "a_Thickness", localBuffer, localShader);
			Shader::SetDataAtInputLocation<float>(0.005f, "a_Fade", localBuffer, localShader);

			ShapeComponent* shapeComp = new ShapeComponent();
			shapeComp->CurrentShape = ShapeTypes::Quad;
			shapeComp->Vertices = CreateRef<std::vector<Math::vec3>>(Shape::s_Quad.GetIndexVertices());
			shapeComp->Indices = CreateRef<std::vector<uint32_t>>(Shape::s_Quad.GetIndices());

			s_CircleInputSpec.Shader = localShader;
			s_CircleInputSpec.Buffer = localBuffer;
			s_CircleInputSpec.ShapeComponent = shapeComp;
		}

		// TODO: Shape Components and Buffers are memory leaks!
	}

	void EditorLayer::OnOverlayRender()
	{
		if (m_SceneState == SceneState::Play)
		{
			Entity camera = Scene::GetActiveScene()->GetPrimaryCameraEntity();
			if (!camera) { return; }
			Renderer::BeginScene(camera.GetComponent<CameraComponent>().Camera, glm::inverse(camera.GetComponent<TransformComponent>().GetTransform()));
		}
		else
		{
			Renderer::BeginScene(m_EditorCamera);
		}

		if (m_ShowPhysicsColliders)
		{
			// Circle Colliders
			{
				auto view = Scene::GetActiveScene()->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

					Math::vec3 translation = tc.Translation + Math::vec3(cc2d.Offset.x, cc2d.Offset.y, 0.001f);

					Math::vec3 scale = tc.Scale * Math::vec3(cc2d.Radius * 2.0f);

					Math::mat4 transform = glm::translate(Math::mat4(1.0f), translation)
						* glm::scale(Math::mat4(1.0f), scale);

					s_CircleInputSpec.TransformMatrix = transform;
					Renderer::SubmitDataToRenderer(s_CircleInputSpec);
				}
			}
			// Box Colliders
			{
				auto view = Scene::GetActiveScene()->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
				for (auto entity : view)
				{
					auto [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

					Math::vec3 translation = tc.Translation + Math::vec3(bc2d.Offset.x, bc2d.Offset.y, 0.001f);
					Math::vec3 scale = tc.Scale * Math::vec3(bc2d.Size * 2.0f, 1.0f);

					Math::mat4 transform = glm::translate(Math::mat4(1.0f), translation)
						* glm::rotate(Math::mat4(1.0f), tc.Rotation.z, Math::vec3(0.0f, 0.0f, 1.0f))
						* glm::scale(Math::mat4(1.0f), scale);

					static Math::vec4 boxColliderColor {0.0f, 1.0f, 0.0f, 1.0f};
					Shader::SetDataAtInputLocation<Math::vec4>(boxColliderColor, "a_Color", s_LineInputSpec.Buffer, s_LineInputSpec.Shader);

					Math::vec3 lineVertices[4];
					for (size_t i = 0; i < 4; i++)
					{
						lineVertices[i] = transform * s_RectangleVertexPositions[i];
					}
					s_OutputVector->clear();
					s_OutputVector->push_back(lineVertices[0]);
					s_OutputVector->push_back(lineVertices[1]);
					s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
					Renderer::SubmitDataToRenderer(s_LineInputSpec);
					s_OutputVector->clear();
					s_OutputVector->push_back(lineVertices[1]);
					s_OutputVector->push_back(lineVertices[2]);
					s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
					Renderer::SubmitDataToRenderer(s_LineInputSpec);
					s_OutputVector->clear();
					s_OutputVector->push_back(lineVertices[2]);
					s_OutputVector->push_back(lineVertices[3]);
					s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
					Renderer::SubmitDataToRenderer(s_LineInputSpec);
					s_OutputVector->clear();
					s_OutputVector->push_back(lineVertices[3]);
					s_OutputVector->push_back(lineVertices[0]);
					s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
					Renderer::SubmitDataToRenderer(s_LineInputSpec);
				}
			}
		}

		if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate || (m_SceneState == SceneState::Play && m_IsPaused))
		{
			// Draw selected entity outline 
			if (Entity selectedEntity = *Scene::GetActiveScene()->GetSelectedEntity()) {
				TransformComponent transform = selectedEntity.GetComponent<TransformComponent>();
				static Math::vec4 selectionColor {1.0f, 0.5f, 0.0f, 1.0f};
				Shader::SetDataAtInputLocation<Math::vec4>(selectionColor, "a_Color", s_LineInputSpec.Buffer, s_LineInputSpec.Shader);

				Math::vec3 lineVertices[8];

				for (size_t i = 0; i < 8; i++)
				{
					lineVertices[i] = transform.GetTransform() * s_CubeVertexPositions[i];
				}

				for (auto& indices : s_CubeIndices)
				{
					s_OutputVector->clear();
					s_OutputVector->push_back(lineVertices[indices.x]);
					s_OutputVector->push_back(lineVertices[indices.y]);
					s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
					Renderer::SubmitDataToRenderer(s_LineInputSpec);
				}

				if (selectedEntity.HasComponent<CameraComponent>() && m_ShowCameraFrustrums)
				{
					DrawFrustrum(selectedEntity);
				}
			}
		}

		Renderer::EndScene();
	}

	void EditorLayer::DrawFrustrum(Entity& entity)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		auto& camera = entity.GetComponent<CameraComponent>();
		float windowWidth = (float)Application::GetCurrentApp().GetWindow().GetWidth();
		float windowHeight = (float)Application::GetCurrentApp().GetWindow().GetHeight();
		Math::vec4 viewport = { 0.0f, 0.0f, windowWidth, windowHeight };
		auto cameraProjectionType = camera.Camera.GetProjectionType();
		Math::vec4 selectionColor { 0.5f, 0.3f, 0.85f, 1.0f };
		Shader::SetDataAtInputLocation<Math::vec4>(selectionColor, "a_Color", s_LineInputSpec.Buffer, s_LineInputSpec.Shader);

		Math::vec3 lineVertices[9];

		for (size_t i = 0; i < 8; i++)
		{
			Math::vec4 vertexPosition = s_FrustrumVertexPositions[i];
			Math::vec4 localSpaceCoordinates = glm::inverse(camera.Camera.GetProjection()) * s_FrustrumVertexPositions[i];
			localSpaceCoordinates = localSpaceCoordinates / localSpaceCoordinates.w; // Perspective Division
			lineVertices[i] = transform.GetTranslation() * transform.GetRotation() * localSpaceCoordinates;
			
		}

		lineVertices[8] = (transform.GetTransform() * Math::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		bool colorChanged = false;
		uint32_t iteration = 0;
		for (auto& indices : s_FrustrumIndices)
		{
			if (iteration >= 12 && !colorChanged)
			{
				selectionColor = { 0.3f, 0.1f, 0.8f, 1.0f };
				Shader::SetDataAtInputLocation<Math::vec4>(selectionColor, "a_Color", s_LineInputSpec.Buffer, s_LineInputSpec.Shader);
				colorChanged = true;
			}
			s_OutputVector->clear();
			s_OutputVector->push_back(lineVertices[indices.x]);
			s_OutputVector->push_back(lineVertices[indices.y]);
			s_LineInputSpec.ShapeComponent->Vertices = s_OutputVector;
			Renderer::SubmitDataToRenderer(s_LineInputSpec);
			iteration++;
		}

	}

	void EditorLayer::NewProject()
	{
		Assets::AssetManager::NewProject();
	}

	bool EditorLayer::OpenProject()
	{
		
		*Scene::GetActiveScene()->GetHoveredEntity() = {};

		std::filesystem::path filepath = Utility::FileDialogs::OpenFile("Kargono Project (*.kproj)\0*.kproj\0");
		if (filepath.empty()) { return false; }

		OpenProject(filepath);
		return true;
	}

	void EditorLayer::OpenProject(const std::filesystem::path& path)
	{
		if (Assets::AssetManager::OpenProject(path))
		{
			auto startSceneHandle = Projects::Project::GetStartSceneHandle();

			if (Script::ScriptEngine::AppDomainExists()){ Script::ScriptEngine::ReloadAssembly(); }
			else { Script::ScriptEngine::InitialAssemblyLoad(); }
			if (m_EditorScene)
			{
				auto view = m_EditorScene->GetAllEntitiesWith<AudioComponent>();
				for (auto& entity : view)
				{
					Entity e = { entity, m_EditorScene.get() };
					auto& audioComponent = e.GetComponent<AudioComponent>();
					audioComponent.Audio.reset();
				}
				m_EditorScene->DestroyAllEntities();
			}
			Assets::AssetManager::ClearAll();
			Assets::AssetManager::DeserializeAll();

			OpenScene(startSceneHandle);

			m_ContentBrowserPanel = CreateScope<ContentBrowserPanel>();

		}
	}

	void EditorLayer::SaveProject()
	{
		Assets::AssetManager::SaveActiveProject((Projects::Project::GetProjectDirectory() / Projects::Project::GetProjectName()).replace_extension(".kproj"));
	}

	void EditorLayer::NewScene()
	{
		std::filesystem::path filepath = Utility::FileDialogs::SaveFile("Kargono Scene (*.kgscene)\0*.kgscene\0");
		if (filepath.empty()) { return; }
		if (Assets::AssetManager::CheckSceneExists(filepath.stem().string()))
		{
			KG_WARN("Attempt to create scene with duplicate name!");
			return;
		}
		m_EditorSceneHandle = Assets::AssetManager::CreateNewScene(filepath.stem().string());

		*Scene::GetActiveScene()->GetHoveredEntity() = {};
		m_EditorScene = Assets::AssetManager::GetScene(m_EditorSceneHandle);
		Scene::SetActiveScene(m_EditorScene);
	}

	void EditorLayer::OpenScene()
	{
		std::filesystem::path filepath = Utility::FileDialogs::OpenFile("Kargono Scene (*.kgscene)\0*.kgscene\0");
		if (!filepath.empty())
		{
			OpenScene(filepath);
		}
	}

	void EditorLayer::OpenScene(const std::filesystem::path& path)
	{
		if (m_SceneState != SceneState::Edit)
		{
			OnSceneStop();
		}

		if (path.extension().string() != ".kgscene")
		{
			KG_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}
		auto [sceneHandle, newScene] = Assets::AssetManager::GetScene(path);

		m_EditorScene = newScene;
		Scene::SetActiveScene(m_EditorScene);
		m_EditorSceneHandle = sceneHandle;

	}

	void EditorLayer::OpenScene(Assets::AssetHandle sceneHandle)
	{
		if (m_SceneState != SceneState::Edit)
		{
			OnSceneStop();
		}

		Ref<Scene> newScene = Assets::AssetManager::GetScene(sceneHandle);
		if (!newScene) { newScene = CreateRef<Scene>(); }

		m_EditorScene = newScene;
		Scene::SetActiveScene(m_EditorScene);
		m_EditorSceneHandle = sceneHandle;
	}

	void EditorLayer::SaveScene()
	{
		SerializeScene(m_EditorScene);
	}

	void EditorLayer::SerializeScene(Ref<Scene> scene)
	{
		Assets::AssetManager::SaveScene(m_EditorSceneHandle, scene);
		
	}

	void EditorLayer::OnScenePlay()
	{
		*Scene::GetActiveScene()->GetHoveredEntity() = {};
		if (m_SceneState == SceneState::Simulate) { OnSceneStop(); }

		m_SceneState = SceneState::Play;
		Scene::SetActiveScene(Scene::Copy(m_EditorScene));
		Scene::GetActiveScene()->OnRuntimeStart();
	}

	void EditorLayer::OnSceneSimulate()
	{
		*Scene::GetActiveScene()->GetHoveredEntity() = {};
		if (m_SceneState == SceneState::Play) { OnSceneStop(); }

		m_SceneState = SceneState::Simulate;
		Scene::SetActiveScene(Scene::Copy(m_EditorScene));
		Scene::GetActiveScene()->OnSimulationStart();
	}
	void EditorLayer::OnSceneStop()
	{
		*Scene::GetActiveScene()->GetHoveredEntity() = {};
		KG_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate, "Unknown Scene State Given to OnSceneStop")

		if (m_SceneState == SceneState::Play) { Scene::GetActiveScene()->OnRuntimeStop(); }
		else if (m_SceneState == SceneState::Simulate) { Scene::GetActiveScene()->OnSimulationStop(); }

		m_SceneState = SceneState::Edit;

		Scene::GetActiveScene()->DestroyAllEntities();
		Scene::SetActiveScene(m_EditorScene);
		Audio::AudioEngine::StopAllAudio();
	}

	void EditorLayer::OnScenePause()
	{
		if (m_SceneState == SceneState::Edit) { return; }

		m_IsPaused = true;
	}

	void EditorLayer::OnDuplicateEntity()
	{
		if (m_SceneState != SceneState::Edit)
			return;

		Entity selectedEntity = *Scene::GetActiveScene()->GetSelectedEntity();
		//KG_CORE_WARN("Trying to duplicate Entity: {} ({})", selectedEntity.GetName(), selectedEntity.GetUUID());
		if (selectedEntity)
		{
			Entity newEntity = m_EditorScene->DuplicateEntity(selectedEntity);
			m_SceneHierarchyPanel->SetSelectedEntity(newEntity);
		}
	}

}