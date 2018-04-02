/*
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package com.sun.javafx.geom.transform;

import com.sun.javafx.geom.BaseBounds;
import com.sun.javafx.geom.Point2D;
import com.sun.javafx.geom.Vec3d;
import com.sun.javafx.geom.Vec3f;

/**
 * A general-purpose 4x4 transform that may or may not be affine. The
 * GeneralTransform is typically used only for projection transforms.
 *
 */
public final class GeneralTransform3D extends Affine3D {

    //The 4x4 double-precision floating point matrix.  The mathematical
    //representation is row major, as in traditional matrix mathematics.

    private double mpx, mpy, mpz, mpw;

    //flag if this is an identity transformation.
    private boolean identity;

    /**
     * Constructs and initializes a transform to the identity matrix.
     */
    public GeneralTransform3D() {
        setIdentity();
    }

    /**
     * Constructs and initializes a transform to the identity matrix.
     */
    public GeneralTransform3D(double mxx, double mxy, double mxz, double mxt,
                    double myx, double myy, double myz, double myt,
                    double mzx, double mzy, double mzz, double mzt,
                    double mpx, double mpy, double mpz, double mpw) {
        this.mxx = mxx;
        this.mxy = mxy;
        this.mxz = mxz;
        this.mxt = mxt;

        this.myx = myx;
        this.myy = myy;
        this.myz = myz;
        this.myt = myt;

        this.mzx = mzx;
        this.mzy = mzy;
        this.mzz = mzz;
        this.mzt = mzt;

        this.mpx = mpx;
        this.mpy = mpy;
        this.mpz = mpz;
        this.mpw = mpw;

        updateState();
    }

    public GeneralTransform3D(BaseTransform transform) {
        setTransform(transform);
    }

    @Override
    public void setTransform(final BaseTransform transform) {
        this.mxx = transform.getMxx();
        this.mxy = transform.getMxy();
        this.mxz = transform.getMxz();
        this.mxt = transform.getMxt();
        this.myx = transform.getMyx();
        this.myy = transform.getMyy();
        this.myz = transform.getMyz();
        this.myt = transform.getMyt();
        this.mzx = transform.getMzx();
        this.mzy = transform.getMzy();
        this.mzz = transform.getMzz();
        this.mzt = transform.getMzt();
        this.mpx = transform.getMpx();
        this.mpy = transform.getMpy();
        this.mpz = transform.getMpz();
        this.mpw = transform.getMpw();
        updateState();
    }

    @Override public double getMpx() { return mpx; }
    @Override public double getMpy() { return mpy; }
    @Override public double getMpz() { return mpz; }
    @Override public double getMpw() { return mpw; }

    @Override
    public Degree getDegree() {
        return Degree.GENERAL_TRANSFORM;
    }

    /**
     * Returns true if the transform is affine. A transform is considered
     * to be affine if the last row of its matrix is (0,0,0,1). Note that
     * an instance of AffineTransform3D is always affine.
     */
    @Override
    public boolean isAffine() {
        if (!isInfOrNaN() &&
                almostZero(mpx) &&
                almostZero(mpy) &&
                almostZero(mpz) &&
                almostOne(mpw)) {

            return true;
        } else {
            return false;
        }
    }

    /**
     * Sets the value of this transform to the specified transform.
     *
     * @param t1 the transform to copy into this transform.
     *
     * @return this transform
     */
    public GeneralTransform3D set(GeneralTransform3D t1) {
        // System.arraycopy(t1.mat, 0, mat, 0, mat.length);
        mxx = t1.mxx;
        mxy = t1.mxy;
        mxz = t1.mxz;
        mxt = t1.mxt;

        myx = t1.myx;
        myy = t1.myy;
        myz = t1.myz;
        myt = t1.myt;

        mzx = t1.mzx;
        mzy = t1.mzy;
        mzz = t1.mzz;
        mzt = t1.mzt;

        mpx = t1.mpx;
        mpy = t1.mpy;
        mpz = t1.mpz;
        mpw = t1.mpw;

        updateState();
        return this;
    }

    /**
     * Sets the matrix values of this transform to the values in the
     * specified array.
     *
     * @param m an array of 16 values to copy into this transform in
     * row major order.
     *
     * @return this transform
     */
    public GeneralTransform3D set(double[] m) {
        // System.arraycopy(m, 0, mat, 0, mat.length);

        mxx = m[ 0];
        mxy = m[ 1];
        mxz = m[ 2];
        mxt = m[ 3];

        myx = m[ 4];
        myy = m[ 5];
        myz = m[ 6];
        myt = m[ 7];

        mzx = m[ 8];
        mzy = m[ 9];
        mzz = m[10];
        mzt = m[11];

        mpx = m[12];
        mpy = m[13];
        mpz = m[14];
        mpw = m[15];

        updateState();
        return this;
    }

