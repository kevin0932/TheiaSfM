#include <stdio.h>
#include <string>
#include <stdint.h>
//using std::string;
#include <sstream>
//using std::stringstream;
#include <iostream>
#include <theia/theia.h>

#include "sqlite3.h"

#include "theia/sfm/estimate_twoview_info.h"

#define DeMoN_Width 256
#define DeMoN_Height 192

const char *database_filepath = "/home/kevin/JohannesCode/south-building-demon/database.db";
const std::string outputCalibrFilePath = "/home/kevin/JohannesCode/theia_trial_demon/southbuildingCalibration/calibration_file.txt";

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
        printf("TEST: image_id = %d, image_name = %s, camera_id = %d, prior_qw = %g, prior_qx = %g, prior_qy = %g, prior_qz = %g, prior_tx = %g, prior_ty = %g, prior_tz = %g, prior_angleaxis_x = %g, prior_angleaxis_y = %g, prior_angleaxis_z = %g\n", sqlite3_column_int(stmt, 0), sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_double(stmt, 3), sqlite3_column_double(stmt, 4), sqlite3_column_double(stmt, 5), sqlite3_column_double(stmt, 6), sqlite3_column_double(stmt, 7), sqlite3_column_double(stmt, 8), sqlite3_column_double(stmt, 9), sqlite3_column_double(stmt, 10), sqlite3_column_double(stmt, 11), sqlite3_column_double(stmt, 12));
        printf("Type: image_id = %d, image_name = %d, camera_id = %d, prior_qw = %d, prior_qx = %d, prior_qy = %d, prior_qz = %d, prior_tx = %d, prior_ty = %d, prior_tz = %d, prior_angleaxis_x = %d, prior_angleaxis_y = %d, prior_angleaxis_z = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4), sqlite3_column_type(stmt, 5), sqlite3_column_type(stmt, 6), sqlite3_column_type(stmt, 7), sqlite3_column_type(stmt, 8), sqlite3_column_type(stmt, 9), sqlite3_column_type(stmt, 10), sqlite3_column_type(stmt, 11), sqlite3_column_type(stmt, 12));
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

//theia::Feature recover_theia_2D_coord_from_1D_idx(uint32_t index_1D, uint32_t nrows = DeMoN_Height, uint32_t ncols = DeMoN_Width)
theia::Feature recover_theia_2D_coord_from_1D_idx(uint32_t index_1D, uint32_t nrows = 2304, uint32_t ncols = 3072)
{
//    std::cout << "DeMoN_Width = " << DeMoN_Width << ", DeMoN_Height = " << DeMoN_Height << std::endl;
    //uint32_t x = index_1D % DeMoN_Width;
    //uint32_t y = uint32_t ( index_1D / DeMoN_Width );
    uint32_t x = index_1D % ncols;
    uint32_t y = uint32_t ( index_1D / ncols );

//    std::cout << "x = " << x << ", y = " << y << std::endl;

    theia::Feature rec_2d_coord;
    rec_2d_coord[0] = x;
    rec_2d_coord[1] = y;
    //std::cout << "x = " << x << ", rec_2d_coord[0] = " << rec_2d_coord[0] << std::endl;
    //std::cout << "y = " << y << ", rec_2d_coord[1] = " << rec_2d_coord[1] << std::endl;

    return rec_2d_coord;
}

bool import_inlier_matches_from_DB(theia::ImagePairMatch &match, long long unsigned int _id = 0)
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
        printf("TEST: pair_id = %lld, rows = %d, cols = %d, config = %d, imgID1 = %d, imgID2 = %d\n", sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 4), sqlite3_column_int(stmt, 5), sqlite3_column_int(stmt, 6));
        //printf("TYPE: pair_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d, config = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4));
        found = true;
        //std::cout << "match data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        //std::cout << "match data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of uint32_t in bytes = " << sizeof(uint32_t) <<std::endl;
        // Get the pointer to data
        uint32_t* p = (uint32_t*)sqlite3_column_blob(stmt,3);
        uint32_t match_size_inBytes = sqlite3_column_bytes(stmt, 3);

        //std::cout << "match data size in bytes = " << match_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            //std::cout << "match pair = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
            theia::FeatureCorrespondence feat_match;
            feat_match.feature1 = recover_theia_2D_coord_from_1D_idx(p[i*2]);
            feat_match.feature2 = recover_theia_2D_coord_from_1D_idx(p[i*2+1]);
            match.correspondences.push_back(feat_match);
            //theia::Feature tmp2Dcoord;
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(256);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(255);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(254);
            //std::cout << "match pair in 2D_coordinate = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
        }

        // get the rotation and translation
        float* R_vec = (float*)sqlite3_column_blob(stmt, 7);
        float* t_vec = (float*)sqlite3_column_blob(stmt, 8);
        uint32_t rot_size_inBytes = sqlite3_column_bytes(stmt, 7);
        std::cout << "rot_size_inBytes = " << rot_size_inBytes << std::endl;
        match.twoview_info.focal_length_1 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.focal_length_2 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.num_verified_matches = num_rows;
        match.twoview_info.visibility_score = num_rows; // temporary solution; should be calculated in a proper way with Theia
        match.twoview_info.imgID1 = sqlite3_column_int(stmt, 5);
        match.twoview_info.imgID2 = sqlite3_column_int(stmt, 6);

        match.twoview_info.rotation_2[0] = R_vec[0];
        match.twoview_info.rotation_2[1] = R_vec[1];
        match.twoview_info.rotation_2[2] = R_vec[2];
        match.twoview_info.position_2[0] = t_vec[0];
        match.twoview_info.position_2[1] = t_vec[1];
        match.twoview_info.position_2[2] = t_vec[2];
        std::cout << "match.twoview_info.rotation_2 = " << match.twoview_info.rotation_2 << std::endl;
        std::cout << "match.twoview_info.position_2 = " << match.twoview_info.position_2 << std::endl;
        match.twoview_info.position_2.normalize();  // It is not mandatory for input from DeMoN, since the translation vectors of prediction is already a unit vector
        std::cout << "match.twoview_info.position_2 after normalized = " << match.twoview_info.position_2 << std::endl;
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

