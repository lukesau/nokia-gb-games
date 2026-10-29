package javax.microedition.lcdui;

/**
 * 96x65 canvas backed by a 1-bit Image. repaint() marks it dirty;
 * serviceRepaints() paints synchronously if dirty and the canvas is the
 * Display's current Displayable (as on a real device, a hidden canvas is
 * not painted). The Graphics state (clip, color, font) is reset before
 * every paint, as MIDP guarantees.
 */
public abstract class Canvas extends Displayable {
    public static final int UP = 1;
    public static final int DOWN = 6;
    public static final int LEFT = 2;
    public static final int RIGHT = 5;
    public static final int FIRE = 8;
    public static final int GAME_A = 9;
    public static final int GAME_B = 10;
    public static final int GAME_C = 11;
    public static final int GAME_D = 12;
    public static final int KEY_NUM0 = 48;
    public static final int KEY_NUM1 = 49;
    public static final int KEY_NUM2 = 50;
    public static final int KEY_NUM3 = 51;
    public static final int KEY_NUM4 = 52;
    public static final int KEY_NUM5 = 53;
    public static final int KEY_NUM6 = 54;
    public static final int KEY_NUM7 = 55;
    public static final int KEY_NUM8 = 56;
    public static final int KEY_NUM9 = 57;
    public static final int KEY_STAR = 42;
    public static final int KEY_POUND = 35;

    public static final int WIDTH = 96;
    public static final int HEIGHT = 65;

    private final Image buffer = Image.createImage(WIDTH, HEIGHT);
    private final Graphics graphics = new Graphics(buffer);
    private boolean dirty = false;
    /** number of paint() calls performed so far (harness statistic) */
    public int paintCount = 0;

    protected Canvas() {
    }

    protected abstract void paint(Graphics g);

    protected void keyPressed(int keyCode) {
    }

    protected void keyReleased(int keyCode) {
    }

    protected void keyRepeated(int keyCode) {
    }

    protected void pointerPressed(int x, int y) {
    }

    protected void pointerReleased(int x, int y) {
    }

    protected void pointerDragged(int x, int y) {
    }

    protected void showNotify() {
    }

    protected void hideNotify() {
    }

    public final void repaint() {
        dirty = true;
    }

    public final void repaint(int x, int y, int w, int h) {
        dirty = true;
    }

    public final void serviceRepaints() {
        if (!dirty) {
            return;
        }
        if (!Display.isCurrent(this)) {
            return;
        }
        dirty = false;
        graphics.reset();
        paintCount++;
        paint(graphics);
    }

    public int getGameAction(int keyCode) {
        switch (keyCode) {
            case -1:
                return UP;
            case -2:
                return DOWN;
            case -3:
                return LEFT;
            case -4:
                return RIGHT;
            case -5:
                return FIRE;
            default:
                return 0;
        }
    }

    public int getKeyCode(int gameAction) {
        switch (gameAction) {
            case UP:
                return -1;
            case DOWN:
                return -2;
            case LEFT:
                return -3;
            case RIGHT:
                return -4;
            case FIRE:
                return -5;
            default:
                return 0;
        }
    }

    public String getKeyName(int keyCode) {
        return String.valueOf(keyCode);
    }

    public boolean hasPointerEvents() {
        return false;
    }

    public boolean hasRepeatEvents() {
        return false;
    }

    public boolean isDoubleBuffered() {
        return true;
    }

    public int getWidth() {
        return WIDTH;
    }

    public int getHeight() {
        return HEIGHT;
    }

    // ---- harness access ----

    /** The canvas's own 96x65 Image (ink 1 = black). */
    public Image getBuffer() {
        return buffer;
    }

    /** The Graphics used for paint(); its drawString log lives here. */
    public Graphics getCanvasGraphics() {
        return graphics;
    }

    public boolean isDirty() {
        return dirty;
    }
}
