package com.nokia.mid.m3d;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Locale;
import javax.microedition.lcdui.Graphics;

/**
 * Logging M3D: keeps OpenGL-style projection/modelview matrix stacks in
 * doubles (M = M * T, glRotate / glFrustum semantics, 16.16 fixed-point
 * inputs) and logs every matrix operation and draw call to a static list
 * instead of rasterizing. The harness drains the list once per tick.
 *
 * Log line formats (no indentation; the harness indents):
 *   matrixMode 5888|5889
 *   frustumxi l r b t n f            (raw 16.16 ints)
 *   loadIdentity [P]
 *   translatexi x y z [P]
 *   rotatexi angle x y z [P]
 *   scalexi x y z [P]
 *   pushMatrix / popMatrix [P]
 *   clear mask
 *   drawElements mode=M count=N color=r,g,b,a tex=none|idx:WxH
 *   drawArrays mode=M first=F count=N color=... tex=...
 *     tri eye=(x,y,z) (x,y,z) (x,y,z) ndc=(x,y,z,w) (x,y,z,w) (x,y,z,w)
 *       eye = modelview * v; ndc = (clip.xyz / clip.w, clip.w) with clip = projection * eye
 *   blit x y w h
 * A trailing " P" marks operations done while the projection matrix was current.
 */
public class M3D {
    public static final int GL_MODELVIEW = 5888;
    public static final int GL_PROJECTION = 5889;
    public static final int GL_TEXTURE_2D = 3553;
    public static final int GL_TRIANGLES = 4;
    public static final int GL_TRIANGLE_STRIP = 5;
    public static final int GL_TRIANGLE_FAN = 6;
    public static final int GL_VERTEX_ARRAY = 32884;
    public static final int GL_TEXTURE_COORD_ARRAY = 32888;

    private static final ArrayList<String> log = new ArrayList<String>();
    /** set by blit(), cleared by drainLog() */
    public static boolean frameComplete = false;

    private double[][] modelview = identity();
    private double[][] projection = identity();
    private final ArrayDeque<double[][]> mvStack = new ArrayDeque<double[][]>();
    private final ArrayDeque<double[][]> projStack = new ArrayDeque<double[][]>();
    private int mode = GL_MODELVIEW;

    private int bufW, bufH;
    private final boolean[] enabled = new boolean[1];
    private boolean texturing = false;
    private boolean vertexArray = false, texCoordArray = false;
    private Texture boundTexture = null;
    private int cr = 255, cg = 255, cb = 255, ca = 255;
    private byte[] vdata;
    private int vsize = 3, vstride = 0;
    private byte[] tdata;
    private int tsize = 2, tstride = 0;

    private M3D() {
    }

    public static M3D createInstance() {
        return new M3D();
    }

    /** Harness: take the accumulated log lines and clear them. */
    public static ArrayList<String> drainLog() {
        ArrayList<String> r = new ArrayList<String>(log);
        log.clear();
        frameComplete = false;
        return r;
    }

    private static void log(String s) {
        log.add(s);
    }

    private String modeTag() {
        return mode == GL_PROJECTION ? " P" : "";
    }

    // ---- setup ----

    public void setupBuffers(int flags, int w, int h) {
        bufW = w;
        bufH = h;
    }

    public void removeBuffers() {
    }

    public void clearColor4ub(byte r, byte g, byte b, byte a) {
    }

    public void enable(int cap) {
        if (cap == GL_TEXTURE_2D) {
            texturing = true;
        }
    }

    public void disable(int cap) {
        if (cap == GL_TEXTURE_2D) {
            texturing = false;
        }
    }

    public void cullFace(int face) {
    }

    public void viewport(int x, int y, int w, int h) {
    }

    public void enableClientState(int arr) {
        if (arr == GL_VERTEX_ARRAY) {
            vertexArray = true;
        } else if (arr == GL_TEXTURE_COORD_ARRAY) {
            texCoordArray = true;
        }
    }

    public void disableClientState(int arr) {
        if (arr == GL_VERTEX_ARRAY) {
            vertexArray = false;
        } else if (arr == GL_TEXTURE_COORD_ARRAY) {
            texCoordArray = false;
        }
    }

    public void clear(int mask) {
        log("clear " + mask);
    }

    // ---- matrices ----

    public void matrixMode(int m) {
        mode = m;
        log("matrixMode " + m);
    }

