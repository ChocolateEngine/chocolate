/*
material.h ( Authored by Demez )


*/
#pragma once

#include "types/databuffer.hh"
#include "../allocator.h"
#include "../shaders/shaders.h"
#include "graphics/imaterial.h"

#include <vulkan/vulkan.hpp>
#include <filesystem>

class Renderer;


class Material: public IMaterial
{
public:
	                    Material() {}
	virtual            ~Material() {}

	void                Init();
	void                Destroy();

	//void                SetHandle( HMaterial mat ) override;
	//HMaterial           GetHandle() override;

	//void                SetName( const char* name ) override;
	//std::string         GetName(  ) override;

	/* Set the shader for the material by the shader name */
	bool                SetShader( const std::string& name ) override;

	std::string         GetShaderName(  ) { return apShader ? apShader->aName : ""; };

	MaterialVar        *GetVar( const std::string& name ) override;
	
	template <typename T>
	MaterialVar        *CreateVar( const std::string& name, T data );

	void                SetVar( const std::string& name, Texture *data ) override;
	void                SetVar( const std::string& name, float data ) override;
	void                SetVar( const std::string& name, int data ) override;
	void                SetVar( const std::string& name, const glm::vec2 &data ) override;
	void                SetVar( const std::string& name, const glm::vec3 &data ) override;
	void                SetVar( const std::string& name, const glm::vec4 &data ) override;

	Texture            *GetTexture( const std::string& name, Texture *fallback = nullptr ) override;

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

	size_t              GetTextureId( const std::string& name, Texture *fallback = nullptr );

	//HMaterial                   aMat;

	BaseShader*                 apShader = nullptr;
	std::vector< MaterialVar* > aVars;
	// std::unordered_map< std::string, MaterialVar* > aVars;
};

