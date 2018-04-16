#include <stdio.h>
#include <string>
#include <stdint.h>
//using std::string;
#include <sstream>
//using std::stringstream;
#include <iostream>
#include <theia/theia.h>

#include "sqlite3.h"

#define DeMoN_Width 256
#define DeMoN_Height 192

DEFINE_string(colmap_camera_txt, "", "colmap_camera_txt to be used.");

const char *database_filepath = "/home/kevin/JohannesCode/south-building-demon/database.db";

bool import_image_from_DB(std::vector<std::string> &image_files, std::vector<int> &cam_ids_by_image, int _id = 0)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "select * from images where image_id = " << _id << ";"; }
    else { ss << "select * from images;"; }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return found;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return found;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("TEST: image_id = %d, image_name = %s, camera_id = %d, prior_qw = %g, prior_qx = %g, prior_qy = %g, prior_qz = %g, prior_tx = %g, prior_ty = %g, prior_tz = %g\n", sqlite3_column_int(stmt, 0), sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_double(stmt, 3), sqlite3_column_double(stmt, 4), sqlite3_column_double(stmt, 5), sqlite3_column_double(stmt, 6), sqlite3_column_double(stmt, 7), sqlite3_column_double(stmt, 8), sqlite3_column_double(stmt, 9));
        printf("Type: image_id = %d, image_name = %d, camera_id = %d, prior_qw = %d, prior_qx = %d, prior_qy = %d, prior_qz = %d, prior_tx = %d, prior_ty = %d, prior_tz = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4), sqlite3_column_type(stmt, 5), sqlite3_column_type(stmt, 6), sqlite3_column_type(stmt, 7), sqlite3_column_type(stmt, 8), sqlite3_column_type(stmt, 9));
        found = true;
        //std::cout << "image_name = " << sqlite3_column_text(stmt, 1) <<std::endl;
        //std::string image_name_ext;
        //std::string image_name_ext = const_cast<std::string> ( sqlite3_column_text(stmt, 1) );
        //std::string *image_name_ext = const_cast<std::string> ( sqlite3_column_text(stmt, 1) );
        std::stringstream tmp_str;
        tmp_str << sqlite3_column_text(stmt, 1);
        std::string image_name_ext = tmp_str.str();
        int cam_id = sqlite3_column_int(stmt, 2);
        image_files.push_back(image_name_ext);
        cam_ids_by_image.push_back(cam_id);
        std::cout << "image_name = " << image_name_ext << ", cam_id = " << cam_id <<std::endl;
    }
    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}

theia::Feature recover_theia_2D_coord_from_1D_idx(uint32_t index_1D, uint32_t nrows = DeMoN_Height, uint32_t ncols = DeMoN_Width)
{
//    std::cout << "DeMoN_Width = " << DeMoN_Width << ", DeMoN_Height = " << DeMoN_Height << std::endl;
    uint32_t x = index_1D % DeMoN_Width;
    uint32_t y = uint32_t ( index_1D / DeMoN_Width );

//    std::cout << "x = " << x << ", y = " << y << std::endl;

    theia::Feature rec_2d_coord;
    rec_2d_coord[0] = x;
    rec_2d_coord[1] = y;
    //std::cout << "x = " << x << ", rec_2d_coord[0] = " << rec_2d_coord[0] << std::endl;
    //std::cout << "y = " << y << ", rec_2d_coord[1] = " << rec_2d_coord[1] << std::endl;

    return rec_2d_coord;
}

