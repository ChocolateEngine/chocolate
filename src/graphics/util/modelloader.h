/*
modelloader.h ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

#pragma once

#include <string>
#include <vector>

class IMesh;
class Material;

// TODO: actually make the class lol

// TODO: use std::filesystem::path
void LoadObj( const std::string &srPath, std::vector<IMesh*> &meshes );
void LoadGltf( const std::string &srPath, std::vector<IMesh*> &meshes );

