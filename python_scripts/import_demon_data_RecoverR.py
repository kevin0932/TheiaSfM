import os
import argparse
import subprocess
import collections
import sqlite3
import h5py
import numpy as np
import math
import functools

_FLOAT_EPS_4 = np.finfo(float).eps * 4.0

SIMPLE_RADIAL_CAMERA_MODEL = 2


def parse_args():
    parser = argparse.ArgumentParser()
    # parser.add_argument("--colmap_path", required=True)
    parser.add_argument("--demon_path", required=True)
    parser.add_argument("--database_path", required=True)
    parser.add_argument("--image_scale", type=float, default=12)
    parser.add_argument("--focal_length", type=float, default=228.13688)
    parser.add_argument("--max_reproj_error", type=float, default=1)
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

def add_image_withRt(connection, cursor, focal_length,
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

    cursor.execute("INSERT INTO images(image_id, name, camera_id, prior_qw, prior_qx, "
                   "prior_qy, prior_qz, prior_tx, prior_ty, prior_tz) "
                   "VALUES(?, ?, ?, 0, 0, 0, 0, 0, 0, 0);",
                   (camera_id, image_name, camera_id))
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


def add_matches_withRt(connection, cursor, image_id1, image_id2,
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

    cursor.execute("INSERT INTO inlier_matches(pair_id, rows, cols, data, "
                   "config, image_id1, image_id2, rotation, translation) VALUES(?, ?, ?, ?, 3, ?, ?, ?, ?);",
                   (image_ids_to_pair_id(image_id1, image_id2),
                    matches.shape[0], matches.shape[1],
                    memoryview(matches), image_id1, image_id2, memoryview(R_vec), memoryview(t_vec)))

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

def main():
    args = parse_args()

    # subprocess.call([os.path.join(args.colmap_path, "database_creator"),
    #                 "--database_path", args.database_path])

    connection = sqlite3.connect(args.database_path)
    cursor = connection.cursor()

    sql_create_images_table = '''CREATE TABLE IF NOT EXISTS images ( image_id integer, name text, camera_id integer, prior_qw real, prior_qx real, prior_qy real, prior_qz real, prior_tx real, prior_ty real, prior_tz real )'''
    create_table(connection, sql_create_images_table)

    sql_create_cameras_table = '''CREATE TABLE IF NOT EXISTS cameras ( item_idx integer, model integer, width integer, height integer, params blob, prior_focal_length real )'''
    create_table(connection, sql_create_cameras_table)

    sql_create_keypoints_table = '''CREATE TABLE IF NOT EXISTS keypoints ( image_id integer, rows integer, cols integer, data blob )'''
    create_table(connection, sql_create_keypoints_table)

    sql_create_inlier_matches_table = '''CREATE TABLE IF NOT EXISTS inlier_matches ( pair_id integer, rows integer, cols integer, data blob, config integer, image_id1 integer, image_id2 integer, rotation blob, translation blob )'''
    create_table(connection, sql_create_inlier_matches_table)


    images = dict()
    image_pairs = set()

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

        # flow12 = data[image_pair12]["flow"]
        # flow21 = data[image_pair21]["flow"]

        flow12 = flow_from_depth(data[image_pair12]["depth_upsampled"],
                                 data[image_pair12]["rotation"],
                                 data[image_pair12]["translation"],
                                 args.focal_length)
        flow21 = flow_from_depth(data[image_pair21]["depth_upsampled"],
                                 data[image_pair21]["rotation"],
                                 data[image_pair21]["translation"],
                                 args.focal_length)
        # tmp = data[image_pair12]["rotation_matrix"]
        # print("isFile = ", isinstance(tmp, h5py.File))
        # print("isGroup = ", isinstance(tmp, h5py.Group))
        # print("isDataset = ", isinstance(tmp, h5py.Dataset))
        # print("rotation = ", tmp.value)
        # print("rotation = ", tmp[:])
        # tmp = data[image_pair12]["depth_upsampled"]
        # print("isFile = ", isinstance(tmp, h5py.File))
        # print("isGroup = ", isinstance(tmp, h5py.Group))
        # print("isDataset = ", isinstance(tmp, h5py.Dataset))
        # print("depth_upsampled = ", tmp.value)
        # print("depth_upsampled = ", tmp[:])
        if image_name1 not in images:
            images[image_name1] = add_image_withRt(
                connection, cursor, args.focal_length, image_name1,
                flow12.shape[1:], args.image_scale)
        if image_name2 not in images:
            images[image_name2] = add_image_withRt(
                connection, cursor, args.focal_length, image_name2,
                flow12.shape[1:], args.image_scale)

        R_mat = data[image_pair12]["rotation"].value
        R_mat = np.array(R_mat, dtype=np.float32)
        eulerAnlges = mat2euler(R_mat)
        recov_angle_axis_result = euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2])
        R_angleaxis = recov_angle_axis_result[0]*(recov_angle_axis_result[1])
        R_angleaxis = np.array(R_angleaxis, dtype=np.float32)
        print("R_angleaxis.shape = ", R_angleaxis.shape)
        t_vec = data[image_pair12]["translation"].value
        t_vec = np.array(t_vec, dtype=np.float32)
        add_matches_withRt(connection, cursor, images[image_name1], images[image_name2], flow12, flow21, args.max_reproj_error, R_angleaxis, t_vec)
        # add_matches(connection, cursor, images[image_name1], images[image_name2], flow12, flow21, args.max_reproj_error)

    cursor.close()
    connection.close()


if __name__ == "__main__":
    main()
