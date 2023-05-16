/* Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#import "HelloCardboardRenderer.h"

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <assert.h>
#include <array>
#include <cmath>

#define LOG_TAG "HelloCardboardCPP"
#define LOGW(...) NSLog(@__VA_ARGS__)

namespace cardboard {
namespace hello_cardboard {

namespace {

// The objects are about 1 meter in radius, so the min/max target distance are
// set so that the objects are always within the room (which is about 5 meters
// across) and the reticle is always closer than any objects.
static constexpr float kMinTargetDistance = 2.5f;
static constexpr float kMaxTargetDistance = 3.5f;
static constexpr float kMinTargetHeight = 0.5f;
static constexpr float kMaxTargetHeight = kMinTargetHeight + 3.0f;

static constexpr float kDefaultFloorHeight = -1.7f;

static constexpr uint64_t kPredictionTimeWithoutVsyncNanos = 50000000;

/**
 * Default near clip plane z-axis coordinate.
 */
static constexpr float kZNear = 0.1f;
/**
 * Default far clip plane z-axis coordinate.
 */
static constexpr float kZFar = 100.f;

// Angle threshold for determining whether the controller is pointing at the
// object.
static constexpr float kAngleLimit = 0.2f;

static constexpr int kTargetMeshCount = 3;

// Simple shaders to render .obj files without any lighting.
constexpr const char* kObjVertexShader =
    R"glsl(
    uniform mat4 u_MVP;
    attribute vec4 a_Position;
    attribute vec2 a_UV;
    varying vec2 v_UV;
    void main() {
      v_UV = a_UV;
      gl_Position = u_MVP * a_Position;
    })glsl";

constexpr const char* kObjFragmentShader =
    R"glsl(
    precision mediump float;
    varying vec2 v_UV;
    uniform sampler2D u_Texture;
    void main() {
      // The y coordinate of this sample's textures is reversed compared to
      // what OpenGL expects, so we invert the y coordinate.
      gl_FragColor = texture2D(u_Texture, vec2(v_UV.x, 1.0 - v_UV.y));
    })glsl";

}  // namespace

HelloCardboardRenderer::HelloCardboardRenderer(CardboardLensDistortion* lensDistortion,
                                 CardboardHeadTracker* headTracker, int width, int height)
    : _targetObjectMeshes(kTargetMeshCount),
      _targetObjectNotSelectedTextures(kTargetMeshCount),
      _targetObjectSelectedTextures(kTargetMeshCount),
      _curTargetObject(RandomUniformInt(kTargetMeshCount)),
      _objProgram(0),
      _objPositionParam(0),
      _objUvParam(0),
      _objModelviewProjectionParam(0),
      _lensDistortion(lensDistortion),
      _headTracker(headTracker),
      _distortionRenderer(nil),
      _width(width),
      _height(height),
      _depthRenderBuffer(0),
      _eyeTexture(0),
      _fbo(0) {
  srand(2);
}

HelloCardboardRenderer::~HelloCardboardRenderer() {}

