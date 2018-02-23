// Copyright (C) 2014 The Regents of the University of California (Regents).
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of The Regents or University of California nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Please contact the author of this library if you have any questions.
// Author: Chris Sweeney (cmsweeney@cs.ucsb.edu)

#include <Eigen/Core>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <theia/theia.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
//////////////////////////
#include "theia/util/util.h"
#include "theia/sfm/reconstruction.h"
#include <ceres/rotation.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#ifdef FREEGLUT
#include <GL/freeglut.h>
#else  // FREEGLUT
#include <GLUT/glut.h>
#endif  // FREEGLUT
#else  // __APPLE__
#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#include <GL/glut.h>
#else  // _WIN32
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif  // _WIN32
#endif  // __APPLE__

DEFINE_string(camera_poses, "", "camera_poses file (.txt) to be viewed.");


double Median(std::vector<double>* data) {
  int n = data->size();
  std::vector<double>::iterator mid_point = data->begin() + n / 2;
  std::nth_element(data->begin(), mid_point, data->end());
  return *mid_point;
}


// Containers for the data.
std::vector<theia::Camera> cameras;
std::vector<Eigen::Vector3d> world_points;
std::vector<Eigen::Vector3f> point_colors;
std::vector<int> num_views_for_track;

// Parameters for OpenGL.
int width = 1200;
int height = 800;

// OpenGL camera parameters.
Eigen::Vector3f viewer_position(0.0, 0.0, 0.0);
float zoom = -50.0;
float delta_zoom = 1.1;

// Rotation values for the navigation
Eigen::Vector2f navigation_rotation(0.0, 0.0);

// Position of the mouse when pressed
int mouse_pressed_x = 0, mouse_pressed_y = 0;
float last_x_offset = 0.0, last_y_offset = 0.0;
// Mouse button states
int left_mouse_button_active = 0, right_mouse_button_active = 0;

// Visualization parameters.
bool draw_cameras = true;
bool draw_axes = true;  //false
float point_size = 1.0;
float normalized_focal_length = 1.0;
int min_num_views_for_track = 3;
double anti_aliasing_blend = 0.01;

void GetPerspectiveParams(double* aspect_ratio, double* fovy) {
  double focal_length = 800.0;
  *aspect_ratio = static_cast<double>(width) / static_cast<double>(height);
  *fovy = 2 * atan(height / (2.0 * focal_length)) * 180.0 / M_PI;
}

void ChangeSize(int w, int h) {
  // Prevent a divide by zero, when window is too short
  // (you cant make a window of zero width).
  if (h == 0) h = 1;

  width = w;
  height = h;

  // Use the Projection Matrix
  glMatrixMode(GL_PROJECTION);

  // Reset Matrix
  glLoadIdentity();

  // Set the viewport to be the entire window
  double aspect_ratio, fovy;
  GetPerspectiveParams(&aspect_ratio, &fovy);
  glViewport(0, 0, w, h);

  // Set the correct perspective.
  gluPerspective(fovy, aspect_ratio, 0.001f, 100000.0f);

  // Get Back to the Reconstructionview
  glMatrixMode(GL_MODELVIEW);
}

void DrawAxes(float length) {
  glPushAttrib(GL_POLYGON_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDisable(GL_LIGHTING);
  glLineWidth(5.0);
  glBegin(GL_LINES);
  glColor3f(1.0, 0.0, 0.0);
  glVertex3f(0, 0, 0);
  glVertex3f(length, 0, 0);

  glColor3f(0.0, 1.0, 0.0);
  glVertex3f(0, 0, 0);
  glVertex3f(0, length, 0);

  glColor3f(0.0, 0.0, 1.0);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, length);
  glEnd();

  glPopAttrib();
  glLineWidth(1.0);
}