    private double[][] cur() {
        return mode == GL_PROJECTION ? projection : modelview;
    }

    private void setCur(double[][] m) {
        if (mode == GL_PROJECTION) {
            projection = m;
        } else {
            modelview = m;
        }
    }

    private void mult(double[][] t) {
        setCur(mul(cur(), t));
    }

    public void loadIdentity() {
        setCur(identity());
        log("loadIdentity" + modeTag());
    }

    public void frustumxi(int l, int r, int b, int t, int n, int f) {
        double L = fx(l), R = fx(r), B = fx(b), T = fx(t), N = fx(n), F = fx(f);
        double[][] m = new double[4][4];
        m[0][0] = 2 * N / (R - L);
        m[0][2] = (R + L) / (R - L);
        m[1][1] = 2 * N / (T - B);
        m[1][2] = (T + B) / (T - B);
        m[2][2] = -(F + N) / (F - N);
        m[2][3] = -2 * F * N / (F - N);
        m[3][2] = -1;
        mult(m);
        log("frustumxi " + l + " " + r + " " + b + " " + t + " " + n + " " + f + modeTag());
    }

    public void translatexi(int x, int y, int z) {
        double[][] m = identity();
        m[0][3] = fx(x);
        m[1][3] = fx(y);
        m[2][3] = fx(z);
        mult(m);
        log("translatexi " + x + " " + y + " " + z + modeTag());
    }

    public void scalexi(int x, int y, int z) {
        double[][] m = identity();
        m[0][0] = fx(x);
        m[1][1] = fx(y);
        m[2][2] = fx(z);
        mult(m);
        log("scalexi " + x + " " + y + " " + z + modeTag());
    }

    public void rotatexi(int angle, int ax, int ay, int az) {
        double a = Math.toRadians(fx(angle));
        double x = fx(ax), y = fx(ay), z = fx(az);
        double len = Math.sqrt(x * x + y * y + z * z);
        if (len > 0) {
            x /= len;
            y /= len;
            z /= len;
        }
        double c = Math.cos(a), s = Math.sin(a), k = 1 - c;
        double[][] m = identity();
        m[0][0] = x * x * k + c;
        m[0][1] = x * y * k - z * s;
        m[0][2] = x * z * k + y * s;
        m[1][0] = y * x * k + z * s;
        m[1][1] = y * y * k + c;
        m[1][2] = y * z * k - x * s;
        m[2][0] = x * z * k - y * s;
        m[2][1] = y * z * k + x * s;
        m[2][2] = z * z * k + c;
        mult(m);
        log("rotatexi " + angle + " " + ax + " " + ay + " " + az + modeTag());
    }

    public void pushMatrix() {
        if (mode == GL_PROJECTION) {
            projStack.push(copy(projection));
        } else {
            mvStack.push(copy(modelview));
        }
        log("pushMatrix" + modeTag());
    }

    public void popMatrix() {
        if (mode == GL_PROJECTION) {
            if (projStack.isEmpty()) {
                throw new IllegalStateException("projection stack underflow");
            }
            projection = projStack.pop();
        } else {
            if (mvStack.isEmpty()) {
                throw new IllegalStateException("modelview stack underflow");
            }
            modelview = mvStack.pop();
        }
        log("popMatrix" + modeTag());
    }

    // ---- state ----

    public void color4ub(byte r, byte g, byte b, byte a) {
        cr = r & 0xff;
        cg = g & 0xff;
        cb = b & 0xff;
        ca = a & 0xff;
    }

    public void bindTexture(int target, Texture t) {
        boundTexture = t;
    }

    public void vertexPointerub(int size, int stride, byte[] data) {
        vsize = size;
        vstride = stride;
        vdata = data;
    }

    public void texCoordPointerub(int size, int stride, byte[] data) {
        tsize = size;
        tstride = stride;
        tdata = data;
    }

    // ---- draws ----

    private double[] vertex(int i) {
        int step = vstride == 0 ? vsize : vstride;
        double[] v = new double[4];
        for (int k = 0; k < 3; k++) {
            v[k] = k < vsize ? vdata[i * step + k] : 0; // signed bytes
        }
        v[3] = 1;
        return v;
    }

    private String drawHeader() {
        return "color=" + cr + "," + cg + "," + cb + "," + ca + " tex="
                + (texturing ? (boundTexture == null ? "unbound" : boundTexture.toString()) : "none");
    }