bool import_inlier_matches_from_DB(theia::ImagePairMatch &match, std::vector<std::array<float, 6>> kp_vector_img1, std::vector<std::array<float, 6>> kp_vector_img2, long long unsigned int _id = 0, int image_height=2304, int image_width=3072)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "select * from inlier_matches where pair_id = " << _id << ";"; }
    else { ss << "select * from inlier_matches;"; }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return found;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return found;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    std::cout<<"testing"<<std::endl;

    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        printf("TEST: pair_id = %lld, rows = %d, cols = %d, config = %d\n", sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 4));
        //printf("TYPE: pair_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d, config = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4));
        found = true;
        //std::cout << "match data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        //std::cout << "match data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of uint32_t in bytes = " << sizeof(uint32_t) <<std::endl;
        // Get the pointer to data
        uint32_t* p = (uint32_t*)sqlite3_column_blob(stmt,3);
        uint32_t match_size_inBytes = sqlite3_column_bytes(stmt, 3);

        std::cout << "match data size in bytes = " << match_size_inBytes << "; num_rows = " << num_rows << "; num_cols = " << num_cols << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            //std::cout << "match pair = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
            theia::FeatureCorrespondence feat_match;
            // feat_match.feature1 = recover_theia_2D_coord_from_1D_idx(p[i*2], image_height, image_width);
            // feat_match.feature2 = recover_theia_2D_coord_from_1D_idx(p[i*2+1], image_height, image_width);
            feat_match.feature1[0] = kp_vector_img1[p[i*2]].at(0);
            feat_match.feature1[1] = kp_vector_img1[p[i*2]].at(1);
            feat_match.feature2[0] = kp_vector_img2[p[i*2+1]].at(0);
            feat_match.feature2[1] = kp_vector_img2[p[i*2+1]].at(1);
            match.correspondences.push_back(feat_match);
            // // //theia::Feature tmp2Dcoord;
            // // //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(256);
            // // //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(255);
            // // //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(254);
            // // // std::cout << "kp pair (content) = (" << kp_vector_img1[p[i*2]] << "), (" << kp_vector_img2[p[i*2+1]] << ")" <<std::endl;
            // std::cout << "kp_vector_img1[p[i*2]] = (" << kp_vector_img1[p[i*2]].at(0) << ", " << kp_vector_img1[p[i*2]].at(1) << ", " << kp_vector_img1[p[i*2]].at(2) << ", " << kp_vector_img1[p[i*2]].at(3) << ")" <<std::endl;
            // std::cout << "kp_vector_img2[p[i*2+1]] = (" << kp_vector_img2[p[i*2+1]].at(0) << ", " << kp_vector_img2[p[i*2+1]].at(1) << ", " << kp_vector_img2[p[i*2+1]].at(2) << ", " << kp_vector_img2[p[i*2+1]].at(3) << ")" <<std::endl;
            // std::cout << "feat_match.feature1 = (" << feat_match.feature1[0] << ", " << feat_match.feature1[1] << ")" <<std::endl;
            // std::cout << "feat_match.feature2 = (" << feat_match.feature2[0] << ", " << feat_match.feature2[1] << ")" <<std::endl;
        }
    }
    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");
    match.twoview_info.num_verified_matches = match.correspondences.size();
    printf("inlier correspondence number = %d\n", match.twoview_info.num_verified_matches);

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}

bool import_camera_from_DB(theia::CameraIntrinsicsPrior &params, int _id = 0)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "SELECT * FROM cameras WHERE camera_id = " << _id << ";"; }
    else { ss << "select * from cameras;"; }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return found;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return found;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        printf("TEST: camera_id = %d, camera_model_id = %d, width = %d, height = %d, prior_focal_length = %g\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3), sqlite3_column_double(stmt, 5));
        printf("Type: camera_id = %d, camera_model_id = %d, width = %d, height = %d, params = %d, prior_focal_length = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4), sqlite3_column_type(stmt, 5));
        found = true;
        std::cout << "tmpParamsBytes = " << sqlite3_column_blob(stmt, 4) <<std::endl;
        //std::cout << "tmpParamsBytes 0 = " << &sqlite3_column_blob(stmt, 4)[0] <<std::endl;
        //std::cout << "tmpParamsBytes 1 = " << &sqlite3_column_blob(stmt, 4)[1] <<std::endl;
        std::cout << "tmpParamsBytes = " << sqlite3_column_bytes(stmt, 4) <<std::endl;
        // Get the pointer to data
        double* p = (double*)sqlite3_column_blob(stmt,4);
        std::cout << "tmpParamsBytes 0 (focal_length_scaled) = " << p[0] <<std::endl; // focal_length_scaled
        std::cout << "tmpParamsBytes 1 (half_width_scaled x) = " << p[1] <<std::endl; //half_width_scaled x
        std::cout << "tmpParamsBytes 2 (half_height_scaled y) = " << p[2] <<std::endl; //half_height_scaled y
        std::cout << "tmpParamsBytes 3 (0) = " << p[3] <<std::endl;
        int img_width = sqlite3_column_int(stmt, 2);
        int img_height = sqlite3_column_int(stmt, 3);
        /*
        // cam intrinsics compatible with DeMoN training dataset
        params.aspect_ratio.value[0] = 1.0;
        params.aspect_ratio.is_set = true;
        params.focal_length.value[0] = 228,136871;
        params.focal_length.is_set = true;
        params.image_width = DeMoN_Width;
        params.image_height = DeMoN_Height;
        params.principal_point.value[0] = DeMoN_Width/2;
        params.principal_point.is_set = true;
        params.principal_point.value[1] = DeMoN_Height/2;
        params.skew.value[0] = 0.0;
        params.skew.is_set = true;
        params.radial_distortion.value[0] = 0.0;
        params.radial_distortion.value[1] = 0.0;
        params.radial_distortion.is_set = false;
        */
        // read from database file
        params.aspect_ratio.value[0] = 1.0;
        params.aspect_ratio.is_set = true;
        params.focal_length.value[0] = p[0];
        params.focal_length.is_set = true;
        params.image_width = img_width;
        params.image_height = img_height;
        params.principal_point.value[0] = p[1];
        params.principal_point.is_set = true;
        params.principal_point.value[1] = p[2];
        params.skew.value[0] = 0.0;
        params.skew.is_set = true;
        params.radial_distortion.value[0] = 0.0;
        params.radial_distortion.value[1] = 0.0;
        params.radial_distortion.is_set = false;

    }
    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}

