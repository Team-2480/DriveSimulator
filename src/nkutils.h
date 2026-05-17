#pragma once

#include "raylib.h"
struct nk_user_font* nk_make_font(Font font, float fontSize);
void nk_free_font(nk_user_font* font);
