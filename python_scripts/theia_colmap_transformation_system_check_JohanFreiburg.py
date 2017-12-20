import os
import argparse
import subprocess
import collections
import sqlite3
import h5py
import numpy as np
import math
import functools

from pyquaternion import Quaternion
import nibabel.quaternions as nq

import PIL.Image
from matplotlib import pyplot as plt
#import os
import sys

_FLOAT_EPS_4 = np.finfo(float).eps * 4.0

SIMPLE_RADIAL_CAMERA_MODEL = 2

Image = collections.namedtuple(
    "Image", ["id", "camera_id", "name", "qvec", "tvec", "rotmat", "angleaxis"])
# ImagePairGT = collections.namedtuple(
#     "ImagePairGT", ["id1", "id2", "qvec12", "tvec12", "camera_id1", "name1", "camera_id2", "name2"])

ImagePairGT = collections.namedtuple(
    "ImagePairGT", ["id1", "id2", "qvec12", "tvec12", "camera_id1", "name1", "camera_id2", "name2", "rotmat12"])

def read_relative_poses_text(path):
    image_pair_gt = {}
    dummy_image_pair_id = 1
    with open(path, "r") as fid:
        while True:
            line = fid.readline()
            if not line:
                break
            line = line.strip()
            if len(line) > 0 and line[0] != "#":
                elems = line.split()
                image_id1 = int(elems[0])
                image_id2 = int(elems[1])
                qvec12 = np.array(tuple(map(float, elems[2:6])))
                tvec12 = np.array(tuple(map(float, elems[6:9])))
                camera_id1 = int(elems[9])
                image_name1 = elems[10]
                camera_id2 = int(elems[11])
                image_name2 = elems[12]
                rotmat_r1 = np.array(tuple(map(float, elems[13:16])))
                rotmat_r2 = np.array(tuple(map(float, elems[16:19])))
                rotmat_r3 = np.array(tuple(map(float, elems[19:22])))
                RelativeRotationMat = np.array([rotmat_r1, rotmat_r2, rotmat_r3])
                # print("RelativeRotationMat.shape = ", RelativeRotationMat.shape)
                image_pair_gt[dummy_image_pair_id] = ImagePairGT(id1=image_id1, id2=image_id2, qvec12=qvec12, tvec12=tvec12, camera_id1=camera_id1, name1=image_name1, camera_id2=camera_id2, name2=image_name2, rotmat12 = RelativeRotationMat)
                dummy_image_pair_id += 1
    return image_pair_gt

def read_images_text(path):
    images = {}
    with open(path, "r") as fid:
        while True:
            line = fid.readline()
            if not line:
                break
            line = line.strip()
            if len(line) > 0 and line[0] != "#":
                elems = line.split()
                image_id = int(elems[0])
                camera_id = int(elems[1])
                image_name = elems[2]
                qvec = np.array(tuple(map(float, elems[3:7])))
                tvec = np.array(tuple(map(float, elems[7:10])))
                rotmat_row0 = np.array(tuple(map(float, elems[10:13])))
                rotmat_row1 = np.array(tuple(map(float, elems[13:16])))
                rotmat_row2 = np.array(tuple(map(float, elems[16:19])))
                rotmat = np.vstack( (rotmat_row0, rotmat_row1) )
                rotmat = np.vstack( (rotmat, rotmat_row2) )
                angleaxis = np.array(tuple(map(float, elems[19:22])))
                # print("rotmat.shape = ", rotmat.shape)
                images[image_id] = Image(id=image_id, camera_id=camera_id, name=image_name, qvec=qvec, tvec=tvec, rotmat=rotmat, angleaxis=angleaxis)
    return images

def read_images_colmap_format_text(path):
    #   IMAGE_ID, QW, QX, QY, QZ, TX, TY, TZ, CAMERA_ID, NAME
    #   POINTS2D[] as (X, Y, POINT3D_ID)
    images = {}
    featureLine = False
    with open(path, "r") as fid:
        while True:
            line = fid.readline()
            if not line:
                break
            line = line.strip()
            if featureLine == True:
                featureLine = False
                continue
            if len(line) > 0 and line[0] != "#" and featureLine != True:
                elems = line.split()
                image_id = int(elems[0])
                camera_id = int(elems[8])
                image_name = elems[9]
                qvec = np.array(tuple(map(float, elems[1:5])))
                tvec = np.array(tuple(map(float, elems[5:8])))
                rotmat_row0 = np.array(tuple(map(float, elems[10:13])))
                rotmat_row1 = np.array(tuple(map(float, elems[13:16])))
                rotmat_row2 = np.array(tuple(map(float, elems[16:19])))
                rotmat = np.vstack( (rotmat_row0, rotmat_row1) )
                rotmat = np.vstack( (rotmat, rotmat_row2) )
                # angleaxis = np.array(tuple(map(float, elems[19:22])))
                angleaxis = np.array([0,0,0])
                # print("rotmat.shape = ", rotmat.shape)
                images[image_id] = Image(id=image_id, camera_id=camera_id, name=image_name, qvec=qvec, tvec=tvec, rotmat=rotmat, angleaxis=angleaxis)
                featureLine = True
    return images
