package javax.microedition.lcdui;

import javax.microedition.midlet.MIDlet;

/** Single display: one current Displayable. */
public class Display {
    private static Display instance;
    private Displayable current;

    private Display() {
    }

    public static Display getDisplay(MIDlet m) {
        if (instance == null) {
            instance = new Display();
        }
        return instance;
    }

    public void setCurrent(Displayable d) {
        current = d;
    }

    public Displayable getCurrent() {
        return current;
    }

    /** Harness helper: is d the displayable currently shown (true when no Display exists yet). */
    static boolean isCurrent(Displayable d) {
        return instance == null || instance.current == d;
    }
}
