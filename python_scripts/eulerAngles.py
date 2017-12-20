# emacs: -*- mode: python-mode; py-indent-offset: 4; indent-tabs-mode: nil -*-
# vi: set ft=python sts=4 ts=4 sw=4 et:
### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ##
#
#   See COPYING file distributed along with the NiBabel package for the
#   copyright and license terms.
#
### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ### ##
''' Module implementing Euler angle rotations and their conversions

See:

* http://en.wikipedia.org/wiki/Rotation_matrix
* http://en.wikipedia.org/wiki/Euler_angles
* http://mathworld.wolfram.com/EulerAngles.html

See also: *Representing Attitude with Euler Angles and Quaternions: A
Reference* (2006) by James Diebel. A cached PDF link last found here:

http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.110.5134

Euler's rotation theorem tells us that any rotation in 3D can be
described by 3 angles.  Let's call the 3 angles the *Euler angle vector*
and call the angles in the vector :math:`alpha`, :math:`beta` and
:math:`gamma`.  The vector is [ :math:`alpha`,
:math:`beta`. :math:`gamma` ] and, in this description, the order of the
parameters specifies the order in which the rotations occur (so the
rotation corresponding to :math:`alpha` is applied first).

In order to specify the meaning of an *Euler angle vector* we need to
specify the axes around which each of the rotations corresponding to
:math:`alpha`, :math:`beta` and :math:`gamma` will occur.

There are therefore three axes for the rotations :math:`alpha`,
:math:`beta` and :math:`gamma`; let's call them :math:`i` :math:`j`,
:math:`k`.

Let us express the rotation :math:`alpha` around axis `i` as a 3 by 3
rotation matrix `A`.  Similarly :math:`beta` around `j` becomes 3 x 3
matrix `B` and :math:`gamma` around `k` becomes matrix `G`.  Then the
whole rotation expressed by the Euler angle vector [ :math:`alpha`,
:math:`beta`. :math:`gamma` ], `R` is given by::

   R = np.dot(G, np.dot(B, A))

See http://mathworld.wolfram.com/EulerAngles.html

The order :math:`G B A` expresses the fact that the rotations are
performed in the order of the vector (:math:`alpha` around axis `i` =
`A` first).

To convert a given Euler angle vector to a meaningful rotation, and a
rotation matrix, we need to define:

* the axes `i`, `j`, `k`
* whether a rotation matrix should be applied on the left of a vector to
  be transformed (vectors are column vectors) or on the right (vectors
  are row vectors).
* whether the rotations move the axes as they are applied (intrinsic
  rotations) - compared the situation where the axes stay fixed and the
  vectors move within the axis frame (extrinsic)
* the handedness of the coordinate system

See: http://en.wikipedia.org/wiki/Rotation_matrix#Ambiguities

We are using the following conventions:

* axes `i`, `j`, `k` are the `z`, `y`, and `x` axes respectively.  Thus
  an Euler angle vector [ :math:`alpha`, :math:`beta`. :math:`gamma` ]
  in our convention implies a :math:`alpha` radian rotation around the
  `z` axis, followed by a :math:`beta` rotation around the `y` axis,
  followed by a :math:`gamma` rotation around the `x` axis.
* the rotation matrix applies on the left, to column vectors on the
  right, so if `R` is the rotation matrix, and `v` is a 3 x N matrix
  with N column vectors, the transformed vector set `vdash` is given by
  ``vdash = np.dot(R, v)``.
* extrinsic rotations - the axes are fixed, and do not move with the
  rotations.
* a right-handed coordinate system

The convention of rotation around ``z``, followed by rotation around
``y``, followed by rotation around ``x``, is known (confusingly) as
"xyz", pitch-roll-yaw, Cardan angles, or Tait-Bryan angles.
'''

import math

import numpy as np
import functools

_FLOAT_EPS_4 = np.finfo(float).eps * 4.0


