#include "gol.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif



#define MAX_FULLNESS_PERCENT 0.75
#define INIT_NODES_COUNT 16384

void pointset_init(PointSet * this) {
    this->nodes = (Node ** ) malloc(INIT_NODES_COUNT * sizeof(Node * ));
    this->number_nodes = INIT_NODES_COUNT;
    for (uint64_t i = 0; i < this->number_nodes; ++i) {
        this->nodes[i] = NULL;
    }
    this->used_nodes = 0;
}

void pointset_destroy(PointSet * this) {
    pointset_clear(this);
    free(this->nodes);
    this->number_nodes = 0;
    this->used_nodes = 0;
}

Point * pointset_toVector(PointSet * this, uint32_t * sizeOut) {
    Point * res = (Point*) malloc(this->used_nodes * sizeof(Point));
    uint32_t j = 0;
    for (uint64_t i = 0; i < this->number_nodes && j < this->used_nodes; ++i) {
        if (this->nodes[i] != NULL) {
            res[j] = this->nodes[i]->point;
            j++;
        }
    }
    *sizeOut = j;
    return res;
}

void pointset_clear(PointSet * this) {
    for (uint64_t i = 0; i < this->number_nodes; ++i) {
        if (this->nodes[i] != NULL) {
            __pointset_free_index(this, i);
        }
    }
    this->used_nodes = 0;
}

int pointset_add(PointSet * this, Point * p) {
    return __pointset_set_add(this, p);
}

int32_t pointset_contains(PointSet * this, Point * p) {
    uint64_t index;
    return __pointset_get_index(this, p, & index);
}

int32_t pointset_remove(PointSet * this, Point * p) {
    uint64_t index;
    int32_t res = __pointset_get_index(this, p, & index);
    if (!res) {
        return res;
    }

    __pointset_free_index(this, index);
  // re-layout nodes
    __pointset_relayout_nodes(this, index, 0);
    this->used_nodes--;
    return 1;
}

void __pointset_free_index(PointSet * this, uint64_t index) {
  free(this->nodes[index]);
  this->nodes[index] = NULL;
}

int __pointset_set_add(PointSet * this, Point * p) {
    uint64_t index;
    if (__pointset_set_contains(this, p)) {
        return 1;
    }

  // Expand nodes if we are close to our desired fullness
    if ((float) this->used_nodes / this->number_nodes > MAX_FULLNESS_PERCENT) {
        uint64_t num_els =this->number_nodes * 2; // we want to double each time
        Node ** tmp = (Node ** ) realloc(this->nodes, num_els * sizeof(Node * ));
        if (tmp == NULL || this->nodes == NULL) // malloc failure
        return 0;

        this->nodes = tmp;
        uint64_t i, orig_num_els = this->number_nodes;
        for (i = orig_num_els; i < num_els; ++i) {
            this->nodes[i] = NULL;
        }

    this->number_nodes = num_els;
    // re-layout all nodes
    __pointset_relayout_nodes(this, 0, 1);
  }

  // add element in
    int32_t res = __pointset_get_index(this, p, & index);
        if (!res) { // this is the first open slot
            __pointset_assign_node(this, p, index);
            this->used_nodes++;
            return 1;
        }
  return res;
}

int32_t __pointset_set_contains(PointSet * this,  Point * p) {
  uint64_t index;
  return __pointset_get_index(this, p, & index);
}

  int32_t __pointset_get_index(PointSet * this, Point * p, uint64_t * index) {
  uint64_t i, idx;
  idx = p->hash % this->number_nodes;
  i = idx;
  while (1) {
    if (this->nodes[i] == NULL) {
      * index = i;
      return 0; // not here OR first open slot
    } else if (p->hash == this->nodes[i]->point.hash && 
              this->nodes[i] -> point.x == p->x && 
              this->nodes[i] -> point.y == p->y) {
      * index = i;
      return 1;
    }
    ++i;
    if (i == this->number_nodes)
      i = 0;
    if (i == idx) // this means we went all the way around and the set is full
      return 0;
  }
}


void __pointset_assign_node(PointSet * this, Point * p, uint64_t index) {
  Node * node = (Node*) malloc(sizeof(Node));
  node_init(node, p);
  this->nodes[index] = node;
}

