#pragma once

#include "Kargono/Core/UUID.h"
#include "Kargono/Renderer/Shader.h"
#include "Kargono/Text/TextEngine.h"

#include <filesystem>
#include <vector>
#include <utility>

namespace Kargono
{

	namespace Assets
	{
		// An AssetHandle is a unique identifier for an Asset.
		using AssetHandle = Kargono::UUID;

		// This enum provides a method to distinguish between different specific
		//		asset types in an Asset. The metadata struct will hold an AssetType.
		enum AssetType
		{
			None = 0,
			Texture = 1,
			Shader = 2,
			Audio = 3,
			Font = 4,
			Scene = 5,
			UIObject = 6
		};
		//==============================
		// AssetType <-> String Conversions
		//==============================
		inline std::string AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::Texture: return "Texture";
			case AssetType::Shader: return "Shader";
			case AssetType::Audio: return "Audio";
			case AssetType::Font: return "Font";
			case AssetType::Scene: return "Scene";
			case AssetType::UIObject: return "UIObject";
			case AssetType::None: return "None";
			}
			KG_CORE_ASSERT(false, "Unknown Type of AssetType.");
			return "";
		}

		inline AssetType StringToAssetType(std::string type)
		{
			if (type == "Texture") { return AssetType::Texture; }
			if (type == "Shader") { return AssetType::Shader; }
			if (type == "Audio") { return AssetType::Audio; }
			if (type == "Font") { return AssetType::Font; }
			if (type == "Scene") { return AssetType::Scene; }
			if (type == "UIObject") { return AssetType::UIObject; }
			if (type == "None") { return AssetType::None; }

			KG_CORE_ASSERT(false, "Unknown Type of AssetType String.");
			return AssetType::None;
		}

		//==============================
		// Metadata Struct
		//==============================
		// This is the generic metadata struct that is directly held inside an asset.
		//		This struct holds generic information about an asset that is universally
		//		needed. The IntermediateLocation provides a relative path from the selected
		//		project's asset directory to the intermediate. As an aside, an intermediate
		//		is file that has been preprocessed and is directly ready to be loaded into
		//		the engine and used without the use of external tools. The checksum is generated
		//		differently based on the file type, but it uniquely identifies the file itself.
		//		The checksum is generally used to ensure duplicate files of the same type are
		//		not being stored. The AssetType allows an easy method to identify the specific
		//		asset type in a generic sense. The SpecificFileData holds a reference to data
		//		that is only needed by this specific type of asset. Ex: A texture might need
		//		to know its width and height, while an audio file might need to know its sample
		//		rate.
		struct Metadata
		{
			std::filesystem::path IntermediateLocation;
			std::string CheckSum;
			Assets::AssetType Type = Assets::AssetType::None;
			Ref<void> SpecificFileData = nullptr;
		};

		//==============================
		// Asset Specific Metadata
		//==============================
		// These next few structs hold data that is specific to a unique asset
		//		type. These metadata structs are meant to be held inside the
		//		metadata struct in the SpecificFileData Ref<void> pointer.

		// This metadata struct mostly holds loading information for the intermediate to load correctly.
		//		When the file is loaded from binary (Intermediate), it needs to know the
		//		image's width, height, and number of channels to interpret the binary
		//		correctly. The InitialFileLocation field is for debugging.
		struct TextureMetaData
		{
			int32_t Width, Height, Channels;
			std::filesystem::path InitialFileLocation;
		};

		// This metadata struct mostly holds loading information for the intermediate to load correctly.
		//		The binary form of Audio is held in a PCM format. The channels, sample rate, TotalPcmFrameCount,
		//		and the TotalSize are used for loading the file correctly. The InitialFileLocation field is for debugging.
		struct AudioMetaData
		{
			uint32_t Channels, SampleRate;
			uint64_t TotalPcmFrameCount, TotalSize;
			std::filesystem::path InitialFileLocation;
		};

		// This metadata struct holds information to correctly interpret the shader binary when creating the final Shader object
		//		in the selected rendering API. The ShaderSpec field holds the specification that was originally used to generate
		//		the shader through the shader builder. The UniformList holds the list of uniforms needed for this shader to
		//		run correctly. The InputBufferLayout is generated when a shader is created in the shader builder (the uniform list too)
		//		and holds the format of per vertex input. Ex: vec3(vertex position), vec3(world position), vec4(color), etc...
		struct ShaderMetaData
		{
			Shader::ShaderSpecification ShaderSpec{};
			UniformBufferList UniformList{};
			InputBufferLayout InputLayout{};
		};

		// This metadata struct holds the information needed to correctly load a selected font
		//		from its intermediate(binary) form. Fonts are originally loaded from a .ttf file which
		//		holds most of this metadata itself. When the font is converted into an intermediate form,
		//		the information that defines each character is saved in the Characters vector. This
		//		information includes the character's height, width, etc... The BufferLocations describe 
		//		the exact location in the intermediate where each character image is stored. The index of the
		//		Characters and BufferLocations vectors correspond with each other. The Character object
		//		holds enough information to get the size of the image inside of the intermediate.
		//		The AverageWidth and AverageHeight fields are generated when loading the .ttf file and are
		//		used for text positioning in the user interface. The InitialFileLocation field is for
		//		debugging.
		struct FontMetaData
		{
			std::vector<uint64_t> BufferLocations{};
			std::vector<std::pair<unsigned char, Font::Character>> Characters{};
			float AverageWidth{ 0 };
			float AverageHeight{ 0 };
			std::filesystem::path InitialFileLocation;
		};

		// This metadata struct is currently empty but keeps a consistent API when working with assets.
		//		There may be later additions to this struct.
		struct SceneMetaData
		{

		};

		// This metadata struct is currently empty but keeps a consistent API when working with assets.
		//		There may be later additions to this struct.
		struct UIObjectMetaData
		{

		};

		//==============================
		// Asset Struct
		//==============================
		// This struct represents an asset managed by the AssetManager class. This asset
		//		has an AssetHandle which uniquely identifies this asset and a metadata
		//		object which holds ancillary information about the asset such as the
		//		asset type, asset location, a checksum, and asset type specific metadata.
		struct Asset
		{
			AssetHandle Handle;
			Metadata Data;
		};
	}
}
