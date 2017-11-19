import os
import argparse
import subprocess
import collections
import sqlite3
import h5py
import numpy as np

import time
import datetime
import random
import struct

SIMPLE_RADIAL_CAMERA_MODEL = 2


def parse_args():
    parser = argparse.ArgumentParser()
    # parser.add_argument("--colmap_path", required=True)
    # parser.add_argument("--demon_path", required=True)
    parser.add_argument("--database_path", required=True)
    # parser.add_argument("--image_scale", type=float, default=12)
    # parser.add_argument("--focal_length", type=float, default=228.13688)
    # parser.add_argument("--max_reproj_error", type=float, default=1)
    args = parser.parse_args()
    return args


def image_ids_to_pair_id(image_id1, image_id2):
    if image_id1 > image_id2:
        return 2147483647 * image_id2 + image_id1
    else:
        return 2147483647 * image_id1 + image_id2


def add_image(connection, cursor, focal_length, image_name, image_shape, image_scale):
    cursor.execute("SELECT image_id FROM images WHERE name=?;", (image_name, ))
    for row in cursor:
        return row[0]

    camera_params = np.array([image_scale * focal_length,
                              image_scale * image_shape[1] / 2,
                              image_scale * image_shape[0] / 2, 0],
                              dtype=np.float64)
    cursor.execute("INSERT INTO cameras(model, width, height, params, "
                   "prior_focal_length) VALUES(?, ?, ?, ?, 0);",
                   (SIMPLE_RADIAL_CAMERA_MODEL,
                    image_scale * image_shape[1],
                    image_scale * image_shape[0],
                    camera_params))
    camera_id = cursor.lastrowid

    cursor.execute("INSERT INTO images(name, camera_id, prior_qw, prior_qx, "
                   "prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) "
                   "VALUES(?, ?, 0, 0, 0, 0, 0, 0, 0);",
                   (image_name, camera_id))
    image_id = cursor.lastrowid

    y, x = np.mgrid[0:image_shape[0], 0:image_shape[1]]
    x = image_scale * (0.5 + x.ravel().astype(np.float32))
    y = image_scale * (0.5 + y.ravel().astype(np.float32))
    o = s = np.zeros_like(x)
    keypoints = np.column_stack((x, y, o, s))
    cursor.execute("INSERT INTO keypoints(image_id, rows, cols, data) "
                   "VALUES(?, ?, ?, ?);",
                   (image_id, keypoints.shape[0], keypoints.shape[1],
                    memoryview(keypoints)))

    connection.commit()

    return image_id


def flow_from_depth(depth, R, T, F):
    K = np.eye(3)
    K[0, 0] = K[1, 1] = F
    K[0, 2] = depth.shape[1] / 2
    K[1, 2] = depth.shape[0] / 2
    y, x = np.mgrid[0:depth.shape[0], 0:depth.shape[1]]
    z = 1 / depth[:]
    coords1 = np.column_stack((x.ravel(), y.ravel(), np.ones_like(x.ravel())))
    coords2 = np.dot(coords1, np.linalg.inv(K).T)
    coords2[:,0] *= z.ravel()
    coords2[:,1] *= z.ravel()
    coords2[:,2] = z.ravel()
    coords2 = np.dot(coords2, R[:].T) + T[:]
    coords2 = np.dot(coords2, K.T)
    coords2 /= coords2[:, 2][:, None]
    flow12 = coords2 - coords1
    flowx = flow12[:, 0].reshape(depth.shape) / depth.shape[1]
    flowy = flow12[:, 1].reshape(depth.shape) / depth.shape[0]
    return np.concatenate((flowx[None], flowy[None]), axis=0)


def flow_to_matches(flow):
    fx = np.round(flow[0] * flow.shape[2]).astype(np.int)
    fy = np.round(flow[1] * flow.shape[1]).astype(np.int)
    y1, x1 = np.mgrid[0:flow.shape[1], 0:flow.shape[2]]
    x2 = x1.ravel() + fx.ravel()
    y2 = y1.ravel() + fy.ravel()
    mask = (x2 >= 0) & (x2 < flow.shape[2]) & \
           (y2 >= 0) & (y2 < flow.shape[1])
    matches = np.zeros((mask.size, 2), dtype=np.uint32)
    matches[:, 0] = np.arange(mask.size)
    matches[:, 1] = y2 * flow.shape[2] + x2
    matches = matches[mask].copy()
    coords1 = np.column_stack((x1.ravel(), y1.ravel()))[mask]
    coords2 = np.column_stack((x2, y2))[mask]
    return matches, coords1, coords2


def cross_check_matches(matches12, coords121, coords122,
                        matches21, coords211, coords212,
                        max_reproj_error):
    if matches12.size == 0 or matches21.size == 0:
        return np.zeros((0, 2), dtype=np.uint32)

    matches121 = collections.defaultdict(list)
    for match, coord in zip(matches12, coords121):
        matches121[match[1]].append((match[0], coord))

    max_reproj_error = max_reproj_error**2

    matches = []
    for match, coord in zip(matches21, coords212):
        if match[0] not in matches121:
            continue
        match121 = matches121[match[0]]
        if len(match121) > 1:
            continue
        # if match121[0][0] == match[1]:
        #     matches.append((match[1], match[0]))
        diff = match121[0][1] - coord
        if diff[0] * diff[0] + diff[1] * diff[1] <= max_reproj_error:
            matches.append((match[1], match[0]))

    return np.array(matches, dtype=np.uint32)


