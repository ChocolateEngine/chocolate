#pragma once

void   Graphics_DestroyShadowRenderPass();
bool   Graphics_CreateShadowRenderPass();

void   Graphics_AddShadowMap( Light_t* spLight );
void   Graphics_DestroyShadowMap( Light_t* spLight );

//Handle Graphics_AddLightBuffer( const char* spBufferName, size_t sBufferSize, Light_t* spLight );
//void   Graphics_DestroyLightBuffer( Light_t* spLight );

u32    Graphics_AllocateLightSlot( Light_t* spLight );
void   Graphics_FreeLightSlot( Light_t* spLight );
void   Graphics_UpdateLightBuffer( Light_t* spLight );

void   Graphics_PrepareShadowRenderLists();
void   Graphics_PrepareLights();
void   Graphics_DestroyLights();

// Are we using any shadowmaps/are any shadowmaps enabled?
bool   Graphics_IsUsingShadowMaps();

void   Graphics_DrawShadowMaps( Handle sCmd, size_t sIndex, u32* viewports, u32 viewportCount );

void   Graphics_ResetShadowMapsRenderList();
