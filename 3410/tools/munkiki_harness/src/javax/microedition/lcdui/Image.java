package javax.microedition.lcdui;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

/** 1-bit image: raster byte per pixel, 1 = black (ink), 0 = white. */
public class Image {
    private final int width;
    private final int height;
    private final byte[] raster;
    private final boolean mutable;
    /** order of creation, for logging */
    public final int serial;
    private static int counter = 0;

    private Image(int w, int h, boolean mutable) {
        this.width = w;
        this.height = h;
        this.raster = new byte[w * h];
        this.mutable = mutable;
        this.serial = counter++;
    }

    public static Image createImage(int w, int h) {
        if (w <= 0 || h <= 0) {
            throw new IllegalArgumentException("createImage " + w + "x" + h);
        }
        return new Image(w, h, true);
    }

    public static Image createImage(Image src) {
        Image img = new Image(src.width, src.height, false);
        System.arraycopy(src.raster, 0, img.raster, 0, src.raster.length);
        return img;
    }

    public static Image createImage(String name) throws IOException {
        InputStream in = Image.class.getResourceAsStream(name);
        if (in == null) {
            throw new IOException("resource not found: " + name);
        }
        ByteArrayOutputStream bos = new ByteArrayOutputStream();
        byte[] buf = new byte[4096];
        int n;
        while ((n = in.read(buf)) > 0) {
            bos.write(buf, 0, n);
        }
        in.close();
        return decodePng(bos.toByteArray());
    }

    public Graphics getGraphics() {
        if (!mutable) {
            throw new IllegalStateException("immutable image");
        }
        return new Graphics(this);
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public boolean isMutable() {
        return mutable;
    }

    /** ink at (x,y): 1 = black. Out of range = 0. */
    public int get(int x, int y) {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return 0;
        }
        return raster[y * width + x];
    }

    public void set(int x, int y, int v) {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return;
        }
        raster[y * width + x] = (byte) v;
    }

    /** The raster, width*height bytes, row-major, 1 = black. */
    public byte[] getRaster() {
        return raster;
    }

    // ---- tiny PNG decoder: palette images, bit depth 1/2/4/8, non-interlaced ----

    static Image decodePng(byte[] data) throws IOException {
        if (data.length < 8 || (data[0] & 0xff) != 0x89 || data[1] != 'P' || data[2] != 'N' || data[3] != 'G') {
            throw new IOException("not a PNG");
        }
        int p = 8;
        int w = 0, h = 0, depth = 0, ctype = 0, interlace = 0;
        byte[] plte = null;
        ByteArrayOutputStream idat = new ByteArrayOutputStream();
        while (p + 8 <= data.length) {
            int len = be32(data, p);
            String type = new String(data, p + 4, 4, "ISO-8859-1");
            int off = p + 8;
            if (type.equals("IHDR")) {
                w = be32(data, off);
                h = be32(data, off + 4);
                depth = data[off + 8] & 0xff;
                ctype = data[off + 9] & 0xff;
                interlace = data[off + 12] & 0xff;
            } else if (type.equals("PLTE")) {
                plte = new byte[len];
                System.arraycopy(data, off, plte, 0, len);
            } else if (type.equals("IDAT")) {
                idat.write(data, off, len);
            } else if (type.equals("IEND")) {
                break;
            }
            p = off + len + 4;
        }
        if (interlace != 0) {
            throw new IOException("interlaced PNG unsupported");
        }
        if (ctype != 3 && ctype != 0) {
            throw new IOException("PNG color type " + ctype + " unsupported");
        }
        if (depth != 1 && depth != 2 && depth != 4 && depth != 8) {
            throw new IOException("PNG bit depth " + depth + " unsupported");
        }
        int stride = (w * depth + 7) / 8;
        byte[] raw = new byte[(stride + 1) * h];
        Inflater inf = new Inflater();
        inf.setInput(idat.toByteArray());
        try {
            int got = 0;
            while (got < raw.length && !inf.finished()) {
                int n = inf.inflate(raw, got, raw.length - got);
                if (n == 0 && (inf.needsInput() || inf.needsDictionary())) {
                    break;
                }
                got += n;
            }
            if (got < raw.length) {
                throw new IOException("short PNG data: " + got + " < " + raw.length);
            }
        } catch (DataFormatException e) {
            throw new IOException("zlib: " + e.getMessage());
        } finally {
            inf.end();
        }
        // unfilter (bpp = 1 byte for all sub-byte palette depths)
        byte[] cur = new byte[stride];
        byte[] prev = new byte[stride];
        Image img = new Image(w, h, false);
        for (int y = 0; y < h; y++) {
            int ft = raw[y * (stride + 1)] & 0xff;
            System.arraycopy(raw, y * (stride + 1) + 1, cur, 0, stride);
            for (int i = 0; i < stride; i++) {
                int a = i > 0 ? cur[i - 1] & 0xff : 0;
                int b = prev[i] & 0xff;
                int c = i > 0 ? prev[i - 1] & 0xff : 0;
                int x = cur[i] & 0xff;
                switch (ft) {
                    case 0:
                        break;
                    case 1:
                        x += a;
                        break;
                    case 2:
                        x += b;
                        break;
                    case 3:
                        x += (a + b) >> 1;
                        break;
                    case 4: {
                        int pp = a + b - c;
                        int pa = Math.abs(pp - a), pb = Math.abs(pp - b), pc = Math.abs(pp - c);
                        x += (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
                        break;
                    }
                    default:
                        throw new IOException("PNG filter " + ft);
                }
                cur[i] = (byte) x;
            }
            for (int x = 0; x < w; x++) {
                int bitpos = x * depth;
                int v = ((cur[bitpos >> 3] & 0xff) >> (8 - depth - (bitpos & 7))) & ((1 << depth) - 1);
                int lum;
                if (ctype == 3) {
                    if (plte == null || v * 3 + 2 >= plte.length) {
                        lum = v == 0 ? 0 : 255;
                    } else {
                        lum = ((plte[v * 3] & 0xff) * 299 + (plte[v * 3 + 1] & 0xff) * 587 + (plte[v * 3 + 2] & 0xff) * 114) / 1000;
                    }
                } else {
                    lum = v * 255 / ((1 << depth) - 1);
                }
                img.raster[y * w + x] = (byte) (lum < 128 ? 1 : 0);
            }
            byte[] t = prev;
            prev = cur;
            cur = t;
        }
        return img;
    }

    private static int be32(byte[] d, int o) {
        return ((d[o] & 0xff) << 24) | ((d[o + 1] & 0xff) << 16) | ((d[o + 2] & 0xff) << 8) | (d[o + 3] & 0xff);
    }
}