    /**
     * Returns a copy of an array of 16 values that contains the 4x4 matrix
     * of this transform. The first four elements of the array will contain
     * the top row of the transform matrix, etc.
     *
     * @param rv the return value, or null
     *
     * @return an array of 16 values
     */
    public double[] get(double[] rv) {
        if (rv == null) {
            rv = new double[16];
        }
        // System.arraycopy(mat, 0, rv, 0, mat.length);

        rv[ 0] = mxx;
        rv[ 1] = mxy;
        rv[ 2] = mxz;
        rv[ 3] = mxt;

        rv[ 4] = myx;
        rv[ 5] = myy;
        rv[ 6] = myz;
        rv[ 7] = myt;

        rv[ 8] = mzx;
        rv[ 9] = mzy;
        rv[10] = mzz;
        rv[11] = mzt;

        rv[12] = mpx;
        rv[13] = mpy;
        rv[14] = mpz;
        rv[15] = mpw;

        return rv;
    }

    public double get(int index) {
        assert ((index >= 0) && (index < 16));
        
        switch(index) {
            case  0: 
                return mxx;
            case 1: 
                return mxy;
            case 2: 
                return mxz;
            case 3: 
                return mxt;

            case 4: 
                return myx;
            case 5: 
                return myy;
            case 6: 
                return myz;
            case 7: 
                return myt;

            case 8: 
                return mzx;
            case 9: 
                return mzy;
           case 10: 
                return mzz;
           case 11: 
                return mzt;

           case 12: 
                return mpx;
           case 13: 
                return mpy;
           case 14: 
                return mpz;
           case 15: 
                return mpw;
           default:
                throw new IllegalArgumentException("Invalid index:" + index);
        }
    }

    private Vec3d tempV3d;

    public BaseBounds transform(BaseBounds src, BaseBounds dst) {
        if (tempV3d == null) {
            tempV3d = new Vec3d();
        }
        return TransformHelper.general3dBoundsTransform(this, src, dst, tempV3d);
    }

    /**
     * Transform 2D point (with z == 0)
     * @param point
     * @param pointOut
     * @return
     */
    public Point2D transform(Point2D point, Point2D pointOut) {
        if (pointOut == null) {
            pointOut = new Point2D();
        }

        double w = (float) (mpx * point.x + mpy * point.y
                 + mpw);

        pointOut.x = (float) (mxx * point.x + mxy * point.y
                + mxt);
        pointOut.y = (float) (myx * point.x + myy * point.y
                + myt);

        pointOut.x /= w;
        pointOut.y /= w;

        return pointOut;
    }

    /**
     * Transforms the point parameter with this transform and
     * places the result into pointOut.  The fourth element of the
     * point input paramter is assumed to be one.
     *
     * @param point the input point to be transformed
     *
     * @param pointOut the transformed point
     *
     * @return the transformed point
     */
    public Vec3d transform(Vec3d point, Vec3d pointOut)  {
        if (pointOut == null) {
            pointOut = new Vec3d();
        }

        // compute here as point.x, point.y and point.z may change
        // in case (point == pointOut), and mpz is usually != 0
        double w = (float) (mpx * point.x + mpy * point.y
                + mpz * point.z + mpw);

        pointOut.x = (float) (mxx * point.x + mxy * point.y
                + mxz * point.z + mxt);
        pointOut.y = (float) (myx * point.x + myy * point.y
                + myz * point.z + myt);
        pointOut.z = (float) (mzx * point.x + mzy * point.y
                + mzz * point.z + mzt);

        if (w != 0.0f) {
            pointOut.x /= w;
            pointOut.y /= w;
            pointOut.z /= w;
        }

        return pointOut;
    }


    /**
     * Transforms the point parameter with this transform and
     * places the result back into point.  The fourth element of the
     * point input paramter is assumed to be one.
     *
     * @param point the input point to be transformed
     *
     * @return the transformed point
     */
    public Vec3d transform(Vec3d point) {
        return transform(point, point);
    }

    /**
     * Transforms the normal parameter by this transform and places the value
     * into normalOut.  The fourth element of the normal is assumed to be zero.
     * Note: For correct lighting results, if a transform has uneven scaling
     * surface normals should transformed by the inverse transpose of
     * the transform. This the responsibility of the application and is not
     * done automatically by this method.
     *
     * @param normal the input normal to be transformed
     *
     * @param normalOut the transformed normal
     *
     * @return the transformed normal
     */
    public Vec3f transformNormal(Vec3f normal, Vec3f normalOut) {
        normal.x =  (float) (mxx*normal.x + mxy*normal.y +
                            mxz*normal.z);
        normal.y =  (float) (myx*normal.x + myy*normal.y +
                            myz*normal.z);
        normal.z =  (float) (mzx*normal.x + mzy*normal.y +
                             mzz*normal.z);
        return normalOut;
    }

