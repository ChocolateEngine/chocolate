#pragma once

#include "gutil.hh"
#include "texture.h"

#include "core/core.h"

class MaterialSystem
{
    MemPool                      aTexPool;
    ResourceManager< Texture2* > aTextures;

    
public:
    MaterialSystem();
    ~MaterialSystem();

    void      Init();

    Handle    CreateTexture( const std::string &srName );
    Texture2  *GetTexture( Handle shTex );

    Handle    CreateMaterial( const std::string &srName );
};

extern MaterialSystem matsys;