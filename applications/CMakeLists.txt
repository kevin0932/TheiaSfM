# Basic programs
add_executable(extract_features extract_features.cc)
target_link_libraries(extract_features theia ${GFLAGS_LIBRARIES})

add_executable(compute_two_view_geometry compute_two_view_geometry.cc)
target_link_libraries(compute_two_view_geometry theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

# Tools for building SfM models
add_executable(build_reconstruction build_reconstruction.cc)
target_link_libraries(build_reconstruction theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(build_1dsfm_reconstruction build_1dsfm_reconstruction.cc)
target_link_libraries(build_1dsfm_reconstruction theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(calibrate_camera_intrinsics calibrate_camera_intrinsics.cc)
target_link_libraries(calibrate_camera_intrinsics theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(undistort_images undistort_images.cc)
target_link_libraries(undistort_images theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

# Reconstruction visualization.
find_package(OpenGL REQUIRED)
if (NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
else (NOT MSVC)
  find_package(GLEW REQUIRED)
  if (GLEW_FOUND)
    message ("-- Found GLEW: ${GLEW_INCLUDE_DIRS}")
  else (GLEW FOUND)
    message (FATAL_ERROR "-- Can't find GLEW. Please set GLEW_INCLUDE_DIR & GLEW_LIBRARY")
  endif (GLEW_FOUND)
  include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLEW_INCLUDE_DIRS} )
  list(APPEND EXTRA_GL_LIBRARIES ${GLEW_LIBRARY} ${GLEW_LIBRARIES})
endif (NOT MSVC)

find_package(GLUT REQUIRED)
if (GLUT_FOUND)
  message ("-- Found GLUT: ${GLUT_INCLUDE_DIR}")
else (GLUT FOUND)
  message (FATAL_ERROR "-- Can't find GLUT. Please set GLUT_INCLUDE_DIR & GLUT_LIBRARY")
endif (GLUT_FOUND)
include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIR} )
list(APPEND EXTRA_GL_LIBRARIES ${GLUT_LIBRARIES})

add_executable(view_reconstruction view_reconstruction.cc)
target_link_libraries(view_reconstruction theia ${GFLAGS_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${EXTRA_GL_LIBRARIES})

# Useful tools for analyzing reconstructions.
add_executable(compute_reconstruction_statistics compute_reconstruction_statistics.cc)
target_link_libraries(compute_reconstruction_statistics theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(colorize_reconstruction colorize_reconstruction.cc)
target_link_libraries(colorize_reconstruction theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(compare_reconstructions compare_reconstructions.cc)
target_link_libraries(compare_reconstructions theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(compute_matching_relative_pose_errors compute_matching_relative_pose_errors.cc)
target_link_libraries(compute_matching_relative_pose_errors theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(evaluate_relative_translation_optimization evaluate_relative_translation_optimization.cc)
target_link_libraries(evaluate_relative_translation_optimization theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(verify_1dsfm_input verify_1dsfm_input.cc)
target_link_libraries(verify_1dsfm_input theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

# File conversions and exporters.
add_executable(convert_sift_key_file convert_sift_key_file.cc)
target_link_libraries(convert_sift_key_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(convert_bundle_file convert_bundle_file.cc)
target_link_libraries(convert_bundle_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(export_to_nvm_file export_to_nvm_file.cc)
target_link_libraries(export_to_nvm_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(convert_nvm_file convert_nvm_file.cc)
target_link_libraries(convert_nvm_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(convert_theia_reconstruction_to_bundler_file convert_theia_reconstruction_to_bundler_file.cc)
target_link_libraries(convert_theia_reconstruction_to_bundler_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(write_reconstruction_ply_file write_reconstruction_ply_file.cc)
target_link_libraries(write_reconstruction_ply_file theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(export_reconstruction_to_pmvs export_reconstruction_to_pmvs.cc)
target_link_libraries(export_reconstruction_to_pmvs theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(create_calibration_file_from_exif create_calibration_file_from_exif.cc)
target_link_libraries(create_calibration_file_from_exif theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(create_reconstruction_from_strecha_dataset create_reconstruction_from_strecha_dataset.cc)
target_link_libraries(create_reconstruction_from_strecha_dataset theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})


###################################################################################################################
###### Kevin's applications ######
add_executable(visualize_matchfile_content visualize_matchfile_content.cc)
target_link_libraries(visualize_matchfile_content theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#find_package (sqlite3)
#if (SQLITE3_FOUND)
#  include_directories(${SQLITE3_INCLUDE_DIRS})
#  add_executable(colmapDB_to_theia_matchfile colmapDB_to_theia_matchfile.cc)
#  target_link_libraries (colmapDB_to_theia_matchfile theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES} ${SQLITE3_LIBRARIES})
#endif (SQLITE3_FOUND)

#set (LIB_DIR_LOCATION /usr/lib/x86_64-linux-gnu)
#find_library(SQLite3 NAMES sqlite3 PATHS ${LIB_DIR_LOCATION})
#set(INCLUDE_DIRS /usr/include)
#add_executable(colmapDB_to_theia_matchfile colmapDB_to_theia_matchfile.cc)
#target_link_libraries(colmapDB_to_theia_matchfile SQLite3)
#target_include_directories(colmapDB_to_theia_matchfile PRIVATE ${INCLUDE_DIRS})

#include_directories(/usr/include)
#link_directories(/usr/lib/x86_64-linux-gnu)
#target_link_libraries (colmapDB_to_theia_matchfile theia sqlite3 ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

include_directories(/usr/include)
link_directories(/usr/lib/x86_64-linux-gnu)
add_executable(colmapDB_to_theia_matchfile colmapDB_to_theia_matchfile.cc)
target_link_libraries(colmapDB_to_theia_matchfile theia sqlite3 ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

#include_directories(/usr/include)
#link_directories(/usr/lib/x86_64-linux-gnu)
add_executable(read_colmap_DB_content read_colmap_DB_content.cc)
target_link_libraries(read_colmap_DB_content theia sqlite3 ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(read_colmap_DB_content_withRt read_colmap_DB_content_withRt.cc)
target_link_libraries(read_colmap_DB_content_withRt theia sqlite3 ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(compare_input_matchfile_and_intermediate_result compare_input_matchfile_and_intermediate_result.cc)
target_link_libraries(compare_input_matchfile_and_intermediate_result theia ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

add_executable(view_camera_poses view_camera_poses.cc)
target_link_libraries(view_camera_poses theia ${GFLAGS_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${EXTRA_GL_LIBRARIES})