void HelloCardboardRenderer::InitializeGl() {
  const int objVertexShader = LoadGLShader(GL_VERTEX_SHADER, kObjVertexShader);
  const int objFragmentShader = LoadGLShader(GL_FRAGMENT_SHADER, kObjFragmentShader);

  _objProgram = glCreateProgram();
  glAttachShader(_objProgram, objVertexShader);
  glAttachShader(_objProgram, objFragmentShader);
  glLinkProgram(_objProgram);
  glUseProgram(_objProgram);
  CheckGLError("Obj program");

  _objPositionParam = glGetAttribLocation(_objProgram, "a_Position");
  _objUvParam = glGetAttribLocation(_objProgram, "a_UV");
  _objModelviewProjectionParam = glGetUniformLocation(_objProgram, "u_MVP");

  _room.Initialize("HelloCardboard.bundle/CubeRoom", _objPositionParam, _objUvParam);
  _roomTex.Initialize("HelloCardboard.bundle/CubeRoom_BakedDiffuse");
  _targetObjectMeshes[0].Initialize("HelloCardboard.bundle/Icosahedron", _objPositionParam, _objUvParam);
  _targetObjectNotSelectedTextures[0].Initialize("HelloCardboard.bundle/Icosahedron_Blue_BakedDiffuse");
  _targetObjectSelectedTextures[0].Initialize("HelloCardboard.bundle/Icosahedron_Pink_BakedDiffuse");
  _targetObjectMeshes[1].Initialize("HelloCardboard.bundle/QuadSphere", _objPositionParam, _objUvParam);
  _targetObjectNotSelectedTextures[1].Initialize("HelloCardboard.bundle/QuadSphere_Blue_BakedDiffuse");
  _targetObjectSelectedTextures[1].Initialize("HelloCardboard.bundle/QuadSphere_Pink_BakedDiffuse");
  _targetObjectMeshes[2].Initialize("HelloCardboard.bundle/TriSphere", _objPositionParam, _objUvParam);
  _targetObjectNotSelectedTextures[2].Initialize("HelloCardboard.bundle/TriSphere_Blue_BakedDiffuse");
  _targetObjectSelectedTextures[2].Initialize("HelloCardboard.bundle/TriSphere_Pink_BakedDiffuse");

  // Object first appears directly in front of user.
  _modelTarget = GLKMatrix4MakeTranslation(0.0f, 1.5f, -kMinTargetDistance);

  // Generate texture to render left and right eyes.
  glGenTextures(1, &_eyeTexture);
  glBindTexture(GL_TEXTURE_2D, _eyeTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

  _leftEyeTexture.texture = _eyeTexture;
  _leftEyeTexture.left_u = 0;
  _leftEyeTexture.right_u = 0.5;
  _leftEyeTexture.top_v = 1;
  _leftEyeTexture.bottom_v = 0;

  _rightEyeTexture.texture = _eyeTexture;
  _rightEyeTexture.left_u = 0.5;
  _rightEyeTexture.right_u = 1;
  _rightEyeTexture.top_v = 1;
  _rightEyeTexture.bottom_v = 0;
  CheckGLError("Create Eye textures");

  // Generate depth buffer to perform depth test.
  glGenRenderbuffers(1, &_depthRenderBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _width, _height);
  CheckGLError("Create Render buffer");

  glGenFramebuffers(1, &_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _eyeTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                            _depthRenderBuffer);

  CheckGLError("Create Frame buffer");

  CardboardLensDistortion_getEyeFromHeadMatrix(_lensDistortion, kLeft, _eyeMatrices[kLeft]);
  CardboardLensDistortion_getEyeFromHeadMatrix(_lensDistortion, kRight, _eyeMatrices[kRight]);
  CardboardLensDistortion_getProjectionMatrix(_lensDistortion, kLeft, kZNear, kZFar,
                                              _projMatrices[kLeft]);
  CardboardLensDistortion_getProjectionMatrix(_lensDistortion, kRight, kZNear, kZFar,
                                              _projMatrices[kRight]);

  CardboardMesh leftMesh;
  CardboardMesh rightMesh;
  CardboardLensDistortion_getDistortionMesh(_lensDistortion, kLeft, &leftMesh);
  CardboardLensDistortion_getDistortionMesh(_lensDistortion, kRight, &rightMesh);

  const CardboardOpenGlEsDistortionRendererConfig config{kGlTexture2D};
  _distortionRenderer = CardboardOpenGlEs2DistortionRenderer_create(&config);
  CardboardDistortionRenderer_setMesh(_distortionRenderer, &leftMesh, kLeft);
  CardboardDistortionRenderer_setMesh(_distortionRenderer, &rightMesh, kRight);
  CheckGLError("Cardboard distortion renderer set up");
}

void HelloCardboardRenderer::DrawFrame() {
  // Update Head Pose.
  _headView = GetPose();

  // Incorporate the floor height into the head_view
  _headView =
      GLKMatrix4Multiply(_headView, GLKMatrix4MakeTranslation(0.0f, kDefaultFloorHeight, 0.0f));

  GLKMatrix4 _leftEyeViewPose =
      GLKMatrix4Multiply(GLKMatrix4MakeWithArray(_eyeMatrices[kLeft]), _headView);
  GLKMatrix4 _rightEyeViewPose =
      GLKMatrix4Multiply(GLKMatrix4MakeWithArray(_eyeMatrices[kRight]), _headView);

  // Get current target.
  GLint renderTarget = 0;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &renderTarget);

  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Draw left eye.
  glViewport(0, 0, _width / 2.0, _height);
  glScissor(0, 0, _width / 2.0, _height);
  DrawWorld(_leftEyeViewPose, GLKMatrix4MakeWithArray(_projMatrices[kLeft]));

  // Draw right eye.
  glViewport(_width / 2.0, 0, _width / 2.0, _height);
  glScissor(_width / 2.0, 0, _width / 2.0, _height);
  DrawWorld(_rightEyeViewPose, GLKMatrix4MakeWithArray(_projMatrices[kRight]));

  // Draw cardboard.
  CardboardDistortionRenderer_renderEyeToDisplay(_distortionRenderer, renderTarget, /*x=*/0,
                                                 /*y=*/0, _width, _height, &_leftEyeTexture,
                                                 &_rightEyeTexture);
  CheckGLError("onDrawFrame");
}