    public void drawElementsub(int mode, int count, byte[] indices) {
        int[] idx = new int[count];
        for (int i = 0; i < count; i++) {
            idx[i] = indices[i] & 0xff;
        }
        log("drawElements mode=" + mode + " count=" + count + " " + drawHeader());
        logPrimitives(mode, idx);
    }

    public void drawArrays(int mode, int first, int count) {
        int[] idx = new int[count];
        for (int i = 0; i < count; i++) {
            idx[i] = first + i;
        }
        log("drawArrays mode=" + mode + " first=" + first + " count=" + count + " " + drawHeader());
        logPrimitives(mode, idx);
    }

    private void logPrimitives(int mode, int[] idx) {
        if (mode == GL_TRIANGLES) {
            for (int i = 0; i + 2 < idx.length; i += 3) {
                logTri(idx[i], idx[i + 1], idx[i + 2]);
            }
        } else if (mode == GL_TRIANGLE_STRIP) {
            for (int i = 0; i + 2 < idx.length; i++) {
                if ((i & 1) == 0) {
                    logTri(idx[i], idx[i + 1], idx[i + 2]);
                } else {
                    logTri(idx[i + 1], idx[i], idx[i + 2]);
                }
            }
        } else if (mode == GL_TRIANGLE_FAN) {
            for (int i = 1; i + 1 < idx.length; i++) {
                logTri(idx[0], idx[i], idx[i + 1]);
            }
        } else {
            StringBuilder sb = new StringBuilder("verts");
            for (int i : idx) {
                double[] e = apply(modelview, vertex(i));
                sb.append(" eye=").append(fmt3(e));
            }
            log(sb.toString());
        }
    }

    private void logTri(int a, int b, int c) {
        int[] v = {a, b, c};
        StringBuilder eye = new StringBuilder();
        StringBuilder ndc = new StringBuilder();
        for (int k = 0; k < 3; k++) {
            double[] e = apply(modelview, vertex(v[k]));
            double[] cl = apply(projection, e);
            double w = cl[3];
            if (k > 0) {
                eye.append(' ');
                ndc.append(' ');
            }
            eye.append(fmt3(e));
            // ndc = clip.xyz / clip.w, followed by clip.w itself
            ndc.append(String.format(Locale.ROOT, "(%.4f,%.4f,%.4f,%.4f)",
                    z(cl[0] / w), z(cl[1] / w), z(cl[2] / w), z(w)));
        }
        log("tri eye=" + eye + " ndc=" + ndc);
    }

    public void blit(Graphics g, int x, int y, int w, int h) {
        // The 3D frame buffer (cleared to white) would be copied here; we
        // only paint the clear color so the sprites drawn afterwards land on
        // a white background.
        g.setColor(0xffffff);
        g.fillRect(x, y, w, h);
        g.setColor(0);
        log("blit " + x + " " + y + " " + w + " " + h);
        frameComplete = true;
    }

    // ---- math helpers ----

    private static double fx(int v) {
        return v / 65536.0;
    }

    private static double[][] identity() {
        double[][] m = new double[4][4];
        for (int i = 0; i < 4; i++) {
            m[i][i] = 1;
        }
        return m;
    }

    private static double[][] copy(double[][] m) {
        double[][] r = new double[4][4];
        for (int i = 0; i < 4; i++) {
            System.arraycopy(m[i], 0, r[i], 0, 4);
        }
        return r;
    }

    private static double[][] mul(double[][] a, double[][] b) {
        double[][] r = new double[4][4];
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                double s = 0;
                for (int k = 0; k < 4; k++) {
                    s += a[i][k] * b[k][j];
                }
                r[i][j] = s;
            }
        }
        return r;
    }

    private static double[] apply(double[][] m, double[] v) {
        double[] r = new double[4];
        for (int i = 0; i < 4; i++) {
            r[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2] + m[i][3] * v[3];
        }
        return r;
    }

    private static String fmt3(double[] v) {
        return String.format(Locale.ROOT, "(%.4f,%.4f,%.4f)", z(v[0]), z(v[1]), z(v[2]));
    }

    /** avoid "-0.0000" */
    private static double z(double d) {
        return Math.abs(d) < 0.00005 ? 0.0 : d;
    }

    /** Harness: current modelview matrix (row-major). */
    public double[][] getModelview() {
        return copy(modelview);
    }

    public double[][] getProjection() {
        return copy(projection);
    }
}
