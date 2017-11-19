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


def add_image(connection, cursor, focal_length,
              image_name, image_shape, image_scale):
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


def main():
    args = parse_args()

    # subprocess.call([os.path.join(args.colmap_path, "database_creator"),
    #                 "--database_path", args.database_path])

    connection = sqlite3.connect(args.database_path)
    cursor = connection.cursor()

    images = dict()
    image_pairs = set()

    # cursor.execute("SELECT * FROM images")
    # # Fetch all the rows in a list of lists.
    # results = cursor.fetchall()
    # print(results)
    # for row in results:
    #     name = row[0]
    #     camera_id = row[1]
    #     print ( "name = %s, camera_id = %s" % (name, camera_id) )

    ## Prepare SQL query to INSERT a record into the database.
    # sqlQuery = "SELECT * FROM images WHERE name=?;"
    image_name = 'P1180141.JPG'
    try:
       # Execute the SQL command
    #    cursor.execute(sqlQuery)
    #    cursor.execute("SELECT * FROM images WHERE name=?;", (image_name,))
       cursor.execute("SELECT * FROM images")
       # Fetch all the rows in a list of lists.
       results = cursor.fetchall()
       print(results)
       for row in results:
          item_idx = row[0]
          name = row[1]
          camera_id = row[2]
          prior_qw = row[3]
          prior_qx = row[4]
          prior_qy = row[5]
          prior_qz = row[6]
          prior_tx = row[7]
          prior_ty = row[8]
          prior_tz = row[9]
          # Now print fetched result
          print ( "item_idx = %s, name = %s, camera_id = %s, prior_qw = %s, prior_qx = %s, prior_qy = %s, prior_qz = %s, prior_tx = %s, prior_ty = %s, prior_tz = %s" % \
                 (item_idx, name, camera_id, prior_qw, prior_qx, prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) )
        #   print ( "name = %s, camera_id = %s" % (name, camera_id) )
    except:
       print ("Error: unable to fetch data")

    # data = h5py.File(args.demon_path)
    # for image_pair12 in data.keys():
    #     print("Processing", image_pair12)
    #
    #     if image_pair12 in image_pairs:
    #         continue
    #
    #     image_name1, image_name2 = image_pair12.split("---")
    #     image_pair21 = "{}---{}".format(image_name2, image_name1)
    #     image_pairs.add(image_pair12)
    #     image_pairs.add(image_pair21)
    #
    #     if image_pair21 not in data:
    #         continue
    #
    #     # flow12 = data[image_pair12]["flow"]
    #     # flow21 = data[image_pair21]["flow"]
    #
    #     flow12 = flow_from_depth(data[image_pair12]["depth_upsampled"],
    #                              data[image_pair12]["rotation"],
    #                              data[image_pair12]["translation"],
    #                              args.focal_length)
    #     flow21 = flow_from_depth(data[image_pair21]["depth_upsampled"],
    #                              data[image_pair21]["rotation"],
    #                              data[image_pair21]["translation"],
    #                              args.focal_length)
    #
    #     if image_name1 not in images:
    #         images[image_name1] = add_image(
    #             connection, cursor, args.focal_length, image_name1,
    #             flow12.shape[1:], args.image_scale)
    #     if image_name2 not in images:
    #         images[image_name2] = add_image(
    #             connection, cursor, args.focal_length, image_name2,
    #             flow12.shape[1:], args.image_scale)
    #
    #     add_matches(connection, cursor, images[image_name1],
    #                 images[image_name2], flow12, flow21, args.max_reproj_error)

    # disconnect from server
    cursor.close()
    connection.close()


if __name__ == "__main__":
    main()