bool import_keypoints_from_DB(int _id = 0)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "select * from keypoints where image_id = " << _id << ";"; }
    else { ss << "select * from keypoints;"; }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return found;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return found;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    std::cout<<"testing"<<std::endl;
    //float kp_array[num_rows][num_cols];
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {

    /*    image_id = rowData[0]
            rows = rowData[1]
            cols = rowData[2]
            dataMemeoryViewBytes = rowData[3]    # data is the memoryview of the original matrix data, which dtype should be float32 (or other 32bits data type)
*/
        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        printf("TEST: image_id = %d, rows = %d, cols = %d\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
        printf("TYPE: image_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3));
        found = true;
        std::cout << "keypoint data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        std::cout << "keypoint data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of float in bytes = " << sizeof(float) <<std::endl;
        // Get the pointer to data
        float* p = (float*)sqlite3_column_blob(stmt,3);
        float keypoints_size_inBytes = sqlite3_column_bytes(stmt, 3);

        float kp_array[num_rows][num_cols];
        std::cout << "keypoint data size in bytes = " << keypoints_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            std::cout << "keypoint data = (" << p[i*6] << ", " << p[i*6+1] << ", " << p[i*6+2] << ", " << p[i*6+3] << ")" <<std::endl;
            kp_array[i][0] = p[i*6];
            kp_array[i][1] = p[i*6+1];
            kp_array[i][2] = p[i*6+2];
            kp_array[i][3] = p[i*6+3];
            kp_array[i][4] = p[i*6+4];
            kp_array[i][5] = p[i*6+5];
        }
        std::cout << "kp_array = " << kp_array << std::endl;
        std::cout << "kp_array[0][:] = " << kp_array[0][0] << ", " << kp_array[0][1] << ", "  << kp_array[0][2] << ", "  << kp_array[0][3] << std::endl;

    }
    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}

bool import_keypoints_by_image_id_from_DB(std::vector<std::array<float, 6>> &kp_array_by_img, int _id = 0)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "select * from keypoints where image_id = " << _id << ";"; }
    else {
        std::cout << "Please specify your image id when retrieving keypoints by image!" << std::endl;   //  ss << "select * from keypoints;";
        return false;
    }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return false;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    std::cout<<"testing"<<std::endl;
    //float kp_array[num_rows][num_cols];
    ret_code = sqlite3_step(stmt);
    //while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {

        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        printf("TEST: image_id = %d, rows = %d, cols = %d\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
        printf("TYPE: image_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3));
        found = true;
        std::cout << "keypoint data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        std::cout << "keypoint data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of float in bytes = " << sizeof(float) <<std::endl;
        // Get the pointer to data
        float* p = (float*)sqlite3_column_blob(stmt,3);
        float keypoints_size_inBytes = sqlite3_column_bytes(stmt, 3);

        //float kp_array[num_rows][num_cols];
        std::cout << "keypoint data size in bytes = " << keypoints_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            std::cout << "keypoint data = (" << p[i*6] << ", " << p[i*6+1] << ", " << p[i*6+2] << ", " << p[i*6+3] << ", " << p[i*6+4] << ", " << p[i*6+5] << ")" <<std::endl;
            std::array<float, 6> kp;
            kp[0] = p[i*6];
            kp[1] = p[i*6+1];
            kp[2] = p[i*6+2];
            kp[3] = p[i*6+3];
            kp[4] = p[i*6+4];
            kp[5] = p[i*6+5];
            kp_array_by_img.push_back(kp);
            //kp_array[i][0] = p[i*6];
            //kp_array[i][1] = p[i*6+1];
            //kp_array[i][2] = p[i*6+2];
            //kp_array[i][3] = p[i*6+3];
        }
        std::cout << "kp_array_by_img nrow = " << kp_array_by_img.size() << ", num_rows = " << num_rows << std::endl;
        if(kp_array_by_img.size() != num_rows) {std::cout<<"something wrong with keypoint retrieval!"<<std::endl;}
        //std::cout << "kp_array[0][:] = " << kp_array[0][0] << ", " << kp_array[0][1] << ", "  << kp_array[0][2] << ", "  << kp_array[0][3] << std::endl;

    //}
    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}


