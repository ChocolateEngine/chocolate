/*
material.h ( Authored by Demez )


*/
#pragma once

#include "../shaders/shaders.h"
#include "graphics/imaterial.h"

#include <vulkan/vulkan.hpp>
#include <filesystem>

class Renderer;


class Material: public IMaterial
{
public:
	                           Material() {}
	virtual                   ~Material() {}

	void                       Init();
	void                       Destroy();

	// Set the shader for the material by the shader name 
	bool                       SetShader( const std::string& name ) override;

	std::string                GetShaderName(  ) { return apShader ? apShader->aName : ""; };

	// ----------------------------------------------------------------------------------------

	MaterialVar               *GetVar( const std::string& name ) override;

	MaterialVar*               GetVar( size_t sIndex ) override;
	size_t                     GetVarCount() override;
	
	template <typename T>
	MaterialVar               *CreateVar( const std::string& name, T data );

	void                       SetVar( const std::string& name, Texture *data ) override;
	void                       SetVar( const std::string& name, float data ) override;
	void                       SetVar( const std::string& name, int data ) override;
	void                       SetVar( const std::string& name, const glm::vec2 &data ) override;
	void                       SetVar( const std::string& name, const glm::vec3 &data ) override;
	void                       SetVar( const std::string& name, const glm::vec4 &data ) override;

	size_t                     GetTextureId( const std::string& name, Texture *fallback = nullptr );
	Texture                   *GetTexture( const std::string& name, Texture *fallback = nullptr ) override;

	// const float&               GetFloat( const std::string& name ) override;
	// const int&                 GetInt( const std::string& name ) override;
	const glm::vec2&           GetVec2( const std::string& name ) override;
	const glm::vec3&           GetVec3( const std::string& name ) override;
	const glm::vec4&           GetVec4( const std::string& name ) override;

	const float&               GetFloat( const std::string& name, const float& fallback = 0.f ) override;
	const int&                 GetInt( const std::string& name, const int& fallback = 0 ) override;
	const glm::vec2&           GetVec2( const std::string& name, const glm::vec2& fallback ) override;
	const glm::vec3&           GetVec3( const std::string& name, const glm::vec3& fallback ) override;
	const glm::vec4&           GetVec4( const std::string& name, const glm::vec4& fallback ) override;

	// ----------------------------------------------------------------------------------------

	// Get Vertex Format used by the current shader
	VertexFormat               GetVertexFormat() override;
	//HMaterial                   aMat;
	
	// cool epic convienence function
	IMaterialSystem*           GetMaterialSystem() override;

	BaseShader*                 apShader = nullptr;
	std::vector< MaterialVar* > aVars;
	// std::unordered_map< std::string, MaterialVar* > aVars;
};

