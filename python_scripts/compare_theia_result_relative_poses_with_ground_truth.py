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

Image = collections.namedtuple("Image", ["id", "camera_id", "name", "qvec", "tvec", "rotmat", "angleaxis"])
# ImagePairGT = collections.namedtuple(
#     "ImagePairGT", ["id1", "id2", "qvec12", "tvec12", "camera_id1", "name1", "camera_id2", "name2"])
ImagePair = collections.namedtuple("ImagePair", ["id1", "id2", "R_rotmat", "R_angleaxis", "t_vec"])

def read_relative_poses_GT(path):
#   IMAGE_ID1, IMAGE_ID2, QW12, QX12, QY12, QZ12, TX12, TY12, TZ12, CAMERA_ID1, NAME1, CAMERA_ID1, NAME2, RotMat[0,0], RotMat[0,1], ..., RotMat[2,2]
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
                R_quaternion = np.array(tuple(map(float, elems[2:6])), dtype=np.float64)
                R_rotmat = np.array(tuple(map(float, elems[13:22])), dtype=np.float64)
                R_rotmat = np.reshape(R_rotmat, [3,3])
                t_vec = np.array(tuple(map(float, elems[6:9])), dtype=np.float64)
                # print("RelativeRotationMat.shape = ", RelativeRotationMat.shape)
                R_angleaxis = rotmat_To_angleaxis(R_rotmat)
                image_pair_gt[dummy_image_pair_id] = ImagePair(id1=image_id1, id2=image_id2, R_rotmat=R_rotmat, R_angleaxis=R_angleaxis, t_vec=t_vec)
                dummy_image_pair_id += 1

    print("total num of GT pairs = ", dummy_image_pair_id-1)
    return image_pair_gt

def read_relative_poses_theia_output(path):
    # ViewID1 58 ViewID2 68 Relavie Poses -> [ 0.0147056 0.772728 -0.252982 ] [ 0.677858 -0.619738 0.395517 ]
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
                image_id1 = int(elems[1])
                image_id2 = int(elems[3])
                if image_id1>=image_id2:
                    # print("skip ", image_id1, " and ", image_id2)
                    continue
                R_angleaxis = np.array(tuple(map(float, elems[8:11])), dtype=np.float64)
                t_vec = np.array(tuple(map(float, elems[13:16])), dtype=np.float64)
                R_rotmat = np.array(tuple(map(float, elems[17:26])), dtype=np.float64)
                R_rotmat = np.reshape(R_rotmat, [3,3])
                image_pair_gt[dummy_image_pair_id] = ImagePair(id1=image_id1, id2=image_id2, R_rotmat=R_rotmat, R_angleaxis=R_angleaxis, t_vec=t_vec)
                dummy_image_pair_id += 1

    print("total pairs = ", dummy_image_pair_id-1)
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

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--relative_poses_GT", required=True)
    parser.add_argument("--relative_poses_theia_output", required=True)
    args = parser.parse_args()
    return args


def image_ids_to_pair_id(image_id1, image_id2):
    if image_id1 > image_id2:
        return 2147483647 * image_id2 + image_id1
    else:
        return 2147483647 * image_id1 + image_id2


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

def main():
    args = parse_args()

    relative_poses_input = read_relative_poses_GT(args.relative_poses_GT)
    relative_poses_output = read_relative_poses_theia_output(args.relative_poses_theia_output)

    print("relative_poses_GT length = ", len(relative_poses_input))
    print("relative_poses_output length = ", len(relative_poses_output))
    print("relative_poses_output type = ", type(relative_poses_output))

    # sorted_relative_poses_output = sorted(relative_poses_output.values())
    # print(sorted_relative_poses_output)

    RotationAngularErrors = []
    RotationAxisErrors = []
    TranslationDisplacementErrors = []
    TranslationAngularErrors = []
    # images = dict()
    # image_pairs = set()

    # for imgIdx in range(len(imagesGT)):
    for pairID_in, val_in in relative_poses_input.items():
        for pairID_out, val_out in relative_poses_output.items():
            if val_in.id1==val_out.id1 and val_in.id2==val_out.id2:
                loop_rotation = np.dot(val_in.R_rotmat.T, val_out.R_rotmat)
                RotationAngularErr = np.linalg.norm(rotmat_To_angleaxis(loop_rotation))
                TransMagInput = np.linalg.norm(val_in.t_vec)
                TransMagOutput = np.linalg.norm(val_out.t_vec)
                TransDistErr = TransMagInput - TransMagOutput   # can be different if normalized or not?
                TransAngularErr = math.acos(np.dot(val_in.t_vec, val_out.t_vec)/(TransMagInput*TransMagOutput))   # can be different if normalized or not?
                RotationAngularErrors.append(RotationAngularErr)
                TranslationAngularErrors.append(TransAngularErr)

    # plot difference in theta (radian)
    # RotationAngularErrorsABS = [abs(x) for x in RotationAngularErrors]
    plt.hist(RotationAngularErrors, bins='auto')  # arguments are passed to np.histogram
    plt.title("Rotation angular error in radians Histogram with 'auto' bins")
    plt.text(0.2, 80, "mean = "+str(np.mean(RotationAngularErrors)))
    plt.text(0.2, 60, "std = "+str(np.std(RotationAngularErrors)))
    # plt.show()
    plt.savefig('/home/kevin/JohannesCode/RotationAngularErrors_hist.png')
    plt.clf()

    # plot difference in theta (degree)
    theta_diff_data_degree = (180 / math.pi) * np.array(RotationAngularErrors, dtype=np.float64)
    plt.hist(theta_diff_data_degree, bins='auto')  # arguments are passed to np.histogram
    plt.title("Rotation angular error in degrees Histogram with 'auto' bins")
    plt.text(0.5, 80, "mean = "+str(np.mean(theta_diff_data_degree)))
    plt.text(0.5, 60, "std = "+str(np.std(theta_diff_data_degree)))
    # plt.show()
    plt.savefig('/home/kevin/JohannesCode/RotationAngularErrors_hist_in_degree.png')
    plt.clf()

    # plot difference in translation angular error
    # print("TranslationDirectionErrors = ", TranslationDirectionErrors)
    # plt.hist(TranslationDirectionErrors, bins='auto')  # arguments are passed to np.histogram
    plt.hist(TranslationAngularErrors, bins = 100)  # arguments are passed to np.histogram
    plt.title("Translation angular error Histogram with 'auto' bins")
    # plt.text(0.8, 150, "mean = "+str(np.mean(TranslationDirectionErrors)))
    # plt.text(0.8, 100, "std = "+str(np.std(TranslationDirectionErrors)))
    # plt.show()
    plt.savefig('/home/kevin/JohannesCode/TranslationAngularErrors.png')
    plt.clf()

    # plot difference in translation angular error
    theta_diff_data_degree = (180 / math.pi) * np.array(TranslationAngularErrors, dtype=np.float64)
    plt.hist(theta_diff_data_degree, bins='auto')  # arguments are passed to np.histogram
    plt.title("Translation angular error in degrees Histogram with 'auto' bins")
    plt.text(0.5, 80, "mean = "+str(np.mean(theta_diff_data_degree)))
    plt.text(0.5, 60, "std = "+str(np.std(theta_diff_data_degree)))
    # plt.show()
    plt.savefig('/home/kevin/JohannesCode/TranslationAngularErrors_in_degree.png')
    plt.clf()

if __name__ == "__main__":
    main()