    /**
     * Transforms the normal parameter by this transform and places the value
     * back into normal.  The fourth element of the normal is assumed to be zero.
     * Note: For correct lighting results, if a transform has uneven scaling
     * surface normals should transformed by the inverse transpose of
     * the transform. This the responsibility of the application and is not
     * done automatically by this method.
     *
     * @param normal the input normal to be transformed
     *
     * @return the transformed normal
     */
    public Vec3f transformNormal(Vec3f normal) {
        return transformNormal(normal, normal);
    }

    /**
     * Sets the value of this transform to a perspective projection transform.
     * This transform maps points from Eye Coordinates (EC)
     * to Clipping Coordinates (CC).
     * Note that the field of view is specified in radians.
     *
     * @param verticalFOV specifies whether the fov is vertical (Y direction).
     *
     * @param fov specifies the field of view in radians
     *
     * @param aspect specifies the aspect ratio. The aspect ratio is the ratio
     * of width to height.
     *
     * @param zNear the distance to the frustum's near clipping plane.
     * This value must be positive, (the value -zNear is the location of the
     * near clip plane).
     *
     * @param zFar the distance to the frustum's far clipping plane
     *
     * @return this transform
     */
    public GeneralTransform3D perspective(boolean verticalFOV,
            double fov, double aspect, double zNear, double zFar) {
        double sine;
        double cotangent;
        double deltaZ;
        double half_fov = fov * 0.5;

        deltaZ = zFar - zNear;
        sine = Math.sin(half_fov);

        cotangent = Math.cos(half_fov) / sine;

        mxx = verticalFOV ? cotangent / aspect : cotangent;
        myy = verticalFOV ? cotangent : cotangent * aspect;
        mzz = -(zFar + zNear) / deltaZ;
        mzt = -2.0 * zNear * zFar / deltaZ;
        mpz = -1.0;
        mxy = mxz = mxt = myx = myz = myt = mzx = mzy = mpx = mpy = mpw = 0;

        updateState();
        return this;
    }

    /**
     * Sets the value of this transform to an orthographic (parallel)
     * projection transform.
     * This transform maps coordinates from Eye Coordinates (EC)
     * to Clipping Coordinates (CC).  Note that unlike the similar function
     * in OpenGL, the clipping coordinates generated by the resulting
     * transform are in a right-handed coordinate system.
     * @param left the vertical line on the left edge of the near
     * clipping plane mapped to the left edge of the graphics window
     * @param right the vertical line on the right edge of the near
     * clipping plane mapped to the right edge of the graphics window
     * @param bottom the horizontal line on the bottom edge of the near
     * clipping plane mapped to the bottom edge of the graphics window
     * @param top the horizontal line on the top edge of the near
     * clipping plane mapped to the top edge of the graphics window
     * @param near the distance to the frustum's near clipping plane
     * (the value -near is the location of the near clip plane)
     * @param far the distance to the frustum's far clipping plane
     *
     * @return this transform
     */
    public GeneralTransform3D ortho(double left, double right, double bottom,
                                    double top, double near, double far) {
        double deltax = 1 / (right - left);
        double deltay = 1 / (top - bottom);
        double deltaz = 1 / (far - near);

        mxx = 2.0 * deltax;
        mxt = -(right + left) * deltax;
        myy = 2.0 * deltay;
        myt = -(top + bottom) * deltay;
        mzz = 2.0 * deltaz;
        mzt = (far + near) * deltaz;
        mxy = mxz = myx = myz = mzx =
                mzy = mpx = mpy = mpz = 0;
        mpw = 1;

        updateState();
        return this;
    }

    public double computeClipZCoord() {
        double zEc = (1.0 - mpw) / mpz;
        double zCc = mzz * zEc + mzt;
        return zCc;
    }

    /**
     * Computes the determinant of this transform.
     *
     * @return the determinant
     */
    @Override
    public double getDeterminant() {
        return determinant();
    }

