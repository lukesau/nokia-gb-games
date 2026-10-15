/* A fixed pattern drawn with the core's primitives. The platform smoke
   builds show it, and the host build writes it as the reference frame. */
#ifndef CORE_TESTCARD_H
#define CORE_TESTCARD_H

/* The one-pixel outline of the 84x48 screen, nothing inside. */
void testcard_frame(void);

/* The outline plus marks that show flips and scaling errors. */
void testcard_draw(void);

#endif
