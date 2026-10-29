package javax.microedition.lcdui;

/**
 * Simple monospace font model:
 * SIZE_SMALL (8): 5 px per char, height 9;
 * SIZE_MEDIUM (0): 6 px, height 11;
 * SIZE_LARGE (16): 8 px, height 13.
 */
public final class Font {
    public static final int STYLE_PLAIN = 0;
    public static final int STYLE_BOLD = 1;
    public static final int STYLE_ITALIC = 2;
    public static final int STYLE_UNDERLINED = 4;
    public static final int SIZE_SMALL = 8;
    public static final int SIZE_MEDIUM = 0;
    public static final int SIZE_LARGE = 16;
    public static final int FACE_SYSTEM = 0;
    public static final int FACE_MONOSPACE = 32;
    public static final int FACE_PROPORTIONAL = 64;

    private final int face;
    private final int style;
    private final int size;

    private static final Font SMALL = new Font(0, 0, SIZE_SMALL);
    private static final Font MEDIUM = new Font(0, 0, SIZE_MEDIUM);
    private static final Font LARGE = new Font(0, 0, SIZE_LARGE);

    /* The phone's fonts' metrics (the port's, from the firmware), read
       from fontmetrics.txt beside the classes: "size height w32 .. w126". */
    private static int[][] widths = new int[3][];
    private static int[] heights = new int[3];
    static {
        try {
            java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader(
                System.getProperty("fontmetrics", "fontmetrics.txt")));
            String line;
            while ((line = r.readLine()) != null) {
                String[] f = line.trim().split("\\s+");
                if (f.length < 97) continue;
                int sz = Integer.parseInt(f[0]);
                int slot = sz == SIZE_SMALL ? 0 : sz == SIZE_LARGE ? 2 : 1;
                heights[slot] = Integer.parseInt(f[1]);
                widths[slot] = new int[95];
                for (int i = 0; i < 95; i++) widths[slot][i] = Integer.parseInt(f[2 + i]);
            }
            r.close();
        } catch (Exception e) {
            System.out.println("fontmetrics problem : " + e);
        }
    }

    private int slot() {
        return size == SIZE_SMALL ? 0 : size == SIZE_LARGE ? 2 : 1;
    }

    private Font(int face, int style, int size) {
        this.face = face;
        this.style = style;
        this.size = size;
    }

    public static Font getFont(int face, int style, int size) {
        if (size == SIZE_SMALL) {
            return SMALL;
        }
        if (size == SIZE_LARGE) {
            return LARGE;
        }
        return MEDIUM;
    }

    public static Font getDefaultFont() {
        return MEDIUM;
    }

    public int getFace() {
        return face;
    }

    public int getStyle() {
        return style;
    }

    public int getSize() {
        return size;
    }

    public int charWidth(char c) {
        int[] w = widths[slot()];
        if (w == null) return 5;
        int i = c - 32;
        if (i < 0 || i >= 95) i = '?' - 32;
        return w[i];
    }

    public int charsWidth(char[] ch, int off, int len) {
        int w = 0;
        for (int i = 0; i < len; i++) w += charWidth(ch[off + i]);
        return w;
    }

    public int stringWidth(String s) {
        int w = 0;
        for (int i = 0; i < s.length(); i++) w += charWidth(s.charAt(i));
        return w;
    }

    public int substringWidth(String s, int off, int len) {
        return stringWidth(s.substring(off, off + len));
    }

    public int getHeight() {
        int h = heights[slot()];
        return h != 0 ? h : 11;
    }

    public int getBaselinePosition() {
        return getHeight() - 2;
    }

    public String toString() {
        return "Font(size=" + size + ")";
    }
}