    public double determinant() {
         // cofactor exapainsion along first row
         return mxx*(myy*(mzz*mpw - mzt*mpz) -
                        myz*(mzy*mpw - mzt*mpy) +
                        myt*(mzy*mpz - mzz*mpy)) -
                mxy*(myx*(mzz*mpw - mzt*mpz) -
                        myz*(mzx*mpw - mzt*mpx) +
                        myt*(mzx*mpz - mzz*mpx)) +
                mxz*(myx*(mzy*mpw - mzt*mpy) -
                        myy*(mzx*mpw - mzt*mpx) +
                        myt*(mzx*mpy - mzy*mpx)) -
                mxt*(myx*(mzy*mpz - mzz*mpy) -
                        myy*(mzx*mpz - mzz*mpx) +
                        myz*(mzx*mpy - mzy*mpx));
    }

    /**
     * Inverts this transform in place.
     *
     * @return this transform
     */
    @Override
    public void invert() {
        invert(this);
    }

    /**
     * General invert routine.  Inverts t1 and places the result in "this".
     * Note that this routine handles both the "this" version and the
     * non-"this" version.
     *
     * Also note that since this routine is slow anyway, we won't worry
     * about allocating a little bit of garbage.
     */
    private GeneralTransform3D invert(GeneralTransform3D t1) {
        double[] tmp = new double[16];
        int[] row_perm = new int[4];

        // Use LU decomposition and backsubstitution code specifically
        // for floating-point 4x4 matrices.
        // Copy source matrix to tmp
        // System.arraycopy(t1.mat, 0, tmp, 0, tmp.length);
        tmp = t1.get(tmp);

        // Calculate LU decomposition: Is the matrix singular?
        if (!luDecomposition(tmp, row_perm)) {
            // Matrix has no inverse
            throw new SingularMatrixException();
        }
        t1.set(tmp);

        // Perform back substitution on the identity matrix
        // luDecomposition will set rot[] & scales[] for use
        // in luBacksubstituation
        mxx = 1.0;  mxy = 0.0;  mxz = 0.0;  mxt = 0.0;
        myx = 0.0;  myy = 1.0;  myz = 0.0;  myt = 0.0;
        mzx = 0.0;  mzy = 0.0;  mzz = 1.0; mzt = 0.0;
        mpx = 0.0; mpy = 0.0; mpz = 0.0; mpw = 1.0;
        final double[] mat = this.get(null);
        luBacksubstitution(tmp, row_perm, mat);
        this.set(mat);

        updateState();
        return this;
    }

    /**
     * Given a 4x4 array "matrix0", this function replaces it with the
     * LU decomposition of a row-wise permutation of itself.  The input
     * parameters are "matrix0" and "dimen".  The array "matrix0" is also
     * an output parameter.  The vector "row_perm[4]" is an output
     * parameter that contains the row permutations resulting from partial
     * pivoting.  The output parameter "even_row_xchg" is 1 when the
     * number of row exchanges is even, or -1 otherwise.  Assumes data
     * type is always double.
     *
     * This function is similar to luDecomposition, except that it
     * is tuned specifically for 4x4 matrices.
     *
     * @return true if the matrix is nonsingular, or false otherwise.
     */
    private static boolean luDecomposition(double[] matrix0,
            int[] row_perm) {

        // Reference: Press, Flannery, Teukolsky, Vetterling,
        //            _Numerical_Recipes_in_C_, Cambridge University Press,
        //            1988, pp 40-45.
        //

        // Can't re-use this temporary since the method is static.
        double row_scale[] = new double[4];

        // Determine implicit scaling information by looping over rows
        {
            int i, j;
            int ptr, rs;
            double big, temp;

            ptr = 0;
            rs = 0;

            // For each row ...
            i = 4;
            while (i-- != 0) {
                big = 0.0;

                // For each column, find the largest element in the row
                j = 4;
                while (j-- != 0) {
                    temp = matrix0[ptr++];
                    temp = Math.abs(temp);
                    if (temp > big) {
                        big = temp;
                    }
                }

                // Is the matrix singular?
                if (big == 0.0) {
                    return false;
                }
                row_scale[rs++] = 1.0 / big;
            }
        }

        {
            int j;
            int mtx;

            mtx = 0;

            // For all columns, execute Crout's method
            for (j = 0; j < 4; j++) {
                int i, imax, k;
                int target, p1, p2;
                double sum, big, temp;

                // Determine elements of upper diagonal matrix U
                for (i = 0; i < j; i++) {
                    target = mtx + (4*i) + j;
                    sum = matrix0[target];
                    k = i;
                    p1 = mtx + (4*i);
                    p2 = mtx + j;
                    while (k-- != 0) {
                        sum -= matrix0[p1] * matrix0[p2];
                        p1++;
                        p2 += 4;
                    }
                    matrix0[target] = sum;
                }

                // Search for largest pivot element and calculate
                // intermediate elements of lower diagonal matrix L.
                big = 0.0;
                imax = -1;
                for (i = j; i < 4; i++) {
                    target = mtx + (4*i) + j;
                    sum = matrix0[target];
                    k = j;
                    p1 = mtx + (4*i);
                    p2 = mtx + j;
                    while (k-- != 0) {
                        sum -= matrix0[p1] * matrix0[p2];
                        p1++;
                        p2 += 4;
                    }
                    matrix0[target] = sum;

                    // Is this the best pivot so far?
                    if ((temp = row_scale[i] * Math.abs(sum)) >= big) {
                        big = temp;
                        imax = i;
                    }
                }

                if (imax < 0) {
                    return false;
                }

                // Is a row exchange necessary?
                if (j != imax) {
                    // Yes: exchange rows
                    k = 4;
                    p1 = mtx + (4*imax);
                    p2 = mtx + (4*j);
                    while (k-- != 0) {
                        temp = matrix0[p1];
                        matrix0[p1++] = matrix0[p2];
                        matrix0[p2++] = temp;
                    }

                    // Record change in scale factor
                    row_scale[imax] = row_scale[j];
                }

                // Record row permutation
                row_perm[j] = imax;

                // Is the matrix singular
                if (matrix0[(mtx + (4*j) + j)] == 0.0) {
                    return false;
                }

                // Divide elements of lower diagonal matrix L by pivot
                if (j != (4-1)) {
                    temp = 1.0 / (matrix0[(mtx + (4*j) + j)]);
                    target = mtx + (4*(j+1)) + j;
                    i = 3 - j;
                    while (i-- != 0) {
                        matrix0[target] *= temp;
                        target += 4;
                    }
                }
            }
        }

        return true;
    }


