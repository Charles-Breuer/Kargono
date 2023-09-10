#include "Kargono/kgpch.h"
#include "Kargono/Scene/Scene.h"
#include "Kargono/Scripting/ScriptEngine.h"
#include "Kargono/Scripting/ScriptGlue.h"
#include "mono/jit/jit.h"
#include "mono/metadata/assembly.h"
#include "mono/metadata/object.h"
#include "mono/metadata/tabledefs.h"
#include "Kargono/Scene/Entity.h"
#include "Kargono/Core/UUID.h"

namespace Kargono
{
	static std::unordered_map<std::string, ScriptFieldType> s_ScriptFieldTypeMap = 
	{
		{"System.Single", ScriptFieldType::Float},
		{"System.Double", ScriptFieldType::Double},

		{"System.Boolean", ScriptFieldType::Bool},
		{"System.Char", ScriptFieldType::Char},
		{"System.Int16", ScriptFieldType::Short},
		{"System.Int32", ScriptFieldType::Int},
		{"System.Int64", ScriptFieldType::Long},

		{"System.Byte", ScriptFieldType::Byte},
		{"System.UInt16", ScriptFieldType::UShort},
		{"System.UInt32", ScriptFieldType::UInt},
		{"System.UInt64", ScriptFieldType::ULong},

		{"Kargono.Vector2", ScriptFieldType::Vector2},
		{"Kargono.Vector3", ScriptFieldType::Vector3},
		{"Kargono.Vector4", ScriptFieldType::Vector4},
		{"Kargono.Entity", ScriptFieldType::Entity},

	};

	namespace Utils
	{
		// TODO: move to FileSystem class
		static char* ReadBytes(const std::filesystem::path& filepath, uint32_t* outSize)
		{
			std::ifstream stream(filepath, std::ios::binary | std::ios::ate);

			if (!stream)
			{
				// Failed to open the file
				return nullptr;
			}

			std::streampos end = stream.tellg();
			stream.seekg(0, std::ios::beg);
			uint64_t size = end - stream.tellg();

			if (size == 0)
			{
				// File is empty
				return nullptr;
			}

			char* buffer = new char[size];
			stream.read((char*)buffer, size);
			stream.close();

			*outSize = (uint32_t)size;
			return buffer;
		}

		static MonoAssembly* LoadMonoAssembly(const std::filesystem::path& assemblyPath)
		{
			uint32_t fileSize = 0;
			char* fileData = ReadBytes(assemblyPath, &fileSize);

			// NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
			MonoImageOpenStatus status;
			MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

			if (status != MONO_IMAGE_OK)
			{
				const char* errorMessage = mono_image_strerror(status);
				// Log some error message using the errorMessage data
				return nullptr;
			}

			MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.string().c_str(), &status, 0);
			mono_image_close(image);

			// Don't forget to free the file data
			delete[] fileData;

			return assembly;
		}

		static void PrintAssemblyTypes(MonoAssembly* assembly)
		{
			MonoImage* image = mono_assembly_get_image(assembly);
			const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
			int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

			for (int32_t i = 0; i < numTypes; i++)
			{
				uint32_t cols[MONO_TYPEDEF_SIZE];
				mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

				

				const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
				const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

				KG_CORE_TRACE("{}.{}", nameSpace, name);
			}
		}