# def read_relative_poses_text(path):
#     image_pair_gt = {}
#     dummy_image_pair_id = 1
#     with open(path, "r") as fid:
#         while True:
#             line = fid.readline()
#             if not line:
#                 break
#             line = line.strip()
#             if len(line) > 0 and line[0] != "#":
#                 elems = line.split()
#                 image_id1 = int(elems[0])
#                 image_id2 = int(elems[1])
#                 qvec12 = np.array(tuple(map(float, elems[2:6])))
#                 tvec12 = np.array(tuple(map(float, elems[6:9])))
#                 camera_id1 = int(elems[9])
#                 image_name1 = elems[10]
#                 camera_id2 = int(elems[11])
#                 image_name2 = elems[12]
#                 image_pair_gt[dummy_image_pair_id] = ImagePairGT(id1=image_id1, id2=image_id2, qvec12=qvec12, tvec12=tvec12, camera_id1=camera_id1, name1=image_name1, camera_id2=camera_id2, name2=image_name2)
#                 dummy_image_pair_id += 1
#     return image_pair_gt

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--colmap_path", required=True)
    parser.add_argument("--demon_path", required=True)
    parser.add_argument("--databaseRt_path", required=True)
    parser.add_argument("--database_path", required=True)
    parser.add_argument("--relative_poses_Output_path", required=True)
    parser.add_argument("--images_path", required=True)
    parser.add_argument("--image_scale", type=float, default=12)
    parser.add_argument("--focal_length", type=float, default=228.13688) # shall we use this one ? 2457.60 / 12 = 204.8
    parser.add_argument("--max_reproj_error", type=float, default=1)
    parser.add_argument("--max_photometric_error", type=float, default=1)
    args = parser.parse_args()
    return args


def image_ids_to_pair_id(image_id1, image_id2):
    if image_id1 > image_id2:
        return 2147483647 * image_id2 + image_id1
    else:
        return 2147483647 * image_id1 + image_id2

def create_table(conn, create_table_sql):
    """ create a table from the create_table_sql statement
    :param conn: Connection object
    :param create_table_sql: a CREATE TABLE statement
    :return:
    """
    # try:
    c = conn.cursor()
    c.execute(create_table_sql)
    # except Error as e:
    #     print(e)


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
                    int(image_scale * image_shape[1]),
                    int(image_scale * image_shape[0]),
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

