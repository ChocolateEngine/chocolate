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
	bool                SetShader( const char* name ) override;
	//HShader             GetShader( const char* name ) override;
	std::string         GetShaderName(  ) { return apShader ? apShader->aName : ""; };

	MaterialVar        *GetVar( const char* name ) override;
	
	template <typename T>
	MaterialVar        *CreateVar( const char* name, T data );

	void                SetVar( const char* name, Texture *data ) override;
	void                SetVar( const char* name, float data ) override;
	void                SetVar( const char* name, int data ) override;
	void                SetVar( const char* name, const glm::vec2 &data ) override;
	void                SetVar( const char* name, const glm::vec3 &data ) override;
	void                SetVar( const char* name, const glm::vec4 &data ) override;

	Texture            *GetTexture( const char* name, Texture *fallback = nullptr ) override;
	float               GetFloat( const char* name, float fallback = 0.f ) override;
	int                 GetInt( const char* name, int fallback = 0 ) override;
	glm::vec2           GetVec2( const char* name, glm::vec2 fallback = {} ) override;
	glm::vec3           GetVec3( const char* name, glm::vec3 fallback = {} ) override;
	glm::vec4           GetVec4( const char* name, glm::vec4 fallback = {} ) override;

	int                 GetTextureId( const char* name, Texture *fallback = nullptr );

	//HMaterial                   aMat;

	BaseShader*                 apShader = nullptr;
	std::vector< MaterialVar* > aVars;
	// std::unordered_map< std::string, MaterialVar* > aVars;
};

