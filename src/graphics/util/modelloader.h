/*
modelloader.h ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

#pragma once

#include <string>
#include <vector>

class Model;

void LoadObj( const std::string &srPath, Model* spModel );
void LoadGltf( const std::string &srPath, const std::string &srExt, Model* spModel );