void __pointset_relayout_nodes(PointSet * this, uint64_t start, short end_on_null) {
  uint64_t index = 0, i;
  for (i = start; i < this->number_nodes; ++i) {
    if (this->nodes[i] != NULL) {
      __pointset_get_index(this, &(this->nodes[i] -> point), & index);
      if (i != index) { // we are moving this node
        __pointset_assign_node(this, &(this->nodes[i] -> point), index);
        __pointset_free_index(this, i);
      }
    } else if (end_on_null == 0 && i != start) {
      break;
    }
  }
}




void game_init(Game * this) {
  pointset_init(&(this->liveCells));

  point_init(&(this->NEIGHBORS[0]), -1, -1);
  point_init(&(this->NEIGHBORS[1]), -1, 0);
  point_init(&(this->NEIGHBORS[2]), -1, 1);
  point_init(&(this->NEIGHBORS[3]), 0, -1);
  point_init(&(this->NEIGHBORS[4]), 0, 1);
  point_init(&(this->NEIGHBORS[5]), 1, -1);
  point_init(&(this->NEIGHBORS[6]), 1, 0);
  point_init(&(this->NEIGHBORS[7]), 1, 1);
}


void game_tick(Game * this){
  uint32_t survivorsCount, newCellCount; 
  Point * _survivors = game_survivors(this, &survivorsCount);
  Point * _newCells = game_newCells(this, &newCellCount);

  pointset_clear(&(this->liveCells));

  for( uint32_t i = 0; i < survivorsCount; ++i) {
    pointset_add(&(this->liveCells), &_survivors[i]);
  }

  free(_survivors);

  for( uint32_t i = 0; i < newCellCount; ++i) {
	  pointset_add(&(this->liveCells), &_newCells[i]);
  }

  free(_newCells);
}


int game_countLiveNeighbors(Game * this, Point * p){
  uint32_t neighborsCount;
  Point * _neighbors = game_neighbors(this, p, &neighborsCount);
  int cnt = 0;
  for (uint32_t i = 0; i < neighborsCount; ++i) {
    if (pointset_contains(&(this->liveCells), &_neighbors[i] )) {
      ++cnt;
    }
  }
  free(_neighbors);
  return cnt;
}

int32_t game_hasTreeLiveNeighbors(Game * this, Point * p) {
  return game_countLiveNeighbors(this, p) == 3 ? 1 : 0;
}

int32_t game_hasTwoOrTreeLiveNeighbors(Game * this, Point * p) {
  int n = game_countLiveNeighbors(this, p);
  return (n == 2 || n == 3) ? 1 : 0;
}

Point * game_neighbors(Game * this, Point * p, uint32_t * sizeOut) {
  uint32_t nCount = sizeof(this->NEIGHBORS) / sizeof(this->NEIGHBORS[0]);
  Point * _neighbors = (Point*) malloc(sizeof(this->NEIGHBORS));
  uint32_t j = 0;
  for ( uint32_t i = 0; i < nCount; i++) {
    int x = p->x + this->NEIGHBORS[i].x;
    int y = p->y + this->NEIGHBORS[i].y;
    if (x >= 0 && y >= 0) {
      point_init(&_neighbors[j], x, y);
      ++j;
    }
  }

  *sizeOut = j;
  return _neighbors;
}


Point * game_newCells(Game * this, uint32_t * sizeOut) {
  uint32_t liveCellCount;
  Point * _liveCells = pointset_toVector(&(this->liveCells), &liveCellCount);
  Point * res = (Point*)malloc(2 * liveCellCount * sizeof(Point));
  uint32_t k = 0;

  for( uint32_t i = 0; i < liveCellCount; ++i) {
    uint32_t neighborsCount;
    Point * _neighbors = game_neighbors(this, &_liveCells[i], &neighborsCount);
    for(uint32_t j = 0; j < neighborsCount; ++j) {
      if (!pointset_contains(&(this->liveCells), &_neighbors[j])) {
        if (game_hasTreeLiveNeighbors(this, &_neighbors[j])) {
          res[k] = _neighbors[j];
		  k++;
        }
      }
    }
    free(_neighbors);
  }
  free(_liveCells);
  *sizeOut = k;
  return res;  
}