bool import_inlier_matches_from_DB_byPairNames(theia::ImagePairMatch &match, const std::string& pair_name)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    ss << "select * from inlier_matches where pair_names = '" << pair_name << "';";
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

    // only 1 row should be selected from inlier_matches database table!
    // ToDo: check if the row is 1!!!
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {

        std::stringstream tmp_str;
        tmp_str << sqlite3_column_text(stmt, 0);
        std::string image_pair_name = tmp_str.str();

        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        // printf("TEST: pair_id = %lld, rows = %d, cols = %d, config = %d, imgID1 = %d, imgID2 = %d, image_name2 = %s, image_name2 = %s\n", sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 4), sqlite3_column_int(stmt, 5), sqlite3_column_int(stmt, 6), sqlite3_column_text(stmt, 9), sqlite3_column_text(stmt, 10));
        printf("TEST: image_pair_name = %s, rows = %d, cols = %d, config = %d, imgID1 = %d, imgID2 = %d, image_name2 = %s, image_name2 = %s\n", sqlite3_column_text(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 4), sqlite3_column_int(stmt, 5), sqlite3_column_int(stmt, 6), sqlite3_column_text(stmt, 9), sqlite3_column_text(stmt, 10));
        //printf("TYPE: pair_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d, config = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4));
        found = true;
        //std::cout << "match data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        //std::cout << "match data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of uint32_t in bytes = " << sizeof(uint32_t) <<std::endl;
        // Get the pointer to data
        uint32_t* p = (uint32_t*)sqlite3_column_blob(stmt,3);
        uint32_t match_size_inBytes = sqlite3_column_bytes(stmt, 3);

        //std::cout << "match data size in bytes = " << match_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            //std::cout << "match pair = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
            theia::FeatureCorrespondence feat_match;
            feat_match.feature1 = recover_theia_2D_coord_from_1D_idx(p[i*2]);
            feat_match.feature2 = recover_theia_2D_coord_from_1D_idx(p[i*2+1]);
            match.correspondences.push_back(feat_match);
            //theia::Feature tmp2Dcoord;
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(256);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(255);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(254);
            //std::cout << "match pair in 2D_coordinate = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
        }

        // get the rotation and translation
        float* R_vec = (float*)sqlite3_column_blob(stmt, 7);
        float* t_vec = (float*)sqlite3_column_blob(stmt, 8);
        uint32_t rot_size_inBytes = sqlite3_column_bytes(stmt, 7);
        std::cout << "rot_size_inBytes = " << rot_size_inBytes << std::endl;
        match.twoview_info.focal_length_1 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.focal_length_2 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.num_verified_matches = num_rows;
        match.twoview_info.visibility_score = num_rows; // temporary solution; should be calculated in a proper way with Theia
        match.twoview_info.imgID1 = sqlite3_column_int(stmt, 5);
        match.twoview_info.imgID2 = sqlite3_column_int(stmt, 6);

        std::stringstream tmp_str1;
        tmp_str1 << sqlite3_column_text(stmt, 9);
        std::string imgNAME1 = tmp_str1.str();
        std::stringstream tmp_str2;
        tmp_str2 << sqlite3_column_text(stmt, 10);
        std::string imgNAME2 = tmp_str2.str();
        if( (match.image1 != imgNAME1) || (match.image2 != imgNAME2) )
        {
            std::cout << "Inconsistent inlier matches pair retrieval!!! Check your image ID and NAME consistency!!!" << std::endl;
            return false;
        }

        match.twoview_info.rotation_2[0] = R_vec[0];
        match.twoview_info.rotation_2[1] = R_vec[1];
        match.twoview_info.rotation_2[2] = R_vec[2];
        match.twoview_info.position_2[0] = t_vec[0];
        match.twoview_info.position_2[1] = t_vec[1];
        match.twoview_info.position_2[2] = t_vec[2];
        std::cout << "match.twoview_info.rotation_2 = " << match.twoview_info.rotation_2 << std::endl;
        std::cout << "match.twoview_info.position_2 = " << match.twoview_info.position_2 << std::endl;
        // should it take normalized translation vectors or the normal one???
        match.twoview_info.position_2.normalize();  // It is not mandatory for input from DeMoN, since the translation vectors of prediction is already a unit vector
        std::cout << "match.twoview_info.position_2 after normalized = " << match.twoview_info.position_2 << std::endl;
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