    /**
     * Solves a set of linear equations.  The input parameters "matrix1",
     * and "row_perm" come from luDecompostionD4x4 and do not change
     * here.  The parameter "matrix2" is a set of column vectors assembled
     * into a 4x4 matrix of floating-point values.  The procedure takes each
     * column of "matrix2" in turn and treats it as the right-hand side of the
     * matrix equation Ax = LUx = b.  The solution vector replaces the
     * original column of the matrix.
     *
     * If "matrix2" is the identity matrix, the procedure replaces its contents
     * with the inverse of the matrix from which "matrix1" was originally
     * derived.
     */
    //
    // Reference: Press, Flannery, Teukolsky, Vetterling,
    //        _Numerical_Recipes_in_C_, Cambridge University Press,
    //        1988, pp 44-45.
    //
    private static void luBacksubstitution(double[] matrix1,
            int[] row_perm,
            double[] matrix2) {

        int i, ii, ip, j, k;
        int rp;
        int cv, rv;

        //      rp = row_perm;
        rp = 0;

        // For each column vector of matrix2 ...
        for (k = 0; k < 4; k++) {
            //      cv = &(matrix2[0][k]);
            cv = k;
            ii = -1;

            // Forward substitution
            for (i = 0; i < 4; i++) {
                double sum;

                ip = row_perm[rp+i];
                sum = matrix2[cv+4*ip];
                matrix2[cv+4*ip] = matrix2[cv+4*i];
                if (ii >= 0) {
                    //              rv = &(matrix1[i][0]);
                    rv = i*4;
                    for (j = ii; j <= i-1; j++) {
                        sum -= matrix1[rv+j] * matrix2[cv+4*j];
                    }
                }
                else if (sum != 0.0) {
                    ii = i;
                }
                matrix2[cv+4*i] = sum;
            }

            // Backsubstitution
            //      rv = &(matrix1[3][0]);
            rv = 3*4;
            matrix2[cv+4*3] /= matrix1[rv+3];

            rv -= 4;
            matrix2[cv+4*2] = (matrix2[cv+4*2] -
                            matrix1[rv+3] * matrix2[cv+4*3]) / matrix1[rv+2];

            rv -= 4;
            matrix2[cv+4*1] = (matrix2[cv+4*1] -
                            matrix1[rv+2] * matrix2[cv+4*2] -
                            matrix1[rv+3] * matrix2[cv+4*3]) / matrix1[rv+1];

            rv -= 4;
            matrix2[cv+4*0] = (matrix2[cv+4*0] -
                            matrix1[rv+1] * matrix2[cv+4*1] -
                            matrix1[rv+2] * matrix2[cv+4*2] -
                            matrix1[rv+3] * matrix2[cv+4*3]) / matrix1[rv+0];
        }
    }


