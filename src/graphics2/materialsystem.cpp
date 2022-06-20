#include "materialsystem.h"

#include "shader.h"

MaterialSystem matsys{};

MaterialSystem::MaterialSystem()
{
}

MaterialSystem::~MaterialSystem()
{

}

void MaterialSystem::Init()
{
    aTexPool.Resize( 1000000 );

    Shader *pShader = new Shader( "shader", "shaders/unlit.vert.spv", "shaders/unlit.frag.spv" );
}

Handle MaterialSystem::CreateTexture( const std::string &srName )
{
    auto pTex = ( Texture2* )aTexPool.Alloc( sizeof( Texture2 ) );
    return aTextures.Add( &pTex );
}

Texture2 *MaterialSystem::GetTexture( Handle shTex )
{
    return *aTextures.Get( shTex );
}

Handle MaterialSystem::CreateMaterial( const std::string &srName )
{
    return 0;
}