bool import_inlier_matches_from_DB_byNAME1andNAME2(theia::ImagePairMatch &match, const std::string& name1, const std::string& name2)
{
    bool found = false;
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    ss << "select * from inlier_matches where image_name1 = '" << name1 << "' AND image_name2 = '" << name2 << "';";
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

    // only 1 row should be selected from inlier_matches database table!
    // ToDo: check if the row is 1!!!
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        uint32_t num_rows = sqlite3_column_int(stmt, 1);
        uint32_t num_cols = sqlite3_column_int(stmt, 2);
        printf("TEST: pair_id = %lld, rows = %d, cols = %d, config = %d, imgID1 = %d, imgID2 = %d, image_name2 = %s, image_name2 = %s\n", sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 4), sqlite3_column_int(stmt, 5), sqlite3_column_int(stmt, 6), sqlite3_column_text(stmt, 9), sqlite3_column_text(stmt, 10));
        //printf("TYPE: pair_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d, config = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4));
        found = true;
        //std::cout << "match data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        //std::cout << "match data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of uint32_t in bytes = " << sizeof(uint32_t) <<std::endl;
        // Get the pointer to data
        uint32_t* p = (uint32_t*)sqlite3_column_blob(stmt,3);
        uint32_t match_size_inBytes = sqlite3_column_bytes(stmt, 3);

        //std::cout << "match data size in bytes = " << match_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            //std::cout << "match pair = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
            theia::FeatureCorrespondence feat_match;
            feat_match.feature1 = recover_theia_2D_coord_from_1D_idx(p[i*2]);
            feat_match.feature2 = recover_theia_2D_coord_from_1D_idx(p[i*2+1]);
            match.correspondences.push_back(feat_match);
            //theia::Feature tmp2Dcoord;
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(256);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(255);
            //tmp2Dcoord = recover_theia_2D_coord_from_1D_idx(254);
            //std::cout << "match pair in 2D_coordinate = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
        }

        // get the rotation and translation
        float* R_vec = (float*)sqlite3_column_blob(stmt, 7);
        float* t_vec = (float*)sqlite3_column_blob(stmt, 8);
        uint32_t rot_size_inBytes = sqlite3_column_bytes(stmt, 7);
        std::cout << "rot_size_inBytes = " << rot_size_inBytes << std::endl;
        match.twoview_info.focal_length_1 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.focal_length_2 = 2457.60; // default focal length is set to 2737.64256,(southbuilding dataset)
        match.twoview_info.num_verified_matches = num_rows;
        match.twoview_info.visibility_score = num_rows; // temporary solution; should be calculated in a proper way with Theia
        match.twoview_info.imgID1 = sqlite3_column_int(stmt, 5);
        match.twoview_info.imgID2 = sqlite3_column_int(stmt, 6);

        std::stringstream tmp_str1;
        tmp_str1 << sqlite3_column_text(stmt, 9);
        std::string imgNAME1 = tmp_str1.str();
        std::stringstream tmp_str2;
        tmp_str2 << sqlite3_column_text(stmt, 10);
        std::string imgNAME2 = tmp_str2.str();
        if( (match.image1 != imgNAME1) || (match.image2 != imgNAME2) )
        {
            std::cout << "Inconsistent inlier matches pair retrieval!!! Check your image ID and NAME consistency!!!" << std::endl;
            return false;
        }

        match.twoview_info.rotation_2[0] = R_vec[0];
        match.twoview_info.rotation_2[1] = R_vec[1];
        match.twoview_info.rotation_2[2] = R_vec[2];
        match.twoview_info.position_2[0] = t_vec[0];
        match.twoview_info.position_2[1] = t_vec[1];
        match.twoview_info.position_2[2] = t_vec[2];
        std::cout << "match.twoview_info.rotation_2 = " << match.twoview_info.rotation_2 << std::endl;
        std::cout << "match.twoview_info.position_2 = " << match.twoview_info.position_2 << std::endl;
        match.twoview_info.position_2.normalize();  // It is not mandatory for input from DeMoN, since the translation vectors of prediction is already a unit vector
        std::cout << "match.twoview_info.position_2 after normalized = " << match.twoview_info.position_2 << std::endl;
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