Point * game_survivors(Game * this, uint32_t * sizeOut) {
  uint32_t liveCellCount;
  Point * _liveCells = pointset_toVector(&(this->liveCells), &liveCellCount);
  Point * res = (Point*)malloc(liveCellCount * sizeof(Point));
  uint32_t k = 0;
  for(uint32_t j = 0; j < liveCellCount; ++j) { 
    if (game_hasTwoOrTreeLiveNeighbors(this, &_liveCells[j])) {
      res[k] = _liveCells[j];
      ++k;
    }
  }
  free(_liveCells);
  *sizeOut = k;
  return res;
}


void game_dump(Game * this) {
  uint32_t liveCellCount;
  Point * _liveCells = pointset_toVector(&(this->liveCells), &liveCellCount);
  

  printf("***********************************\n");
   for(uint32_t j = 0; j < liveCellCount; ++j) { 
    printf("%d %d\n", _liveCells[j].x, _liveCells[j].y);
   }
  free(_liveCells);
  printf("-----------------------------------\n");
 }

void game_dumpToBMP(Game * this, char * filePath) {
  uint32_t liveCellCount;
  Point * _liveCells = pointset_toVector(&(this->liveCells), &liveCellCount);
  int maxX = INT_MIN;
  int maxY = INT_MIN;
  for(uint32_t i = 0; i < liveCellCount; ++i) {
    maxX = max(maxX, _liveCells[i].x);
    maxY = max(maxY, _liveCells[i].y);
  }


  int height = maxY + 10; // 10 - border size
  int width = maxX + 10;

  BITMAPFILEMAGIC magic;
  magic.magic[0] = 'B';
  magic.magic[1] = 'M';

  FILE * fptr;
  if ((fptr = fopen(filePath, "wb+")) == NULL) {
    fprintf(stderr, "Unable to open BMP file \"%s\"\n", filePath);
    exit(-1);
  }

  fwrite((char * )( & magic), sizeof(magic), 1, fptr);

  BITMAPFILEHEADER2 header = {
    0
  };
  header.bmp_offset = sizeof(BITMAPFILEMAGIC) + sizeof(BITMAPFILEHEADER2) +
    sizeof(INFOHEADER) + 2 * sizeof(RGBQUAD);

  // TODO: vv These lines are lazy and bad. 
  int bytes_per_row = 0;
  for (int i = 0; i < width; i += 32)
    bytes_per_row += 4;
  header.file_size = header.bmp_offset + bytes_per_row * width;

  fwrite((char * )( & header), sizeof(header), 1, fptr);
  INFOHEADER dib_info = {
    0
  };
  dib_info.size = sizeof(INFOHEADER);
  dib_info.width = width;
  dib_info.height = height;
  dib_info.planes = 1;
  dib_info.bits = 1; // monochrome (то есть занимает 1 бит)
  dib_info.compression = 0;
  dib_info.imagesize = 0;
  dib_info.xresolution = 200;
  dib_info.yresolution = 200;
  dib_info.ncolours = 2;
  dib_info.importantcolours = 0;
  fwrite((char * )( & dib_info), sizeof(dib_info), 1, fptr);

  // Color palettes. 
  // First is the '0' color...
  RGBQUAD off_color;
  off_color.rgbRed = 0;
  off_color.rgbGreen = 255;
  off_color.rgbBlue = 255;
  off_color.rgbReserved = 0;
  fwrite((char * )( & off_color), sizeof(off_color), 1, fptr);
  // ...then the '1' color
  RGBQUAD on_color;
  on_color.rgbRed = 255;
  on_color.rgbGreen = 0;
  on_color.rgbBlue = 255;
  on_color.rgbReserved = 0;
  fwrite((char * )( & on_color), sizeof(on_color), 1, fptr);

  for (int row = height - 1; row >= 0; --row) {
    int bytes_written = 0;
    int bit = 7;
    char next_byte = '\0';

    for (int col = 0; col < width; col++) {
      Point p;
      point_init(&p, col, row);
      next_byte += pointset_contains(&(this->liveCells), &p) ? (1 << bit) : 0;

      if (bit > 0) {
        --bit;
      } else {
        fwrite(&next_byte, sizeof(next_byte), 1, fptr);
        ++bytes_written;
        bit = 7;
        next_byte = '\0';
      }
    }

    if (width % 8 != 0) {
      fwrite(&next_byte, sizeof(next_byte), 1, fptr);
      ++bytes_written;
    }

    // Rows are padded so that they're always a multiple of 4
    // bytes. This line skips the padding at the end of each row.
    for (int i = 0; i < 4 - bytes_written % 4; i++) {
      char zero = 0; 
      fwrite(&zero, sizeof(zero), 1, fptr);
    }
  }

  fclose(fptr);

}