void DrawCamera(const theia::Camera& camera) {
  glPushMatrix();
  Eigen::Matrix4d transformation_matrix = Eigen::Matrix4d::Zero();
  transformation_matrix.block<3, 3>(0, 0) =
      camera.GetOrientationAsRotationMatrix().transpose();
  transformation_matrix.col(3).head<3>() = camera.GetPosition();
  transformation_matrix(3, 3) = 1.0;

  // Apply world pose transformation.
  glMultMatrixd(reinterpret_cast<GLdouble*>(transformation_matrix.data()));

  // Draw Cameras.
  glColor3f(1.0, 0.0, 0.0);

  // Create the camera wireframe. If intrinsic parameters are not set then use
  // the focal length as a guess.
  const float image_width =
      (camera.ImageWidth() == 0) ? camera.FocalLength() : camera.ImageWidth();
  const float image_height =
      (camera.ImageHeight() == 0) ? camera.FocalLength() : camera.ImageHeight();
  const float normalized_width = (image_width / 2.0) / camera.FocalLength();
  const float normalized_height = (image_height / 2.0) / camera.FocalLength();

  const Eigen::Vector3f top_left =
      normalized_focal_length *
      Eigen::Vector3f(-normalized_width, -normalized_height, 1);
  const Eigen::Vector3f top_right =
      normalized_focal_length *
      Eigen::Vector3f(normalized_width, -normalized_height, 1);
  const Eigen::Vector3f bottom_right =
      normalized_focal_length *
      Eigen::Vector3f(normalized_width, normalized_height, 1);
  const Eigen::Vector3f bottom_left =
      normalized_focal_length *
      Eigen::Vector3f(-normalized_width, normalized_height, 1);

  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(top_right[0], top_right[1], top_right[2]);
  glVertex3f(top_left[0], top_left[1], top_left[2]);
  glVertex3f(bottom_left[0], bottom_left[1], bottom_left[2]);
  glVertex3f(bottom_right[0], bottom_right[1], bottom_right[2]);
  glVertex3f(top_right[0], top_right[1], top_right[2]);
  glEnd();
  glPopMatrix();
}

void DrawPoints(const float point_scale,
                const float color_scale,
                const float alpha_scale) {
  const float default_point_size = point_size;
  const float default_alpha_scale = anti_aliasing_blend;

  // TODO(cmsweeney): Render points with the actual 3D point color! This would
  // require Theia to save the colors during feature extraction.
  //const Eigen::Vector3f default_color(0.05, 0.05, 0.05);

  // Enable anti-aliasing for round points and alpha blending that helps make
  // points look nicer.
  glDisable(GL_LIGHTING);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_BLEND);
  glEnable(GL_POINT_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // The coordinates for calculating point attenuation. This allows for points
  // to get smaller as the OpenGL camera moves farther away.
  GLfloat point_size_coords[3];
  point_size_coords[0] = 1.0f;
  point_size_coords[1] = 0.055f;
  point_size_coords[2] = 0.0f;
  glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, point_size_coords);


  glPointSize(point_scale * default_point_size);
  glBegin(GL_POINTS);
  for (int i = 0; i < world_points.size(); i++) {
    if (num_views_for_track[i] < min_num_views_for_track) {
      continue;
    }
    const Eigen::Vector3f color = point_colors[i] / 255.0;
    glColor4f(color_scale * color[0],
              color_scale * color[1],
              color_scale * color[2],
              alpha_scale * default_alpha_scale);

    glVertex3d(world_points[i].x(), world_points[i].y(), world_points[i].z());
  }
  glEnd();
}

void RenderScene() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Transformation to the viewer origin.
  glTranslatef(0.0, 0.0, zoom);
  glRotatef(navigation_rotation[0], 1.0f, 0.0f, 0.0f);
  glRotatef(navigation_rotation[1], 0.0f, 1.0f, 0.0f);
  if (draw_axes) {
    DrawAxes(10.0);
  }

  // Transformation from the viewer origin to the reconstruction origin.
  glTranslatef(viewer_position[0], viewer_position[1], viewer_position[2]);

  glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

  // Each 3D point is rendered 3 times with different point sizes, color
  // intensity, and alpha blending. This allows for a more complete texture-like
  // rendering of the 3D points. These values were found to experimentally
  // produce nice visualizations on most scenes.
  const float small_point_scale = 1.0, medium_point_scale = 5.0,
              large_point_scale = 10.0;
  const float small_color_scale = 1.0, medium_color_scale = 1.2,
              large_color_scale = 1.5;
  const float small_alpha_scale = 1.0, medium_alpha_scale = 2.1,
              large_alpha_scale = 3.3;

  DrawPoints(small_point_scale, small_color_scale, small_alpha_scale);
  DrawPoints(medium_point_scale, medium_color_scale, medium_alpha_scale);
  DrawPoints(large_point_scale, large_color_scale, large_alpha_scale);

  // Draw the cameras.
  if (draw_cameras) {
    for (int i = 0; i < cameras.size(); i++) {
      DrawCamera(cameras[i]);
    }
  }

  glutSwapBuffers();
}

