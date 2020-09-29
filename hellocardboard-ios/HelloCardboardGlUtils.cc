/*
 * Copyright 2019 Google LLC
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
#include "HelloCardboardGlUtils.h"

#include <string.h>  // Needed for strtok_r and strstr
#include <unistd.h>

#include <array>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include "HelloCardboardFileUtils.h"

namespace cardboard {
namespace hello_cardboard {
namespace {

/**
 * Loads obj file from resources from the app.
 *
 * This sample uses the .obj format since .obj is straightforward to parse and
 * the sample is intended to be self-contained, but a real application
 * should probably use a library to load a more modern format, such as FBX or
 * glTF.
 *
 * @param fileName name of the obj file.
 * @param out_vertices, output vertices.
 * @param out_normals, output normals.
 * @param out_uv, output texture UV coordinates.
 * @param out_indices, output triangle indices.
 * @return true if obj is loaded correctly, otherwise false.
 */
bool LoadObjFile(const std::string& fileName, std::vector<GLfloat>* outVertices,
                 std::vector<GLfloat>* outNormals, std::vector<GLfloat>* outUv,
                 std::vector<GLushort>* outIndices) {
  std::vector<GLfloat> tempPositions;
  std::vector<GLfloat> tempNormals;
  std::vector<GLfloat> tempUvs;
  std::vector<GLushort> vertexIndices;
  std::vector<GLushort> normalIndices;
  std::vector<GLushort> uvIndices;

  std::string fileBuffer = LoadTextFile(fileName);

  std::stringstream fileStringStream(fileBuffer);

  while (fileStringStream && !fileStringStream.eof()) {
    char lineHeader[128];
    fileStringStream.getline(lineHeader, 128);

    if (lineHeader[0] == '\0') {
      continue;
    } else if (lineHeader[0] == 'v' && lineHeader[1] == 'n') {
      // Parse vertex normal.
      GLfloat normal[3];
      int matches = sscanf(lineHeader, "vn %f %f %f\n", &normal[0], &normal[1],
                           &normal[2]);
      if (matches != 3) {
        std::cout
            << "Format of 'vn float float float' required for each normal line";
        return false;
      }

      tempNormals.push_back(normal[0]);
      tempNormals.push_back(normal[1]);
      tempNormals.push_back(normal[2]);
    } else if (lineHeader[0] == 'v' && lineHeader[1] == 't') {
      // Parse texture uv.
      GLfloat uv[2];
      int matches = sscanf(lineHeader, "vt %f %f\n", &uv[0], &uv[1]);
      if (matches != 2) {
        std::cout
            << "Format of 'vt float float' required for each texture uv line";
        return false;
      }

      tempUvs.push_back(uv[0]);
      tempUvs.push_back(uv[1]);
    } else if (lineHeader[0] == 'v') {
      // Parse vertex.
      GLfloat vertex[3];
      int matches = sscanf(lineHeader, "v %f %f %f\n", &vertex[0], &vertex[1],
                           &vertex[2]);
      if (matches != 3) {
        std::cout
            << "Format of 'v float float float' required for each vertex line";
        return false;
      }

      tempPositions.push_back(vertex[0]);
      tempPositions.push_back(vertex[1]);
      tempPositions.push_back(vertex[2]);
    } else if (lineHeader[0] == 'f') {
      // Actual faces information starts from the second character.
      char* faceLine = &lineHeader[1];

      unsigned int vertexIndex[4];
      unsigned int normalIndex[4];
      unsigned int textureIndex[4];

      std::vector<char*> perVertexInfoList;
      char* perVertexInfoListCStr;
      char* faceLineIter = faceLine;
      while ((perVertexInfoListCStr =
                  strtok_r(faceLineIter, " ", &faceLineIter))) {
        // Divide each faces information into individual positions.
        perVertexInfoList.push_back(perVertexInfoListCStr);
      }

      bool isNormalAvailable = false;
      bool isUvAvailable = false;
      for (int i = 0; i < perVertexInfoList.size(); ++i) {
        char* perVertexInfo;
        int perVertexInfoCount = 0;

        const bool isVertexNormalOnlyFace =
            strstr(perVertexInfoList[i], "//") != nullptr;

        char* perVertexInfoIter = perVertexInfoList[i];
        while ((perVertexInfo =
                    strtok_r(perVertexInfoIter, "/", &perVertexInfoIter))) {
          // write only normal and vert values.
          switch (perVertexInfoCount) {
            case 0:
              // Write to vertex indices.
              vertexIndex[i] = atoi(perVertexInfo);  // NOLINT
              break;
            case 1:
              // Write to texture indices.
              if (isVertexNormalOnlyFace) {
                normalIndex[i] = atoi(perVertexInfo);  // NOLINT
                isNormalAvailable = true;
              } else {
                textureIndex[i] = atoi(perVertexInfo);  // NOLINT
                isUvAvailable = true;
              }
              break;
            case 2:
              // Write to normal indices.
              if (!isVertexNormalOnlyFace) {
                normalIndex[i] = atoi(perVertexInfo);  // NOLINT
                isNormalAvailable = true;
                break;
              }
              [[clang::fallthrough]];
              // Fallthrough to error case because if there's no texture coords,
              // there should only be 2 indices per vertex (position and
              // normal).
            default:
              // Error formatting.
              std::cout << "Format of 'f int/int/int int/int/int int/int/int "
                           "(int/int/int)' "
                           "or 'f int//int int//int int//int (int//int)' "
                           "required for "
                           "each face";
              return false;
          }
          perVertexInfoCount++;
        }
      }

      const int verticesCount = perVertexInfoList.size();
      for (int i = 2; i < verticesCount; ++i) {
        vertexIndices.push_back(vertexIndex[0] - 1);
        vertexIndices.push_back(vertexIndex[i - 1] - 1);
        vertexIndices.push_back(vertexIndex[i] - 1);

        if (isNormalAvailable) {
          normalIndices.push_back(normalIndex[0] - 1);
          normalIndices.push_back(normalIndex[i - 1] - 1);
          normalIndices.push_back(normalIndex[i] - 1);
        }

        if (isUvAvailable) {
          uvIndices.push_back(textureIndex[0] - 1);
          uvIndices.push_back(textureIndex[i - 1] - 1);
          uvIndices.push_back(textureIndex[i] - 1);
        }
      }
    }
  }

  const bool isNormalAvailable = !normalIndices.empty();
  const bool isUvAvailable = !uvIndices.empty();

  if (isNormalAvailable && normalIndices.size() != vertexIndices.size()) {
    std::cout << "Obj normal indices does not equal to vertex indices.";
    return false;
  }

  if (isUvAvailable && uvIndices.size() != vertexIndices.size()) {
    std::cout << "Obj UV indices does not equal to vertex indices.";
    return false;
  }

  for (unsigned int i = 0; i < vertexIndices.size(); i++) {
    unsigned int vertex_index = vertexIndices[i];
    outVertices->push_back(tempPositions[vertex_index * 3]);
    outVertices->push_back(tempPositions[vertex_index * 3 + 1]);
    outVertices->push_back(tempPositions[vertex_index * 3 + 2]);
    outIndices->push_back(i);

    if (isNormalAvailable) {
      unsigned int normal_index = normalIndices[i];
      outNormals->push_back(tempNormals[normal_index * 3]);
      outNormals->push_back(tempNormals[normal_index * 3 + 1]);
      outNormals->push_back(tempNormals[normal_index * 3 + 2]);
    }

    if (isUvAvailable) {
      unsigned int uv_index = uvIndices[i];
      outUv->push_back(tempUvs[uv_index * 2]);
      outUv->push_back(tempUvs[uv_index * 2 + 1]);
    }
  }

  return true;
}

