#ifndef PAPERSCOPE_LAUNCHER_SRC_TEXT_RENDERER_H_
#define PAPERSCOPE_LAUNCHER_SRC_TEXT_RENDERER_H_

#include <android/asset_manager.h>
#include <ft2build.h>
#include <freetype/freetype.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <GLES2/gl2.h>
#include "util.h"

namespace cardboard_demo {

// Holds state information for a single character glyph.
struct Character {
  GLuint texture_id{};  // ID handle of the glyph texture
  uint32_t width{};
  uint32_t height{};
  int32_t bearing_x{};
  int32_t bearing_y{};
  int64_t advance{};
};

// Renders text using OpenGL and FreeType.
class TextRenderer {
 public:
  TextRenderer();
  ~TextRenderer();

  // Initializes shaders and OpenGL buffers. Must be called on the GL thread.
  void Init();

  // Loads a font from the given asset manager.
  // The font data is copied and managed by this class.
  bool LoadFont(AAssetManager* asset_mgr, const std::string& font_path);

  // Renders a line of text.
  void RenderText(const std::string& text, cardboard_demo::Matrix4x4 mvp,
                  float spacing = 1.0f, float x_offset = 0.0f,
                  float y_offset = 0.0f);

  /**
   * Returns the width of the text in pixels, given the text and spacing.
   *
   * @param text The text to measure.
   * @param spacing The spacing between characters.
   * @return The width of the text in pixels.
   */
  float GetTextWidth(const std::string& text, float spacing = 1.0f);

  /**
   * Sets the text color.
   *
   * @param r The red component of the color from 0.0f to 1.0f.
   * @param g The green component of the color from 0.0f to 1.0f.
   * @param b The blue component of the color from 0.0f to 1.0f.
   */
  void SetTextColor(float r, float g, float b);

 private:
  // Saves the current OpenGL state.
  void SaveGLState();
  // Restores the OpenGL state to its previous state.
  void RestoreGLState() const;

  /**
   * Loads a glyph for the given codepoint.
   *
   * @param codepoint The codepoint of the glyph to load.
   * @return True if the glyph was loaded successfully, false otherwise.
   */
  void LoadGlyph(uint32_t codepoint);

  /**
   * Loads glyphs for the given string.
   *
   * @param utf32_text The UTF-32 string to load glyphs for.
   */
  void LoadGlypsForString(const std::u32string& utf32_text);

  /**
   * Reloads characters. Needed after surface is created.
   * This happens when blocking the phone or when the user sends the app to the background.
   */
  void ReloadCharacters();

  /**
   * Creates a texture for the given glyph.
   *
   * @return The texture ID of the glyph.
   */
  GLuint CreateTextureForGlyph() const;

  FT_Library ft_library_;
  FT_Face ft_face_;
  std::vector<FT_Byte> font_data_;  // To keep font data in memory.
  std::unordered_map<uint32_t, Character> characters_;

  GLuint vbo_{};
  GLuint shader_program_{};
  GLint mvp_param_{};
  GLint text_color_param_{};
  GLint vertex_param_{};

  GLint last_program_{};
  GLboolean last_blend_enabled_{};
  GLint last_blend_src_{};
  GLint last_blend_dst_{};
  GLboolean last_cull_face_enabled_{};
  GLboolean last_depth_test_enabled_{};

  std::array<float, 3> text_color_ {1.0f, 1.0f, 1.0f};
};

}  // namespace cardboard_demo

#endif  // PAPERSCOPE_LAUNCHER_SRC_TEXT_RENDERER_H_