void MouseButton(int button, int state, int x, int y) {
  // get the mouse buttons
  if (button == GLUT_RIGHT_BUTTON) {
    if (state == GLUT_DOWN) {
      right_mouse_button_active += 1;
    } else {
      right_mouse_button_active -= 1;
    }
  } else if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      left_mouse_button_active += 1;
      last_x_offset = 0.0;
      last_y_offset = 0.0;
    } else {
      left_mouse_button_active -= 1;
    }
  }

  // scroll event - wheel reports as button 3 (scroll up) and button 4 (scroll
  // down)
  if ((button == 3) || (button == 4)) {
    // Each wheel event reports like a button click, GLUT_DOWN then GLUT_UP
    if (state == GLUT_UP) return;  // Disregard redundant GLUT_UP events
    if (button == 3) {
      zoom *= delta_zoom;
    } else {
      zoom /= delta_zoom;
    }
  }

  mouse_pressed_x = x;
  mouse_pressed_y = y;
}

void MouseMove(int x, int y) {
  float x_offset = 0.0, y_offset = 0.0;

  // Rotation controls
  if (right_mouse_button_active) {
    navigation_rotation[0] += ((mouse_pressed_y - y) * 180.0f) / 200.0f;
    navigation_rotation[1] += ((mouse_pressed_x - x) * 180.0f) / 200.0f;

    mouse_pressed_y = y;
    mouse_pressed_x = x;

  } else if (left_mouse_button_active) {
    float delta_x = 0, delta_y = 0;
    const Eigen::AngleAxisf rotation(
        Eigen::AngleAxisf(theia::DegToRad(navigation_rotation[0]),
                          Eigen::Vector3f::UnitX()) *
        Eigen::AngleAxisf(theia::DegToRad(navigation_rotation[1]),
                          Eigen::Vector3f::UnitY()));

    // Panning controls.
    x_offset = (mouse_pressed_x - x);
    if (last_x_offset != 0.0) {
      delta_x = -(x_offset - last_x_offset) / 8.0;
    }
    last_x_offset = x_offset;

    y_offset = (mouse_pressed_y - y);
    if (last_y_offset != 0.0) {
      delta_y = (y_offset - last_y_offset) / 8.0;
    }
    last_y_offset = y_offset;

    // Compute the new viewer origin origin.
    viewer_position +=
        rotation.inverse() * Eigen::Vector3f(delta_x, delta_y, 0);
  }
}

void Keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'r':  // reset viewpoint
      viewer_position.setZero();
      zoom = -50.0;
      navigation_rotation.setZero();
      mouse_pressed_x = 0;
      mouse_pressed_y = 0;
      last_x_offset = 0.0;
      last_y_offset = 0.0;
      left_mouse_button_active = 0;
      right_mouse_button_active = 0;
      point_size = 1.0;
      break;
    case 'z':
      zoom *= delta_zoom;
      break;
    case 'Z':
      zoom /= delta_zoom;
      break;
    case 'p':
      point_size /= 1.2;
      break;
    case 'P':
      point_size *= 1.2;
      break;
    case 'f':
      normalized_focal_length /= 1.2;
      break;
    case 'F':
      normalized_focal_length *= 1.2;
      break;
    case 'c':
      draw_cameras = !draw_cameras;
      break;
    case 'a':
      draw_axes = !draw_axes;
      break;
    case 't':
      ++min_num_views_for_track;
      break;
    case 'T':
      --min_num_views_for_track;
      break;
    case 'b':
      if (anti_aliasing_blend > 0) {
        anti_aliasing_blend -= 0.01;
      }
      break;
    case 'B':
      if (anti_aliasing_blend < 1.0) {
        anti_aliasing_blend += 0.01;
      }
      break;
  }
}

