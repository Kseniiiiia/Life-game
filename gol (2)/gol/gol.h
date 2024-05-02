#ifndef __gol_h__
#define __gol_h__

#include <inttypes.h>       /* uint64_t */

// Bitmap

typedef struct {
  unsigned short int type; /* Magic identifier            */
  unsigned int size; /* File size in bytes          */
  unsigned short int reserved1, reserved2;
  unsigned int offset; /* Offset to image data, bytes */
}
HEADER;

typedef struct {
  unsigned int size; /* Header size in bytes      */
  int width, height; /* Width and height of image */
  unsigned short int planes; /* Number of colour planes   */
  unsigned short int bits; /* Bits per pixel            */
  unsigned int compression; /* Compression type          */
  unsigned int imagesize; /* Image size in bytes       */
  int xresolution, yresolution; /* Pixels per meter          */
  unsigned int ncolours; /* Number of colours         */
  unsigned int importantcolours; /* Important colours         */
}
INFOHEADER;


# define BF_TYPE 0x4D42 /* "MB" */

typedef struct /**** Colormap entry structure ****/ {
  unsigned char rgbBlue; /* Blue value */
  unsigned char rgbGreen; /* Green value */
  unsigned char rgbRed; /* Red value */
  unsigned char rgbReserved; /* Reserved */
}
RGBQUAD;

///< Length in bytes of the file identifier.
#define BMP_MAGIC_ID  2 

/// Windows BMP-specific format data
typedef struct {
  unsigned char magic[BMP_MAGIC_ID]; ///< 'B' and 'M'
}
BITMAPFILEMAGIC;

/**
 * Generic 14-byte bitmap header
 */
typedef struct {
  uint32_t file_size; ///< The number of bytes in the bitmap file.
  uint16_t creator1; ///< Two bytes reserved.
  uint16_t creator2; ///< Two bytes reserved.
  uint32_t bmp_offset; ///< Offset from beginning to bitmap bits.
}
BITMAPFILEHEADER2;

typedef struct {
  int32_t x;
  int32_t y;
  uint64_t hash;
} Point; // структура точка, которая описывается кординатами Х и У, а также хэш-кодом для хранения в set


uint64_t point_hashCode(Point * p) {
    return (uint64_t) p->x + (INT32_MAX) * ((uint64_t) p->y);
} // считаем хэш-код

void point_init(Point * p, int32_t x, int32_t y) {
  p->x = x;
  p->y = y;
  p->hash = point_hashCode(p);
}  // типа конструктор

typedef struct {
  Point point;
} Node;  // это то что мы храним в set

void node_init(Node * node, Point * p) {
  node->point = *p;
}

typedef struct {
  Node ** nodes;
  uint64_t number_nodes;
  uint64_t used_nodes;
} PointSet; // структура описывающая множество точек


  
void pointset_init(PointSet * this);
void pointset_destroy(PointSet * this);

void pointset_clear(PointSet * this);

int pointset_add(PointSet * this, Point * p);
int32_t pointset_remove(PointSet * this, Point * p);
int32_t pointset_contains(PointSet * this, Point * p);
Point * pointset_toVector(PointSet * this, uint32_t * sizeOut);  // возвращает указатель на массив


void __pointset_free_index(PointSet * this, uint64_t index);
int __pointset_set_add(PointSet * this,  Point * p);
int32_t __pointset_set_contains(PointSet * this,  Point * p);
int32_t __pointset_get_index(PointSet * this, Point * p, uint64_t * index);
void __pointset_assign_node(PointSet * this, Point * p, uint64_t index);
void __pointset_relayout_nodes(PointSet * this, uint64_t start, short end_on_null);



typedef struct {
  PointSet liveCells;
  Point NEIGHBORS[8];    // относительные кординаты соседних точек
} Game;


void game_init(Game * this);
void game_destroy(Game * this);

void game_tick(Game * this);   // один шаг по времени

void game_dump(Game * this);
void game_dumpToBMP(Game * this, char * filePath);


int game_countLiveNeighbors(Game * this, Point * p);
int32_t game_hasTreeLiveNeighbors(Game * this, Point * p);
int32_t game_hasTwoOrTreeLiveNeighbors(Game * this, Point * p);
Point * game_neighbors(Game * this, Point * p, uint32_t * sizeOut);
Point * game_newCells(Game * this, uint32_t * sizeOut);   // возвращает клетки, которые родились
Point * game_survivors(Game * this, uint32_t * sizeOut);
Point * game_toDeadNeighbors(Game * this, Point * p);


#endif