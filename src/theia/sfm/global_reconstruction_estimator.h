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

#ifndef THEIA_SFM_GLOBAL_RECONSTRUCTION_ESTIMATOR_H_
#define THEIA_SFM_GLOBAL_RECONSTRUCTION_ESTIMATOR_H_

#include "theia/sfm/bundle_adjustment/bundle_adjustment.h"
#include "theia/sfm/filter_view_pairs_from_relative_translation.h"
#include "theia/sfm/reconstruction_estimator.h"
#include "theia/sfm/reconstruction_estimator_options.h"
#include "theia/sfm/types.h"
#include "theia/solvers/sample_consensus_estimator.h"
#include "theia/util/util.h"
////////////////////////////////////////////////
#include <fstream>  // NOLINT
#include <iostream>  // NOLINT
#include <string>
#include <unordered_map>
#include <vector>
#include "theia/sfm/view_graph/view_graph.h"
#include "theia/math/rotation.h"
#include "theia/sfm/camera/camera.h"
////////////////////////////////////////////////
namespace theia {

class Reconstruction;
class ViewGraph;

// Estimates the camera position and 3D structure of the scene using global
// methods to estimate camera poses. First, rotation is estimated globally
// then the position is estimated using a global optimization.
//
// The pipeline for estimating camera poses and structure is as follows:
//   1) Filter potentially bad pairwise geometries by enforcing a loop
//      constaint on rotations that form a triplet.
//   2) Initialize focal lengths.
//   3) Estimate the global rotation for each camera.
//   4) Remove any pairwise geometries where the relative rotation is not
//      consistent with the global rotation.
//   5) Optimize the relative translation given the known rotations.
//   6) Filter potentially bad relative translations.
//   7) Estimate positions.
//   8) Estimate structure.
//   9) Bundle adjustment.
//   10) Retriangulate, and bundle adjust.
//
// After each filtering step we remove any views which are no longer connected
// to the largest connected component in the view graph.
class GlobalReconstructionEstimator : public ReconstructionEstimator {
 public:
  GlobalReconstructionEstimator(
      const ReconstructionEstimatorOptions& options);

  ReconstructionEstimatorSummary Estimate(ViewGraph* view_graph,
                                          Reconstruction* reconstruction);

 private:
  bool FilterInitialViewGraph();
  void CalibrateCameras();
  bool EstimateGlobalRotations();
  void FilterRotations();
  void OptimizePairwiseTranslations();
  void FilterRelativeTranslation();
  bool EstimatePosition();
  void EstimateStructure();
  bool BundleAdjustment();
  // Bundle adjust only the camera positions and points. The camera orientations
  // and intrinsics are held constant.
  bool BundleAdjustCameraPositionsAndPoints();

  ViewGraph* view_graph_;
  Reconstruction* reconstruction_;

  ReconstructionEstimatorOptions options_;
  FilterViewPairsFromRelativeTranslationOptions translation_filter_options_;
  BundleAdjustmentOptions bundle_adjustment_options_;
  RansacParameters ransac_params_;

  std::unordered_map<ViewId, Eigen::Vector3d> orientations_;
  std::unordered_map<ViewId, Eigen::Vector3d> positions_;

  DISALLOW_COPY_AND_ASSIGN(GlobalReconstructionEstimator);

  // DEBUG Code by Kevin
  // bool write_orientations_to_txt(const std::string intermediate_rotation_filepath);
  // bool write_positions_to_txt(const std::string intermediate_position_filepath);
  bool write_poses_to_txt(const std::string& intermediate_pose_filepath)
  {
      std::ofstream ofs(intermediate_pose_filepath.c_str(), std::ios::out);
      if (!ofs.is_open()) {
        LOG(ERROR) << "Cannot write intermediate results (poses) file from " << intermediate_pose_filepath;
        return false;
      }
      // theia viewid is 0-based, using the index in C++ directly
      for (std::pair<ViewId, Eigen::Vector3d> element : orientations_)
      {
          ofs << element.first+1 << " " << element.second[0] << " " << element.second[1] << " " << element.second[2] << " ";
          Eigen::Vector3d position_by_ViewId = positions_[element.first];
          ofs << position_by_ViewId[0] << " " << position_by_ViewId[1] << " " << position_by_ViewId[2] << "\n";
      }

      ofs.close();
      return true;
  }

