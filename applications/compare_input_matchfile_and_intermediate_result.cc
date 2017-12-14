// Copyright (C) 2015 The Regents of the University of California (Regents).
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

#include <memory>
#include <string>
#include <sstream>
#include <fstream>
DEFINE_string(
    matches, "",
    "Matches file that has been written with WriteMatchesAndGeometry");

DEFINE_string(intermediate_result,
              "",
              "intermediate result text files saved during theia reconstruction.");

using theia::Reconstruction;
using theia::TrackId;
using theia::ViewId;

// Computes the error in the relative rotation based on the ground truth
// rotations rotation1 and rotation2 (which specify world-to-camera
// transformations).
double ComputeRelativeRotationError(const Eigen::Matrix3d& rotation1,
                                    const Eigen::Matrix3d& rotation2,
                                    const Eigen::Vector3d& relative_rotation) {
  Eigen::Matrix3d relative_rotation_matrix;
  ceres::AngleAxisToRotationMatrix(
      relative_rotation.data(),
      ceres::ColumnMajorAdapter3x3(relative_rotation_matrix.data()));
  const Eigen::Matrix3d loop_rotation = relative_rotation_matrix.transpose() *
                                        (rotation2 * rotation1.transpose());
  Eigen::Vector3d loop_rotation_aa;
  ceres::RotationMatrixToAngleAxis(
      ceres::ColumnMajorAdapter3x3(loop_rotation.data()),
      loop_rotation_aa.data());
  return theia::RadToDeg(loop_rotation_aa.norm());
}

double ComputeRelativeTranslationError(
    const Eigen::Vector3d& position1,
    const Eigen::Vector3d& position2,
    const Eigen::Matrix3d& rotation1,
    const Eigen::Vector3d& relative_translation) {
  const Eigen::Vector3d world_translation =
      rotation1 * (position2 - position1).normalized();
  return theia::RadToDeg(acos(
      theia::Clamp(relative_translation.dot(world_translation), -1.0, 1.0)));
}

void EvaluateRelativeError(
    const std::vector<std::string>& view_names,
    const std::vector<theia::ImagePairMatch>& matches,
    const std::unordered_map<ViewId, Eigen::Vector3d>& orientations_,
    const std::unordered_map<ViewId, Eigen::Vector3d>& positions_) {
  // For each edge, get the rotate translation and check the error.
  int num_matches_evaluated = 0;
  const std::vector<double> histogram_bins = {2,  5,   10,  15,  25,  50,
                                              90, 135, 180, 225, 270, 316};

  theia::PoseError relative_pose_error(histogram_bins, histogram_bins);
  for (const auto& match : matches) {
    const std::string view1_name = match.image1;
    const std::string view2_name = match.image2;

    const ViewId view_id1 =
        match.twoview_info.imgID1;
    const ViewId view_id2 =
        match.twoview_info.imgID2;
    //onst theia::View* view1 = gt_reconstruction.View(view_id1);
    //const theia::View* view2 = gt_reconstruction.View(view_id2);
    std::unordered_map<ViewId,Eigen::Vector3d>::const_iterator got1 = orientations_.find (view_id1);
    std::unordered_map<ViewId,Eigen::Vector3d>::const_iterator got2 = orientations_.find (view_id2);

    /*if (  )
      std::cout << "not found";
    else
      std::cout << got->first << " is " << got->second; */

    if (got1 == orientations_.end() || got2 == orientations_.end()) {
      continue;
    }
    //const theia::Camera& camera1 = view1->Camera();
    //const theia::Camera& camera2 = view2->Camera();
    Eigen::Matrix3d tmp_rotation_mat1;
    ceres::AngleAxisToRotationMatrix( got1->second.data(), ceres::ColumnMajorAdapter3x3( tmp_rotation_mat1.data() ) );
    Eigen::Matrix3d tmp_rotation_mat2;
    ceres::AngleAxisToRotationMatrix( got2->second.data(), ceres::ColumnMajorAdapter3x3( tmp_rotation_mat2.data() ) );

    const double rotation_angular_error =
        ComputeRelativeRotationError(tmp_rotation_mat1,
                                     tmp_rotation_mat2,
                                     match.twoview_info.rotation_2);
    const double translation_angular_error = ComputeRelativeTranslationError(
        got1->second,
        got2->second,
        tmp_rotation_mat1,
        match.twoview_info.position_2);

    relative_pose_error.AddError(rotation_angular_error,
                                 translation_angular_error);
    ++num_matches_evaluated;
  }
  LOG(INFO) << "Evaluated " << view_names.size() << " common views containing "
            << num_matches_evaluated << " two-view matches.";
  LOG(INFO) << relative_pose_error.PrintMeanMedianHistogram();
  // std::cout << "Evaluated " << view_names.size() << " common views containing "
  //           << num_matches_evaluated << " two-view matches." << std::endl;
  // std::cout << relative_pose_error.PrintMeanMedianHistogram() << std::endl;
}

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  THEIA_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

  LOG(INFO) << "Reading the matches.";

  std::vector<std::string> view_names;
  std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
  std::vector<theia::ImagePairMatch> matches;
  CHECK(theia::ReadMatchesAndGeometry(FLAGS_matches,
                                      &view_names,
                                      &camera_intrinsics_prior,
                                      &matches))
      << "Could not read matches from " << FLAGS_matches;

  std::unique_ptr<theia::Reconstruction> reconstruction(
      new theia::Reconstruction());
  // CHECK(theia::ReadReconstruction(FLAGS_reconstruction, reconstruction.get()))
  //     << "Could not read reconstruction from " << FLAGS_reconstruction;

  std::unordered_map<ViewId, Eigen::Vector3d> orientations_;
  std::unordered_map<ViewId, Eigen::Vector3d> positions_;

  std::ifstream ifs(FLAGS_intermediate_result, std::ios::in);
  if (!ifs.is_open()) {
    LOG(ERROR) << "Cannot read intermediate results (poses) file from " << FLAGS_intermediate_result;
    return false;
  }
  // theia viewid is 0-based, using the index in C++ directly
  std::string line;
  while ( std::getline (ifs,line) )
  {
    std::pair<ViewId, Eigen::Vector3d> rotation;
    std::pair<ViewId, Eigen::Vector3d> position;
    std::istringstream iss(line);
    if (!(iss >> rotation.first >> rotation.second[0] >> rotation.second[1] >> rotation.second[2]>> position.second[0] >> position.second[1] >> position.second[2]))
    {
        break;
    } // error
    //rotation.first = rotation.first - 1;//theia viewid is 0-based, using the index in C++ directly
    rotation.first = rotation.first;//1-based viewID can be used which is compatible with the data I saved in the matchfile
    position.first = rotation.first;

    orientations_.insert(rotation);
    positions_.insert(position);
  }
  ifs.close();

  std::cout<<"test for reading is done!"<< std::endl;
  EvaluateRelativeError(view_names, matches, orientations_, positions_);
  std::cout<<"test for EvaluateRelativeError is done!"<< std::endl;

  return 0;
}