float VectorNorm(const std::array<float, 3>& vec) {
  return std::sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

float VectorDotProduct(const std::array<float, 3>& vec1,
                       const std::array<float, 3>& vec2) {
  float product = 0;
  for (int i = 0; i < 3; i++) {
    product += vec1[i] * vec2[i];
  }
  return product;
}

}  // anonymous namespace

float RandomUniformFloat(float min, float max) {
  return static_cast<float>(static_cast<double>(rand()) /
                            static_cast<double>(RAND_MAX)) *
             (max - min) +
         min;
}

int RandomUniformInt(int max_val) { return rand() % max_val; }

float AngleBetweenVectors(const std::array<float, 3>& vec1,
                          const std::array<float, 3>& vec2) {
  return std::acos(
      std::max(-1.f, std::min(1.f, VectorDotProduct(vec1, vec2) /
                                       (VectorNorm(vec1) * VectorNorm(vec2)))));
}

GLuint LoadGLShader(GLenum type, const char* shader_source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);

  // Get the compilation status.
  GLint compile_status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

  // If the compilation failed, delete the shader and show an error.
  if (compile_status == 0) {
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
    if (info_len == 0) {
      return 0;
    }

    std::vector<char> info_string(info_len);
    glGetShaderInfoLog(shader, info_string.size(), nullptr, info_string.data());
    std::cout << "Could not compile shader of type: " << type << ": "
              << info_string.data();
    glDeleteShader(shader);
    return 0;
  } else {
    return shader;
  }
}

void CheckGLError(const char* label) {
  int gl_error = glGetError();
  if (gl_error != GL_NO_ERROR) {
    std::cout << "GL error @" << label << ": " << gl_error;
  }
  assert(glGetError() == GL_NO_ERROR);
}

TexturedMesh::TexturedMesh()
    : _vertices(), _uv(), _indices(), _positionAttrib(0), _uvAttrib(0) {}

bool TexturedMesh::Initialize(std::string objFilePath, GLuint positionAttrib,
                              GLuint uvAttrib) {
  _positionAttrib = positionAttrib;
  _uvAttrib = uvAttrib;
  // We don't use normals for anything so we discard them.
  std::vector<GLfloat> normals;
  if (!LoadObjFile(objFilePath, &_vertices, &normals, &_uv, &_indices)) {
    return false;
  }
  return true;
}

void TexturedMesh::Draw() const {
  glEnableVertexAttribArray(_positionAttrib);
  glVertexAttribPointer(_positionAttrib, 3, GL_FLOAT, false, 0,
                        _vertices.data());
  glEnableVertexAttribArray(_uvAttrib);
  glVertexAttribPointer(_uvAttrib, 2, GL_FLOAT, false, 0, _uv.data());

  glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT,
                 _indices.data());
}

Texture::Texture() : _textureId(0) {}

Texture::~Texture() {
  if (_textureId != 0) {
    glDeleteTextures(1, &_textureId);
  }
}

bool Texture::Initialize(std::string imageName) {
  glGenTextures(1, &_textureId);
  Bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GLubyte* imageData;
  GLuint width, height;
  LoadPngFile(imageName, &imageData, &width, &height);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, imageData);
  glGenerateMipmap(GL_TEXTURE_2D);
  return true;
}

void Texture::Bind() const {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _textureId);
}

}  // namespace hello_cardboard
}  // namespace cardboard