def add_image_withRt(connection, cursor, focal_length,
              image_name, image_shape, image_scale, qvec, tvec, angleaxis):
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
                    int(image_scale * image_shape[1]),
                    int(image_scale * image_shape[0]),
                    camera_params))
    camera_id = cursor.lastrowid

    cursor.execute("INSERT INTO images(image_id, name, camera_id, prior_qw, prior_qx, "
                   "prior_qy, prior_qz, prior_tx, prior_ty, prior_tz, prior_angleaxis_x, prior_angleaxis_y, prior_angleaxis_z) "
                   "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                   (camera_id, image_name, camera_id, qvec[0], qvec[1], qvec[2], qvec[3], tvec[0], tvec[1], tvec[2], angleaxis[0], angleaxis[1], angleaxis[2]))
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

def add_image_withRt_colmap(connection, cursor, focal_length,
              image_name, image_shape, image_scale, qvec, tvec):
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
                    int(image_scale * image_shape[1]),
                    int(image_scale * image_shape[0]),
                    camera_params))
    camera_id = cursor.lastrowid

    cursor.execute("INSERT INTO images(image_id, name, camera_id, prior_qw, prior_qx, "
                   "prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) "
                   "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                   (camera_id, image_name, camera_id, qvec[0], qvec[1], qvec[2], qvec[3], tvec[0], tvec[1], tvec[2]))
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
    # print(type(R))
    # print(type(R[:]))
    # print(R.shape)
    # print(R[:].shape)
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


def add_matches_withRt(connection, cursor, image_pair12, image_id1, image_id2, image_name1, image_name2,
                flow12, flow21, max_reproj_error, R_vec, t_vec):
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
    print("  => Cross-checked", matches.shape[0], "matches")

    cursor.execute("INSERT INTO inlier_matches(pair_names, rows, cols, data, "
                   "config, image_id1, image_id2, rotation, translation, image_name1, image_name2) VALUES(?, ?, ?, ?, 3, ?, ?, ?, ?, ?, ?);",
                   (image_pair12,        #image_ids_to_pair_id(image_id1, image_id2),
                    matches.shape[0], matches.shape[1],
                    memoryview(matches), image_id1, image_id2, memoryview(R_vec), memoryview(t_vec), image_name1, image_name2))

    connection.commit()

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

def recover_2D_coord_from_1D_idx(index_1D, nrows = 2304, ncols = 3072):
    x = index_1D % ncols
    y = int ( index_1D / ncols )
    return x, y

def photometric_check(matches, max_photometric_error, img1PIL, img2PIL):
    matchesFiltered = []
    for i in range(matches.shape[0]):
        x1, y1 = recover_2D_coord_from_1D_idx(matches[i,0])
        x2, y2 = recover_2D_coord_from_1D_idx(matches[i,1])
        #print(x1, y1, x2, y2)
        #r1, g1, b1 = img1PIL.getpixel((x1, y1))
        #r2, g2, b2 = img2PIL.getpixel((x2, y2))
        r1, g1, b1 = img1PIL.getpixel((int(x1), int(y1)))
        r2, g2, b2 = img2PIL.getpixel((int(x2), int(y2)))

        if (r1-r2)**2+(g1-g2)**2+(b1-b2)**2 < max_photometric_error**2:
            matchesFiltered.append([matches[i,0], matches[i,1]])

    matches_final = np.array(matchesFiltered)
    return matches_final

def add_matches_withRt_photochecked(connection, cursor, image_pair12, image_id1, image_id2, image_name1, image_name2,
                flow12, flow21, max_reproj_error, R_vec, t_vec, max_photometric_error, img1PIL, img2PIL):
    matches12, coords121, coords122 = flow_to_matches(flow12)
    matches21, coords211, coords212 = flow_to_matches(flow21)

    print("  => Found", matches12.size, "<->", matches21.size, "matches")

    matches = cross_check_matches(matches12, coords121, coords122,
                                  matches21, coords211, coords212,
                                  max_reproj_error)

    if matches.size == 0:
        return

    # matches = matches[::10].copy()

    #print("  => Cross-checked", matches.size, "matches")
    print("  => Cross-checked", matches.shape[0], "matches")

    #print(type(matches))
    matches = photometric_check(matches, max_photometric_error, img1PIL, img2PIL)
    #print("  => photo-checked", matches.size, "matches")
    print("  => photo-checked", matches.shape[0], "matches")

    if matches.size == 0:
        return

    cursor.execute("INSERT INTO inlier_matches(pair_names, rows, cols, data, "
                   "config, image_id1, image_id2, rotation, translation, image_name1, image_name2) VALUES(?, ?, ?, ?, 3, ?, ?, ?, ?, ?, ?);",
                   (image_pair12,    #image_ids_to_pair_id(image_id1, image_id2),
                    matches.shape[0], matches.shape[1],
                    memoryview(matches), image_id1, image_id2, memoryview(R_vec), memoryview(t_vec), image_name1, image_name2))

    connection.commit()

def add_matches_photochecked(connection, cursor, image_id1, image_id2,
                flow12, flow21, max_reproj_error, max_photometric_error, img1PIL, img2PIL):
    matches12, coords121, coords122 = flow_to_matches(flow12)
    matches21, coords211, coords212 = flow_to_matches(flow21)

    print("  => Found", matches12.size, "<->", matches21.size, "matches")

    matches = cross_check_matches(matches12, coords121, coords122,
                                  matches21, coords211, coords212,
                                  max_reproj_error)

    if matches.size == 0:
        return

    # matches = matches[::10].copy()

    #print("  => Cross-checked", matches.size, "matches")

    #print(type(matches))
    matches = photometric_check(matches, max_photometric_error, img1PIL, img2PIL)
    #print("  => photo-checked", matches.size, "matches")
    #print("  => photo-checked", matches.shape[0], "matches")

    if matches.size == 0:
        return


    cursor.execute("INSERT INTO inlier_matches(pair_id, rows, cols, data, "
                   "config) VALUES(?, ?, ?, ?, 3);",
                   (image_ids_to_pair_id(image_id1, image_id2),
                    matches.shape[0], matches.shape[1],
                    memoryview(matches)))

    connection.commit()

def euler2quat(z=0, y=0, x=0):
    ''' Return quaternion corresponding to these Euler angles

    Uses the z, then y, then x convention above

    Parameters
    ----------
    z : scalar
       Rotation angle in radians around z-axis (performed first)
    y : scalar
       Rotation angle in radians around y-axis
    x : scalar
       Rotation angle in radians around x-axis (performed last)

    Returns
    -------
    quat : array shape (4,)
       Quaternion in w, x, y z (real, then vector) format

    Notes
    -----
    We can derive this formula in Sympy using:

    1. Formula giving quaternion corresponding to rotation of theta radians
       about arbitrary axis:
       http://mathworld.wolfram.com/EulerParameters.html
    2. Generated formulae from 1.) for quaternions corresponding to
       theta radians rotations about ``x, y, z`` axes
    3. Apply quaternion multiplication formula -
       http://en.wikipedia.org/wiki/Quaternions#Hamilton_product - to
       formulae from 2.) to give formula for combined rotations.
    '''
    z = z/2.0
    y = y/2.0
    x = x/2.0
    cz = math.cos(z)
    sz = math.sin(z)
    cy = math.cos(y)
    sy = math.sin(y)
    cx = math.cos(x)
    sx = math.sin(x)
    return np.array([
             cx*cy*cz - sx*sy*sz,
             cx*sy*sz + cy*cz*sx,
             cx*cz*sy - sx*cy*sz,
             cx*cy*sz + sx*cz*sy])



def mat2euler(M, cy_thresh=None):
    ''' Discover Euler angle vector from 3x3 matrix

    Uses the conventions above.

    Parameters
    ----------
    M : array-like, shape (3,3)
    cy_thresh : None or scalar, optional
       threshold below which to give up on straightforward arctan for
       estimating x rotation.  If None (default), estimate from
       precision of input.

    Returns
    -------
    z : scalar
    y : scalar
    x : scalar
       Rotations in radians around z, y, x axes, respectively

    Notes
    -----
    If there was no numerical error, the routine could be derived using
    Sympy expression for z then y then x rotation matrix, which is::

      [                       cos(y)*cos(z),                       -cos(y)*sin(z),         sin(y)],
      [cos(x)*sin(z) + cos(z)*sin(x)*sin(y), cos(x)*cos(z) - sin(x)*sin(y)*sin(z), -cos(y)*sin(x)],
      [sin(x)*sin(z) - cos(x)*cos(z)*sin(y), cos(z)*sin(x) + cos(x)*sin(y)*sin(z),  cos(x)*cos(y)]

    with the obvious derivations for z, y, and x

       z = atan2(-r12, r11)
       y = asin(r13)
       x = atan2(-r23, r33)

    Problems arise when cos(y) is close to zero, because both of::

       z = atan2(cos(y)*sin(z), cos(y)*cos(z))
       x = atan2(cos(y)*sin(x), cos(x)*cos(y))

    will be close to atan2(0, 0), and highly unstable.

    The ``cy`` fix for numerical instability below is from: *Graphics
    Gems IV*, Paul Heckbert (editor), Academic Press, 1994, ISBN:
    0123361559.  Specifically it comes from EulerAngles.c by Ken
    Shoemake, and deals with the case where cos(y) is close to zero:

    See: http://www.graphicsgems.org/

    The code appears to be licensed (from the website) as "can be used
    without restrictions".
    '''
    M = np.asarray(M)
    if cy_thresh is None:
        try:
            cy_thresh = np.finfo(M.dtype).eps * 4
        except ValueError:
            cy_thresh = _FLOAT_EPS_4
    r11, r12, r13, r21, r22, r23, r31, r32, r33 = M.flat
    # cy: sqrt((cos(y)*cos(z))**2 + (cos(x)*cos(y))**2)
    cy = math.sqrt(r33*r33 + r23*r23)
    if cy > cy_thresh: # cos(y) not close to zero, standard form
        z = math.atan2(-r12,  r11) # atan2(cos(y)*sin(z), cos(y)*cos(z))
        y = math.atan2(r13,  cy) # atan2(sin(y), cy)
        x = math.atan2(-r23, r33) # atan2(cos(y)*sin(x), cos(x)*cos(y))
    else: # cos(y) (close to) zero, so x -> 0.0 (see above)
        # so r21 -> sin(z), r22 -> cos(z) and
        z = math.atan2(r21,  r22)
        y = math.atan2(r13,  cy) # atan2(sin(y), cy)
        x = 0.0
    return z, y, x

def euler2angle_axis(z=0, y=0, x=0):
    ''' Return angle, axis corresponding to these Euler angles

    Uses the z, then y, then x convention above

    Parameters
    ----------
    z : scalar
       Rotation angle in radians around z-axis (performed first)
    y : scalar
       Rotation angle in radians around y-axis
    x : scalar
       Rotation angle in radians around x-axis (performed last)

    Returns
    -------
    theta : scalar
       angle of rotation
    vector : array shape (3,)
       axis around which rotation occurs

    Examples
    --------
    >>> theta, vec = euler2angle_axis(0, 1.5, 0)
    >>> print(theta)
    1.5
    >>> np.allclose(vec, [0, 1, 0])
    True
    '''
    # delayed import to avoid cyclic dependencies
    import nibabel.quaternions as nq
    return nq.quat2angle_axis(euler2quat(z, y, x))

def normalizeQuaternion(qvec1):
    qvec1_norm = np.linalg.norm(qvec1)
    if (qvec1_norm == 0):
        normalised_qvec1 = np.array([1, qvec1[1], qvec1[2], qvec1[3]])
    else:
        normalised_qvec1 = (1.0/qvec1_norm) * qvec1
    return normalised_qvec1

def quaternionMultiplication(qvec1, qvec2):
    qprod = qvec1
    qprod[0] = qvec2[0]*qvec1[0] - qvec2[1]*qvec1[1] - qvec2[2]*qvec1[2] - qvec2[3]*qvec1[3]
    qprod[1] = qvec2[0]*qvec1[1] + qvec2[1]*qvec1[0] - qvec2[2]*qvec1[3] + qvec2[3]*qvec1[2]
    qprod[2] = qvec2[0]*qvec1[2] + qvec2[1]*qvec1[3] + qvec2[2]*qvec1[0] - qvec2[3]*qvec1[1]
    qprod[3] = qvec2[0]*qvec1[3] - qvec2[1]*qvec1[2] + qvec2[2]*qvec1[1] + qvec2[3]*qvec1[0]
    return qprod

def quaternion_mult(q,r):
    return [r[0]*q[0]-r[1]*q[1]-r[2]*q[2]-r[3]*q[3],
            r[0]*q[1]+r[1]*q[0]-r[2]*q[3]+r[3]*q[2],
            r[0]*q[2]+r[1]*q[3]+r[2]*q[0]-r[3]*q[1],
            r[0]*q[3]-r[1]*q[2]+r[2]*q[1]+r[3]*q[0]]

def quaternionRotatePoint(q, point):
    #print("point.shape = ", point.shape)
    #r = [0]+point
    r = np.array([0, point[0], point[1], point[2]])
    #print("r.shape = ", r.shape)
    q_conj = [q[0],-1*q[1],-1*q[2],-1*q[3]]
    return quaternion_mult(quaternion_mult(q,r),q_conj)[1:]

def relativePose_from_AbsolutePose(qvec1, tvec1, qvec2, tvec2):
    normalised_qvec1 = normalizeQuaternion(qvec1)
    inv_normalized_qvec1 = np.array([normalised_qvec1[0], -normalised_qvec1[1], -normalised_qvec1[2], -normalised_qvec1[3]])
    qvec12 = quaternionMultiplication(normalizeQuaternion(inv_normalized_qvec1), normalizeQuaternion(qvec2))
    tvec12 = tvec2 - quaternionRotatePoint(normalizeQuaternion(qvec12), tvec1)
    return qvec12, tvec12

# http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm
# http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.184.3942&rep=rep1&type=pdf
def quaternion2RotMat(qw, qx, qy, qz):
    sqw = qw*qw
    sqx = qx*qx
    sqy = qy*qy
    sqz = qz*qz

    # invs (inverse square length) is only required if quaternion is not already normalised
    invs = 1 / (sqx + sqy + sqz + sqw)
    m00 = ( sqx - sqy - sqz + sqw)*invs     # since sqw + sqx + sqy + sqz =1/invs*invs
    m11 = (-sqx + sqy - sqz + sqw)*invs
    m22 = (-sqx - sqy + sqz + sqw)*invs

    tmp1 = qx*qy;
    tmp2 = qz*qw;
    m10 = 2.0 * (tmp1 + tmp2)*invs
    m01 = 2.0 * (tmp1 - tmp2)*invs

    tmp1 = qx*qz
    tmp2 = qy*qw
    m20 = 2.0 * (tmp1 - tmp2)*invs
    m02 = 2.0 * (tmp1 + tmp2)*invs
    tmp1 = qy*qz
    tmp2 = qx*qw
    m21 = 2.0 * (tmp1 + tmp2)*invs ;
    m12 = 2.0 * (tmp1 - tmp2)*invs

    rotation_matrix = np.array( [[m00, m01, m02], [m10, m11, m12], [m20, m21, m22]] )
    return rotation_matrix


def rotmat_To_angleaxis(image_pair12_rotmat):
    eulerAnlges = mat2euler(image_pair12_rotmat)
    recov_angle_axis_result = euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2])
    R_angleaxis = recov_angle_axis_result[0]*(recov_angle_axis_result[1])
    R_angleaxis = np.array(R_angleaxis, dtype=np.float32)
    return R_angleaxis

