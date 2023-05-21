#include <ncursesw/ncurses.h>
#include <stdint.h>
#include <assert.h>

#define PI (3.141592653f)

#define MAX(a, b) (((a)>(b))? (a) : (b))
#define MIN(a, b) (((a)<(b))? (a) : (b))

#define WH (LINES)
#define WW (COLS)

#define BRAILLE_W (2)
#define BRAILLE_H (4)
#define BRAILLE_VAL (0x2800)