bool import_camera_from_DB(std::vector<theia::CameraIntrinsicsPrior> &camera_intrinsics_prior, std::vector<int> &cam_ids_by_image)
{
    bool found = false;
    theia::CameraIntrinsicsPrior param;

    sqlite3* db;
    sqlite3_stmt* stmt;
    std::stringstream ss;

    // create sql statement string
    ss << "select * from cameras;";
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
    int camIdx = 0;
    while((ret_code = sqlite3_step(stmt)) == SQLITE_ROW) {
        // printf("TEST: camera_id = %d, camera_model_id = %d, width = %d, height = %d, prior_focal_length = %g\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3), sqlite3_column_double(stmt, 5));
        // printf("Type: camera_id = %d, camera_model_id = %d, width = %d, height = %d, params = %d, prior_focal_length = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4), sqlite3_column_type(stmt, 5));
        printf("TEST: camera_model_id = %d, width = %d, height = %d, prior_focal_length = %g\n", sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3), sqlite3_column_double(stmt, 5));
        printf("Type: camera_model_id = %d, width = %d, height = %d, params = %d, prior_focal_length = %d\n", sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4), sqlite3_column_type(stmt, 5));

        found = true;
        // std::cout << "tmpParamsBytes = " << sqlite3_column_blob(stmt, 4) <<std::endl;
        // std::cout << "tmpParamsBytes = " << sqlite3_column_bytes(stmt, 4) <<std::endl;
        // Get the pointer to data
        double* p = (double*)sqlite3_column_blob(stmt,4);
        std::cout << "tmpParamsBytes 0 (focal_length_scaled) = " << p[0] <<std::endl; // focal_length_scaled
        std::cout << "tmpParamsBytes 1 (half_width_scaled x) = " << p[1] <<std::endl; //half_width_scaled x
        std::cout << "tmpParamsBytes 2 (half_height_scaled y) = " << p[2] <<std::endl; //half_height_scaled y
        std::cout << "tmpParamsBytes 3 (0) = " << p[3] <<std::endl;
        int img_width = sqlite3_column_int(stmt, 2);
        int img_height = sqlite3_column_int(stmt, 3);
        std::cout << "img_width = " << img_width <<std::endl;
        std::cout << "img_height = " << img_height <<std::endl;
        /*
        // cam intrinsics compatible with DeMoN training dataset
        param.aspect_ratio.value[0] = 1.0;
        param.aspect_ratio.is_set = true;
        param.focal_length.value[0] = 228,136871;
        param.focal_length.is_set = true;
        param.image_width = DeMoN_Width;
        param.image_height = DeMoN_Height;
        param.principal_point.value[0] = DeMoN_Width/2;
        param.principal_point.is_set = true;
        param.principal_point.value[1] = DeMoN_Height/2;
        param.skew.value[0] = 0.0;
        param.skew.is_set = true;
        param.radial_distortion.value[0] = 0.0;
        param.radial_distortion.value[1] = 0.0;
        param.radial_distortion.is_set = false;
        */
        // read from database file
        param.aspect_ratio.value[0] = 1.0;
        param.aspect_ratio.is_set = true;
        param.focal_length.value[0] = p[0];
        if(p[0]!=2457.60)
            std::cout << "database focal length is " << p[0] << ", which should be 2457.60" << std::endl;
        param.focal_length.is_set = true;
        param.image_width = img_width;
        param.image_height = img_height;
        param.principal_point.value[0] = p[1];
        param.principal_point.is_set = true;
        param.principal_point.value[1] = p[2];
        param.skew.value[0] = 0.0;
        param.skew.is_set = true;
        param.radial_distortion.value[0] = 0.0;
        param.radial_distortion.value[1] = 0.0;
        param.radial_distortion.is_set = false;
        std::cout << "cam_ids_by_image[camIdx] = " << cam_ids_by_image[camIdx] << std::endl;
        camera_intrinsics_prior[cam_ids_by_image[camIdx]-1] = param;
        camIdx++;
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