    /**
     * Sets the value of this transform to the result of multiplying itself
     * with transform t1 : this = this * t1.
      *
     * @param t1 the other transform
     *
     * @return this transform
     */
    public GeneralTransform3D mul(BaseTransform t1) {

        if (t1 instanceof GeneralTransform3D) {
            return mul((GeneralTransform3D) t1);
        }

        if (t1.isIdentity()) {
            return this;
        }

        double tmp0, tmp1, tmp2, tmp3;
        double tmp4, tmp5, tmp6, tmp7;
        double tmp8, tmp9, tmp10, tmp11;
        double tmp12, tmp13, tmp14, tmp15;

        double Txx = t1.getMxx();
        double Txy = t1.getMxy();
        double Txz = t1.getMxz();
        double Txt = t1.getMxt();
        double Tyx = t1.getMyx();
        double Tyy = t1.getMyy();
        double Tyz = t1.getMyz();
        double Tyt = t1.getMyt();
        double Tzx = t1.getMzx();
        double Tzy = t1.getMzy();
        double Tzz = t1.getMzz();
        double Tzt = t1.getMzt();

        tmp0 = mxx * Txx + mxy * Tyx + mxz * Tzx;
        tmp1 = mxx * Txy + mxy * Tyy + mxz * Tzy;
        tmp2 = mxx * Txz + mxy * Tyz + mxz * Tzz;
        tmp3 = mxx * Txt + mxy * Tyt + mxz * Tzt + mxt;
        tmp4 = myx * Txx + myy * Tyx + myz * Tzx;
        tmp5 = myx * Txy + myy * Tyy + myz * Tzy;
        tmp6 = myx * Txz + myy * Tyz + myz * Tzz;
        tmp7 = myx * Txt + myy * Tyt + myz * Tzt + myt;
        tmp8 = mzx * Txx + mzy * Tyx + mzz * Tzx;
        tmp9 = mzx * Txy + mzy * Tyy + mzz * Tzy;
        tmp10 = mzx * Txz + mzy * Tyz + mzz * Tzz;
        tmp11 = mzx * Txt + mzy * Tyt + mzz * Tzt + mzt;
        if (isAffine()) {
            tmp12 = tmp13 = tmp14 = 0;
            tmp15 = 1;
        }
        else {
            tmp12 = mpx * Txx + mpy * Tyx + mpz * Tzx;
            tmp13 = mpx * Txy + mpy * Tyy + mpz * Tzy;
            tmp14 = mpx * Txz + mpy * Tyz + mpz * Tzz;
            tmp15 = mpx * Txt + mpy * Tyt + mpz * Tzt + mpw;
        }

        mxx = tmp0;
        mxy = tmp1;
        mxz = tmp2;
        mxt = tmp3;
        myx = tmp4;
        myy = tmp5;
        myz = tmp6;
        myt = tmp7;
        mzx = tmp8;
        mzy = tmp9;
        mzz = tmp10;
        mzt = tmp11;
        mpx = tmp12;
        mpy = tmp13;
        mpz = tmp14;
        mpw = tmp15;

        updateState();
        return this;
    }

    /**
     * Sets the value of this transform to the result of multiplying itself
     * with a scale transform:
     * <pre>
     * scaletx =
     *     [ sx  0  0  0 ]
     *     [  0 sy  0  0 ]
     *     [  0  0 sz  0 ]
     *     [  0  0  0  1 ].
     * this = this * scaletx.
     * </pre>
     *
     * @param sx the X coordinate scale factor
     * @param sy the Y coordinate scale factor
     * @param sz the Z coordinate scale factor
     *
     * @return this transform
     */
    @Override
    public void scale(double sx, double sy, double sz) {
        boolean update = false;

        if (sx != 1.0) {
            mxx  *= sx;
            myx  *= sx;
            mzx  *= sx;
            mpx *= sx;
            update = true;
        }
        if (sy != 1.0) {
            mxy  *= sy;
            myy  *= sy;
            mzy  *= sy;
            mpy *= sy;
            update = true;
        }
        if (sz != 1.0) {
            mxz  *= sz;
            myz  *= sz;
            mzz *= sz;
            mpz *= sz;
            update = true;
        }

        if (update) {
            updateState();
        }
    }

