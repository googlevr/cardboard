#include "text_renderer.h"

#include <android/asset_manager.h>
#include <android/log.h>

#include "util.h"

namespace cardboard_demo {
namespace {

constexpr const char* kTextVertexShader = R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Vertex; // vec4(pos.x, pos.y, tex.x, tex.y)
    varying vec2 v_TexCoords;

    void main() {
        gl_Position = u_MVP * vec4(a_Vertex.xy, 0.0, 1.0);
        v_TexCoords = a_Vertex.zw;
    })glsl";

constexpr const char* kTextFragmentShader = R"glsl(
    precision mediump float;
    varying vec2 v_TexCoords;
    uniform sampler2D u_Texture;
    uniform vec3 u_TextColor;

    void main() {
        // Use the red channel of the texture (luminance) as alpha.
        float alpha = texture2D(u_Texture, v_TexCoords).r;
        gl_FragColor = vec4(u_TextColor, alpha);
    })glsl";

// Converts a UTF-8 encoded string to a UTF-32 encoded string.
std::u32string Utf8ToUtf32(const std::string& s) {
  std::u32string result;
  if (s.empty()) {
    return result;
  }
  result.reserve(s.length());

  for (size_t i = 0; i < s.length();) {
    char32_t code_point;
    const unsigned char c = s[i];

    if (c < 0x80) {  // 1-byte sequence
      code_point = c;
      i += 1;
    } else if ((c & 0xE0) == 0xC0) {  // 2-byte sequence
      if (i + 1 >= s.length()) break;
      code_point = ((c & 0x1F) << 6) | (s[i + 1] & 0x3F);
      i += 2;
    } else if ((c & 0xF0) == 0xE0) {  // 3-byte sequence
      if (i + 2 >= s.length()) break;
      code_point =
          ((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F);
      i += 3;
    } else if ((c & 0xF8) == 0xF0) {  // 4-byte sequence
      if (i + 3 >= s.length()) break;
      code_point = ((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) |
                   ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F);
      i += 4;
    } else {
      // Invalid UTF-8 sequence, skip this byte.
      i += 1;
      continue;
    }
    result.push_back(code_point);
  }
  return result;
}
}  // namespace

TextRenderer::TextRenderer()
    : ft_library_{nullptr},
      ft_face_{nullptr},
      vbo_{0},
      shader_program_{0},
      mvp_param_{-1},
      text_color_param_{-1},
      vertex_param_{-1} {
  if (FT_Init_FreeType(&ft_library_)) {
    __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "Could not initialize FreeType library");
  }
}

TextRenderer::~TextRenderer() {
  if (ft_face_) {
    FT_Done_Face(ft_face_);
  }
  if (ft_library_) {
    FT_Done_FreeType(ft_library_);
  }

  for (auto const& [codepoint, character] : characters_) {
    glDeleteTextures(1, &character.texture_id);
  }
}

void TextRenderer::Init() {
  const GLuint vertex_shader{
      cardboard_demo::LoadGLShader(GL_VERTEX_SHADER, kTextVertexShader)};
  const GLuint fragment_shader{
      cardboard_demo::LoadGLShader(GL_FRAGMENT_SHADER, kTextFragmentShader)};

  shader_program_ = glCreateProgram();
  glAttachShader(shader_program_, vertex_shader);
  glAttachShader(shader_program_, fragment_shader);
  glLinkProgram(shader_program_);

  mvp_param_ = glGetUniformLocation(shader_program_, "u_MVP");
  text_color_param_ = glGetUniformLocation(shader_program_, "u_TextColor");
  vertex_param_ = glGetAttribLocation(shader_program_, "a_Vertex");

  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr,
               GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Invalidate all textures in case of GL context loss.
  for (auto& character : characters_) {
    character.second.texture_id = 0;
  }
  cardboard_demo::CHECKGLERROR("TextRenderer::Init");
}

bool TextRenderer::LoadFont(AAssetManager* asset_mgr,
                            const std::string& font_path) {
  if (asset_mgr == nullptr) {
    __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "Asset manager is null");
    return false;
  }
  if (!ft_library_) {
    __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "FreeType library is not initialized");
    return false;
  }

  AAsset* asset{
      AAssetManager_open(asset_mgr, font_path.c_str(), AASSET_MODE_BUFFER)};
  if (!asset) {
    __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "Could not open font asset");
    return false;
  }

  const void* font_buffer{AAsset_getBuffer(asset)};
  const auto font_buffer_size{static_cast<size_t>(AAsset_getLength(asset))};

  font_data_.assign(
      static_cast<const FT_Byte*>(font_buffer),
      static_cast<const FT_Byte*>(font_buffer) + font_buffer_size);

  AAsset_close(asset);

  if (FT_New_Memory_Face(ft_library_, font_data_.data(), font_data_.size(), 0,
                         &ft_face_)) {
        __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "Could not create FreeType face");
    return false;
  }

  FT_Set_Pixel_Sizes(ft_face_, 0, 48);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (!characters_.empty()) {
    ReloadCharacters();
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  cardboard_demo::CHECKGLERROR("TextRenderer::LoadFont");

  return true;
}

