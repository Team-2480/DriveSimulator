#pragma once

#include "raylib.h"
RenderTexture2D LoadRenderTextureDepthTex(int width, int height);
void UnloadRenderTextureDepthTex(RenderTexture2D target);