def TheiaClamp( f, a, b):
    return max(a, min(f, b))


def main():
    args = parse_args()

    subprocess.call([os.path.join(args.colmap_path, "database_creator"),
                    "--database_path", args.database_path])

    connection = sqlite3.connect(args.databaseRt_path)
    cursor = connection.cursor()
    connectionNoRt = sqlite3.connect(args.database_path)
    cursorNoRt = connectionNoRt.cursor()

    sql_create_imagesModi_table = '''CREATE TABLE IF NOT EXISTS images ( image_id integer, name text, camera_id integer, prior_qw real, prior_qx real, prior_qy real, prior_qz real, prior_tx real, prior_ty real, prior_tz real, prior_angleaxis_x real, prior_angleaxis_y real, prior_angleaxis_z real )'''
    create_table(connection, sql_create_imagesModi_table)
    sql_create_imagesColMAP_table = '''CREATE TABLE IF NOT EXISTS images ( image_id integer, name text, camera_id integer, prior_qw real, prior_qx real, prior_qy real, prior_qz real, prior_tx real, prior_ty real, prior_tz real )'''
    create_table(connectionNoRt, sql_create_imagesColMAP_table)

    sql_create_cameras_table = '''CREATE TABLE IF NOT EXISTS cameras ( item_idx integer, model integer, width integer, height integer, params blob, prior_focal_length real )'''
    create_table(connection, sql_create_cameras_table)
    create_table(connectionNoRt, sql_create_cameras_table)

    sql_create_keypoints_table = '''CREATE TABLE IF NOT EXISTS keypoints ( image_id integer, rows integer, cols integer, data blob )'''
    create_table(connection, sql_create_keypoints_table)
    create_table(connectionNoRt, sql_create_keypoints_table)

    # sql_create_inlier_matches_table = '''CREATE TABLE IF NOT EXISTS inlier_matches ( pair_id integer, rows integer, cols integer, data blob, config integer, image_id1 integer, image_id2 integer, rotation blob, translation blob )'''
    # sql_create_inlier_matches_table = '''CREATE TABLE IF NOT EXISTS inlier_matches ( pair_id integer, rows integer, cols integer, data blob, config integer, image_id1 integer, image_id2 integer, rotation blob, translation blob, image_name1 text, image_name2 text )'''
    sql_create_inlier_matches_table = '''CREATE TABLE IF NOT EXISTS inlier_matches ( pair_names text, rows integer, cols integer, data blob, config integer, image_id1 integer, image_id2 integer, rotation blob, translation blob, image_name1 text, image_name2 text )'''
    create_table(connection, sql_create_inlier_matches_table)
    sql_create_inlier_matchesNoRt_table = '''CREATE TABLE IF NOT EXISTS inlier_matches ( pair_id integer, rows integer, cols integer, data blob, config integer )'''
    create_table(connectionNoRt, sql_create_inlier_matchesNoRt_table)


    images = dict()
    image_pairs = set()

    GTfilepath = '/home/kevin/JohannesCode/southbuilding_RtAngleAxis_groundtruth_from_colmap.txt'
    imagesGT = read_images_text(GTfilepath)
    # GTfilepath = '/home/kevin/JohannesCode/ws1/sparse/0/textfiles_final/images.txt'
    # imagesGT = read_images_colmap_format_text(GTfilepath)


    # relativePoses_GTfilepath = '/home/kevin/JohannesCode/ws1/sparse/0/textfiles_RelativePoses/relative_poses.txt'
    # relativePosesGT = read_relative_poses_text(relativePoses_GTfilepath)

    # relativePoses_outputGTfilepath = '/home/kevin/JohannesCode/southbuilding_RelativePoses_Quaternion_AngleAxis_groundtruth_from_colmap.txt'
    relativePoses_outputGTfilepath = args.relative_poses_Output_path

    relativePoses_outputGTfile = open(relativePoses_outputGTfilepath,'w')


    # for imgIdx in range(len(imagesGT)):
    for imgIdx, val in imagesGT.items():
        print("imgIdx = ", imgIdx)
        images[val.name] = add_image_withRt(
            connection, cursor, args.focal_length, val.name,
            np.array([192, 256]), args.image_scale, val.qvec, val.tvec, val.angleaxis)
        images[val.name] = add_image_withRt_colmap(
            connectionNoRt, cursorNoRt, args.focal_length, val.name,
            np.array([192, 256]), args.image_scale, val.qvec, val.tvec)

    data = h5py.File(args.demon_path)
    for image_pair12 in data.keys():
        print("Processing", image_pair12)

        if image_pair12 in image_pairs:
            continue

        image_name1, image_name2 = image_pair12.split("---")
        image_pair21 = "{}---{}".format(image_name2, image_name1)
        image_pairs.add(image_pair12)
        image_pairs.add(image_pair21)

        if image_pair21 not in data:
            continue

        ### further filtering the image pairs by prediction sym error ### Freiburg's data
        pred_rotmat12 = data[image_pair12]["rotation"].value
        # pred_rotmat12 = data[image_pair12]["rotation_matrix"].value
        pred_rotmat21 = data[image_pair21]["rotation"].value
        # pred_rotmat21 = data[image_pair21]["rotation_matrix"].value
        pred_trans12 = data[image_pair12]["translation"].value
        pred_trans21 = data[image_pair21]["translation"].value

        pred_rotmat12angleaxis = rotmat_To_angleaxis(pred_rotmat12)
        pred_rotmat21angleaxis = rotmat_To_angleaxis(pred_rotmat21)
        # theta_err_abs = abs(np.linalg.norm(pred_rotmat12angleaxis) - np.linalg.norm(pred_rotmat21angleaxis))
        loop_rotation = np.dot(pred_rotmat12.T, pred_rotmat21)
        RotationAngularErr = np.linalg.norm(rotmat_To_angleaxis(loop_rotation))
        TransMagInput = np.linalg.norm(pred_trans12)
        TransMagOutput = np.linalg.norm(pred_trans21)
        TransDistErr = TransMagInput - TransMagOutput   # can be different if normalized or not?
        # tmp = TheiaClamp(np.dot(TransVec1, TransVec2)/(TransMagInput*TransMagOutput), -1, 1)   # can be different if normalized or not?
        tmp = TheiaClamp(np.dot(pred_trans12, -pred_trans21)/(TransMagInput*TransMagOutput), -1, 1)   # can be different if normalized or not?
        TransAngularErr = math.acos( tmp )
        if RotationAngularErr > 7.5: # chosen by observing sym_err_hist
            print("image_pair12 ", image_pair12, " is skipped because of large sym error!!!")
            continue

        # ### further filtering the image pairs by prediction sym error
        # pred_rotmat12 = data[image_pair12]["rotation_matrix"].value
        # pred_rotmat21 = data[image_pair21]["rotation_matrix"].value
        #
        # pred_rotmat12angleaxis = rotmat_To_angleaxis(pred_rotmat12)
        # pred_rotmat21angleaxis = rotmat_To_angleaxis(pred_rotmat21)
        # theta_err_abs = abs(np.linalg.norm(pred_rotmat12angleaxis) - np.linalg.norm(pred_rotmat21angleaxis))
        # if theta_err_abs > 6.6: # chosen by observing sym_err_hist
        #     print("image_pair12 ", image_pair12, " is skipped because of large sym error!!!")
        #     continue

        # flow12 = data[image_pair12]["flow"]
        # flow21 = data[image_pair21]["flow"]

        imagepath1 = os.path.join(args.images_path, image_name1)
        imagepath2 = os.path.join(args.images_path, image_name2)


        img1PIL = PIL.Image.open(imagepath1)
        img2PIL = PIL.Image.open(imagepath2)


        # retrieve the ground truth Rt from colmap result
        for imgIdx, val in imagesGT.items():
            if val.name == image_name1:
                image_indexGT_from_name1 = imgIdx
                print("the image id of name" + image_name1 + " is ", imgIdx)
            if val.name == image_name2:
                image_indexGT_from_name2 = imgIdx
                print("the image id of name" + image_name2 + " is ", imgIdx)

        # # retrieve the ground truth relative poses from colmap result
        # for imgPairIdx, val in relativePosesGT.items():
        #     if val.name1 == image_name1 and val.name2 == image_name2:
        #         imagePair_indexGT_12 = imgPairIdx
        #         print("the image pair id of name1 " + image_name1 + " --- name2 " + image_name2 + " is ", imgPairIdx)
        #     if val.name1 == image_name2 and val.name2 == image_name1:
        #         imagePair_indexGT_21 = imgPairIdx
        #         print("the image pair id of name1 " + image_name2 + " --- name2 " + image_name1 + " is ", imgPairIdx)
        img1qvec = imagesGT[image_indexGT_from_name1].qvec
        img1tvec = imagesGT[image_indexGT_from_name1].tvec
        img2qvec = imagesGT[image_indexGT_from_name2].qvec
        img2tvec = imagesGT[image_indexGT_from_name2].tvec
        img1rotmat = imagesGT[image_indexGT_from_name1].rotmat
        img2rotmat = imagesGT[image_indexGT_from_name2].rotmat
        # check quaternion to rotation matrix conversion

        # # calculate relative poses according to the mechanism in twoview_info.h by TheiaSfM
        # # The relative rotation of camera2 is: R_12 = R2 * R1^t.
        image_pair12_rotmat = np.dot(img2rotmat, img1rotmat.T)
        image_pair21_rotmat = np.dot(img1rotmat, img2rotmat.T)
        # image_pair12_rotmat = np.dot(img2rotmat.T, img1rotmat)
        # image_pair21_rotmat = np.dot(img1rotmat.T, img2rotmat)

        # # Compute the position of camera 2 in the coordinate system of camera 1 using
        # # the standard projection equation:
        # #     X' = R * (X - c)
        # # which yields:
        # #     c2' = R1 * (c2 - c1).
        # image_pair12_transVec = np.dot(img1rotmat, (img2tvec-img1tvec))
        # image_pair21_transVec = np.dot(img2rotmat, (img1tvec-img2tvec))
        # image_pair12_transVec = np.dot( img1rotmat, (np.dot(img1rotmat.T,img1tvec)-np.dot(img2rotmat.T,img2tvec)) )
        # image_pair21_transVec = np.dot( img2rotmat, (np.dot(img2rotmat.T,img2tvec)-np.dot(img1rotmat.T,img1tvec)) )
        image_pair12_transVec = - np.dot( image_pair12_rotmat, np.dot( img1rotmat, (np.dot(img1rotmat.T,img1tvec)-np.dot(img2rotmat.T,img2tvec)) ) )
        image_pair21_transVec = - np.dot( image_pair21_rotmat, np.dot( img2rotmat, (np.dot(img2rotmat.T,img2tvec)-np.dot(img1rotmat.T,img1tvec)) ) )

        # qvec12 = relativePosesGT[imagePair_indexGT_12].qvec12
        # qvec21 = relativePosesGT[imagePair_indexGT_21].qvec12
        # image_pair12_rotmat = quaternion2RotMat(qvec12[0], qvec12[1], qvec12[2], qvec12[3])
        # image_pair21_rotmat = quaternion2RotMat(qvec21[0], qvec21[1], qvec21[2], qvec21[3])
        # image_pair12_transVec = relativePosesGT[imagePair_indexGT_12].tvec12
        # image_pair21_transVec = relativePosesGT[imagePair_indexGT_21].tvec12

        flow12 = flow_from_depth(data[image_pair12]["depth_upsampled"],
                                 #data[image_pair12]["rotation"],
                                 #data[image_pair12]["translation"],
                                 #(image_pair12_rotmat),
                                 #(image_pair12_transVec),
                                 image_pair12_rotmat,
                                 image_pair12_transVec,
                                 args.focal_length)
        flow21 = flow_from_depth(data[image_pair21]["depth_upsampled"],
                                 #data[image_pair21]["rotation"],
                                 #data[image_pair21]["translation"],
                                 #(image_pair21_rotmat),
                                 #(image_pair21_transVec),
                                 image_pair21_rotmat,
                                 image_pair21_transVec,
                                 args.focal_length)
        # # tmp = data[image_pair12]["rotation_matrix"]
        # # print("isFile = ", isinstance(tmp, h5py.File))
        # # print("isGroup = ", isinstance(tmp, h5py.Group))
        # # print("isDataset = ", isinstance(tmp, h5py.Dataset))
        # # print("rotation = ", tmp.value)
        # # print("rotation = ", tmp[:])
        # # tmp = data[image_pair12]["depth_upsampled"]
        # # print("isFile = ", isinstance(tmp, h5py.File))
        # # print("isGroup = ", isinstance(tmp, h5py.Group))
        # # print("isDataset = ", isinstance(tmp, h5py.Dataset))
        # # print("depth_upsampled = ", tmp.value)
        # # print("depth_upsampled = ", tmp[:])
        # if image_name1 not in images:
        #     images[image_name1] = add_image_withRt(
        #         connection, cursor, args.focal_length, image_name1,
        #         flow12.shape[1:], args.image_scale, imagesGT[image_indexGT_from_name1].qvec, imagesGT[image_indexGT_from_name1].tvec)
        #     images[image_name1] = add_image_withRt(
        #         connectionNoRt, cursorNoRt, args.focal_length, image_name1,
        #         flow12.shape[1:], args.image_scale, imagesGT[image_indexGT_from_name1].qvec, imagesGT[image_indexGT_from_name1].tvec)
        # if image_name2 not in images:
        #     images[image_name2] = add_image_withRt(
        #         connection, cursor, args.focal_length, image_name2,
        #         flow12.shape[1:], args.image_scale, imagesGT[image_indexGT_from_name2].qvec, imagesGT[image_indexGT_from_name2].tvec)
        #     images[image_name2] = add_image_withRt(
        #         connectionNoRt, cursorNoRt, args.focal_length, image_name2,
        #         flow12.shape[1:], args.image_scale, imagesGT[image_indexGT_from_name2].qvec, imagesGT[image_indexGT_from_name2].tvec)

        #R_mat = data[image_pair12]["rotation"].value
        #R_mat = np.array(R_mat, dtype=np.float32)
        eulerAnlges = mat2euler(image_pair12_rotmat)
        recov_angle_axis_result = euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2])
        R_angleaxis = recov_angle_axis_result[0]*(recov_angle_axis_result[1])
        R_angleaxis = np.array(R_angleaxis, dtype=np.float32)

        ### convert numpy array's data type to be np.float32, which will be read later by c++ code of modified Theia-SfM
        t_Vec_npfloat32 = np.array(np.dot( img1rotmat, (np.dot(img1rotmat.T,img1tvec)-np.dot(img2rotmat.T,img2tvec)) ), dtype=np.float32)
        #print("R_angleaxis.shape = ", R_angleaxis.shape)
        #t_vec = data[image_pair12]["translation"].value
        #t_vec = np.array(t_vec, dtype=np.float32)
        # #add_matches_withRt(connection, cursor, images[image_name1], images[image_name2], flow12, flow21, args.max_reproj_error, R_angleaxis, image_pair12_transVec)
        # #add_matches(connectionNoRt, cursorNoRt, images[image_name1], images[image_name2], flow12, flow21, args.max_reproj_error)
        # add_matches_withRt_photochecked(connection, cursor, images[image_name1], images[image_name2], image_name1, image_name2, flow12, flow21, args.max_reproj_error, R_angleaxis, t_Vec_npfloat32, args.max_photometric_error, img1PIL, img2PIL)
        # add_matches_photochecked(connectionNoRt, cursorNoRt, images[image_name1], images[image_name2], flow12, flow21, args.max_reproj_error, args.max_photometric_error, img1PIL, img2PIL)
        add_matches_withRt_photochecked(connection, cursor, image_pair12, image_indexGT_from_name1, image_indexGT_from_name2, image_name1, image_name2, flow12, flow21, args.max_reproj_error, R_angleaxis, t_Vec_npfloat32, args.max_photometric_error, img1PIL, img2PIL)
        add_matches_photochecked(connectionNoRt, cursorNoRt, image_indexGT_from_name1, image_indexGT_from_name2, flow12, flow21, args.max_reproj_error, args.max_photometric_error, img1PIL, img2PIL)

        relativePoses_outputGTfile.write("%s %s %s %s %s %s %s %f %f %f %f %f %f\n" % (image_pair12, image_indexGT_from_name1, images[image_name1], image_name1, image_indexGT_from_name2, images[image_name2], image_name2, t_Vec_npfloat32[0], t_Vec_npfloat32[1], t_Vec_npfloat32[2], R_angleaxis[0], R_angleaxis[1], R_angleaxis[2]))

    relativePoses_outputGTfile.close()
    cursor.close()
    connection.close()
    cursorNoRt.close()
    connectionNoRt.close()


if __name__ == "__main__":
    main()
