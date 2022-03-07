#include "materialsystem.h"

MaterialSystem matsys{};

MaterialSystem::MaterialSystem()
{
    aTexPool.Resize( 1000000 );
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