bool import_keypoints_all_images_from_DB(std::vector<std::vector<std::array<float, 6>>> &kp_array_all_images, int _id = 0)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    // if _id is not 0, search for id, otherwise print all IDs
    // this can also be achieved with the default sqlite3_bind* utilities
    if(_id) { ss << "select * from keypoints where image_id = " << _id << ";"; }
    else {
        //std::cout << "Please specify your image id when retrieving keypoints by image!" << std::endl;   //  ss << "select * from keypoints;";
        //return false;
        ss << "select * from keypoints;";
    }
    std::string sql(ss.str());

    //the resulting sql statement
    printf("sql to be executed: %s\n", sql.c_str());

    //get link to database object
    if(sqlite3_open(database_filepath, &db) != SQLITE_OK) {
        printf("ERROR: can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
    }

    // compile sql statement to binary
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        printf("ERROR: while compiling sql: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        sqlite3_finalize(stmt);
        return false;
    }

    // execute sql statement, and while there are rows returned, print ID
    int ret_code = 0;
    std::cout<<"testing"<<std::endl;
    //float kp_array[num_rows][num_cols];
    int num_images = 0;
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {

        num_images++;
        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        printf("TEST: image_id = %d, rows = %d, cols = %d\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2));
        //printf("TYPE: image_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3));
        found = true;
        std::cout << "keypoint data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        std::cout << "keypoint data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of float in bytes = " << sizeof(float) <<std::endl;
        // Get the pointer to data
        float* p = (float*)sqlite3_column_blob(stmt,3);
        float keypoints_size_inBytes = sqlite3_column_bytes(stmt, 3);

        //float kp_array[num_rows][num_cols];
        std::vector<std::array<float, 6>> kp_array_by_image;
        std::cout << "keypoint data size in bytes = " << keypoints_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            if(i==num_rows-1) {std::cout << "keypoint data = (" << p[i*6] << ", " << p[i*6+1] << ", " << p[i*6+2] << ", " << p[i*6+3] << ", " << p[i*6+4] << ", " << p[i*6+5] << ")" <<std::endl;}
            std::array<float, 6> kp;
            kp[0] = p[i*6];
            kp[1] = p[i*6+1];
            kp[2] = p[i*6+2];
            kp[3] = p[i*6+3];
            //kp_array_by_image.push_back(kp);
            kp_array_by_image.push_back(kp);
            //kp_array[i][0] = p[i*6];
            //kp_array[i][1] = p[i*6+1];
            //kp_array[i][2] = p[i*6+2];
            //kp_array[i][3] = p[i*6+3];
        }
        std::cout << "kp_array_by_image nrow = " << kp_array_by_image.size() << ", num_rows = " << num_rows << std::endl;
        if(kp_array_by_image.size() != num_rows) {std::cout<<"something wrong with keypoint retrieval --- inner loop!"<<std::endl;}
        //std::cout << "kp_array[0][:] = " << kp_array[0][0] << ", " << kp_array[0][1] << ", "  << kp_array[0][2] << ", "  << kp_array[0][3] << std::endl;

        kp_array_all_images.push_back(kp_array_by_image);
        kp_array_by_image.clear();  // clear vector holding kp data for a image for later update
    }
    std::cout << "kp_array_all_images nrow = " << kp_array_all_images.size() << ", total number of images = " << num_images << std::endl;
    if(kp_array_all_images.size() != num_images) {std::cout<<"something wrong with keypoint retrieval --- outer loop, image number is inconsistent!"<<std::endl;}

    if(ret_code != SQLITE_DONE) {
        //this error handling could be done better, but it works
        printf("ERROR: while performing sql: %s\n", sqlite3_errmsg(db));
        printf("ret_code = %d\n", ret_code);
    }

    printf("entry %s\n", found ? "found" : "not found");

    //release resources
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return found;
}

struct Pt
{
    bool detected = false;
    Eigen::Vector2d pt;
};