void set_camera_params(theia::CameraIntrinsicsPrior &params, int _id = 0)
{
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
        params.focal_length.value[0] = 2457.60;
        params.focal_length.is_set = true;
        params.image_width = 3072;
        params.image_height = 2304;
        params.principal_point.value[0] = 1536;
        params.principal_point.is_set = true;
        params.principal_point.value[1] = 1152;
        params.skew.value[0] = 0.0;
        params.skew.is_set = true;
        params.radial_distortion.value[0] = 0.0;
        params.radial_distortion.value[1] = 0.0;
        params.radial_distortion.is_set = false;
        std::cout << "cam params set!" << std::endl;

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
            std::cout << "keypoint data = (" << p[i*4] << ", " << p[i*4+1] << ", " << p[i*4+2] << ", " << p[i*4+3] << ")" <<std::endl;
            kp_array[i][0] = p[i*4];
            kp_array[i][1] = p[i*4+1];
            kp_array[i][2] = p[i*4+2];
            kp_array[i][3] = p[i*4+3];
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

bool import_keypoints_by_image_id_from_DB(std::vector<std::array<float, 4>> &kp_array_by_img, int _id = 0)
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
            std::cout << "keypoint data = (" << p[i*4] << ", " << p[i*4+1] << ", " << p[i*4+2] << ", " << p[i*4+3] << ")" <<std::endl;
            std::array<float, 4> kp;
            kp[0] = p[i*4];
            kp[1] = p[i*4+1];
            kp[2] = p[i*4+2];
            kp[3] = p[i*4+3];
            kp_array_by_img.push_back(kp);
            //kp_array[i][0] = p[i*4];
            //kp_array[i][1] = p[i*4+1];
            //kp_array[i][2] = p[i*4+2];
            //kp_array[i][3] = p[i*4+3];
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


bool import_keypoints_all_images_from_DB(std::vector<std::vector<std::array<float, 4>>> &kp_array_all_images, int _id = 0)
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
        std::vector<std::array<float, 4>> kp_array_by_image;
        std::cout << "keypoint data size in bytes = " << keypoints_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            if(i==num_rows-1) {std::cout << "keypoint data = (" << p[i*4] << ", " << p[i*4+1] << ", " << p[i*4+2] << ", " << p[i*4+3] << ")" <<std::endl;}
            std::array<float, 4> kp;
            kp[0] = p[i*4];
            kp[1] = p[i*4+1];
            kp[2] = p[i*4+2];
            kp[3] = p[i*4+3];
            //kp_array_by_image.push_back(kp);
            kp_array_by_image.push_back(kp);
            //kp_array[i][0] = p[i*4];
            //kp_array[i][1] = p[i*4+1];
            //kp_array[i][2] = p[i*4+2];
            //kp_array[i][3] = p[i*4+3];
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


// Compute the visibility score of the inliers in the images.
int ComputeVisibilityScoreOfInliers_Copy(
    const theia::CameraIntrinsicsPrior& intrinsics1,
    const theia::CameraIntrinsicsPrior& intrinsics2,
    const std::vector<theia::FeatureCorrespondence>& correspondences,
    const std::vector<int>& inlier_indices) {
  static const int kNumPyramidLevels = 6;
  // If the image dimensions are not available, do not make any assumptions
  // about what they might be. Instead, we return the number of inliers as a
  // default.
  if (intrinsics1.image_width == 0 || intrinsics1.image_height == 0 ||
      intrinsics2.image_width == 0 || intrinsics2.image_height == 0) {
    return inlier_indices.size();
  }

  // Compute the visibility score for all inliers.
  theia::VisibilityPyramid pyramid1(
      intrinsics1.image_width, intrinsics1.image_height, kNumPyramidLevels);
  theia::VisibilityPyramid pyramid2(
      intrinsics2.image_width, intrinsics2.image_height, kNumPyramidLevels);
  for (const int i : inlier_indices) {
    const theia::FeatureCorrespondence& match = correspondences[i];
    pyramid1.AddPoint(match.feature1);
    pyramid2.AddPoint(match.feature2);
  }
  // Return the summed score.
  return pyramid1.ComputeScore() + pyramid2.ComputeScore();
}

// The following function reads from colmap database file and store them in theia objects (image_files, matches, cam_intrinsic_prior)
// Then write those values to match file (only inlier matches, the checking was done in Johannes's python code) by cereal serialization, which is compatible for theia input format!
void write_DB_matches_to_matchfile_cereal(const std::string & filename) //std::vector<std::string> &image_files,
{
    std::vector<std::string> image_files;
    std::vector<std::vector<Pt>> points;
    std::vector<int> cam_ids_by_image;

    // retrieve all images and corresponding camera ids (e.g. 128 images for southbuilding dataset)
    // Here I try to use identical cam_id and image_id, all 1-based! i.e. for southbuilding dataset, image_id = 1 - 128!
    if(!import_image_from_DB(image_files, cam_ids_by_image))
    {
        std::cout << "importing images from database failed! please check your database format (original colmap version or modified with Rt one)!" << std::endl;
    }

    if(image_files.size()!=cam_ids_by_image.size())
    {
        std::cout << "Warning (write_DB_matches_to_matchfile_cereal): inconsistent image and cam id numbers!" << std::endl;
    }

    // for (auto i = image_files.begin(); i != image_files.end(); ++i)
    // {
    //     std::cout << *i << ", ";
    // }
    // std::cout<<std::endl;

    // std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    // theia::CameraIntrinsicsPrior params;
    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior(cam_ids_by_image.size());

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
    std::cout << "test1~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
    if(!import_camera_from_DB(camera_intrinsics_prior, cam_ids_by_image))
    {
        std::cout << "importing camera intrinsic priors from database failed!" << std::endl;
    }
    std::cout << "test2~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

    // for(size_t i = 0; i < image_files.size(); ++i)
    // {
    //     set_camera_params(params);
    //     camera_intrinsics_prior.push_back(params);
    // }

    if(theia::WriteCalibration(outputCalibrFilePath, image_files, camera_intrinsics_prior))
    {
        std::cout << "calibration file was written successfully!" << std::endl;
    }

    // think about the keypoints data (Johannes may not use this one)
    bool retrieveKPbool;
    //std::vector<std::array<float, 4>> kp_array_by_img;
    std::vector<std::vector<std::array<float, 4>>> kp_array_all_images;
    retrieveKPbool = import_keypoints_all_images_from_DB(kp_array_all_images);



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
            // importInlierMatchDB = import_inlier_matches_from_DB(match, cur_pair_id);
            // importInlierMatchDB = import_inlier_matches_from_DB_byNAME1andNAME2(match, match.image1, match.image2);
            std::string pair_name =  match.image1 + "---" + match.image2;
            std::cout << "to be selected: pair_name = " << pair_name << std::endl;
            importInlierMatchDB = import_inlier_matches_from_DB_byPairNames(match, pair_name);

            if( importInlierMatchDB == true )
            {
                matches.push_back(match);
            }
            else
            {
                std::cout << "image pair (" << match.image1 << ", " << match.image2 << ") is skipped because there are no inlier matches!" << std::endl;
            }
            match.correspondences.clear();
        }
    }


    for(int matchIdx = 0;matchIdx<matches.size();matchIdx++)
    {
        // std::cout << "before: matches[matchIdx].twoview_info.visibility_score = " << matches[matchIdx].twoview_info.visibility_score << std::endl;
        std::vector<int> inlier_indices;
        for(int i = 0;i<matches[matchIdx].correspondences.size();i++)
        {
            inlier_indices.push_back(i);
        }

        matches[matchIdx].twoview_info.visibility_score = ComputeVisibilityScoreOfInliers_Copy(camera_intrinsics_prior[matches[matchIdx].twoview_info.imgID1-1],
                                                camera_intrinsics_prior[matches[matchIdx].twoview_info.imgID2-1], matches[matchIdx].correspondences, inlier_indices);
        // std::cout << "after: matches[matchIdx].twoview_info.visibility_score = " << matches[matchIdx].twoview_info.visibility_score << std::endl;
    }

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

    // DEBUG: write all matches pair (Relative Poses) to a text file just for debugging!
    const std::string relative_poses_file = "/home/kevin/JohannesCode/RelativePosesFromTheia.txt";
    std::ofstream ofs_matches(relative_poses_file.c_str(), std::ios::out);
    if (!ofs_matches.is_open()) {
      LOG(ERROR) << "Cannot write the relative poses file from " << relative_poses_file;
      //return false;
    }

    for(int matchIdx = 0;matchIdx<matches.size();matchIdx++)
    {
        ofs_matches << matches[matchIdx].twoview_info.imgID1 << " " << matches[matchIdx].image1 << " "<< matches[matchIdx].twoview_info.imgID2 << " " << matches[matchIdx].image2 << " ";

        ofs_matches << matches[matchIdx].twoview_info.rotation_2[0] << " " << matches[matchIdx].twoview_info.rotation_2[1] << " " << matches[matchIdx].twoview_info.rotation_2[2] << " ";
        ofs_matches << matches[matchIdx].twoview_info.position_2[0] << " " << matches[matchIdx].twoview_info.position_2[1] << " " << matches[matchIdx].twoview_info.position_2[2];

        ofs_matches << "\n";
    }
    ofs_matches.close();
    std::cout << "relative poses from theia is written!" << std::endl;

}