def add_matches(connection, cursor, image_id1, image_id2,
                flow12, flow21, max_reproj_error):
    matches12, coords121, coords122 = flow_to_matches(flow12)
    matches21, coords211, coords212 = flow_to_matches(flow21)

    print("  => Found", matches12.size, "<->", matches21.size, "matches")

    matches = cross_check_matches(matches12, coords121, coords122,
                                  matches21, coords211, coords212,
                                  max_reproj_error)

    if matches.size == 0:
        return

    # matches = matches[::10].copy()

    print("  => Cross-checked", matches.size, "matches")

    cursor.execute("INSERT INTO inlier_matches(pair_id, rows, cols, data, "
                   "config) VALUES(?, ?, ?, ?, 3);",
                   (image_ids_to_pair_id(image_id1, image_id2),
                    matches.shape[0], matches.shape[1],
                    memoryview(matches)))

    connection.commit()


def read_images_from_db(db_file, read_all_bool = True, image_name = 'P1180141.JPG', print_items_bool = False):
    connection = sqlite3.connect(db_file)
    cursor = connection.cursor()

    try:
        # Execute the SQL command
        if read_all_bool == False:
            cursor.execute("SELECT * FROM images WHERE name=?;", (image_name,))
        else:
            cursor.execute("SELECT * FROM images")
        # Fetch all the rows in a list of lists.
        results = cursor.fetchall()
        if len(results) == 0:
            print("Warning: there is no image_name = ", image_name, " in the given database!")
        if print_items_bool==True:
            print(results)
        for row in results:
            image_id = row[0]
            name = row[1]
            camera_id = row[2]
            prior_qw = row[3]
            prior_qx = row[4]
            prior_qy = row[5]
            prior_qz = row[6]
            prior_tx = row[7]
            prior_ty = row[8]
            prior_tz = row[9]
            if print_items_bool==True:
                # Now print fetched result
                print ( "image_id = %s, name = %s, camera_id = %s, prior_qw = %s, prior_qx = %s, prior_qy = %s, prior_qz = %s, prior_tx = %s, prior_ty = %s, prior_tz = %s" % \
                 (image_id, name, camera_id, prior_qw, prior_qx, prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) )
                #   print ( "name = %s, camera_id = %s" % (name, camera_id) )
    except:
        print ("Error: unable to fetch images data")

    # disconnect from server
    cursor.close()
    connection.close()

def read_cameras_from_db(db_file, read_all_bool = True, camera_id = 2, print_items_bool = False):
    connection = sqlite3.connect(db_file)
    cursor = connection.cursor()

    try:
        # Execute the SQL command
        if read_all_bool == False:
            cursor.execute("SELECT * FROM cameras WHERE camera_id=?;", (camera_id,))
        else:
            cursor.execute("SELECT * FROM cameras")
        # Fetch all the rows in a list of lists.
        results = cursor.fetchall()
        if len(results) == 0:
            print("Warning: there is no camera_id = ", camera_id, " in the given database!")
        if print_items_bool==True:
            print(results)
        for row in results:
            item_idx = row[0]
            camera_id = row[1]
            width = row[2]
            height = row[3]
            #   params = struct.unpack('f', row[4])
            #   params = struct.unpack_from('d', row[4])
            prior_focal_length = row[5]
            tmpParamsBytes = row[4]
            focal_length_scaled = struct.unpack_from('d', tmpParamsBytes[0:8])
            # print(focal_length_scaled)
            imgHalfWidth_scaled = struct.unpack_from('d', tmpParamsBytes[8:16])
            # print(imgHalfWidth_scaled)
            imgHalfHeight_scaled = struct.unpack_from('d', tmpParamsBytes[16:24])
            # print(imgHalfHeight_scaled)
            Float64_4th_zero = struct.unpack_from('d', tmpParamsBytes[24:32])
            # print(Float64_4th_zero)

            camera_params_scaled = np.array([focal_length_scaled, imgHalfWidth_scaled, imgHalfHeight_scaled, Float64_4th_zero], dtype=np.float64)
            if print_items_bool==True:
                # Now print fetched result
                print ( "item_idx = %s, camera_id = %s, width = %s, height = %s, params = %s, prior_focal_length = %s" % (item_idx, camera_id, width, height, camera_params_scaled, prior_focal_length) )
    except:
        print("Error: unable to fetch cameras data")

    # disconnect from server
    cursor.close()
    connection.close()

