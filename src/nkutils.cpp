#include "raylib.h"
#include "scene.h"

struct nk_user_font* nk_make_font(Font font, float fontSize) {
  struct Font* newFont = (struct Font*)MemAlloc(sizeof(struct Font));

  // Use the default font size if desired.
  if (fontSize <= 0.0f) {
    fontSize = 12;
  }
  newFont->baseSize = font.baseSize;
  newFont->glyphCount = font.glyphCount;
  newFont->glyphPadding = font.glyphPadding;
  newFont->glyphs = font.glyphs;
  newFont->recs = font.recs;
  newFont->texture = font.texture;

  // Create the nuklear user font.
  struct nk_user_font* userFont =
      (struct nk_user_font*)MemAlloc(sizeof(struct nk_user_font));
  userFont->userdata = nk_handle_ptr(newFont);
  userFont->height = fontSize;
  userFont->width = nk_raylib_font_get_text_width_user_font;
  return userFont;
}

void nk_free_font(nk_user_font* font) {
  MemFree(font->userdata.ptr);
  MemFree(font);
}