void CameraCenterNormalize(  std::unordered_map<theia::ViewId, Eigen::Matrix3d>& orientations_, std::unordered_map<theia::ViewId, Eigen::Vector3d>& positions_) {
  // First normalize the position so that the marginal median of the camera
  // positions is at the origin.
  std::vector<std::vector<double> > camera_positions(3);
  Eigen::Vector3d median_camera_position;
  for (std::pair<theia::ViewId, Eigen::Vector3d> element : positions_) {
    const Eigen::Vector3d point = element.second;
    camera_positions[0].push_back(point[0]);
    camera_positions[1].push_back(point[1]);
    camera_positions[2].push_back(point[2]);
  }
  std::cout << "DEBUG 3 a ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  median_camera_position(0) = Median(&camera_positions[0]);
  median_camera_position(1) = Median(&camera_positions[1]);
  median_camera_position(2) = Median(&camera_positions[2]);

  std::cout << "DEBUG 3 b ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
  // TransformReconstruction(
  //     Eigen::Matrix3d::Identity(), -median_camera_position, 1.0, this);
  for (std::pair<theia::ViewId, Eigen::Matrix3d> element : orientations_)
  {
      element.second = element.second * Eigen::Matrix3d::Identity().transpose();
      Eigen::Vector3d camera_position = positions_[element.first];
      const double scale = 1.0;
      positions_[element.first] = scale * Eigen::Matrix3d::Identity() * (camera_position) + (-median_camera_position);
  }
  std::cout << "DEBUG 3 c ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // Most images are taken relatively upright with the x-direction of the image
  // parallel to the ground plane. We can solve for the transformation that
  // tries to best align the x-directions to the ground plane by finding the
  // null vector of the covariance matrix of per-camera x-directions.
  Eigen::Matrix3d correlation;
  correlation.setZero();
  for (std::pair<theia::ViewId, Eigen::Matrix3d> element : orientations_)
  {
    const Eigen::Vector3d x = element.second.transpose() * Eigen::Vector3d(1.0, 0.0, 0.0);
    correlation += x * x.transpose();
  }
  std::cout << "DEBUG 3 d ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // The up-direction is computed as the null vector of the covariance matrix.
  Eigen::JacobiSVD<Eigen::Matrix3d> svd(correlation, Eigen::ComputeFullV);
  const Eigen::Vector3d plane_normal = svd.matrixV().rightCols<1>();
  std::cout << "DEBUG 3 e ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // We want the coordinate system to be such that the cameras lie on the x-z
  // plane with the y vector pointing up. Thus, the plane normal should be equal
  // to the positive y-direction.
  Eigen::Matrix3d rotation = Eigen::Quaterniond::FromTwoVectors(plane_normal, Eigen::Vector3d(0, 1, 0)) .toRotationMatrix();
  // TransformReconstruction(rotation, Eigen::Vector3d::Zero(), 1.0, this);
  for (std::pair<theia::ViewId, Eigen::Matrix3d> element : orientations_)
  {
      element.second = element.second * rotation.transpose();
      Eigen::Vector3d camera_position = positions_[element.first];
      const double scale = 1.0;
      positions_[element.first] = scale * rotation * (camera_position) + (Eigen::Vector3d::Zero());
  }
  std::cout << "DEBUG 3 f ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

}

