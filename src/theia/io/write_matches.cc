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

#include "theia/io/write_matches.h"

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <glog/logging.h>
#include <cstdlib>
#include <fstream>   // NOLINT
#include <iostream>  // NOLINT
#include <string>

#include "theia/matching/image_pair_match.h"
#include "theia/sfm/camera_intrinsics_prior.h"

namespace theia {

bool WriteMatchesAndGeometry(
    const std::string& matches_file,
    const std::vector<std::string>& view_names,
    const std::vector<CameraIntrinsicsPrior>& camera_intrinsics_prior,
    const std::vector<ImagePairMatch>& matches) {
  CHECK_EQ(view_names.size(), camera_intrinsics_prior.size());

  // Return false if the file cannot be opened for writing.
  std::ofstream matches_writer(matches_file, std::ios::out | std::ios::binary);
  if (!matches_writer.is_open()) {
    LOG(ERROR) << "Could not open the matches file: " << matches_file
               << " for writing.";
    return false;
  }

  // Make sure that Cereal is able to finish executing before returning.
  {
    cereal::PortableBinaryOutputArchive output_archive(matches_writer);
    output_archive(view_names, camera_intrinsics_prior, matches);
  }

  if(WriteVisibilityScores(matches_file, view_names, camera_intrinsics_prior, matches)==true)
  {
      std::cout << "visibility score is written to a file successfully!" << std::endl;
  }

  return true;
}

bool WriteVisibilityScores(
    const std::string& matches_file,
    const std::vector<std::string>& view_names,
    const std::vector<CameraIntrinsicsPrior>& camera_intrinsics_prior,
    const std::vector<ImagePairMatch>& matches) {

    std::string visibility_score_file;// = matches_file;

    std::stringstream ss(matches_file);

    // while( ss.good() )
    // {
        std::string substr;
        getline( ss, substr, '.' );
        // visibility_score_file = substr;
        visibility_score_file.append(substr);
        visibility_score_file.append("_visibility.txt");
    // }
    std::cout << "visibility_score_file = " << visibility_score_file << std::endl;

    std::ofstream ofs(visibility_score_file.c_str(), std::ios::out);
    if (!ofs.is_open()) {
      LOG(ERROR) << "Cannot write the visibility score file from " << visibility_score_file;
      return false;
    }

    for(int matchIdx = 0;matchIdx<matches.size();matchIdx++)
    {
        ofs << matches[matchIdx].twoview_info.imgID1 << " " << matches[matchIdx].twoview_info.imgID2 << " " << matches[matchIdx].twoview_info.visibility_score << "\n" ;
    }
    ofs.close();
  // CHECK_EQ(view_names.size(), camera_intrinsics_prior.size());
  //
  // // Return false if the file cannot be opened for writing.
  // std::ofstream matches_writer(matches_file, std::ios::out | std::ios::binary);
  // if (!matches_writer.is_open()) {
  //   LOG(ERROR) << "Could not open the matches file: " << matches_file
  //              << " for writing.";
  //   return false;
  // }
  //
  // // Make sure that Cereal is able to finish executing before returning.
  // {
  //   cereal::PortableBinaryOutputArchive output_archive(matches_writer);
  //   output_archive(view_names, camera_intrinsics_prior, matches);
  // }
  //
    return true;
}

}  // namespace theia