void import_DeMoN_relativeRt_from_DB_matches(const std::string & filename)
{
    std::vector<std::string> image_files;
    std::vector<std::vector<Pt>> points;
    std::vector<int> cam_ids_by_image;

    // retrieve all images and corresponding camera ids (e.g. 128 images for southbuilding dataset)
    // Here I try to use identical cam_id and image_id, all 1-based! i.e. for southbuilding dataset, image_id = 1 - 128!
    if(!import_image_from_DB(image_files, cam_ids_by_image))
    {
        std::cout << "importing images from database failed! please check your database format (original colmap version or modified with Rt one)!" << std::endl;
    }

    if(image_files.size()!=cam_ids_by_image.size())
    {
        std::cout << "Warning (import_DeMoN_relativeRt_from_DB_matches): inconsistent image and cam id numbers!" << std::endl;
    }

    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior(cam_ids_by_image.size());

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
    if(!import_camera_from_DB(camera_intrinsics_prior, cam_ids_by_image))
    {
        std::cout << "importing camera intrinsic priors from database failed!" << std::endl;
    }

    // for(size_t i = 0; i < image_files.size(); ++i)
    // {
    //     set_camera_params(params);
    //     camera_intrinsics_prior.push_back(params);
    // }

    if(theia::WriteCalibration(outputCalibrFilePath, image_files, camera_intrinsics_prior))
    {
        std::cout << "calibration file was written successfully!" << std::endl;
    }

    // think about the keypoints data (Johannes may not use this one)
    bool retrieveKPbool;
    std::vector<std::vector<std::array<float, 4>>> kp_array_all_images;
    retrieveKPbool = import_keypoints_all_images_from_DB(kp_array_all_images);



    bool importInlierMatchDB;
    std::vector<theia::ImagePairMatch> matches;

    for(int img_id1 = 0;img_id1<image_files.size();img_id1++)
    {
        for(int img_id2 = img_id1+1;img_id2<image_files.size();img_id2++)
        {
            theia::ImagePairMatch match;
            match.image1 = image_files[img_id1];
            match.image2 = image_files[img_id2];
            long long unsigned int cur_pair_id = image_ids_to_pair_id((img_id1+1), (img_id2+1));    // be careful about the index start! c++ vector index starts from 0 while image id from 1!
            // importInlierMatchDB = import_inlier_matches_from_DB(match, cur_pair_id);
            // importInlierMatchDB = import_inlier_matches_from_DB_byNAME1andNAME2(match, match.image1, match.image2);
            std::string pair_name =  match.image1 + "---" + match.image2;
            std::cout << "to be selected: pair_name = " << pair_name << std::endl;
            importInlierMatchDB = import_inlier_matches_from_DB_byPairNames(match, pair_name);

            if( importInlierMatchDB == true )
            {
                matches.push_back(match);
            }
            else
            {
                std::cout << "image pair (" << match.image1 << ", " << match.image2 << ") is skipped because there are no inlier matches!" << std::endl;
            }
            match.correspondences.clear();
        }
    }


    for(int matchIdx = 0;matchIdx<matches.size();matchIdx++)
    {
        // std::cout << "before: matches[matchIdx].twoview_info.visibility_score = " << matches[matchIdx].twoview_info.visibility_score << std::endl;
        std::vector<int> inlier_indices;
        for(int i = 0;i<matches[matchIdx].correspondences.size();i++)
        {
            inlier_indices.push_back(i);
        }

        matches[matchIdx].twoview_info.visibility_score = ComputeVisibilityScoreOfInliers_Copy(camera_intrinsics_prior[matches[matchIdx].twoview_info.imgID1-1],
                                                camera_intrinsics_prior[matches[matchIdx].twoview_info.imgID2-1], matches[matchIdx].correspondences, inlier_indices);
        // std::cout << "after: matches[matchIdx].twoview_info.visibility_score = " << matches[matchIdx].twoview_info.visibility_score << std::endl;
    }

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

    // DEBUG: write all matches pair (Relative Poses) to a text file just for debugging!
    const std::string relative_poses_file = "/home/kevin/JohannesCode/RelativePosesFromTheia.txt";
    std::ofstream ofs_matches(relative_poses_file.c_str(), std::ios::out);
    if (!ofs_matches.is_open()) {
      LOG(ERROR) << "Cannot write the relative poses file from " << relative_poses_file;
      //return false;
    }

    for(int matchIdx = 0;matchIdx<matches.size();matchIdx++)
    {
        ofs_matches << matches[matchIdx].twoview_info.imgID1 << " " << matches[matchIdx].image1 << " "<< matches[matchIdx].twoview_info.imgID2 << " " << matches[matchIdx].image2 << " ";

        ofs_matches << matches[matchIdx].twoview_info.rotation_2[0] << " " << matches[matchIdx].twoview_info.rotation_2[1] << " " << matches[matchIdx].twoview_info.rotation_2[2] << " ";
        ofs_matches << matches[matchIdx].twoview_info.position_2[0] << " " << matches[matchIdx].twoview_info.position_2[1] << " " << matches[matchIdx].twoview_info.position_2[2];

        ofs_matches << "\n";
    }
    ofs_matches.close();
    std::cout << "relative poses from theia is written!" << std::endl;

}