void loadImage(const char * path, PointSet * points) {
  unsigned char pixels;
  HEADER header;
  INFOHEADER infoheader;
  FILE * fptr;

  if ((fptr = fopen(path, "r")) == NULL) {
    fprintf(stderr, "Unable to open BMP file \"%s\"\n", path);
    exit(-1);
  }

  fread(&header.type, 2, 1, fptr);
  printf("ID is: %d, should be %d\n", header.type, 'M' * 256 + 'B');
  fread(&header.size, 4, 1, fptr);
  printf("File size is %d bytes\n", header.size);
  fread(&header.reserved1, 2, 1, fptr);
  fread(&header.reserved2, 2, 1, fptr);
  fread(&header.offset, 4, 1, fptr);
  printf("Offset to image data is %d bytes\n", header.offset);

  /* Read and check the information header */
  if (fread( & infoheader, sizeof(INFOHEADER), 1, fptr) != 1) {
    fprintf(stderr, "Failed to read BMP info header\n");
    exit(-1);
  }
  printf("Image size = %d x %d\n", infoheader.width, infoheader.height);
  printf("Number of colour planes is %d\n", infoheader.planes);
  printf("Bits per pixel is %d\n", infoheader.bits);
  printf("Compression type is %d\n", infoheader.compression);
  printf("Number of colours is %d\n", infoheader.ncolours);
  printf("Number of required colours is %d\n",
    infoheader.importantcolours);

  fseek(fptr, header.offset, SEEK_SET);
  /* считываем картинку */
  for (int j = 0; j < infoheader.height; j++) {
    for (int i = 0; i < infoheader.width / 8; i++) {
      if (fread( & pixels, sizeof(unsigned char), 1, fptr) != 1) {
        fprintf(stderr, "Image read failed\n");
        exit(-1);
      }
      if (pixels > 0) {
        for (int k = 0; k < 8; ++k) {
          unsigned char mask = 1 << (7 - k);
          if ((pixels & mask) != 0) {
            Point p;
			point_init(&p, 8 * i + k, infoheader.height-j);
            pointset_add(points, &p);
          }
        }
      }
    } /* i */
  } /* j */
  fclose(fptr);
}



int main(int argc, char * argv[]) {
	char initFileName[255] = { 0 };
  char outputDir[255] = {0};
  int maxIter = 10;
  int dumpFreq = 0;

  for (int i = 1; i < 2 * (argc / 2); i += 2) {
    char * argName = argv[i];
    char * argVal = argv[i + 1];
    if (strcmp(argName, "--input") == 0) {
      strncpy(initFileName, argVal, sizeof(initFileName));
    } else if (strcmp(argName, "--output") == 0) {
      strncpy(outputDir, argVal, sizeof(outputDir));
    } else if (strcmp(argName, "--max_iter") == 0) {
      maxIter = atoi(argVal);
    } else if (strcmp(argName, "--dump_freq")) {
      dumpFreq = atoi(argVal);
    }
  }

  if (argc == 1 || strlen(initFileName) == 0 || strlen(outputDir) == 0 || dumpFreq < 0) {
    printf("Parameters:\n");
    printf("--input input_file.bmp\n");
    printf("--output dir_name\n");
    printf("--max_iter N\n");
    printf("--dump_freq N\n");
    return 0;
  }


  Game game;
  game_init(&game);

  loadImage(initFileName, &game.liveCells);

  for (int i = 0; i < maxIter; ++i) {
    if (dumpFreq == 0 || (dumpFreq != 0 && (i % dumpFreq) == 0)) {
      char outFileName[255];
      //snprintf(outFileName, sizeof(outFileName), "%s/image%d.bmp", outputDir, i);
	  sprintf(outFileName, "%s/image%d.bmp", outputDir, i);
      game_dumpToBMP(&game,outFileName);
	  /*game_dump(&game);*/
    }
    game_tick(&game);
  }
  return 0;
}

//"C:\Users\Kseniya Pashkoff\CLionProjects\gol (2)\gol.exe" --input game_initial.bmp --output ./images/ --max_iter 5 --dump_freq 1