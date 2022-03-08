#include "materialsystem.h"

#include "shader.h"

MaterialSystem matsys{};

MaterialSystem::MaterialSystem()
{
    aTexPool.Resize( 1000000 );

    Shader *pShader = new Shader( "shader", "shaders/basic3d.vert.spv" );
}

MaterialSystem::~MaterialSystem()
{

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