package com.nokia.mid.ui;

import javax.microedition.lcdui.Graphics;

public class DirectUtils {
    private DirectUtils() {
    }

    public static DirectGraphics getDirectGraphics(Graphics g) {
        return new DirectGraphicsImpl(g);
    }
}