def euler2mat(z=0, y=0, x=0):
    ''' Return matrix for rotations around z, y and x axes

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
    M : array shape (3,3)
       Rotation matrix giving same rotation as for given angles

    Examples
    --------
    >>> zrot = 1.3 # radians
    >>> yrot = -0.1
    >>> xrot = 0.2
    >>> M = euler2mat(zrot, yrot, xrot)
    >>> M.shape == (3, 3)
    True

    The output rotation matrix is equal to the composition of the
    individual rotations

    >>> M1 = euler2mat(zrot)
    >>> M2 = euler2mat(0, yrot)
    >>> M3 = euler2mat(0, 0, xrot)
    >>> composed_M = np.dot(M3, np.dot(M2, M1))
    >>> np.allclose(M, composed_M)
    True

    You can specify rotations by named arguments

    >>> np.all(M3 == euler2mat(x=xrot))
    True

    When applying M to a vector, the vector should column vector to the
    right of M.  If the right hand side is a 2D array rather than a
    vector, then each column of the 2D array represents a vector.

    >>> vec = np.array([1, 0, 0]).reshape((3,1))
    >>> v2 = np.dot(M, vec)
    >>> vecs = np.array([[1, 0, 0],[0, 1, 0]]).T # giving 3x2 array
    >>> vecs2 = np.dot(M, vecs)

    Rotations are counter-clockwise.

    >>> zred = np.dot(euler2mat(z=np.pi/2), np.eye(3))
    >>> np.allclose(zred, [[0, -1, 0],[1, 0, 0], [0, 0, 1]])
    True
    >>> yred = np.dot(euler2mat(y=np.pi/2), np.eye(3))
    >>> np.allclose(yred, [[0, 0, 1],[0, 1, 0], [-1, 0, 0]])
    True
    >>> xred = np.dot(euler2mat(x=np.pi/2), np.eye(3))
    >>> np.allclose(xred, [[1, 0, 0],[0, 0, -1], [0, 1, 0]])
    True

    Notes
    -----
    The direction of rotation is given by the right-hand rule (orient
    the thumb of the right hand along the axis around which the rotation
    occurs, with the end of the thumb at the positive end of the axis;
    curl your fingers; the direction your fingers curl is the direction
    of rotation).  Therefore, the rotations are counterclockwise if
    looking along the axis of rotation from positive to negative.
    '''
    Ms = []
    if z:
        cosz = math.cos(z)
        sinz = math.sin(z)
        Ms.append(np.array(
                [[cosz, -sinz, 0],
                 [sinz, cosz, 0],
                 [0, 0, 1]]))
    if y:
        cosy = math.cos(y)
        siny = math.sin(y)
        Ms.append(np.array(
                [[cosy, 0, siny],
                 [0, 1, 0],
                 [-siny, 0, cosy]]))
    if x:
        cosx = math.cos(x)
        sinx = math.sin(x)
        Ms.append(np.array(
                [[1, 0, 0],
                 [0, cosx, -sinx],
                 [0, sinx, cosx]]))
    if Ms:
        return functools.reduce(np.dot, Ms[::-1])
    return np.eye(3)


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


def quat2euler(q):
    ''' Return Euler angles corresponding to quaternion `q`

    Parameters
    ----------
    q : 4 element sequence
       w, x, y, z of quaternion

    Returns
    -------
    z : scalar
       Rotation angle in radians around z-axis (performed first)
    y : scalar
       Rotation angle in radians around y-axis
    x : scalar
       Rotation angle in radians around x-axis (performed last)

    Notes
    -----
    It's possible to reduce the amount of calculation a little, by
    combining parts of the ``quat2mat`` and ``mat2euler`` functions, but
    the reduction in computation is small, and the code repetition is
    large.
    '''
    # delayed import to avoid cyclic dependencies
    import nibabel.quaternions as nq
    return mat2euler(nq.quat2mat(q))


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


def angle_axis2euler(theta, vector, is_normalized=False):
    ''' Convert angle, axis pair to Euler angles

    Parameters
    ----------
    theta : scalar
       angle of rotation
    vector : 3 element sequence
       vector specifying axis for rotation.
    is_normalized : bool, optional
       True if vector is already normalized (has norm of 1).  Default
       False

    Returns
    -------
    z : scalar
    y : scalar
    x : scalar
       Rotations in radians around z, y, x axes, respectively

    Examples
    --------
    >>> z, y, x = angle_axis2euler(0, [1, 0, 0])
    >>> np.allclose((z, y, x), 0)
    True

    Notes
    -----
    It's possible to reduce the amount of calculation a little, by
    combining parts of the ``angle_axis2mat`` and ``mat2euler``
    functions, but the reduction in computation is small, and the code
    repetition is large.
    '''
    # delayed import to avoid cyclic dependencies
    import nibabel.quaternions as nq
    M = nq.angle_axis2mat(theta, vector, is_normalized)
    return mat2euler(M)


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