		ScriptFieldType MonoTypeToScriptFieldType(MonoType* monoType)
		{
			std::string typeName = mono_type_get_name(monoType);

			auto it = s_ScriptFieldTypeMap.find(typeName);

			if (it == s_ScriptFieldTypeMap.end())
			{
				KG_CORE_ERROR("Unknown type: {}", typeName);
				return ScriptFieldType::None;
			}
			return it->second;
		}

	}

	struct ScriptEngineData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* AppDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreAssemblyImage= nullptr;

		MonoAssembly* AppAssembly = nullptr;
		MonoImage* AppAssemblyImage = nullptr;

		ScriptClass EntityClass;

		std::unordered_map<std::string, Ref<ScriptClass>> EntityClasses;
		std::unordered_map<UUID, Ref<ScriptInstance>> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		// Runtime
		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_ScriptData = nullptr;


	void ScriptEngine::Init()
	{
		s_ScriptData = new ScriptEngineData();
		InitMono();
		LoadAssembly("Resources/Scripts/Kargono-ScriptCore.dll");
		LoadAppAssembly("SandboxProject/Assets/Scripts/Binaries/Sandbox.dll");
		LoadAssemblyClasses();

		ScriptGlue::RegisterComponents();
		ScriptGlue::RegisterFunctions();

		s_ScriptData->EntityClass = ScriptClass("Kargono", "Entity", true);


#if 0
		// Retrieve and instantiate Class (with constructor)

		MonoObject* instance = s_ScriptData->EntityClass.Instantiate();
		// 2. Call Method
		MonoMethod* printMessageFunc = s_ScriptData->EntityClass.GetMethod("PrintMessage", 0);
		s_ScriptData->EntityClass.InvokeMethod(instance, printMessageFunc, nullptr);

		// 3. Call Method with Parameters
		MonoMethod* printIntFunc = s_ScriptData->EntityClass.GetMethod("PrintInt", 1);

		int value = 5;
		int value2 = 8;
		void* param = &value;

		void* params[2] =
		{
			&value,
			&value2
		};

		MonoString* monoString = mono_string_new(s_ScriptData->AppDomain, "Hello World from C++!");
		void* stringParam = monoString;

		s_ScriptData->EntityClass.InvokeMethod(instance, printIntFunc, &param);

		MonoMethod* printIntsFunc = s_ScriptData->EntityClass.GetMethod("PrintInts", 2);

		s_ScriptData->EntityClass.InvokeMethod(instance, printIntsFunc, params);

		MonoMethod* printCustomMessageFunc = s_ScriptData->EntityClass.GetMethod("PrintCustomMessage", 1);
		s_ScriptData->EntityClass.InvokeMethod(instance, printCustomMessageFunc, &stringParam);

		//KG_CORE_ASSERT(false, "AHHH")
#endif

	}
	void ScriptEngine::Shutdown()
	{
		ShutdownMono();
		delete s_ScriptData;
		s_ScriptData = nullptr;
	}

	void ScriptEngine::InitMono()
	{
		mono_set_assemblies_path("mono/lib");

		MonoDomain* rootDomain = mono_jit_init("KargonoJITRuntime");

		KG_CORE_ASSERT(rootDomain, "Mono Domain not loaded correctly!")

		// Store the root domain pointer
		s_ScriptData->RootDomain = rootDomain;


	}
	void ScriptEngine::ShutdownMono()
	{
		// ugh mono
		//mono_domain_unload(s_ScriptData->AppDomain);
		s_ScriptData->AppDomain = nullptr;
		//mono_jit_cleanup(s_ScriptData->RootDomain);
		s_ScriptData->RootDomain = nullptr;
	}

	void ScriptEngine::LoadAssembly(const std::filesystem::path& filepath)
	{
		// Create an App Domain
		s_ScriptData->AppDomain = mono_domain_create_appdomain((char*)"KargonoScriptRuntime", nullptr);
		mono_domain_set(s_ScriptData->AppDomain, true);

		// Move this maybe
		s_ScriptData->CoreAssembly = Utils::LoadMonoAssembly(filepath);
		s_ScriptData->CoreAssemblyImage = mono_assembly_get_image(s_ScriptData->CoreAssembly);
		//Utils::PrintAssemblyTypes(s_ScriptData->CoreAssembly);
	}

	void ScriptEngine::LoadAppAssembly(const std::filesystem::path& filepath)
	{
		// Move this maybe
		s_ScriptData->AppAssembly = Utils::LoadMonoAssembly(filepath);
		s_ScriptData->AppAssemblyImage = mono_assembly_get_image(s_ScriptData->AppAssembly);
		//Utils::PrintAssemblyTypes(s_ScriptData->AppAssembly);
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_ScriptData->SceneContext = scene;
	}

	bool ScriptEngine::EntityClassExists(const std::string& fullClassName)
	{
		return s_ScriptData->EntityClasses.find(fullClassName) != s_ScriptData->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		const auto& sc = entity.GetComponent<ScriptComponent>();
		if (ScriptEngine::EntityClassExists(sc.ClassName))
		{
			UUID entityID = entity.GetUUID();

			Ref<ScriptInstance> instance = CreateRef<ScriptInstance>(s_ScriptData->EntityClasses[sc.ClassName], entity);
			s_ScriptData->EntityInstances[entity.GetUUID()] = instance;

			// Copy field values
			if (s_ScriptData->EntityScriptFields.find(entityID) != s_ScriptData->EntityScriptFields.end())
			{
				const ScriptFieldMap& fieldMap = s_ScriptData->EntityScriptFields.at(entityID);
				for (const auto& [name, fieldInstance] : fieldMap)
				{
					instance->SetFieldValueInternal(name, fieldInstance.m_Buffer);
				}
			}

			instance->InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Timestep ts)
	{

		UUID entityUUID = entity.GetUUID();
		KG_CORE_ASSERT(s_ScriptData->EntityInstances.find(entityUUID) != s_ScriptData->EntityInstances.end())

		Ref<ScriptInstance> instance = s_ScriptData->EntityInstances[entityUUID];

		instance->InvokeOnUpdate(ts);
	}

	Ref<ScriptInstance> ScriptEngine::GetEntityScriptInstance(UUID entityID)
	{
		auto it = s_ScriptData->EntityInstances.find(entityID);
		if (it == s_ScriptData->EntityInstances.end()) { return nullptr; }

		return it->second;
	}


	void ScriptEngine::OnRuntimeStop()
	{
		s_ScriptData->SceneContext = nullptr;

		s_ScriptData->EntityInstances.clear();
	}

	void ScriptEngine::LoadAssemblyClasses()
	{
		s_ScriptData->EntityClasses.clear();

		const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(s_ScriptData->AppAssemblyImage, MONO_TABLE_TYPEDEF);
		int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);
		MonoClass* entityClass = mono_class_from_name(s_ScriptData->CoreAssemblyImage, "Kargono", "Entity");

		for (int32_t i = 0; i < numTypes; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

			const char* nameSpace = mono_metadata_string_heap(s_ScriptData->AppAssemblyImage, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* className = mono_metadata_string_heap(s_ScriptData->AppAssemblyImage, cols[MONO_TYPEDEF_NAME]);
			std::string fullName;
			if (strlen(nameSpace) != 0) { fullName = fmt::format("{}.{}", nameSpace, className); }
			else { fullName = className; }

			MonoClass* monoClass = mono_class_from_name(s_ScriptData->AppAssemblyImage, nameSpace, className);

			if (monoClass == entityClass) { continue; }

			bool isEntity = mono_class_is_subclass_of(monoClass, entityClass, false);

			if (!isEntity) { continue; }

			Ref<ScriptClass> scriptClass = CreateRef<ScriptClass>(nameSpace, className);
			s_ScriptData->EntityClasses[fullName] = scriptClass;

			int fieldCount = mono_class_num_fields(monoClass);
			KG_CORE_WARN("{} has {} fields:", className, fieldCount);
			void* iterator = nullptr;
			while (MonoClassField* field = mono_class_get_fields(monoClass, &iterator))
			{
				const char* fieldName = mono_field_get_name(field);
				uint32_t flags = mono_field_get_flags(field);
				if (flags & FIELD_ATTRIBUTE_PUBLIC)
				{
					MonoType* type = mono_field_get_type(field);
					ScriptFieldType fieldType = Utils::MonoTypeToScriptFieldType(type);
					KG_CORE_WARN("  {} ({})", fieldName, Utils::ScriptFieldTypeToString(fieldType));

					scriptClass->m_Fields[fieldName] = { fieldType, fieldName, field };
				}
			}
		}

		//mono_field_get_value();
	}

	MonoImage* ScriptEngine::GetCoreAssemblyImage()
	{
		return s_ScriptData->CoreAssemblyImage;
	}

	MonoObject* ScriptEngine::InstantiateClass(MonoClass* monoClass)
	{
		MonoObject* instance = mono_object_new(s_ScriptData->AppDomain, monoClass);
		mono_runtime_object_init(instance);
		return instance;
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_ScriptData->SceneContext;
	}

	Ref<ScriptClass> ScriptEngine::GetEntityClass(const std::string& name)
	{
		if (s_ScriptData->EntityClasses.find(name) == s_ScriptData->EntityClasses.end()) { return nullptr; }

		return s_ScriptData->EntityClasses.at(name);
	}

	std::unordered_map<std::string, Ref<ScriptClass>> ScriptEngine::GetEntityClasses()
	{
		return s_ScriptData->EntityClasses;
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		KG_CORE_ASSERT(entity)
		UUID entityID = entity.GetUUID();
		//KG_CORE_ASSERT(s_ScriptData->EntityScriptFields.find(entity.GetUUID()) != s_ScriptData->EntityScriptFields.end())

		return s_ScriptData->EntityScriptFields[entityID];
	}

	ScriptClass::ScriptClass(const std::string& classNamespace, const std::string& className, bool isCore)
		: m_ClassNamespace(classNamespace), m_ClassName(className)
	{
		m_MonoClass = mono_class_from_name(isCore ? s_ScriptData->CoreAssemblyImage : s_ScriptData->AppAssemblyImage,
		                                   classNamespace.c_str(), className.c_str());

	}

	MonoObject* ScriptClass::Instantiate()
	{
		return ScriptEngine::InstantiateClass(m_MonoClass);
	}

	MonoMethod* ScriptClass::GetMethod(const std::string& name, int parameterCount)
	{
		return mono_class_get_method_from_name(m_MonoClass, name.c_str(), parameterCount);
	}

	MonoObject* ScriptClass::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
	{
		return mono_runtime_invoke(method, instance, params, nullptr);
	}

	
	
	ScriptInstance::ScriptInstance(Ref<ScriptClass> scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_Instance = scriptClass->Instantiate();

		m_Constructor = s_ScriptData->EntityClass.GetMethod(".ctor", 1);

		m_OnCreateMethod = scriptClass->GetMethod("OnCreate", 0);
		m_OnUpdateMethod = scriptClass->GetMethod("OnUpdate", 1);

		// Call Entity Constructor
		{
			UUID entityID = entity.GetUUID();
			void* param = &entityID;
			m_ScriptClass->InvokeMethod(m_Instance, m_Constructor, &param);
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		if (m_OnCreateMethod) { m_ScriptClass->InvokeMethod(m_Instance, m_OnCreateMethod);}
	}

	void ScriptInstance::InvokeOnUpdate(float ts)
	{
		if (m_OnUpdateMethod)
		{
			void* param = &ts;
			m_ScriptClass->InvokeMethod(m_Instance, m_OnUpdateMethod, &param);
		}
	}

	bool ScriptInstance::GetFieldValueInternal(const std::string& name, void* buffer)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end()) { return false; }

		const ScriptField& field = it->second;
		void* result;
		mono_field_get_value(m_Instance, field.ClassField, buffer);
		return true;

	}

	bool ScriptInstance::SetFieldValueInternal(const std::string& name, const void* value)
	{
		const auto& fields = m_ScriptClass->GetFields();
		auto it = fields.find(name);
		if (it == fields.end()) { return false; }

		const ScriptField& field = it->second;
		void* result;
		mono_field_set_value(m_Instance, field.ClassField, (void*)value);
		return true;

	}

}
