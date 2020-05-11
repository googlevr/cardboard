/*
 * Copyright 2019 Google LLC. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "distortion_renderer.h"

#include <vector>

#include "screen_params.h"
#include "util/logging.h"

namespace cardboard {

static const char* kDistortionVertexShader =
    R"glsl(
  attribute vec2 aPosition;
  attribute vec2 aTexCoords;
  varying vec2 vTexCoords;
  void main() {
    gl_Position = vec4(aPosition, 0, 1);
    vTexCoords = aTexCoords;
  }
  )glsl";

static const char* kDistortionFragmentShader =
    R"glsl(
  precision mediump float;
  varying vec2 vTexCoords;
  uniform sampler2D uTexture;
  uniform vec2 uStart;
  uniform vec2 uEnd;
  void main() {
    vec2 coords = uStart + vTexCoords * (uEnd - uStart);
    gl_FragColor = texture2D(uTexture, coords);
  }
  )glsl";

void CheckGlError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    CARDBOARD_LOGE("GL error %s: %d", label, gl_error);
  }
}

DistortionRenderer::DistortionRenderer()
    : vertices_vbo_{0, 0},
      uvs_vbo_{0, 0},
      elements_vbo_{0, 0},
      elements_count_{0, 0} {
  program_ = CreateProgram(kDistortionVertexShader, kDistortionFragmentShader);
  attrib_pos_ = glGetAttribLocation(program_, "aPosition");
  attrib_tex_ = glGetAttribLocation(program_, "aTexCoords");
  uniform_start_ = glGetUniformLocation(program_, "uStart");
  uniform_end_ = glGetUniformLocation(program_, "uEnd");

  // Gen buffers, one per eye.
  glGenBuffers(2, &vertices_vbo_[0]);
  glGenBuffers(2, &uvs_vbo_[0]);
  glGenBuffers(2, &elements_vbo_[0]);
  CheckGlError("DistortionRendererSetUp");
}

DistortionRenderer::~DistortionRenderer() {
  glDeleteBuffers(2, &vertices_vbo_[0]);
  glDeleteBuffers(2, &uvs_vbo_[0]);
  glDeleteBuffers(2, &elements_vbo_[0]);
  CheckGlError("~DistortionRenderer");
}

void DistortionRenderer::SetMesh(const CardboardMesh* mesh, CardboardEye eye) {
  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_[eye]);
  glBufferData(
      GL_ARRAY_BUFFER,
      mesh->n_vertices * sizeof(float) * 2,  // Two components per vertex
      mesh->vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo_[eye]);
  glBufferData(GL_ARRAY_BUFFER,
               mesh->n_vertices * sizeof(float) * 2,  // Two components per uv
               mesh->uvs, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_[eye]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->n_indices * sizeof(int),
               mesh->indices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  CheckGlError("SetMesh");
  elements_count_[eye] = mesh->n_indices;
}

void DistortionRenderer::RenderEyeToDisplay(
    int target_display, int x, int y, int width, int height,
    const CardboardEyeTextureDescription* left_eye,
    const CardboardEyeTextureDescription* right_eye) const {
  if (elements_count_[0] == 0 || elements_count_[1] == 0) {
    CARDBOARD_LOGE(
        "Distortion mesh is empty. DistortionRenderer::SetMesh was not called "
        "yet.");
    return;
  }

  glViewport(x, y, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, target_display);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_CULL_FACE);
  glClearColor(.0f, .0f, .0f, 1.0f);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  glUseProgram(program_);

  glEnable(GL_SCISSOR_TEST);
  glScissor(x, y, width / 2, height);
  RenderDistortionMesh(left_eye, kLeft);

  glScissor(x + width / 2, y, width / 2, height);
  RenderDistortionMesh(right_eye, kRight);

  // Active GL_TEXTURE0 effectively enables the first texture that is
  // deactiviated by the DistortionRenderer. Binding array buffer and element
  // array buffer to the reserved value zero effectively unbinds the buffer
  // objects that are previously bound by the DistortionRenderer.
  glActiveTexture(GL_TEXTURE0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Disable scissor test.
  glDisable(GL_SCISSOR_TEST);
  CheckGlError("RenderEyeToDisplay");
}

void DistortionRenderer::RenderDistortionMesh(
    const CardboardEyeTextureDescription* eye_description,
    CardboardEye eye) const {
  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_[eye]);
  glVertexAttribPointer(
      attrib_pos_,
      2,  // 2 components per vertex
      GL_FLOAT, false,
      0,  // Stride and offset 0, as we are using different vbos.
      0);
  glEnableVertexAttribArray(attrib_pos_);

  glBindBuffer(GL_ARRAY_BUFFER, uvs_vbo_[eye]);
  glVertexAttribPointer(attrib_tex_,
                        2,  // 2 components per uv
                        GL_FLOAT, false, 0, 0);
  glEnableVertexAttribArray(attrib_tex_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, eye_description->texture);

  glUniform2f(uniform_start_, eye_description->left_u,
              eye_description->bottom_v);
  glUniform2f(uniform_end_, eye_description->right_u, eye_description->top_v);

  // Draw with indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_vbo_[eye]);
  glDrawElements(GL_TRIANGLE_STRIP, elements_count_[eye], GL_UNSIGNED_INT, 0);
  CheckGlError("RenderDistortionMesh");
}

GLuint DistortionRenderer::LoadShader(GLenum shader_type, const char* source) {
  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  CheckGlError("glCompileShader");
  GLint result = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
  if (result == GL_FALSE) {
    int log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length == 0) {
      return 0;
    }

    std::vector<char> log_string(log_length);
    glGetShaderInfoLog(shader, log_length, nullptr, log_string.data());
    CARDBOARD_LOGE("Could not compile shader of type %d: %s", shader_type,
                   log_string.data());

    shader = 0;
  }

  return shader;
}

GLuint DistortionRenderer::CreateProgram(const char* vertex,
                                         const char* fragment) {
  GLuint vertex_shader = LoadShader(GL_VERTEX_SHADER, vertex);
  if (vertex_shader == 0) {
    return 0;
  }

  GLuint fragment_shader = LoadShader(GL_FRAGMENT_SHADER, fragment);
  if (fragment_shader == 0) {
    return 0;
  }

  GLuint program = glCreateProgram();

  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  CheckGlError("glLinkProgram");

  GLint result = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &result);
  if (result == GL_FALSE) {
    int log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length == 0) {
      return 0;
    }

    std::vector<char> log_string(log_length);
    glGetShaderInfoLog(program, log_length, nullptr, log_string.data());
    CARDBOARD_LOGE("Could not compile program: %s", log_string.data());

    return 0;
  }

  glDetachShader(program, vertex_shader);
  glDetachShader(program, fragment_shader);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  CheckGlError("GlCreateProgram");

  return program;
}

}  // namespace cardboard
