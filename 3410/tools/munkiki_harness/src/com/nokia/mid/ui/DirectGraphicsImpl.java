package com.nokia.mid.ui;

import javax.microedition.lcdui.Graphics;

/**
 * drawPixels for the two 1-bit formats:
 *
 * TYPE_BYTE_1_GRAY (1): row-major, 8 pixels per byte, MSB = leftmost,
 *   scanlength pixels per row (bytes per row = scanlength/8), bit index =
 *   offset + row*scanlength + col.
 *
 * TYPE_BYTE_1_GRAY_VERTICAL (-1): column-major within 8-row bands, each
 *   byte holds 8 vertically adjacent pixels, LSB = topmost; byte index =
 *   offset/8 + (row/8)*scanlength + col, bit = row%8. (Verified against the
 *   game's imageV3 character sprites.)
 *
 * In both: pixel bit 1 = black. Mask bit 1 = draw, 0 = transparent.
 * FLIP_HORIZONTAL (0x2000) mirrors the block left-right. The clip of the
 * Graphics is honored.
 */
class DirectGraphicsImpl implements DirectGraphics {
    private final Graphics g;

    DirectGraphicsImpl(Graphics g) {
        this.g = g;
    }

    public void drawPixels(byte[] pixels, byte[] mask, int offset, int scanlength,
                           int x, int y, int width, int height, int manipulation, int format) {
        if (format != TYPE_BYTE_1_GRAY && format != TYPE_BYTE_1_GRAY_VERTICAL) {
            throw new IllegalArgumentException("drawPixels format " + format);
        }
        if ((manipulation & ~(FLIP_HORIZONTAL | FLIP_VERTICAL)) != 0) {
            throw new IllegalArgumentException("drawPixels manipulation " + manipulation);
        }
        boolean flipH = (manipulation & FLIP_HORIZONTAL) != 0;
        boolean flipV = (manipulation & FLIP_VERTICAL) != 0;
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                int byteIdx, bit;
                if (format == TYPE_BYTE_1_GRAY) {
                    int bitIdx = offset + row * scanlength + col;
                    byteIdx = bitIdx >> 3;
                    bit = 7 - (bitIdx & 7);
                } else {
                    byteIdx = (offset >> 3) + (row >> 3) * scanlength + col;
                    bit = row & 7;
                }
                if (mask != null && ((mask[byteIdx] >> bit) & 1) == 0) {
                    continue;
                }
                int v = (pixels[byteIdx] >> bit) & 1;
                int dx = x + (flipH ? width - 1 - col : col);
                int dy = y + (flipV ? height - 1 - row : row);
                g.plotInk(dx, dy, v);
            }
        }
    }
}