/*** TO DO: adapt Johannes's python code about cross-match-checking to C++ in the following function, so that it can generate matchfile directly from DeMoN's prediction h5py file *******************/
void write_retrieved_matches_to_matchfile_cereal(const std::vector<std::vector<Pt>> &points, const std::vector<std::string> & view_names, const std::string & filename, double focal, int width, int height, int nbRows, int nbCols)
{
    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    theia::CameraIntrinsicsPrior params;
    params.aspect_ratio.value[0] = 1.0;
    params.aspect_ratio.is_set = true;
    params.focal_length.value[0] = focal;
    params.focal_length.is_set = true;
    params.image_width = width;
    params.image_height = height;
    params.principal_point.value[0] = width/2;
    params.principal_point.is_set = true;
    params.principal_point.value[1] = height/2;
    params.skew.value[0] = 0.0;
    params.skew.is_set = true;
    params.radial_distortion.value[0] = 0.0;
    params.radial_distortion.value[1] = 0.0;
    params.radial_distortion.is_set = false;
    for(size_t i = 0; i < view_names.size(); ++i)
        camera_intrinsics_prior.push_back(params);


    std::vector<theia::ImagePairMatch> matches;
    for(size_t i = 0; i < view_names.size(); ++i)
    {
        const auto  & pts1 = points[i];
        for(size_t j = i + 1; j < view_names.size(); ++j)
        {
            const auto  & pts2 = points[j];
            int count = 0;

            for(int y = 0; y < nbRows; ++y){
                for(int x = 0; x < nbCols; ++x){
                    const auto & pt1 = pts1[y * nbCols + x];
                    const auto & pt2 = pts2[y * nbCols + x];
                    if (pt1.detected && pt2.detected){
                        ++count;
                    }
                }
            }

            if (count >= 16){
                count = 0;
                theia::ImagePairMatch match;
                match.image1 = view_names[i];
                match.image2 = view_names[j];

                for(int y = 0; y < nbRows; ++y){
                    for(int x = 0; x < nbCols; ++x){
                        const auto & pt1 = pts1[y * nbCols + x];
                        const auto & pt2 = pts2[y * nbCols + x];
                        if (pt1.detected && pt2.detected){
                            ++count;
                            theia::FeatureCorrespondence mat;
                            mat.feature1 = pt1.pt;
                            mat.feature2 = pt2.pt;
                            match.correspondences.push_back(mat);
                        }
                    }
                }
                matches.push_back(match);
            }
        }
    }

    theia::WriteMatchesAndGeometry(filename, view_names, camera_intrinsics_prior, matches);
}

long long unsigned int image_ids_to_pair_id(int image_id1, int image_id2)
{
    long long unsigned int id1 = static_cast<long long unsigned int>(image_id1);
    long long unsigned int id2 = static_cast<long long unsigned int>(image_id2);
    //std::cout<<"int = "<<image_id1<<", llu = "<<id1<<std::endl;
    if(id1 > id2)
        return 2147483647 * id2 + id1;
    else
        return 2147483647 * id1 + id2;
}

struct optCamera {
    std::string cam_model;
    int tmpCamID;
    int camHeight;
    int camWidth;
    int halfPrincipalX;
    int halfPrincipalY;
    double camFocalLength;
    double camRadialParam;
};