def read_keypoints_from_db(db_file, read_all_bool = True, image_id = '0', print_items_bool = False):
    connection = sqlite3.connect(db_file)
    cursor = connection.cursor()
    try:
        # Execute the SQL command
        if read_all_bool == False:
            cursor.execute("SELECT * FROM keypoints WHERE image_id=?;", (image_id,))
        else:
            cursor.execute("SELECT * FROM keypoints")

        # Fetch all the rows in a list of lists.
        results = cursor.fetchall()
        if len(results) == 0:
            print("Warning: there is no image_id = ", image_id)
        for rowData in results:
            image_id = rowData[0]
            rows = rowData[1]
            cols = rowData[2]
            dataMemeoryViewBytes = rowData[3]    # data is the memoryview of the original matrix data, which dtype should be float32 (or other 32bits data type)
            if rows > 0:
                data = np.fromstring(rowData[3], dtype=np.float32).reshape(-1, cols)
                print("data shape = ", data.shape)
            else:
                print("Warning: no keypoints retrieved for image_id = ", image_id)
            # data = np.zeros(shape=(rows,cols), dtype=np.float32) # here the default dtype is float64! Be ware of the data in binary is 32 bits for keyports
            # for row_idx in range(rows):
            #     for col_idx in range(cols):
            #         # print((row_idx*cols+col_idx)*4)
            #         data[row_idx, col_idx] = struct.unpack_from( 'f', dataMemeoryViewBytes[(row_idx*cols+col_idx)*4:((row_idx*cols+col_idx)*4+4)] )[0]

            if print_items_bool == True:
                # Now print fetched result
                print( "image_id = %i, rows = %i, cols = %i, the data numpy array is as the following:\n" % (image_id, rows, cols) )
                if rows > 0:
                    print(data)

    except:
        print ("Error: unable to fetch keypoints data")
    # disconnect from server
    cursor.close()
    connection.close()

def read_inlier_matches_from_db(db_file, read_all_bool = True, pair_id = '264140488707', print_items_bool = False):
    connection = sqlite3.connect(db_file)
    cursor = connection.cursor()
    try:
        # Execute the SQL command
        if read_all_bool == False:
            cursor.execute("SELECT * FROM inlier_matches WHERE pair_id=?;", (pair_id,))
        else:
            cursor.execute("SELECT * FROM inlier_matches")

        # Fetch all the rows in a list of lists.
        results = cursor.fetchall()
        if len(results) == 0:
            print("Warning: there is no pair_id = ", pair_id, " in the given database!")
        for rowData in results:
            pair_id = rowData[0]
            rows = rowData[1]   # row = 0 and data is none when there are no inlier_matches?
            cols = rowData[2]
            dataMemeoryViewBytes = rowData[3]    # data is the memoryview of the original matrix data, which dtype should be np.uint32
            if rows > 0:
                data = np.fromstring(rowData[3], dtype=np.uint32).reshape(-1, 2)
                print("data shape = ", data.shape)
            else:
                print("Warning: no inlier_matches retrieved for pair_id = ", pair_id)
            config = rowData[4]
            # data = np.zeros(shape=(rows,cols), dtype=np.uint32) # here the default dtype is float64! Be ware of the data in binary is 32 bits for keyports
            # if rows <= 0:
            #     print("Warning: no inlier_matches retrieved for pair_id = ", pair_id)
            # for row_idx in range(rows):
            #     for col_idx in range(cols):
            #         # print((row_idx*cols+col_idx)*4)
            #         data[row_idx, col_idx] = struct.unpack_from( 'I', dataMemeoryViewBytes[(row_idx*cols+col_idx)*4:((row_idx*cols+col_idx)*4+4)] )[0]  # 'I' for np.uint32
            if print_items_bool == True:
                # Now print fetched result
                print( "pair_id = %i, rows = %i, cols = %i, config = %i, the data numpy array is as the following:\n" % (pair_id, rows, cols, config) )
                if rows > 0:
                    print(data)

    except:
        print ("Error: unable to fetch inlier_matches data")
    # disconnect from server
    cursor.close()
    connection.close()

###### check what tables the given database.db file contents and print out (or export) the contents if required
def check_all_tables_in_db(db_file, print_table_content_bool = False):
    conn = sqlite3.connect(db_file)
    c = conn.cursor()
    c.execute("SELECT name FROM sqlite_master WHERE type='table';")
    results = c.fetchall()
    print(results)
    for table in results:
        if print_table_content_bool == True:
            print("table: ", table[0])
            print("table.dtype: ", type(table[0]))
            curTmp = conn.cursor()
            SQLquery = "SELECT * FROM "+table[0]+";"
        #    yield list( c.execute(SQLquery) )
            curTmp.execute(SQLquery)
            tmp = curTmp.fetchall()
            print(tmp)
            curTmp.close()
    # disconnect from server
    c.close()
    conn.close()

def main():
    args = parse_args()

    # images = dict()
    # image_pairs = set()

    try:
        check_all_tables_in_db(args.database_path, False)
        # read_images_from_db(args.database_path, True, 'P1180141.JPG', True)
        # read_cameras_from_db(args.database_path, True, 2, True)
        read_keypoints_from_db(args.database_path, False, 1, True)
        # read_inlier_matches_from_db(args.database_path, True, 264140123703, True) # pair_id=264140488707 returns bytes data, while some of them return NoneType
    except:
        print ("Error: unable to fetch data in Main()")

if __name__ == "__main__":
    main()
