package com.nokia.mid.sound;

import java.util.ArrayList;

/** Sounds are numbered in order of construction; play() logs the index. */
public class Sound {
    public static final int FORMAT_TONE = 1;
    public static final int FORMAT_WAV = 5;
    public static final int SOUND_PLAYING = 0;
    public static final int SOUND_STOPPED = 1;
    public static final int SOUND_UNINITIALIZED = 3;

    private static int counter = 0;
    public static final ArrayList<Integer> played = new ArrayList<Integer>();

    public final int index;
    private byte[] data;
    private int type;
    private int gain = 255;

    public Sound(byte[] data, int type) {
        this.data = data;
        this.type = type;
        this.index = counter++;
    }

    public Sound(int freq, long duration) {
        this.index = counter++;
    }

    public void init(byte[] data, int type) {
        this.data = data;
        this.type = type;
    }

    public void init(int freq, long duration) {
    }

    public void play(int loop) {
        played.add(index);
    }

    public void stop() {
    }

    public void release() {
    }

    public void resume() {
    }

    public void setGain(int gain) {
        this.gain = gain;
    }

    public int getGain() {
        return gain;
    }

    public int getState() {
        return SOUND_STOPPED;
    }

    public byte[] getData() {
        return data;
    }

    public int getType() {
        return type;
    }

    /** Harness: drain the list of played sound indices. */
    public static int[] drainPlayed() {
        int[] r = new int[played.size()];
        for (int i = 0; i < r.length; i++) {
            r[i] = played.get(i);
        }
        played.clear();
        return r;
    }
}
