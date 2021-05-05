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
#import <GLKit/GLKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#include <memory>
#include <string>
#include <thread>  // NOLINT
#include <vector>

#import "HelloCardboardGlUtils.h"
#include "cardboard.h"

namespace cardboard {
namespace hello_cardboard {

class HelloCardboardRenderer {
 public:
  /**
   * Create a HelloCardboardRenderer using a given |lensDistortion|,
   * |headTracker| and screen |width| and |height| .
   *
   */
  HelloCardboardRenderer(CardboardLensDistortion* lensDistortion,
                  CardboardHeadTracker* headTracker, int width, int height);

  /**
   * Destructor.
   */
  ~HelloCardboardRenderer();

  /**
   * Initialize any GL-related objects. This should be called on the rendering
   * thread with a valid GL context.
   */
  void InitializeGl();

  /**
   * Draw the HelloCardboard scene. This should be called on the rendering thread.
   */
  void DrawFrame();

  /**
   * Hides the target object if it's being targeted.
   */
  void OnTriggerEvent();

 private:
  int CreateTexture(int width, int height, int textureFormat, int textureType);

  /**
   * Gets head's pose as a 4x4 matrix.
   *
   * @return matrix containing head's pose.
   */
  GLKMatrix4 GetPose();

  /**
   * Draws all world-space objects for an eye.
   *
   * @param viewMatrix View matrix for an eye.
   * @param projectionMatrix Projection matrix for an eye.
   */
  void DrawWorld(const GLKMatrix4 viewMatrix,
                 const GLKMatrix4 projectionMatrix);

  /**
   * Draws the target object.
   *
   * We've set all of our transformation matrices. Now we simply pass them
   * into the shader.
   */
  void DrawTarget();

  /**
   * Draws the room.
   */
  void DrawRoom();

  /**
   * Finds a new random position for the target object.
   */
  void HideTarget();

  /**
   * Checks if user is looking at the target object by calculating
   * whether the angle between the user's gaze orientation and the
   * vector pointing towards the object is lower than some threshold.
   *
   * @return true if the user is pointing at the target object.
   */
  bool IsPointingAtTarget();

  TexturedMesh _room;
  Texture _roomTex;

  std::vector<TexturedMesh> _targetObjectMeshes;
  std::vector<Texture> _targetObjectNotSelectedTextures;
  std::vector<Texture> _targetObjectSelectedTextures;
  int _curTargetObject;

  int _objProgram;
  int _objPositionParam;
  int _objUvParam;
  int _objModelviewProjectionParam;

  GLKMatrix4 _modelTarget;
  GLKMatrix4 _modelviewProjectionTarget;
  GLKMatrix4 _modelviewProjectionRoom;

  GLKMatrix4 _headView;
  GLKMatrix4 _modelview;

  CardboardLensDistortion* _lensDistortion;
  CardboardHeadTracker* _headTracker;
  CardboardDistortionRenderer* _distortionRenderer;

  float _projMatrices[2][16];
  float _eyeMatrices[2][16];

  int _width;
  int _height;

  GLuint _depthRenderBuffer;
  GLuint _eyeTexture;
  GLuint _fbo;

  CardboardEyeTextureDescription _leftEyeTexture;
  CardboardEyeTextureDescription _rightEyeTexture;
};

}  // namespace hello_cardboard
}  // namespace cardboard
