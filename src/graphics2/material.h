/*
material.h ( Authored by Demez )


*/
#pragma once

#include "graphics/imaterial.h"

static std::string gEmpty;


struct MaterialVk
{
	
};



// old

class Material: public IMaterial
{
public:
	                           Material() {}
	virtual                   ~Material() {}

	void                       Init();
	void                       Destroy();

	// Set the shader for the material by the shader name 
	bool                       SetShader( std::string_view name ) override;

	// const std::string&         GetShaderName() { return apShader ? apShader->aName : gEmpty; };
	// const std::string&         GetShaderName() { return gEmpty; };

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

	HShader                    aShader = InvalidHandle;

	// BaseShader*                 apShader = nullptr;
	std::vector< MaterialVar* > aVars;
	// std::unordered_map< std::string, MaterialVar* > aVars;
};