  // DEBUG Code by Kevin
  bool write_relative_poses_to_txt(const std::string& intermediate_pose_filepath)
  {
      std::ofstream ofs(intermediate_pose_filepath.c_str(), std::ios::out);
      if (!ofs.is_open()) {
        LOG(ERROR) << "Cannot write intermediate results (relative poses) file from " << intermediate_pose_filepath;
        return false;
      }

      // TwoViewInfoFromTwoCameras(const Camera& camera1, const Camera& camera2, TwoViewInfo* info)
      Camera cam1;
      Camera cam2;
      // cam1.SetPosition(const Eigen::Vector3d& position);
      // cam1.SetOrientationFromAngleAxis(const Eigen::Vector3d& angle_axis);
      // cam1.SetPrincipalPoint(const double principal_point_x, const double principal_point_y);
      // cam1.SetImageSize(const int image_width, const int image_height);
      // cam1.SetFocalLength(const double focal_length);
      // Temporary solutions: what if BA optimize the corresponding params????????????
      cam1.SetPrincipalPoint(1536, 1152);
      cam1.SetImageSize(3072, 2304);
      // cam1.SetFocalLength(2737.64);
      cam1.SetFocalLength(2457.60);
      cam2.SetPrincipalPoint(1536, 1152);
      cam2.SetImageSize(3072, 2304);
      // cam2.SetFocalLength(2737.64);
      cam2.SetFocalLength(2457.60);

      TwoViewInfo tmpTwoView_Info;

      // theia viewid is 0-based, using the index in C++ directly
      for (std::pair<ViewId, Eigen::Vector3d> rot1 : orientations_)
      {
          for (std::pair<ViewId, Eigen::Vector3d> rot2 : orientations_)
          {
              Eigen::Vector3d position1_by_ViewId = positions_[rot1.first];
              Eigen::Vector3d position2_by_ViewId = positions_[rot2.first];
              cam1.SetPosition(position1_by_ViewId);
              cam2.SetPosition(position2_by_ViewId);
              cam1.SetOrientationFromAngleAxis(rot1.second);
              cam2.SetOrientationFromAngleAxis(rot2.second);
              TwoViewInfoFromTwoCameras(cam1, cam2, &tmpTwoView_Info);

              Eigen::Matrix3d tmp_rotation_mat;
              ceres::AngleAxisToRotationMatrix( tmpTwoView_Info.rotation_2.data(), ceres::ColumnMajorAdapter3x3( tmp_rotation_mat.data() ) );

              ofs << "ViewID1 " << rot1.first+1 << " " << "ViewID2 " << rot2.first+1 << " ";
              ofs << "Relavie Poses -> [ " << tmpTwoView_Info.rotation_2[0] << " " << tmpTwoView_Info.rotation_2[1] << " " << tmpTwoView_Info.rotation_2[2] << " ] [ ";
              ofs << tmpTwoView_Info.position_2[0] << " " << tmpTwoView_Info.position_2[1] << " " << tmpTwoView_Info.position_2[2] << " ] "  << tmp_rotation_mat(0,0) << " " << tmp_rotation_mat(0,1) << " " << tmp_rotation_mat(0,2) << " " << tmp_rotation_mat(1,0) << " " << tmp_rotation_mat(1,1) << " " << tmp_rotation_mat(1,2) << " " << tmp_rotation_mat(2,0) << " " << tmp_rotation_mat(2,1) << " " << tmp_rotation_mat(2,2) << "\n";
          }
      }

      ofs.close();
      return true;
  }

  // DEBUG Code by Kevin
  bool write_viewgraph_edges_to_txt(const std::string& viewgraph_edges_filepath)
  {
      std::ofstream ofs(viewgraph_edges_filepath.c_str(), std::ios::out);
      if (!ofs.is_open()) {
        LOG(ERROR) << "Cannot write viewgraph edges (view_pairs, twoview_infos) file from " << viewgraph_edges_filepath;
        return false;
      }

      const std::unordered_map<ViewIdPair, TwoViewInfo> tmpViemGraphEdgeMaps = view_graph_->GetAllEdges();

      // theia viewid is 0-based, using the index in C++ directly
      for (std::pair<ViewIdPair, TwoViewInfo> element : tmpViemGraphEdgeMaps)
      {
          Eigen::Matrix3d tmp_rotation_mat;
          ceres::AngleAxisToRotationMatrix( element.second.rotation_2.data(), ceres::ColumnMajorAdapter3x3( tmp_rotation_mat.data() ) );
          ofs << element.first.first+1 << " " << element.second.imgID1 << " " << element.first.second+1 << " " << element.second.imgID2 << " " << element.second.rotation_2[0] << " " << element.second.rotation_2[1] << " " << element.second.rotation_2[2] << " " << element.second.position_2[0] << " " << element.second.position_2[1] << " " << element.second.position_2[2] << " " << tmp_rotation_mat(0,0) << " " << tmp_rotation_mat(0,1) << " " << tmp_rotation_mat(0,2) << " " << tmp_rotation_mat(1,0) << " " << tmp_rotation_mat(1,1) << " " << tmp_rotation_mat(1,2) << " " << tmp_rotation_mat(2,0) << " " << tmp_rotation_mat(2,1) << " " << tmp_rotation_mat(2,2) << "\n";
      }

      ofs.close();
      return true;
  }

};

}  // namespace theia

#endif  // THEIA_SFM_GLOBAL_RECONSTRUCTION_ESTIMATOR_H_