def main():
    # # idMat = np.eye(3)
    # # Processing P1180151.JPG---P1180153.JPG
    # # testRmat = np.array([[ 0.99280155,-0.0412883,0.11242928], [0.04363471, 0.99887645, -0.01848888], [-0.11153959, 0.0232616, 0.99348772]])
    # # testRmat = np.array([[ 0.947427, 0.035579, 0.317987], [-0.0170551, 0.998001, -0.0608497], [-0.319516, 0.0522274, 0.94614]])
    # # testRmat = np.array([[0.999496, 0.0312212, 0.00579196], [-0.0311686, 0.999474, -0.00897326], [-0.00606907, 0.00878821, 0.999943]])
    # testRmat = np.array([[0.975346, -0.112274, -0.189986], [0.114838, 0.993381, 0.00250306], [0.188448, -0.024259, 0.981784]])
    # # R_angleaxis =  [ 0.02092701  0.11226213  0.0425668 ]  or (0.12187147397636398, array([ 0.17171376,  0.92115177,  0.34927612]))
    # print(testRmat)
    # eulerAnlges = mat2euler(testRmat)
    # # print(eulerAnlges)
    # # print(euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2]))
    # recov_angle_axis_result = euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2])
    # # print(type(recov_angle_axis_result))
    # # print(type(recov_angle_axis_result[0]))
    # # print(type(recov_angle_axis_result[1]))
    # print("recovered angle axis value (with magnitude and in radians) = ", recov_angle_axis_result[0]*(recov_angle_axis_result[1]))

    #############   IMAGE_ID1, IMAGE_ID2, QW12, QX12, QY12, QZ12, TX12, TY12, TZ12, CAMERA_ID1, NAME1, CAMERA_ID1, NAME2, RotMat[0,0]
    # 1 2 0.999864 0.00444097 0.00296566 -0.0155996 -0.13027 -0.0106842 -0.0746331 1 P1180141.JPG 2 P1180142.JPG 0.999496 0.0312212 0.00579196 -0.0311686 0.999474 -0.00897326 -0.00606907 0.00878821 0.999943
    # angle axis12=0.008882 0.005932 -0.031201
    # 2 1 0.999864 -0.00444097 -0.00296566 0.0155996 0.129418 0.0154016 0.0752875 2 P1180142.JPG 1 P1180141.JPG 0.999496 -0.0311686 -0.00606907 0.0312212 0.999474 0.00878821 0.00579196 -0.00897326 0.999943
    # angle axis21=...
    # 1 1 P1180141.JPG 0.865083 0.017533 0.479916 -0.144927 -0.683235 1.003010 3.631200 0.497353 0.267576 0.825253 -0.233920 0.957378 -0.169440 -0.835417 -0.108771 0.538746 0.036733 1.005471 -0.303636
    # 2 2 P1180142.JPG 0.861204 0.028429 0.482787 -0.156323 -0.760813 0.980505 3.569320 0.484960 0.296702 0.822667 -0.241802 0.949510 -0.199907 -0.840444 -0.101975 0.532217 0.059643 1.012884 -0.327965

    qvec12 = np.array([0.999864, 0.00444097, 0.00296566, -0.0155996])
    tvec12 = np.array([-0.13027, -0.0106842, -0.0746331])
    angleaxis12 = np.array([0.008882, 0.005932, -0.031201])
    RotMat12byColmap = np.array([[0.999496, 0.0312212, 0.00579196], [-0.0311686, 0.999474, -0.00897326], [-0.00606907, 0.00878821, 0.999943]])
    RotMat12byPython = quaternion2RotMat(qvec12[0], qvec12[1], qvec12[2], qvec12[3])
    print("RotMat12byColmap = ", RotMat12byColmap)
    # print("RotMat12byPython = ", RotMat12byPython)
    # print("np.round(RotMat12byPython, 6) = ", np.round(RotMat12byPython, 6))
    # print("(RotMat12byColmap==RotMat12byPython) = ", RotMat12byColmap==np.round(RotMat12byPython, 6))
    # print("norm(RotMat12byColmap-RotMat12byPython) = ", np.linalg.norm(RotMat12byColmap-RotMat12byPython))

    qvec21 = np.array([0.999864, -0.00444097, -0.00296566, 0.0155996])
    tvec21 = np.array([0.129418, 0.0154016, 0.0752875])
    # angleaxis21 = np.array([0.008882, 0.005932, -0.031201])
    RotMat21byColmap = np.array([[0.999496, -0.0311686, -0.00606907], [0.0312212, 0.999474, 0.00878821], [0.00579196, -0.00897326, 0.999943]])
    RotMat21byPython = quaternion2RotMat(qvec21[0], qvec21[1], qvec21[2], qvec21[3])
    # print("norm(RotMat21byColmap-RotMat21byPython) = ", np.linalg.norm(RotMat21byColmap-RotMat21byPython))

    qvec1 = np.array([0.865083, 0.017533, 0.479916, -0.144927])
    tvec1 = np.array([-0.683235, 1.003010, 3.631200])
    angleaxis1 = np.array([0.036733, 1.005471, -0.303636])
    RotMat1 = np.array([[0.497353, 0.267576, 0.825253], [-0.233920, 0.957378, -0.169440], [-0.835417, -0.108771, 0.538746]])

    qvec2 = np.array([0.861204, 0.028429, 0.482787, -0.156323])
    tvec2 = np.array([-0.760813, 0.980505, 3.569320])
    angleaxis2 = np.array([0.059643, 1.012884, -0.327965])
    RotMat2 = np.array([[0.484960, 0.296702, 0.822667], [-0.241802, 0.949510, -0.199907], [-0.840444, -0.101975, 0.532217]])

    # Colmap convention
    RotMat12Test = np.dot(RotMat1.T, RotMat2)
    print("RotMat12Test = ", RotMat12Test)
    RotMat12Test2 = np.dot(RotMat1, RotMat2.T)
    print("RotMat12Test2 = ", RotMat12Test2)

    # theia convention
    # R_12 = R2 * R1^t, c2' = R1 * (c2 - c1)
    RotMat12Theia = np.dot(RotMat2, RotMat1.T)################################################
    RotMat21Theia = np.dot(RotMat1, RotMat2.T)
    print("RotMat12Theia = ", RotMat12Theia)
    # print("det(RotMat12Theia) = ", np.linalg.det(RotMat12Theia))
    # print("RotMat21Theia = ", RotMat21Theia)
    tvec12Theia = np.dot(RotMat1, (tvec2 - tvec1))
    tvec21Theia = np.dot(RotMat2, (tvec1 - tvec2))
    print("tvec12Theia = ", tvec12Theia)
    print("tvec12 = ", tvec12)
    # print("tvec21Theia = ", tvec21Theia)
    print("norm(tvec12Theia) = ", np.linalg.norm(tvec12Theia))
    print("norm(tvec12) = ", np.linalg.norm(tvec12))
    # print("norm(tvec21Theia) = ", np.linalg.norm(tvec21Theia))

    # # img1 = P1180151.JPG, img2 = P1180153.JPG
    # rotation =  np.array([-0.00639922, 0.27083576, -0.06973372]);
    # translation =   np.array([-0.98955333, 0.00689735, 0.09152839]);
    # print(euler2mat(rotation[2],rotation[1], rotation[0]))
    # print(euler2mat(rotation[0],rotation[1], rotation[2]))
    #
    # # img1 = P1180153.JPG, img2 = P1180151.JPG
    # rotation =  np.array([-0.09177648, 0.44861105, -0.19247155]);
    # translation =   np.array([-0.93988287, -0.0554528, 0.17447308]);
    # print(euler2mat(rotation[2],rotation[1], rotation[0]))
    # print(euler2mat(rotation[0],rotation[1], rotation[2]))

    # testRmat = np.array([[0.975346, -0.112274, -0.189986], [0.114838, 0.993381, 0.00250306], [0.188448, -0.024259, 0.981784]])
    # testRmat = np.array([[0.993732, -0.0131249, -0.111014], [0.021238, 0.997163, 0.0722189], [0.109751, -0.074124, 0.991191]])
    # testRmat = np.array([[0.995526, 0.0405761, 0.085336], [-0.0367629, 0.998274, -0.0457907], [-0.0870468, 0.0424486, 0.995299]])
    testRmat = np.array([[0.998328, 0.0361585, -0.0450936], [-0.0365612, 0.999298, -0.00813713], [0.0447677, 0.0097722, 0.99895]])


    print(testRmat)
    eulerAnlges = mat2euler(testRmat)
    recov_angle_axis_result = euler2angle_axis(eulerAnlges[0], eulerAnlges[1], eulerAnlges[2])
    print("recovered angle axis value (with magnitude and in radians) = ", recov_angle_axis_result[0]*(recov_angle_axis_result[1]))

    return 0

if __name__ == "__main__":
    main()