void HelloCardboardRenderer::OnTriggerEvent() {
  if (IsPointingAtTarget()) {
    HideTarget();
  }
}

GLKMatrix4 HelloCardboardRenderer::GetPose() {
  float outPosition[3];
  float outOrientation[4];
  CardboardHeadTracker_getPose(
      _headTracker, clock_gettime_nsec_np(CLOCK_UPTIME_RAW) + kPredictionTimeWithoutVsyncNanos,
      kLandscapeLeft, outPosition, outOrientation);
  return GLKMatrix4Multiply(
      GLKMatrix4MakeTranslation(outPosition[0], outPosition[1], outPosition[2]),
      GLKMatrix4MakeWithQuaternion(GLKQuaternionMakeWithArray(outOrientation)));
}

/**
 * Draws a frame for an eye.
 */
void HelloCardboardRenderer::DrawWorld(const GLKMatrix4 viewMatrix, const GLKMatrix4 projectionMatrix) {
  glClearColor(0.1f, 0.1f, 0.1f, 0.5f);  // Dark background so text shows up.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _modelview = GLKMatrix4Multiply(viewMatrix, _modelTarget);
  _modelviewProjectionTarget = GLKMatrix4Multiply(projectionMatrix, _modelview);
  DrawTarget();

  _modelviewProjectionRoom = GLKMatrix4Multiply(projectionMatrix, viewMatrix);
  DrawRoom();
}

void HelloCardboardRenderer::DrawTarget() {
  glUseProgram(_objProgram);

  glUniformMatrix4fv(_objModelviewProjectionParam, 1, GL_FALSE, _modelviewProjectionTarget.m);

  if (IsPointingAtTarget()) {
    _targetObjectSelectedTextures[_curTargetObject].Bind();
  } else {
    _targetObjectNotSelectedTextures[_curTargetObject].Bind();
  }
  _targetObjectMeshes[_curTargetObject].Draw();

  CheckGLError("Drawing target object");
}

void HelloCardboardRenderer::DrawRoom() {
  glUseProgram(_objProgram);

  glUniformMatrix4fv(_objModelviewProjectionParam, 1, GL_FALSE, _modelviewProjectionRoom.m);

  _roomTex.Bind();
  _room.Draw();
  CheckGLError("Drawing room");
}

void HelloCardboardRenderer::HideTarget() {
  _curTargetObject = RandomUniformInt(kTargetMeshCount);

  float angle = RandomUniformFloat(-M_PI, M_PI);
  float distance = RandomUniformFloat(kMinTargetDistance, kMaxTargetDistance);
  float height = RandomUniformFloat(kMinTargetHeight, kMaxTargetHeight);
  GLKVector3 targetPosition = {{std::cos(angle) * distance, height, std::sin(angle) * distance}};

  _modelTarget =
      GLKMatrix4MakeTranslation(targetPosition.v[0], targetPosition.v[1], targetPosition.v[2]);
}

bool HelloCardboardRenderer::IsPointingAtTarget() {
  // Compute vectors pointing towards and towards the target object
  // in head space.
  std::array<float, 3> pointVector = {0.0f, 0.0f, -1.0f};
  GLKMatrix4 headFromTarget = GLKMatrix4Multiply(_headView, _modelTarget);
  GLKVector4 targetVector = GLKMatrix4MultiplyVector4(headFromTarget, {{0.f, 0.f, 0.f, 1.f}});
  std::array<float, 3> targetVectorArray = {targetVector.v[0], targetVector.v[1],
                                            targetVector.v[2]};

  float angle = AngleBetweenVectors(pointVector, targetVectorArray);
  return angle < kAngleLimit;
}

}  // namespace hello_cardboard
}  // namespace cardboard