    /**
     * Sets the value of this transform to the result of multiplying itself
     * with transform t1 : this = this * t1.
      *
     * @param t1 the other transform
     *
     * @return this transform
     */
    public GeneralTransform3D mul(GeneralTransform3D t1) {
        if (t1.isIdentity()) {
            return this;
        }

        double tmp0, tmp1, tmp2, tmp3;
        double tmp4, tmp5, tmp6, tmp7;
        double tmp8, tmp9, tmp10, tmp11;
        double tmp12, tmp13, tmp14, tmp15;

        if (t1.isAffine()) {
            tmp0 = mxx * t1.mxx + mxy * t1.myx + mxz * t1.mzx;
            tmp1 = mxx * t1.mxy + mxy * t1.myy + mxz * t1.mzy;
            tmp2 = mxx * t1.mxz + mxy * t1.myz + mxz * t1.mzz;
            tmp3 = mxx * t1.mxt + mxy * t1.myt + mxz * t1.mzt + mxt;
            tmp4 = myx * t1.mxx + myy * t1.myx + myz * t1.mzx;
            tmp5 = myx * t1.mxy + myy * t1.myy + myz * t1.mzy;
            tmp6 = myx * t1.mxz + myy * t1.myz + myz * t1.mzz;
            tmp7 = myx * t1.mxt + myy * t1.myt + myz * t1.mzt + myt;
            tmp8 = mzx * t1.mxx + mzy * t1.myx + mzz * t1.mzx;
            tmp9 = mzx * t1.mxy + mzy * t1.myy + mzz * t1.mzy;
            tmp10 = mzx * t1.mxz + mzy * t1.myz + mzz * t1.mzz;
            tmp11 = mzx * t1.mxt + mzy * t1.myt + mzz * t1.mzt + mzt;
            if (isAffine()) {
                tmp12 = tmp13 = tmp14 = 0;
                tmp15 = 1;
            }
            else {
                tmp12 = mpx * t1.mxx + mpy * t1.myx +
                        mpz * t1.mzx;
                tmp13 = mpx * t1.mxy + mpy * t1.myy +
                        mpz * t1.mzy;
                tmp14 = mpx * t1.mxz + mpy * t1.myz +
                        mpz * t1.mzz;
                tmp15 = mpx * t1.mxt + mpy * t1.myt +
                        mpz * t1.mzt + mpw;
            }
        } else {
            tmp0 = mxx * t1.mxx + mxy * t1.myx + mxz * t1.mzx +
                    mxt * t1.mpx;
            tmp1 = mxx * t1.mxy + mxy * t1.myy + mxz * t1.mzy +
                    mxt * t1.mpy;
            tmp2 = mxx * t1.mxz + mxy * t1.myz + mxz * t1.mzz +
                    mxt * t1.mpz;
            tmp3 = mxx * t1.mxt + mxy * t1.myt + mxz * t1.mzt +
                    mxt * t1.mpw;
            tmp4 = myx * t1.mxx + myy * t1.myx + myz * t1.mzx +
                    myt * t1.mpx;
            tmp5 = myx * t1.mxy + myy * t1.myy + myz * t1.mzy +
                    myt * t1.mpy;
            tmp6 = myx * t1.mxz + myy * t1.myz + myz * t1.mzz +
                    myt * t1.mpz;
            tmp7 = myx * t1.mxt + myy * t1.myt + myz * t1.mzt +
                    myt * t1.mpw;
            tmp8 = mzx * t1.mxx + mzy * t1.myx + mzz * t1.mzx +
                    mzt * t1.mpx;
            tmp9 = mzx * t1.mxy + mzy * t1.myy + mzz * t1.mzy +
                    mzt * t1.mpy;
            tmp10 = mzx * t1.mxz + mzy * t1.myz +
                    mzz * t1.mzz + mzt * t1.mpz;

            tmp11 = mzx * t1.mxt + mzy * t1.myt +
                    mzz * t1.mzt + mzt * t1.mpw;
            if (isAffine()) {
                tmp12 = t1.mpx;
                tmp13 = t1.mpy;
                tmp14 = t1.mpz;
                tmp15 = t1.mpw;
            } else {
                tmp12 = mpx * t1.mxx + mpy * t1.myx +
                        mpz * t1.mzx + mpw * t1.mpx;
                tmp13 = mpx * t1.mxy + mpy * t1.myy +
                        mpz * t1.mzy + mpw * t1.mpy;
                tmp14 = mpx * t1.mxz + mpy * t1.myz +
                        mpz * t1.mzz + mpw * t1.mpz;
                tmp15 = mpx * t1.mxt + mpy * t1.myt +
                        mpz * t1.mzt + mpw * t1.mpw;
            }
        }

        mxx = tmp0;
        mxy = tmp1;
        mxz = tmp2;
        mxt = tmp3;
        myx = tmp4;
        myy = tmp5;
        myz = tmp6;
        myt = tmp7;
        mzx = tmp8;
        mzy = tmp9;
        mzz = tmp10;
        mzt = tmp11;
        mpx = tmp12;
        mpy = tmp13;
        mpz = tmp14;
        mpw = tmp15;

        updateState();
        return this;
    }

