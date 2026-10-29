package com.nokia.mid.m3d;

import javax.microedition.lcdui.Image;

public class Texture {
    private static int counter = 0;

    public final int target;
    public final int format;
    public final Image image;
    /** order of creation, 0.. */
    public final int index;

    public Texture(int target, int format, Image image) {
        this.target = target;
        this.format = format;
        this.image = image;
        this.index = counter++;
    }

    public String toString() {
        return index + ":" + (image == null ? "null" : image.getWidth() + "x" + image.getHeight());
    }
}