void TextRenderer::RenderText(const std::string& text,
                              cardboard_demo::Matrix4x4 mvp, float spacing,
                              float x_offset, float y_offset) {
  const std::u32string utf32_text {Utf8ToUtf32(text)};
  LoadGlypsForString(utf32_text);

  // Save the current GL state that will be changed.
  SaveGLState();

  glUseProgram(shader_program_);
  glUniformMatrix4fv(mvp_param_, 1, GL_FALSE, mvp.ToGlArray().data());
  glUniform3f(text_color_param_,
              text_color_[0],
              text_color_[1],
              text_color_[2]);

  glActiveTexture(GL_TEXTURE0);

  // Set GL state for text rendering.
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glEnableVertexAttribArray(vertex_param_);
  glVertexAttribPointer(vertex_param_, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        nullptr);

  float x{x_offset};
  float y{y_offset * (ft_face_->size->metrics.height >> 6)};
  for (uint32_t c : utf32_text) {
    if (c == '\n') {
      x = x_offset;
      y -= (ft_face_->size->metrics.height >> 6);
      continue;
    }

    const auto it = characters_.find(c);
    if (it == characters_.end()) {
      __android_log_print(ANDROID_LOG_ERROR,
                          "TextRenderer",
                          "Glyph %d not found", c);
      continue;
    }
    const Character& ch = it->second;

    const float xpos{x + ch.bearing_x};
    const float ypos{y - (ch.height - ch.bearing_y)};
    const auto w{static_cast<float>(ch.width)};
    const auto h{static_cast<float>(ch.height)};

    const float vertices[6][4] = {                             // First triangle
                                  {xpos, ypos + h, 0.0, 0.0},  // top left
                                  {xpos, ypos, 0.0, 1.0},      // bottom left
                                  {xpos + w, ypos, 1.0, 1.0},  // bottom right

                                  // Second triangle
                                  {xpos, ypos + h, 0.0, 0.0},  // top right
                                  {xpos + w, ypos, 1.0, 1.0},  // bottom right
                                  {xpos + w, ypos + h, 1.0, 0.0}};  // top right

    glBindTexture(GL_TEXTURE_2D, ch.texture_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    x += (ch.advance >> 6) * spacing;
  }

  glDisableVertexAttribArray(vertex_param_);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  RestoreGLState();
}

void TextRenderer::SaveGLState() {
  glGetIntegerv(GL_CURRENT_PROGRAM, &last_program_);
  glGetBooleanv(GL_BLEND, &last_blend_enabled_);
  glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_);
  glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_);
  glGetBooleanv(GL_CULL_FACE, &last_cull_face_enabled_);
  glGetBooleanv(GL_DEPTH_TEST, &last_depth_test_enabled_);
}

void TextRenderer::RestoreGLState() const {
  glUseProgram(last_program_);
  if (last_blend_enabled_) {
    glEnable(GL_BLEND);
    glBlendFunc(last_blend_src_, last_blend_dst_);
  } else {
    glDisable(GL_BLEND);
  }
  if (last_cull_face_enabled_) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
  if (last_depth_test_enabled_) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}

float TextRenderer::GetTextWidth(const std::string& text, float spacing) {
  float width {0.0f};

  const std::u32string utf32_text = Utf8ToUtf32(text);
  LoadGlypsForString(utf32_text);
  for (uint32_t codepoint : utf32_text) {
    const auto it = characters_.find(codepoint);
    if (it == characters_.end()) {
      __android_log_print(ANDROID_LOG_ERROR,
                          "TextRenderer",
                          "Glyph %d not found", codepoint);
      continue;
    }

    const Character& ch {it->second};
    width += (ch.advance >> 6) * spacing;
  }
  return width;
}

void TextRenderer::SetTextColor(float r, float g, float b) {
  text_color_ = {r, g, b};
}

void TextRenderer::LoadGlypsForString(const std::u32string& utf32_text) {
  for (auto codepoint : utf32_text) {
    LoadGlyph(codepoint);
  }
}

void TextRenderer::LoadGlyph(uint32_t codepoint) {
  const auto it = characters_.find(codepoint);
  if (it != characters_.end() && it->second.texture_id != 0 &&
      glIsTexture(it->second.texture_id)) {
    return;
  }

  GLuint old_texture_id = 0;
  if (it != characters_.end()) {
    old_texture_id = it->second.texture_id;
  }

  // Otherwise load from FreeType
  if (FT_Load_Char(ft_face_, codepoint, FT_LOAD_RENDER)) {
    __android_log_print(ANDROID_LOG_ERROR, "TextRenderer",
                        "Failed to load glyph %d",
                        codepoint);
    return;
  }

  const GLuint texture {CreateTextureForGlyph()};

  if (old_texture_id != 0 && glIsTexture(old_texture_id)) {
    glDeleteTextures(1, &old_texture_id);
  }

  characters_[codepoint] = {texture,
    ft_face_->glyph->bitmap.width,
    ft_face_->glyph->bitmap.rows,
    ft_face_->glyph->bitmap_left,
    ft_face_->glyph->bitmap_top,
    ft_face_->glyph->advance.x};
}

void TextRenderer::ReloadCharacters() {
  for (auto &[codepoint, character] : characters_) {
    if (FT_Load_Char(ft_face_, codepoint, FT_LOAD_RENDER)) {
      continue;
    }
    character.texture_id = CreateTextureForGlyph();
  }
}

GLuint TextRenderer::CreateTextureForGlyph() const {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, ft_face_->glyph->bitmap.width,
                 ft_face_->glyph->bitmap.rows, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, ft_face_->glyph->bitmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

}  // namespace cardboard_demo