// The following function reads from colmap database file and store them in theia objects (image_files, matches, cam_intrinsic_prior)
// Then write those values to match file (only inlier matches, the checking was done in Johannes's python code) by cereal serialization, which is compatible for theia input format!
void write_DB_matches_to_matchfile_cereal(const std::string & filename, const std::string colmap_camera_file) //std::vector<std::string> &image_files,
{
    std::vector<std::string> image_files;
    std::vector<std::vector<Pt>> points;
    std::vector<int> cam_ids_by_image;

    // retrieve all images and corresponding camera ids (e.g. 128 images for southbuilding dataset)
    bool importImgDB;
    importImgDB = import_image_from_DB(image_files, cam_ids_by_image);
    // //DEBUG
    // return;

    if(image_files.size()!=cam_ids_by_image.size())
    {
        std::cout << "Warning (write_DB_matches_to_matchfile_cereal): inconsistent image and cam id numbers!" << std::endl;
        return;
    }

    for (auto i = image_files.begin(); i != image_files.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    std::cout<<std::endl;
    // // //DEBUG
    // return;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // read camera parameters optimized by Colmap!
    std::vector<optCamera> organized_optimized_cameras(cam_ids_by_image.size());
    std::ifstream ifs(colmap_camera_file.c_str(), std::ios::in);
    if (!ifs.is_open()) {
      LOG(ERROR) << "Cannot read the colmap_camera_file from " << colmap_camera_file;
      // return false;
    }
    // theia viewid is 0-based, using the index in C++ directly
    // while the real view id/ cam id/ image id is 1-based, e.g. for southbuilding image 1 to 128!
    std::string line;
    while ( std::getline (ifs,line) )
    {
        // skip any header lines or comment lines
        if(line[0]=='#')
        {
            continue;
        }

        // std::string cam_model;
        // int tmpCamID;
        // int camHeight;
        // int camWidth;
        // int halfPrincipalX;
        // int halfPrincipalY;
        // double camFocalLength;
        // double camRadialParam;

        optCamera curCam;
        std::istringstream iss(line);
        // if (!(iss >> tmpCamID >> cam_model >> camWidth >> camHeight >> camFocalLength >> halfPrincipalX >> halfPrincipalY >> camRadialParam))
        if (!(iss >> curCam.tmpCamID >> curCam.cam_model >> curCam.camWidth >> curCam.camHeight >> curCam.camFocalLength >> curCam.halfPrincipalX >> curCam.halfPrincipalY >> curCam.camRadialParam))
        {
            break;
        } // error
        // std::cout << tmpCamID << " " << cam_model << " " << camWidth << " " << camHeight << " " << camFocalLength << " " << halfPrincipalX << " " << halfPrincipalY << " " << camRadialParam << std::endl;
        std::cout << "reading optimized camera parameters: " << curCam.tmpCamID << " " << curCam.cam_model << " " << curCam.camWidth << " " << curCam.camHeight << " " << curCam.camFocalLength << " " << curCam.halfPrincipalX << " " << curCam.halfPrincipalY << " " << curCam.camRadialParam << std::endl;

        organized_optimized_cameras[curCam.tmpCamID-1] = curCam;
    }
    ifs.close();
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    // theia::CameraIntrinsicsPrior params;
    // set here if you want the shared intrinsics hard-coded
/*    params.aspect_ratio.value[0] = 1.0;
    params.aspect_ratio.is_set = true;
    params.focal_length.value[0] = focal;
    params.focal_length.is_set = true;
    params.image_width = width;
    params.image_height = height;
    params.principal_point.value[0] = width/2;
    params.principal_point.is_set = true;
    params.principal_point.value[1] = height/2;
    params.skew.value[0] = 0.0;
    params.skew.is_set = true;
    params.radial_distortion.value[0] = 0.0;
    params.radial_distortion.value[1] = 0.0;
    params.radial_distortion.is_set = false;
*/
    std::cout << " reading opt cam params is done!" << std::endl;
    bool importCamDB;
    int cam_model_id = 1;
    for(size_t i = 0; i < cam_ids_by_image.size(); ++i)
    {
        // importCamDB = import_camera_from_DB(params, cam_model_id);
        // camera_intrinsics_prior.push_back(params);
        // // // debug
        // // return;
        theia::CameraIntrinsicsPrior params;
        params.aspect_ratio.value[0] = 1.0;
        params.aspect_ratio.is_set = true;
        params.focal_length.value[0] = organized_optimized_cameras[cam_ids_by_image[i]-1].camFocalLength;
        //// Just a test ////
        params.focal_length.value[0] = 2737;
        params.focal_length.is_set = true;
        params.image_width = organized_optimized_cameras[cam_ids_by_image[i]-1].camWidth;
        params.image_height = organized_optimized_cameras[cam_ids_by_image[i]-1].camHeight;
        params.principal_point.value[0] = organized_optimized_cameras[cam_ids_by_image[i]-1].halfPrincipalX;
        params.principal_point.is_set = true;
        params.principal_point.value[1] = organized_optimized_cameras[cam_ids_by_image[i]-1].halfPrincipalY;
        params.skew.value[0] = 0.0;
        params.skew.is_set = true;
        // params.radial_distortion.value[0] = 0.0;
        params.radial_distortion.value[0] = organized_optimized_cameras[cam_ids_by_image[i]-1].camRadialParam;
        params.radial_distortion.value[1] = 0.0;
        params.radial_distortion.is_set = false;

        camera_intrinsics_prior.push_back(params);
    }
    std::cout << " saving camera_intrinsics_prior from opt cam params is done!" << std::endl;

    int image_width = camera_intrinsics_prior[0].image_width;
    int image_height = camera_intrinsics_prior[0].image_height;

    // think about the keypoints data (Johannes may not use this one)
    bool retrieveKPbool;
    //std::vector<std::array<float, 6>> kp_array_by_img;
    std::vector<std::vector<std::array<float, 6>>> kp_array_all_images;
    retrieveKPbool = import_keypoints_all_images_from_DB(kp_array_all_images);
    // // // debug
    // return;


    bool importInlierMatchDB;
    std::vector<theia::ImagePairMatch> matches;
/*  test code for importing only the first pair!
    theia::ImagePairMatch match;
    match.image1 = image_files[0];
    match.image2 = image_files[1];

    long long unsigned int cur_pair_id = image_ids_to_pair_id(1, 2);
    std::cout << "current pair id = " << cur_pair_id << std::endl;
    importInlierMatchDB = import_inlier_matches_from_DB(match, cur_pair_id);
    std::cout << "inlier match pair: img1 = " << match.image1 << ", img2 = " << match.image2 << ", feature correspondence num = " << match.correspondences.size() << std::endl;
    matches.push_back(match);
*/

    for(int img_id1 = 0;img_id1<image_files.size();img_id1++)
    {
        for(int img_id2 = img_id1+1;img_id2<image_files.size();img_id2++)
        {
            theia::ImagePairMatch match;
            match.image1 = image_files[img_id1];
            match.image2 = image_files[img_id2];
            long long unsigned int cur_pair_id = image_ids_to_pair_id((img_id1+1), (img_id2+1));    // be careful about the index start! c++ vector index starts from 0 while image id from 1!

            std::vector<std::array<float, 6>> kp_vector_img1 = kp_array_all_images[img_id1];
            std::vector<std::array<float, 6>> kp_vector_img2 = kp_array_all_images[img_id2];

            importInlierMatchDB = import_inlier_matches_from_DB(match, kp_vector_img1, kp_vector_img2, cur_pair_id, image_height, image_width);
            if( importInlierMatchDB == true )
            {
                // matches.push_back(match);
                ////////////////////////////////////////////////////////////////////////////////////////////////
                theia::CameraIntrinsicsPrior intrinsics1 = camera_intrinsics_prior[img_id1];
                theia::CameraIntrinsicsPrior intrinsics2 = camera_intrinsics_prior[img_id2];
                std::vector<theia::FeatureCorrespondence> tmpCorrespondences;
                tmpCorrespondences = std::move(match.correspondences);
                theia::TwoViewInfo tmpPtrTwoview_info;
                std::vector<int> tmpPtrInlier_indices;
                // std::cout<< "debug 2 " << std::endl;
                tmpPtrInlier_indices.clear();
                // std::cout<< "debug 2.5 " << std::endl;
                theia::EstimateTwoViewInfoOptions tmpOptions;
                // std::cout<< "debug 3 " << std::endl;
                std::cout<< "camera_intrinsics_prior[img_id1].image_width = " << camera_intrinsics_prior[img_id1].image_width << std::endl;
                std::cout<< "camera_intrinsics_prior[img_id1].focal_length.value[0] = " << camera_intrinsics_prior[img_id1].focal_length.value[0] << std::endl;
                std::cout<< "tmpOptions.expected_ransac_confidence = " << tmpOptions.expected_ransac_confidence << std::endl;
                tmpOptions.max_sampson_error_pixels = 6;
                // tmpOptions.max_sampson_error_pixels = 1;
                // tmpOptions.max_sampson_error_pixels = 0.1;
                std::cout<< "tmpOptions.max_sampson_error_pixels = " << tmpOptions.max_sampson_error_pixels << std::endl;
                // std::cout<< "debug 4 " << std::endl;
                if(tmpCorrespondences.size()<5)
                {
                    std::cout << "less than 5 correspondences!!!! cannot EstimateTwoViewInfo()" << std::endl;
                    continue;
                }
                if(EstimateTwoViewInfo(tmpOptions, intrinsics1, intrinsics2, tmpCorrespondences, &tmpPtrTwoview_info, &tmpPtrInlier_indices))
                {
                    std::cout << "Before substitution: match.twoview_info.rotation_2 =[" << match.twoview_info.rotation_2[0] << ", " << match.twoview_info.rotation_2[1] << ", " << match.twoview_info.rotation_2[2] << "]" << std::endl;
                    std::cout << "After estimation: tmpPtrTwoview_info.rotation_2 =[" << tmpPtrTwoview_info.rotation_2[0] << ", " << tmpPtrTwoview_info.rotation_2[1] << ", " << tmpPtrTwoview_info.rotation_2[2] << "]" << std::endl;
                    std::cout << "Before substitution: match.twoview_info.position_2 = [" << match.twoview_info.position_2[0] << ", " << match.twoview_info.position_2[1] << ", " << match.twoview_info.position_2[2] << "]" << std::endl;
                    std::cout << "After estimation: tmpPtrTwoview_info.position_2 = [" << tmpPtrTwoview_info.position_2[0] << ", " << tmpPtrTwoview_info.position_2[1] << ", " << tmpPtrTwoview_info.position_2[2] << "]" << std::endl;
                    std::cout << "input correspondences num = " << tmpCorrespondences.size() << ";  input num_verified_matches = " << match.twoview_info.num_verified_matches << std::endl;
                    std::cout << "inlier num = " << tmpPtrInlier_indices.size() << std::endl;
                    std::cout << "tmpPtrTwoview_info.focal_length_1 = " << tmpPtrTwoview_info.focal_length_1 << std::endl;
                    match.correspondences.clear();
                    for(uint32_t tmp_i = 0;tmp_i<tmpPtrInlier_indices.size();tmp_i++)
                    {
                        match.correspondences.push_back(tmpCorrespondences[tmpPtrInlier_indices[tmp_i]]);
                    }
                    match.twoview_info.rotation_2 = tmpPtrTwoview_info.rotation_2;
                    match.twoview_info.position_2 = tmpPtrTwoview_info.position_2;
                    match.twoview_info.num_verified_matches = tmpPtrInlier_indices.size();
                    std::cout << "output correspondences num = " << match.correspondences.size() << ";  num_verified_matches = " << match.twoview_info.num_verified_matches << std::endl;
                    matches.push_back(match);
                    std::cout << "match.twoview_info.position_2 = " << match.twoview_info.position_2 << ", match.twoview_info.rotation_2 = " << match.twoview_info.rotation_2 << std::endl;
                }
                else
                {
                    std::cout << "two_view_estimation fails! it will be skipped but please check the correspondence matching process!" << std::endl;
                }
                ////////////////////////////////////////////////////////////////////////////////////////////////

            }
            else
            {
                std::cout << "image pair (" << match.image1 << ", " << match.image2 << ") is skipped becausre there are no inlier matches!" << std::endl;
            }
            match.correspondences.clear();
        }
    }

    // output some statistics
    std::cout << "no. of input image pairs = " << matches.size() << std::endl;
    float numFeatureMatches = 0;
    int numPairsWithInliers = 0;
    for(int cnt = 0;cnt<matches.size();cnt++)
    {
        if(matches[cnt].twoview_info.num_verified_matches!=matches[cnt].correspondences.size())
        {
            std::cout << "warning: matches[cnt].twoview_info.num_verified_matches!=matches[cnt].correspondences.size()" << std::endl;
        }
        numFeatureMatches += matches[cnt].twoview_info.num_verified_matches;
        if(matches[cnt].twoview_info.num_verified_matches>0)
        {
            numPairsWithInliers += 1;
        }
    }
    std::cout << "avg. no. of feature matches = " << numFeatureMatches / float(matches.size()) << std::endl;
    std::cout << "tobal no. of feature matches = " << numFeatureMatches << std::endl;
    std::cout << "no. of pairs with inlier matches = " << numPairsWithInliers << std::endl;


/*
    for(size_t i = 0; i < image_files.size(); ++i)
    {
        const auto  & pts1 = points[i];
        for(size_t j = i + 1; j < image_files.size(); ++j)
        {
            const auto  & pts2 = points[j];
            int count = 0;

            for(int y = 0; y < nbRows; ++y){
                for(int x = 0; x < nbCols; ++x){
                    const auto & pt1 = pts1[y * nbCols + x];
                    const auto & pt2 = pts2[y * nbCols + x];
                    if (pt1.detected && pt2.detected){
                        ++count;
                    }
                }
            }

            if (count >= 16){
                count = 0;
                theia::ImagePairMatch match;
                match.image1 = image_files[i];
                match.image2 = image_files[j];

                for(int y = 0; y < nbRows; ++y){
                    for(int x = 0; x < nbCols; ++x){
                        const auto & pt1 = pts1[y * nbCols + x];
                        const auto & pt2 = pts2[y * nbCols + x];
                        if (pt1.detected && pt2.detected){
                            ++count;
                            theia::FeatureCorrespondence mat;
                            mat.feature1 = pt1.pt;
                            mat.feature2 = pt2.pt;
                            match.correspondences.push_back(mat);
                        }
                    }
                }
                matches.push_back(match);
            }
        }
    }
*/
    theia::WriteMatchesAndGeometry(filename, image_files, camera_intrinsics_prior, matches);
}

int main(int argc, char* argv[])
{
    THEIA_GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    std::string colmap_camera_file = FLAGS_colmap_camera_txt;

    bool testDB;
//    testDB = import_camera_from_DB();
//   testDB = import_image_from_DB();
//    testDB = import_inlier_matches_from_DB();
    // testDB = import_keypoints_from_DB(1);
    write_DB_matches_to_matchfile_cereal("testfile.cereal", colmap_camera_file);
    return 0;
}