int main(int argc, char* argv[]) {
  THEIA_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  // // Output as a binary file.
  // std::unique_ptr<theia::Reconstruction> reconstruction(
  //     new theia::Reconstruction());
  // CHECK(ReadReconstruction(FLAGS_reconstruction, reconstruction.get()))
  //     << "Could not read reconstruction file.";

  // Read in the global poses of cameras/views, in format like 86 0.00230003 2.99124 -0.877153 -10.6104 0.665666 2.16593
  std::unordered_map<theia::ViewId, Eigen::Vector3d> orientations_;
  std::unordered_map<theia::ViewId, Eigen::Matrix3d> rotmats_;
  std::unordered_map<theia::ViewId, Eigen::Vector3d> positions_;

  std::ifstream ifs(FLAGS_camera_poses, std::ios::in);
  if (!ifs.is_open()) {
    LOG(ERROR) << "Cannot read camera poses file from " << FLAGS_camera_poses;
    return false;
  }

  std::cout << "DEBUG 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // theia viewid is 0-based, using the index in C++ directly
  // while the real view id/ cam id/ image id is 1-based, e.g. for southbuilding image 1 to 128!
  std::string line;
  while ( std::getline (ifs,line) )
  {
    std::pair<theia::ViewId, Eigen::Vector3d> rotation;
    std::pair<theia::ViewId, Eigen::Vector3d> position;
    std::istringstream iss(line);
    if (!(iss >> rotation.first >> rotation.second[0] >> rotation.second[1] >> rotation.second[2]>> position.second[0] >> position.second[1] >> position.second[2]))
    {
        break;
    } // error

    std::cout << rotation.second[0] << ", " << rotation.second[1] << ", " << rotation.second[2] << ", " << position.second[0] << ", " << position.second[1] << ", " << position.second[2] << std::endl;

    //rotation.first = rotation.first - 1;//theia viewid is 0-based, using the index in C++ directly
    //rotation.first = rotation.first;//1-based viewID can be used which is compatible with the data I saved in the matchfile
    position.first = rotation.first;

    orientations_.insert(rotation);
    positions_.insert(position);

    Eigen::Matrix3d rotmat;
    // ceres::AngleAxisToRotationMatrix( rotation.second, ceres::ColumnMajorAdapter3x3(rotmat.data()));
    ceres::AngleAxisToRotationMatrix( rotation.second.data(), ceres::ColumnMajorAdapter3x3(rotmat.data()));
    rotmats_.insert({rotation.first, rotmat});
  }
  ifs.close();

  std::cout << "DEBUG 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // // Centers the reconstruction based on the absolute deviation of 3D points.
  // reconstruction->Normalize();
  CameraCenterNormalize(rotmats_, positions_);
  std::cout << "DEBUG 3 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // Set up camera drawing.
  cameras.reserve(orientations_.size());
  std::cout << "DEBUG 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  for (std::pair<theia::ViewId, Eigen::Vector3d> element : orientations_)
  {
    // Eigen::Vector3d position_by_ViewId = positions_[element.first];
    Eigen::Vector3d position_by_ViewId = 1 * positions_[element.first];    // is a scale required?????

    theia::Camera tmpCamera;
    // Remeber to change the camera intrinsics for different dataset!
    //tmpCamera.SetFocalLength(const double focal_length);
    //tmpCamera.SetImageSize(const int image_width, const int image_height);
    tmpCamera.SetFocalLength(2457.60);
    tmpCamera.SetImageSize(3072, 2304);
    //tmpCamera.SetPosition(const Eigen::Vector3d& position);
    //tmpCamera.SetOrientationFromRotationMatrix(const Eigen::Matrix3d& rotation);
    //tmpCamera.SetOrientationFromAngleAxis(const Eigen::Vector3d& angle_axis);
    tmpCamera.SetOrientationFromAngleAxis(element.second);
    tmpCamera.SetPosition(position_by_ViewId);

    cameras.emplace_back(tmpCamera);
  }
  std::cout << "DEBUG 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  // // Set up world points and colors.
  // world_points.reserve(reconstruction->NumTracks());
  // point_colors.reserve(reconstruction->NumTracks());
  // for (const theia::TrackId track_id : reconstruction->TrackIds()) {
  //   const auto* track = reconstruction->Track(track_id);
  //   if (track == nullptr || !track->IsEstimated()) {
  //     continue;
  //   }
  //   world_points.emplace_back(track->Point().hnormalized());
  //   point_colors.emplace_back(track->Color().cast<float>());
  //   num_views_for_track.emplace_back(track->NumViews());
  // }
  //
  // reconstruction.release();

  // Set up opengl and glut.
  glutInit(&argc, argv);
  glutInitWindowPosition(600, 600);
  glutInitWindowSize(1200, 800);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("Theia Reconstruction Viewer");

#ifdef _WIN32
  // Set up glew.
  CHECK_EQ(GLEW_OK, glewInit())
      << "Failed initializing GLEW.";
#endif

  // Set the camera
  gluLookAt(0.0f, 0.0f, -6.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

  // register callbacks
  glutDisplayFunc(RenderScene);
  glutReshapeFunc(ChangeSize);
  glutMouseFunc(MouseButton);
  glutMotionFunc(MouseMove);
  glutKeyboardFunc(Keyboard);
  glutIdleFunc(RenderScene);

  // enter GLUT event processing loop
  glutMainLoop();

  return 0;
}