int main()
{
    // init data placeholders to save matches data from colmap database file.
    //std::vector<std::string> image_files;
    //std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    //std::vector<theia::ImagePairMatch> image_matches;

    //bool testDB;
//    testDB = import_camera_from_DB();
//   testDB = import_image_from_DB();
//    testDB = import_inlier_matches_from_DB();
    // testDB = import_keypoints_from_DB(1);

    // std::unordered_map<theia::ViewId, Eigen::Matrix3d> orientations_;
    // std::unordered_map<theia::ViewId, Eigen::Vector3d> positions_;
    // std::unordered_map<theia::ViewId, std::string> image_names_;
    std::vector<Eigen::Matrix3d> orientations_;
    std::vector<Eigen::Vector3d> positions_;
    std::vector<std::string> image_names_;
    // read relative poses from colmap result!
    const std::string colmap_global_extrinsics_file = "/home/kevin/JohannesCode/ws1/sparse/0/textfiles_final/images.txt";
    std::ifstream ifs(colmap_global_extrinsics_file.c_str(), std::ios::in);
    if (!ifs.is_open()) {
      LOG(ERROR) << "Cannot read the colmap_global_extrinsics_file from " << colmap_global_extrinsics_file;
      return false;
    }
    // theia viewid is 0-based, using the index in C++ directly
    // while the real view id/ cam id/ image id is 1-based, e.g. for southbuilding image 1 to 128!
    std::string line;
    bool featureLine = false;
    while ( std::getline (ifs,line) )
    {
        // skip any header lines or comment lines
        if(line[0]=='#' || featureLine==true)
        {
            featureLine = false;
            continue;
        }
        // std::pair<theia::ViewId, Eigen::Matrix3d> rotation;
        // std::pair<theia::ViewId, Eigen::Vector4d> qvec;
        // std::pair<theia::ViewId, Eigen::Vector3d> position;
        // std::pair<theia::ViewId, std::string> image_name;
        Eigen::Matrix3d rotation;
        Eigen::Vector4d qvec;
        Eigen::Vector3d position;
        std::string image_name;
        // std::string tmpName;
        int tmpCamID;
        std::istringstream iss(line);
        // if (!(iss >> rotation.first >> qvec.second[0] >> qvec.second[1] >> qvec.second[2] >> qvec.second[3] >> position.second[0] >> position.second[1] >> position.second[2] >> tmpCamID >> image_name.second >> rotation.second(0,0) >> rotation.second(0,1) >> rotation.second(0,2) >> rotation.second(1,0) >> rotation.second(1,1) >> rotation.second(1,2) >> rotation.second(2,0) >> rotation.second(2,1) >> rotation.second(2,2)))
        if (!(iss >> tmpCamID >> qvec[0] >> qvec[1] >> qvec[2] >> qvec[3] >> position[0] >> position[1] >> position[2] >> tmpCamID >> image_name >> rotation(0,0) >> rotation(0,1) >> rotation(0,2) >> rotation(1,0) >> rotation(1,1) >> rotation(1,2) >> rotation(2,0) >> rotation(2,1) >> rotation(2,2)))
        {
            break;
        } // error
        //rotation.first = rotation.first - 1;//theia viewid is 0-based, using the index in C++ directly
        //rotation.first = rotation.first;//1-based viewID can be used which is compatible with the data I saved in the matchfile
        // position.first = rotation.first;
        // image_name.first = rotation.first;
        // std::cout << position.second[0] << " " << position.second[1] << " " << position.second[2] << std::endl;
        // //std::cout << rotation.second(0,0) << " " << rotation.second(0,1) << " " << rotation.second(0,2) << " " << rotation.second(1,0) << " " << rotation.second(1,1) << " " << rotation.second(1,2) << " " << rotation.second(2,0) << " " << rotation.second(2,1) << " " << rotation.second(2,2) << std::endl;
        // //LOG(ERROR) << rotation.second(0,0) << " " << rotation.second(0,1) << " " << rotation.second(0,2) << " " << rotation.second(1,0) << " " << rotation.second(1,1) << " " << rotation.second(1,2) << " " << rotation.second(2,0) << " " << rotation.second(2,1) << " " << rotation.second(2,2) << std::endl;
        // rotation.second = rotation.second.transpose();  // colmap result is extrinsic [R|t]???? theia camera use global pose [Rc|C]?
        // position.second = - (rotation.second.transpose() *  position.second);
        // rotation.second = rotation.second;  // colmap result is extrinsic [R|t]???? the same with theia?
        // position.second = - (rotation.second.transpose() *  position.second);   // colmap keeps extrinsic t, while theia has camera position C?
        position = - (rotation.transpose() *  position);   // colmap keeps extrinsic t, while theia has camera position C?
        orientations_.push_back(rotation); // colmap result is global pose? not extrinsic R, t????
        positions_.push_back(position);
        image_names_.push_back(image_name);
        // std::cout << rotation.second(0,0) << " " << rotation.second(0,1) << " " << rotation.second(0,2) << " " << rotation.second(1,0) << " " << rotation.second(1,1) << " " << rotation.second(1,2) << " " << rotation.second(2,0) << " " << rotation.second(2,1) << " " << rotation.second(2,2) << std::endl;
        // std::cout << position.second[0] << " " << position.second[1] << " " << position.second[2] << std::endl;
        featureLine = true;
    }
    ifs.close();



    std::string theia_matches_file = "/home/kevin/JohannesCode/theia_trial_demon/matchfiles/matchefile_05122017.cereal";
    std::vector<std::string> theia_view_names;
    std::vector<theia::CameraIntrinsicsPrior> theia_camera_intrinsics_prior;
    std::vector<theia::ImagePairMatch> theia_matches;
    std::vector<uint32_t> matchIdx_ToBeRemoved;

    if(!ReadMatchesAndGeometry(theia_matches_file, &theia_view_names, &theia_camera_intrinsics_prior, &theia_matches))
    {
        std::cout << "reading from pre-saved theia matches file fails! the path is set to " << theia_matches_file << std::endl;
    }
    std::cout << "theia_view_names[0] = " << theia_view_names[0] << std::endl;
    std::cout << "theia_matches[0].image1 = " << theia_matches[0].image1 << std::endl;
    std::cout << "theia_matches[0].image2 = " << theia_matches[0].image2 << std::endl;



    for(uint32_t match_idx = 0;match_idx<theia_matches.size();match_idx++)
    {
        theia::ImagePairMatch cur_match;
        cur_match.image1 = theia_matches[match_idx].image1;
        cur_match.image2 = theia_matches[match_idx].image2;
        std::string pair_name =  cur_match.image1 + "---" + cur_match.image2;
        std::cout << "to be changed: pair_name = " << pair_name << std::endl;
        bool importMatchRtFromDB = import_inlier_matches_from_DB_byPairNames(cur_match, pair_name);

        std::cout << "Before : theia_matches[match_idx].twoview_info.rotation_2 =[" << theia_matches[match_idx].twoview_info.rotation_2[0] << ", " << theia_matches[match_idx].twoview_info.rotation_2[1] << ", " << theia_matches[match_idx].twoview_info.rotation_2[2] << "]" << std::endl;
        std::cout << "Before : theia_matches[match_idx].twoview_info.position_2 = [" << theia_matches[match_idx].twoview_info.position_2[0] << ", " << theia_matches[match_idx].twoview_info.position_2[1] << ", " << theia_matches[match_idx].twoview_info.position_2[2] << "]" << std::endl;

        //Eigen::Vector3d relativeR12aa;
        //Eigen::Vector3d relativePos12aa;
        if( importMatchRtFromDB == true )
        {
            //relativeR12aa = cur_match.twoview_info.rotation_2;
            //relativePos12aa = cur_match.twoview_info.position_2;
            theia_matches[match_idx].twoview_info.rotation_2 = cur_match.twoview_info.rotation_2;
            theia_matches[match_idx].twoview_info.position_2 = cur_match.twoview_info.position_2;
        }
        else
        {
            std::cout << "image pair (" << cur_match.image1 << ", " << cur_match.image2 << ") does not exist in the provided database!" << std::endl;
            //return 0;
            matchIdx_ToBeRemoved.push_back(match_idx);
        }

        theia_matches[match_idx].twoview_info.focal_length_1 = 2457.60;
        theia_matches[match_idx].twoview_info.focal_length_2 = 2457.60;
        std::cout << "After : theia_matches[match_idx].twoview_info.rotation_2 =[" << theia_matches[match_idx].twoview_info.rotation_2[0] << ", " << theia_matches[match_idx].twoview_info.rotation_2[1] << ", " << theia_matches[match_idx].twoview_info.rotation_2[2] << "]" << std::endl;
        std::cout << "After : theia_matches[match_idx].twoview_info.position_2 = [" << theia_matches[match_idx].twoview_info.position_2[0] << ", " << theia_matches[match_idx].twoview_info.position_2[1] << ", " << theia_matches[match_idx].twoview_info.position_2[2] << "]" << std::endl;
    }
    std::cout << "Before Clean: theia_matches.size() = " << theia_matches.size() << std::endl;
    for(int matchToBeRemoved = matchIdx_ToBeRemoved.size()-1; matchToBeRemoved>=0; matchToBeRemoved--)
    {
        theia_matches.erase(theia_matches.begin()+matchIdx_ToBeRemoved[matchToBeRemoved]);
    }
    std::cout << "After Clean: theia_matches.size() = " << theia_matches.size() << std::endl;

    for(int camCalibrID = 0; camCalibrID<theia_camera_intrinsics_prior.size(); camCalibrID++)
    {
        theia_camera_intrinsics_prior[camCalibrID].focal_length.value[0] = 2457.60;
    }

    // write_DB_matches_to_matchfile_cereal("testfile.cereal");
    // std::string tmpStr = theia_matches_file.erase(theia_matches_file.c_str().end()-7);
    std::string tmpStr = theia_matches_file.substr(0, theia_matches_file.size()-7);
    std::string output_theia_matches_file = tmpStr.append("_DeMoNpredictions.cereal");
    if(!WriteMatchesAndGeometry(output_theia_matches_file.c_str(), theia_view_names, theia_camera_intrinsics_prior, theia_matches))
    {
        std::cout << "saving modified theia matches file fails in the path " << output_theia_matches_file << std::endl;
    }
    return 0;
}
