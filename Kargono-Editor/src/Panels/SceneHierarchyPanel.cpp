#include "Panels/SceneHierarchyPanel.h"
#include "Kargono/Scene/Components.h"
#include "Kargono/Scripting/ScriptEngine.h"
#include "Kargono/Renderer/Texture.h"
#include "Kargono/UI/UI.h"
#include "Kargono/Assets/AssetManager.h"
#include "Kargono/Renderer/Shape.h"

#include <imgui.h>
#include "imgui_internal.h"
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

/* The Microsoft C++ compiler is non-compliant with the C++ standard and needs
 * the following definition to disable a security warning on std::strncpy().
 */
#ifdef _MSVC_LANG
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace Kargono
{

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> context)
	{
		SetContext(context);
	}
	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}
	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		if (m_Context)
		{

			m_Context->m_Registry.each([&](auto entityID)
				{
					Entity entity{ entityID, m_Context.get() };
					DrawEntityNode(entity);
				});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) { m_SelectionContext = {}; }

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1 | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create Empty Entity")) { m_Context->CreateEntity("Empty Entity"); }
				ImGui::EndPopup();
			}

		}
		ImGui::End();

		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}
	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}
	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity")) { entityDeleted = true; }
			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());
			if (opened)
				ImGui::TreePop();
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
			{
				m_SelectionContext = {};
			}
		}
	}

	static void DrawVec3Control(const std::string& label, glm::vec3& values, 
		float resetValue = 0.0f, float columnWidth = 100.0f)
	{

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize)) { values.x = resetValue; }
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize)) { values.y = resetValue; }
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize)) { values.z = resetValue; }
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | 
			ImGuiTreeNodeFlags_AllowItemOverlap | 
			ImGuiTreeNodeFlags_Framed | 
			ImGuiTreeNodeFlags_SpanAvailWidth | 
			ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();

			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar();
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight}))
			{
				ImGui::OpenPopup("ComponentSettings");
			}
			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove Component"))
				{
					removeComponent = true;
				}
				ImGui::EndPopup();
			}
			if (open)
			{
				//ImGui::ColorEdit4("Color", glm::value_ptr(src.Color));
				uiFunction(component);
				ImGui::TreePop();
			}
			if (removeComponent) { entity.RemoveComponent<T>(); }
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256] = {};
			strncpy_s(buffer, tag.c_str(), sizeof(buffer));

			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponent");
		}

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<ScriptComponent>("Script");
			DisplayAddComponentEntry<ShapeComponent>("Shape");
			DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
			DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [](auto& component)
			{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotation = glm::degrees(component.Rotation);
				DrawVec3Control("Rotation", rotation);
				component.Rotation = glm::radians(rotation);
				DrawVec3Control("Scale", component.Scale, 1.0f);
			});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
			{
				auto& camera = component.Camera;

				ImGui::Checkbox("Primary", &component.Primary);


				const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
				const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
				if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
						if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
						{
							currentProjectionTypeString = projectionTypeStrings[i];
							camera.SetProjectionType((SceneCamera::ProjectionType)i);
						}
						if (isSelected) { ImGui::SetItemDefaultFocus(); }
					}
					ImGui::EndCombo();
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					float verticalFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
					if (ImGui::DragFloat("Vertical FOV", &verticalFOV)) { camera.SetPerspectiveVerticalFOV(glm::radians(verticalFOV)); }
					float perspectiveNear = camera.GetPerspectiveNearClip();
					if (ImGui::DragFloat("Near Plane", &perspectiveNear)) { camera.SetPerspectiveNearClip(perspectiveNear); }
					float perspectiveFar = camera.GetPerspectiveFarClip();
					if (ImGui::DragFloat("Far Plane", &perspectiveFar)) { camera.SetPerspectiveFarClip(perspectiveFar); }
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					float orthoSize = camera.GetOrthographicSize();
					if (ImGui::DragFloat("Size", &orthoSize)) { camera.SetOrthographicSize(orthoSize); }
					float orthoNear = camera.GetOrthographicNearClip();
					if (ImGui::DragFloat("Near Plane", &orthoNear)) { camera.SetOrthographicNearClip(orthoNear); }
					float orthoFar = camera.GetOrthographicFarClip();
					if (ImGui::DragFloat("Far Plane", &orthoFar)) { camera.SetOrthographicFarClip(orthoFar); }

					ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
				}
			});

		DrawComponent<ScriptComponent>("Script", entity, [entity, this](auto& component) mutable
		{
			const bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

			static char buffer[64];
			strcpy_s(buffer, component.ClassName.c_str());

			UI::ScopedStyleColor textColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.0f), !scriptClassExists);

			if (ImGui::InputText("Class", buffer, sizeof(buffer)))
			{
				component.ClassName = buffer;
				return;
			}

			// Fields
			

			bool sceneRunning = m_Context->IsRunning();
			if (sceneRunning)
			{
				Ref<ScriptInstance> scriptInstance = ScriptEngine::GetEntityScriptInstance(entity.GetUUID());
			   if (scriptInstance)
			   {
				   const auto& fields = scriptInstance->GetScriptClass()->GetFields();
				   for (const auto& [name, field] : fields)
				   {
					   if (field.Type == ScriptFieldType::Float)
					   {
						   float data = scriptInstance->GetFieldValue<float>(name);
						   if (ImGui::DragFloat(name.c_str(), &data))
						   {
							   scriptInstance->SetFieldValue(name, data);
						   }
					   }
				   }
			   }
			}
			else
			{
				if (scriptClassExists)
				{
					Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(component.ClassName);
					const auto& fields = entityClass->GetFields();

					auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
					for (const auto& [name, field] : fields)
					{
							// Field has been set in editor
						if (entityFields.find(name) != entityFields.end())
						{
							ScriptFieldInstance& scriptField = entityFields.at(name);
							// Display control to set it maybe
							if (field.Type == ScriptFieldType::Float)
							{
								float data = scriptField.GetValue<float>();
								if (ImGui::DragFloat(name.c_str(), &data)){scriptField.SetValue<float>(data);}
							}
						}
						else
						{
							// Display control to set it maybe
							if (field.Type == ScriptFieldType::Float)
							{
								float data = 0.0f;
								if (ImGui::DragFloat(name.c_str(), &data))
								{
									ScriptFieldInstance& fieldInstance = entityFields[name];
									fieldInstance.Field = field;
									fieldInstance.SetValue(data);
								}
							}
						}
					}
				}
			}

		});

		DrawComponent<ShapeComponent>("Shape", entity, [](auto& component)
		{
			//=========================
			// Lambda Functions for Later Use (Scroll down to find main function body)
			//=========================

			// This Lambda updates the shader after a checkbox is clicked that changes the component.ShaderSpecification
			// Another function of this lambda is to ensure data inside the old buffer is transferred to the new buffer
			// if they represent the same input
			auto updateComponent = [&]()
			{
				// Get Previous Buffer and Previous Shader
				Buffer oldBuffer = component.ShaderData;
				Ref<Shader> oldShader = component.Shader;
				// Get New Shader
				auto [newShaderAssetHandle, newShader] = AssetManager::GetShader(component.ShaderSpecification);
				// Assign New Shader to Component
				component.ShaderHandle = newShaderAssetHandle;
				component.Shader = newShader;
				// Create New Buffer with New ShaderLayout Size and Set Entire Buffer to Zero
				Buffer newBuffer(newShader->GetInputLayout().GetStride() * sizeof(uint8_t));
				newBuffer.SetDataToByte(0);

				// Transfer Data from Old Buffer to New Buffer if applicable
				// This for loop checks if each element in the old shader exists in the new shader.
				// If an element does exist in both shaders, the data from the old buffer is
				// transferred to the new buffer!
				for (const auto& element : oldShader->GetInputLayout().GetElements())
				{
					if (newShader->GetInputLayout().FindElementByName(element.Name))
					{
						// Get Location of Old Data Pointer
						std::size_t oldLocation = element.Offset;
						uint8_t* oldLocationPointer = oldBuffer.As<uint8_t>(oldLocation);

						// Get Location of New Data Pointer
						uint8_t* newLocationPointer = Shader::GetInputLocation<uint8_t>(element.Name, newBuffer, newShader);

						// Get Size of Data to Transfer
						std::size_t size = element.Size;

						// Final Memory Copy
						memcpy_s(newLocationPointer, size, oldLocationPointer, size);
					}
				}

				// Assign and Zero Out New Buffer
				component.ShaderData = newBuffer;

				// Clear old buffer
				if (oldBuffer) { oldBuffer.Release(); }
			};
			//=========================
			// More Lambdas: ShaderSpecification UI Code
			//=========================

			// These lambdas provide UI for user manipulating of shader specifications and input values
			auto AddFlatColorSection = [&]()
			{
				if (ImGui::Button("Select Color Input")) { ImGui::OpenPopup("Color Type Selection"); }
				ImGui::SameLine();
				ImGui::TextUnformatted(Shader::ColorInputTypeToString(component.ShaderSpecification.ColorInput).c_str());
				if (ImGui::BeginPopup("Color Type Selection"))
				{
					if (ImGui::Selectable("No Color"))
					{
						component.ShaderSpecification.ColorInput = Shader::ColorInputType::None;
						updateComponent();
					}

					if (ImGui::Selectable("Flat Color"))
					{
						component.ShaderSpecification.ColorInput = Shader::ColorInputType::FlatColor;
						updateComponent();
						Shader::SetDataAtInputLocation<glm::vec4>({ 1.0f, 1.0f, 1.0f, 1.0f }, "a_Color", component.ShaderData, component.Shader);
					}
					if (ImGui::Selectable("Vertex Color"))
					{
						glm::vec4 transferColor {1.0f, 1.0f, 1.0f, 1.0f};
						if (component.ShaderSpecification.ColorInput == Shader::ColorInputType::FlatColor)
						{
							transferColor = *Shader::GetInputLocation<glm::vec4>("a_Color", component.ShaderData, component.Shader);
						}
						component.ShaderSpecification.ColorInput = Shader::ColorInputType::VertexColor;
						updateComponent();
						if (component.VertexColors) { component.VertexColors->clear(); }
						component.VertexColors = CreateRef<std::vector<glm::vec4>>();
						for (uint32_t iterator{0}; iterator < component.Vertices->size(); iterator++)
						{
							component.VertexColors->push_back(transferColor);
						}
					}
					
					ImGui::EndPopup();
				}

				if (component.ShaderSpecification.ColorInput == Shader::ColorInputType::None) { return; }

				if (component.ShaderSpecification.ColorInput == Shader::ColorInputType::FlatColor)
				{
					glm::vec4* color = Shader::GetInputLocation<glm::vec4>("a_Color", component.ShaderData, component.Shader);
					ImGui::ColorEdit4("Color", glm::value_ptr(*color));
				}
				if (component.ShaderSpecification.ColorInput == Shader::ColorInputType::VertexColor)
				{
					uint32_t iterator{ 1 };
					for (auto& color : *component.VertexColors)
					{
						ImGui::ColorEdit4(("Vertex " + std::to_string(iterator)).c_str(), glm::value_ptr(color));
						iterator++;
					}
				}
			};

			auto AddTextureSection = [&]()
			{
				if (ImGui::Checkbox("Add Texture", &component.ShaderSpecification.AddTexture))
				{

					updateComponent();
					// Checkbox is switched on
					if (component.ShaderSpecification.AddTexture)
					{
						if (component.CurrentShape == Shape::ShapeTypes::Cube || component.CurrentShape == Shape::ShapeTypes::Pyramid)
						{
							component.ShaderSpecification.RenderType = Shape::RenderingType::DrawTriangle;
							updateComponent();
							component.Vertices = CreateRef<std::vector<glm::vec3>>(Shape::ShapeTypeToShape(component.CurrentShape).GetTriangleVertices());
							component.TextureCoordinates = CreateRef<std::vector<glm::vec2>>(Shape::ShapeTypeToShape(component.CurrentShape).GetTriangleTextureCoordinates());
							if (component.VertexColors) { component.VertexColors->resize(component.Vertices->size(), { 1.0f, 1.0f, 1.0f, 1.0f }); }
						}
						Shader::SetDataAtInputLocation<float>(1.0f, "a_TilingFactor", component.ShaderData, component.Shader);
					}
					// Checkbox is switched off
					if (!component.ShaderSpecification.AddTexture)
					{
						if (component.CurrentShape == Shape::ShapeTypes::Cube || component.CurrentShape == Shape::ShapeTypes::Pyramid)
						{
							component.ShaderSpecification.RenderType = Shape::RenderingType::DrawIndex;
							updateComponent();
							component.Vertices = CreateRef<std::vector<glm::vec3>>(Shape::ShapeTypeToShape(component.CurrentShape).GetIndexVertices());
							component.Indices = CreateRef<std::vector<uint32_t>>(Shape::ShapeTypeToShape(component.CurrentShape).GetIndices());
							component.TextureCoordinates = CreateRef<std::vector<glm::vec2>>(Shape::ShapeTypeToShape(component.CurrentShape).GetIndexTextureCoordinates());
							if (component.VertexColors) { component.VertexColors->resize(component.Vertices->size(), { 1.0f, 1.0f, 1.0f, 1.0f }); }
						}
					}
				}
				if (component.ShaderSpecification.AddTexture)
				{
					ImGui::Button("Texture", ImVec2(100.0f, 0.0f));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							const wchar_t* path = (const wchar_t*)payload->Data;
							std::filesystem::path texturePath(path);
							AssetHandle currentHandle = AssetManager::ImportNewTextureFromFile(texturePath);
							component.TextureHandle = currentHandle;
							Ref<Texture2D> texture = AssetManager::GetTexture(currentHandle);
							if (texture->IsLoaded())
								component.Texture = texture;
							else
								KG_WARN("Could not load texture {0}", texturePath.filename().string());
						}
						ImGui::EndDragDropTarget();
					}

					float* tilingFactor = Shader::GetInputLocation<float>("a_TilingFactor", component.ShaderData, component.Shader);
					ImGui::DragFloat("Tiling Factor", tilingFactor, 0.1f, 0.0f, 100.0f);
				}
			};

			auto AddCircleShapeSection = [&]()
			{
				if (ImGui::Checkbox("Add Circle Shape", &component.ShaderSpecification.AddCircleShape))
				{
					updateComponent();
					if (component.ShaderSpecification.AddCircleShape)
					{
						Shader::SetDataAtInputLocation<float>(1.0f, "a_Thickness", component.ShaderData, component.Shader);
						Shader::SetDataAtInputLocation<float>(0.005f, "a_Fade", component.ShaderData, component.Shader);
					}
				}
				if (component.ShaderSpecification.AddCircleShape)
				{
					float* thickness = Shader::GetInputLocation<float>("a_Thickness", component.ShaderData, component.Shader);
					ImGui::DragFloat("Thickness", thickness, 0.025f, 0.0f, 1.0f);

					float* fade = Shader::GetInputLocation<float>("a_Fade", component.ShaderData, component.Shader);
					ImGui::DragFloat("Fade", fade, 0.00025f, 0.0f, 1.0f);
				}
			};

			auto AddProjectionMatrixSection = [&]()
			{
				if (ImGui::Checkbox("Add Projection Matrix", &component.ShaderSpecification.AddProjectionMatrix)) { updateComponent(); }
			};

			auto AddEntityIDSection = [&]()
			{
				if (ImGui::Checkbox("Add Entity ID", &component.ShaderSpecification.AddEntityID)) { updateComponent(); }
			};
			

			//=========================
			// Beginning of Main Functionality
			//=========================
			// Display Selection Popup Button to choose Shape
			Shape::ShapeTypes selectedShape = component.CurrentShape;
			if (ImGui::Button("Select a Shape")) { ImGui::OpenPopup("Shape Selection"); }
			ImGui::SameLine();
			ImGui::TextUnformatted(Shape::ShapeTypeToString(selectedShape).c_str());
			if (ImGui::BeginPopup("Shape Selection"))
			{
				for (const auto& shape : Shape::s_AllShapes)
				{
					if (ImGui::Selectable(Shape::ShapeTypeToString(shape->GetShapeType()).c_str()))
					{
						component.CurrentShape = shape->GetShapeType();
						component.ShaderSpecification.RenderType = shape->GetRenderingType();
						if (shape->GetRenderingType() == Shape::RenderingType::DrawIndex)
						{
							component.Vertices = CreateRef<std::vector<glm::vec3>>(shape->GetIndexVertices());
							component.Indices = CreateRef<std::vector<uint32_t>>(shape->GetIndices());
							component.TextureCoordinates = CreateRef<std::vector<glm::vec2>>(shape->GetIndexTextureCoordinates());
						}
						if (shape->GetRenderingType() == Shape::RenderingType::DrawTriangle)
						{
							component.Vertices = CreateRef<std::vector<glm::vec3>>(shape->GetTriangleVertices());
							component.TextureCoordinates = CreateRef<std::vector<glm::vec2>>(shape->GetTriangleTextureCoordinates());
						}
						if (component.CurrentShape == Shape::ShapeTypes::Cube || component.CurrentShape == Shape::ShapeTypes::Pyramid)
						{
							//component.ShaderSpecification.AddTexture = false;
							component.ShaderSpecification.AddCircleShape = false;
						}
						updateComponent();
						if (component.VertexColors) { component.VertexColors->resize(component.Vertices->size(), { 1.0f, 1.0f, 1.0f, 1.0f }); }
					}
				}
				ImGui::EndPopup();
			}
			ImGui::Separator();
			ImGui::Text("Shader Specification");

			// This section displays the shader specification options available for the chosen object
			if (selectedShape == Shape::ShapeTypes::None) { return; }
			if (selectedShape == Shape::ShapeTypes::Quad)
			{
				AddFlatColorSection();
				AddTextureSection();
				AddCircleShapeSection();
				AddProjectionMatrixSection();
				AddEntityIDSection();
			}
			if (selectedShape == Shape::ShapeTypes::Cube || selectedShape == Shape::ShapeTypes::Pyramid)
			{
				AddFlatColorSection();
				AddTextureSection();
				AddProjectionMatrixSection();
				AddEntityIDSection();
			}
			
		});

		DrawComponent<Rigidbody2DComponent>("Rigidbody 2D", entity, [](auto& component)
			{
				const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic"};
				const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];
				if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
						if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
						{
							currentBodyTypeString = bodyTypeStrings[i];
							component.Type = (Rigidbody2DComponent::BodyType)i;
						}
						if (isSelected) { ImGui::SetItemDefaultFocus(); }
					}
					ImGui::EndCombo();
				}

				ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
			});

		DrawComponent<BoxCollider2DComponent>("Box Collider 2D", entity, [](auto& component)
			{
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.05f);
				ImGui::DragFloat2("Size", glm::value_ptr(component.Size), 0.05f);
				ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
			});

		DrawComponent<CircleCollider2DComponent>("Circle Collider 2D", entity, [](auto& component)
			{
				ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset));
				ImGui::DragFloat("Radius", &component.Radius, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
			});
	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}
}
