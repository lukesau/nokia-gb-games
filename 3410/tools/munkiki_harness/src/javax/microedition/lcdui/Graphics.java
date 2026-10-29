package javax.microedition.lcdui;

import java.util.ArrayList;

/**
 * Graphics over a 1-bit Image raster (ink 1 = black). Strings are not
 * rendered; drawString calls are recorded in stringLog as "text|x|y".
 */
public class Graphics {
    public static final int HCENTER = 1;
    public static final int VCENTER = 2;
    public static final int LEFT = 4;
    public static final int RIGHT = 8;
    public static final int TOP = 16;
    public static final int BOTTOM = 32;
    public static final int BASELINE = 64;
    public static final int SOLID = 0;
    public static final int DOTTED = 1;

    final Image image;
    int clipX, clipY, clipW, clipH;
    /** 1 = black, 0 = white */
    int ink = 1;
    int rgb = 0;
    Font font = Font.getDefaultFont();
    public final ArrayList<String> stringLog = new ArrayList<String>();

    Graphics(Image image) {
        this.image = image;
        reset();
    }

    /** MIDP state at the start of a paint: full clip, black, default font. */
    public void reset() {
        clipX = 0;
        clipY = 0;
        clipW = image.getWidth();
        clipH = image.getHeight();
        ink = 1;
        rgb = 0;
        font = Font.getDefaultFont();
    }

    public Image getImage() {
        return image;
    }

    public void setClip(int x, int y, int w, int h) {
        clipX = x;
        clipY = y;
        clipW = w;
        clipH = h;
    }

    public void clipRect(int x, int y, int w, int h) {
        int x0 = Math.max(clipX, x);
        int y0 = Math.max(clipY, y);
        int x1 = Math.min(clipX + clipW, x + w);
        int y1 = Math.min(clipY + clipH, y + h);
        clipX = x0;
        clipY = y0;
        clipW = Math.max(0, x1 - x0);
        clipH = Math.max(0, y1 - y0);
    }

    public int getClipX() {
        return clipX;
    }

    public int getClipY() {
        return clipY;
    }

    public int getClipWidth() {
        return clipW;
    }

    public int getClipHeight() {
        return clipH;
    }

    public void setColor(int rgb) {
        this.rgb = rgb;
        int r = (rgb >> 16) & 0xff, g = (rgb >> 8) & 0xff, b = rgb & 0xff;
        int lum = (r * 299 + g * 587 + b * 114) / 1000;
        ink = lum < 128 ? 1 : 0;
    }

    public void setColor(int r, int g, int b) {
        setColor((r << 16) | (g << 8) | b);
    }

    public void setGrayScale(int v) {
        setColor(v, v, v);
    }

    public int getColor() {
        return rgb;
    }

    public void setFont(Font f) {
        font = f == null ? Font.getDefaultFont() : f;
    }

    public Font getFont() {
        return font;
    }

    public void setStrokeStyle(int s) {
    }

    public int getStrokeStyle() {
        return SOLID;
    }

    public void translate(int x, int y) {
        throw new UnsupportedOperationException("translate");
    }

    public int getTranslateX() {
        return 0;
    }

    public int getTranslateY() {
        return 0;
    }

    // ---- primitives ----

    /** Plot with the current ink, honoring the clip and the raster bounds. */
    public void plot(int x, int y) {
        plotInk(x, y, ink);
    }

    public void plotInk(int x, int y, int v) {
        if (x < clipX || y < clipY || x >= clipX + clipW || y >= clipY + clipH) {
            return;
        }
        image.set(x, y, v);
    }

    public void fillRect(int x, int y, int w, int h) {
        if (w <= 0 || h <= 0) {
            return;
        }
        int x0 = Math.max(x, Math.max(clipX, 0));
        int y0 = Math.max(y, Math.max(clipY, 0));
        int x1 = Math.min(x + w, Math.min(clipX + clipW, image.getWidth()));
        int y1 = Math.min(y + h, Math.min(clipY + clipH, image.getHeight()));
        for (int yy = y0; yy < y1; yy++) {
            for (int xx = x0; xx < x1; xx++) {
                image.set(xx, yy, ink);
            }
        }
    }

    /** MIDP: outline covers x..x+w, y..y+h inclusive. */
    public void drawRect(int x, int y, int w, int h) {
        if (w < 0 || h < 0) {
            return;
        }
        drawLine(x, y, x + w, y);
        drawLine(x, y + h, x + w, y + h);
        drawLine(x, y, x, y + h);
        drawLine(x + w, y, x + w, y + h);
    }

    public void drawLine(int x0, int y0, int x1, int y1) {
        int dx = Math.abs(x1 - x0), dy = Math.abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;
        int x = x0, y = y0;
        while (true) {
            plot(x, y);
            if (x == x1 && y == y1) {
                break;
            }
            int e2 = err << 1;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }

    /**
     * Approximate arc: the ellipse inscribed in the (w+1)x(h+1) box, angles
     * in degrees, 0 = 3 o'clock, counter-clockwise positive.
     */
    public void drawArc(int x, int y, int w, int h, int startAngle, int arcAngle) {
        if (w < 0 || h < 0) {
            return;
        }
        double cx = x + w / 2.0, cy = y + h / 2.0;
        double rx = w / 2.0, ry = h / 2.0;
        int steps = Math.max(8, (int) (Math.abs(arcAngle) * Math.max(rx, ry) / 10) + 8);
        for (int i = 0; i <= steps; i++) {
            double a = Math.toRadians(startAngle + arcAngle * (double) i / steps);
            int px = (int) Math.round(cx + rx * Math.cos(a));
            int py = (int) Math.round(cy - ry * Math.sin(a));
            plot(px, py);
        }
    }

    public void fillArc(int x, int y, int w, int h, int startAngle, int arcAngle) {
        drawArc(x, y, w, h, startAngle, arcAngle);
    }

    public void drawImage(Image img, int x, int y, int anchor) {
        if (img == null) {
            throw new NullPointerException("drawImage null image");
        }
        int w = img.getWidth(), h = img.getHeight();
        if ((anchor & HCENTER) != 0) {
            x -= w / 2;
        } else if ((anchor & RIGHT) != 0) {
            x -= w;
        }
        if ((anchor & VCENTER) != 0) {
            y -= h / 2;
        } else if ((anchor & BOTTOM) != 0) {
            y -= h;
        }
        for (int yy = 0; yy < h; yy++) {
            int dy = y + yy;
            if (dy < clipY || dy >= clipY + clipH) {
                continue;
            }
            for (int xx = 0; xx < w; xx++) {
                int dx = x + xx;
                if (dx < clipX || dx >= clipX + clipW) {
                    continue;
                }
                image.set(dx, dy, img.get(xx, yy));
            }
        }
    }

    /** Every drawString on any Graphics, as "imageSerial:text|x|y" (harness drains it). */
    public static final ArrayList<String> allStringLog = new ArrayList<String>();

    public void drawString(String s, int x, int y, int anchor) {
        stringLog.add(s + "|" + x + "|" + y);
        allStringLog.add(image.serial + ":" + s + "|" + x + "|" + y);
    }

    public void drawChar(char c, int x, int y, int anchor) {
        drawString(String.valueOf(c), x, y, anchor);
    }

    public void drawSubstring(String s, int off, int len, int x, int y, int anchor) {
        drawString(s.substring(off, off + len), x, y, anchor);
    }
}