    public void preConcatenate(BaseTransform transform) {
        switch (transform.getDegree()) {
            case IDENTITY:
                return;
            case TRANSLATE_2D:
                preTranslate(transform.getMxt(), transform.getMyt(), 0.0);
                return;
            case TRANSLATE_3D:
                preTranslate(transform.getMxt(), transform.getMyt(), transform.getMzt());
                return;
        }
        final double Txx = transform.getMxx();
        final double Txy = transform.getMxy();
        final double Txz = transform.getMxz();
        final double Txt = transform.getMxt();
        final double Tyx = transform.getMyx();
        final double Tyy = transform.getMyy();
        final double Tyz = transform.getMyz();
        final double Tyt = transform.getMyt();
        final double Tzx = transform.getMzx();
        final double Tzy = transform.getMzy();
        final double Tzz = transform.getMzz();
        final double Tzt = transform.getMzt();
        final double Tpx = transform.getMpx();
        final double Tpy = transform.getMpy();
        final double Tpz = transform.getMpz();
        final double Tpw = transform.getMpw();
        final double rxx = (Txx * mxx + Txy * myx + Txz * mzx + Txt * mpx);
        final double rxy = (Txx * mxy + Txy * myy + Txz * mzy + Txt * mpy);
        final double rxz = (Txx * mxz + Txy * myz + Txz * mzz + Txt * mpz);
        final double rxt = (Txx * mxt + Txy * myt + Txz * mzt + Txt * mpw);
        final double ryx = (Tyx * mxx + Tyy * myx + Tyz * mzx + Tyt * mpx);
        final double ryy = (Tyx * mxy + Tyy * myy + Tyz * mzy + Tyt * mpy);
        final double ryz = (Tyx * mxz + Tyy * myz + Tyz * mzz + Tyt * mpz);
        final double ryt = (Tyx * mxt + Tyy * myt + Tyz * mzt + Tyt * mpw);
        final double rzx = (Tzx * mxx + Tzy * myx + Tzz * mzx + Tzt * mpx);
        final double rzy = (Tzx * mxy + Tzy * myy + Tzz * mzy + Tzt * mpy);
        final double rzz = (Tzx * mxz + Tzy * myz + Tzz * mzz + Tzt * mpz);
        final double rzt = (Tzx * mxt + Tzy * myt + Tzz * mzt + Tzt * mpw);
        final double rpx = (Tpx * mxx + Tpy * myx + Tpz * mzx + Tpw * mpx);
        final double rpy = (Tpx * mxy + Tpy * myy + Tpz * mzy + Tpw * mpy);
        final double rpz = (Tpx * mxz + Tpy * myz + Tpz * mzz + Tpw * mpz);
        final double rpw = (Tpx * mxt + Tpy * myt + Tpz * mzt + Tpw * mpw);
        this.mxx = rxx;
        this.mxy = rxy;
        this.mxz = rxz;
        this.mxt = rxt;
        this.myx = ryx;
        this.myy = ryy;
        this.myz = ryz;
        this.myt = ryt;
        this.mzx = rzx;
        this.mzy = rzy;
        this.mzz = rzz;
        this.mzt = rzt;
        this.mpx = rpx;
        this.mpy = rpy;
        this.mpz = rpz;
        this.mpw = rpw;
        updateState();
    }

    /**
     * Sets this transform to the identity matrix.
     *
     * @return void
     */
    public void setIdentity() {
        setToIdentity();
    }

    /**
     * Returns true if the transform is identity. A transform is considered
     * to be identity if the diagonal elements of its matrix is all 1s
     * otherwise 0s.
     */
    @Override
    public boolean isIdentity() {
        return super.isIdentity() && identity;
    }

    @Override
    protected void reset3Delements() {
        super.reset3Delements();
        mpx = 0.0; mpy = 0.0; mpz = 0.0; mpw = 1.0;
        identity = true;
    }

    @Override
    protected void updateState() {
        //set the identity flag.
        identity = mpx == 0.0 && mpy == 0.0 && mpz == 0.0;
        super.updateState();
    }

    // Check whether matrix has an Infinity or NaN value. If so, don't treat it
    // as affine.
    boolean isInfOrNaN() {
        // The following is a faster version of the check.
        // Instead of 3 tests per array element (Double.isInfinite is 2 tests),
        // for a total of 48 tests, we will do 16 multiplies and 1 test.
        final double[] mat = get(null);
        double d = 0.0;
        for (int i = 0; i < mat.length; i++) {
            d *= mat[i];
        }

        return d != 0.0;
    }

    static boolean isAffine(double[] mat) {
        if (almostZero(mat[12]) &&
                almostZero(mat[13]) &&
                almostZero(mat[14]) &&
                almostOne(mat[15])) {

            return true;
        } else {
            return false;
        }
    }

    public GeneralTransform3D copy() {
        GeneralTransform3D newTransform = new GeneralTransform3D();
        newTransform.set(this);
        return newTransform;
    }
}
