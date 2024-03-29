#pragma once

#include "Kargono/Math/Math.h"

#include <string>
#include <vector>

namespace Kargono
{
	class Shape
	{
	public:

		enum class RenderingType
		{
			None = 0, DrawIndex, DrawTriangle, DrawLine
		};

		enum class ShapeTypes
		{
			None = 0, Quad, Cube, Pyramid
		};

		static std::string RenderingTypeToString(Shape::RenderingType renderType)
		{
			switch (renderType)
			{
			case Shape::RenderingType::None: return "None";
			case Shape::RenderingType::DrawIndex: return "DrawIndex";
			case Shape::RenderingType::DrawTriangle: return "DrawTriangle";
			case Shape::RenderingType::DrawLine: return "DrawLine";
			}
			KG_CORE_ASSERT(false, "Unknown Data Type sent to RenderingTypeToString Function");
			return "None";
		}

		static Shape::RenderingType StringToRenderingType(std::string_view string)
		{
			if (string == "None") { return Shape::RenderingType::None; }
			if (string == "DrawIndex") { return Shape::RenderingType::DrawIndex; }
			if (string == "DrawTriangle") { return Shape::RenderingType::DrawTriangle; }
			if (string == "DrawLine") { return Shape::RenderingType::DrawLine; }

			KG_CORE_ASSERT(false, "Unknown Data Type sent to StringToRenderingType Function");
			return Shape::RenderingType::None;
		}

		static std::string ShapeTypeToString(Shape::ShapeTypes shapeType)
		{
			switch (shapeType)
			{
			case Shape::ShapeTypes::None: return "None";
			case Shape::ShapeTypes::Quad: return "Quad";
			case Shape::ShapeTypes::Cube: return "Cube";
			case Shape::ShapeTypes::Pyramid: return "Pyramid";
			}
			KG_CORE_ASSERT(false, "Unknown Data Type sent to ShapeTypeToString Function");
			return "None";
		}

		static Shape::ShapeTypes StringToShapeType(std::string_view string)
		{
			if (string == "None") { return Shape::ShapeTypes::None; }
			if (string == "Quad") { return Shape::ShapeTypes::Quad; }
			if (string == "Cube") { return Shape::ShapeTypes::Cube; }
			if (string == "Pyramid") { return Shape::ShapeTypes::Pyramid; }

			KG_CORE_ASSERT(false, "Unknown Data Type sent to StringToShapeType Function");
			return Shape::ShapeTypes::None;
		}

		static const Shape& ShapeTypeToShape(Shape::ShapeTypes shapeType)
		{
			switch (shapeType)
			{
			case Shape::ShapeTypes::Pyramid: return Shape::s_Pyramid;
			case Shape::ShapeTypes::Quad: return Shape::s_Quad;
			case Shape::ShapeTypes::Cube: return Shape::s_Cube;
			case Shape::ShapeTypes::None: return Shape::s_None;
			}

			KG_CORE_ASSERT(false, "Unknown Data Type sent to StringToShapeType Function");
			return Shape::s_None;
		}

		using Vertex = Math::vec3;
	public:
		//============================================================
		// Constructors
		//============================================================
		Shape() = default;
		Shape(std::string_view name, const std::vector<Vertex>& indexVertices, const std::vector<Math::vec2>& indexTexCoordinates,
			const std::vector<uint32_t>& indices,
			const std::vector<Vertex>& triangleVertices, const std::vector<Math::vec2>& triangleTexCoordinates,
			Shape::RenderingType renderingType, Shape::ShapeTypes shapeType,
			const std::vector<std::string>& restrictedSpecs)
			: m_Name{ name }, m_IndexVertices{ indexVertices }, m_IndexTextureCoordinates{ indexTexCoordinates },m_Indices{ indices },
			m_TriangleVertices{triangleVertices}, m_TriangleTextureCoordinates{triangleTexCoordinates},
		m_RenderingType{ renderingType }, m_ShapeType{shapeType}, m_RestrictedSpecs(restrictedSpecs) {}
		std::string_view GetName() const { return m_Name; }
		//============================================================
		// Getters/Setters
		//============================================================
		// Index Getter/Setters
		const std::vector<Vertex>& GetIndexVertices() const { return m_IndexVertices; }
		const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
		const std::vector<Math::vec2>& GetIndexTextureCoordinates() const { return m_IndexTextureCoordinates; }

		const std::vector<Vertex>& GetTriangleVertices() const { return m_TriangleVertices; }
		const std::vector<Math::vec2>& GetTriangleTextureCoordinates() const { return m_TriangleTextureCoordinates; }

		// Triangle Getters/Setters

		// Type Information Getters
		Shape::RenderingType GetRenderingType() const { return m_RenderingType; }
		Shape::ShapeTypes GetShapeType() const { return m_ShapeType; }
		const std::vector<std::string>& GetRestrictedSpecs() const { return m_RestrictedSpecs; }

	public:
		//============================================================
		// Static Object Declarations
		//============================================================
		static Shape s_Pyramid;
		static Shape s_Quad;
		static Shape s_Cube;
		static Shape s_None;

		inline static std::vector<Shape*> s_AllShapes {&s_Quad, &s_Cube, &s_None, &s_Pyramid};
	private:
		std::string m_Name {};
		// Indices Data
		std::vector<Vertex> m_IndexVertices {};
		std::vector<Math::vec2> m_IndexTextureCoordinates {};
		std::vector<uint32_t> m_Indices {};
		// Triangle Rendering Data
		std::vector<Vertex> m_TriangleVertices {};
		std::vector<Math::vec2> m_TriangleTextureCoordinates {};
		// Type Information
		RenderingType m_RenderingType { Shape::RenderingType::None};
		Shape::ShapeTypes m_ShapeType{ Shape::ShapeTypes::None };

		// Editor Information
		// This field labels shader specifications that should not be visible in the SceneHierarchy Panel
		// for this particular object.
		std::vector<std::string> m_RestrictedSpecs {};
	};

}
