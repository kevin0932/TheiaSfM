#include <stdio.h>
#include <string>
#include <stdint.h>
//using std::string;
#include <sstream>
//using std::stringstream;
#include <iostream>
#include <theia/theia.h>

#include "sqlite3.h"

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
    if(sqlite3_open("/home/kevin/JohannesCode/south-building-demon/database.db", &db) != SQLITE_OK) {
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

bool import_inlier_matches_from_DB(int _id = 0)
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
    if(sqlite3_open("/home/kevin/JohannesCode/south-building-demon/database.db", &db) != SQLITE_OK) {
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
        printf("TYPE: pair_id = %d, rows = %d, cols = %d, dataMemeoryViewBytes = %d, config = %d\n", sqlite3_column_type(stmt, 0), sqlite3_column_type(stmt, 1), sqlite3_column_type(stmt, 2), sqlite3_column_type(stmt, 3), sqlite3_column_type(stmt, 4));
        found = true;
        std::cout << "match data = " << sqlite3_column_blob(stmt, 3) <<std::endl;
        std::cout << "match data size in bytes = " << sqlite3_column_bytes(stmt, 3) << "; size of uint32_t in bytes = " << sizeof(uint32_t) <<std::endl;
        // Get the pointer to data
        uint32_t* p = (uint32_t*)sqlite3_column_blob(stmt,3);
        uint32_t match_size_inBytes = sqlite3_column_bytes(stmt, 3);

        std::cout << "match data size in bytes = " << match_size_inBytes << std::endl;
        for(auto i=0;i<num_rows;i++)
        {
            std::cout << "match pair = (" << p[i*2] << ", " << p[i*2+1] << ")" <<std::endl;
        }

        /*std::cout << "match data 1 = " << p[1] <<std::endl;
        std::cout << "match data 2 = " << p[2] <<std::endl;
        std::cout << "match data 3 = " << p[3] <<std::endl;*/
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
    if(sqlite3_open("/home/kevin/JohannesCode/south-building-demon/database.db", &db) != SQLITE_OK) {
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
    if(sqlite3_open("/home/kevin/JohannesCode/south-building-demon/database.db", &db) != SQLITE_OK) {
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

// The following function reads from colmap database file and store them in theia objects (image_files, matches, cam_intrinsic_prior)
// Then write those values to match file (only inlier matches, the checking was done in Johannes's python code) by cereal serialization, which is compatible for theia input format!
void write_DB_matches_to_matchfile_cereal(std::vector<std::string> &image_files, const std::string & filename)
{
    std::vector<std::vector<Pt>> points;
    std::vector<int> cam_ids_by_image;
    bool importImgDB;
    importImgDB = import_image_from_DB(image_files, cam_ids_by_image);

    if(image_files.size()!=cam_ids_by_image.size())
    {
        std::cout << "Warning (write_DB_matches_to_matchfile_cereal): inconsistent image and cam id numbers!" << std::endl;
    }

    for (auto i = image_files.begin(); i != image_files.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    std::cout<<std::endl;

    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    theia::CameraIntrinsicsPrior params;
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

    bool importCamDB;
    int cam_model_id = 1;
    for(size_t i = 0; i < image_files.size(); ++i)
    {
        importCamDB = import_camera_from_DB(params, cam_model_id);
        camera_intrinsics_prior.push_back(params);
    }


    std::vector<theia::ImagePairMatch> matches;

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

int main()
{
    // init data placeholders to save matches data from colmap database file.
    std::vector<std::string> image_files;
    std::vector<theia::CameraIntrinsicsPrior> camera_intrinsics_prior;
    std::vector<theia::ImagePairMatch> image_matches;

    bool testDB;
//    testDB = import_camera_from_DB();
//   testDB = import_image_from_DB();
//    testDB = import_inlier_matches_from_DB();
    // testDB = import_keypoints_from_DB(1);
    write_DB_matches_to_matchfile_cereal(image_files,"testfile.cereal");
    return 0;
}
