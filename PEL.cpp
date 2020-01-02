/******************************************************************************
 * PEL: Predictive Edge Linking
 * 
 * Copyright 2015 Cuneyt Akinlar (cakinlar@anadolu.edu.tr)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "EdgeMap.h"
#include "PEL.h"

// Helper function prototypes
static void FillGaps1(unsigned char *edgeImg, int width, int height);
static void FillGaps2(unsigned char *edgeImg, int width, int height);

EdgeMap *PELWalk8Dirs(unsigned char *edgeImg, int width, int height, int MIN_SEGMENT_LEN);
static void JoinNeighborEdgeSegments(EdgeMap *map);
static void ThinEdgeSegments(EdgeMap *map, int MIN_SEGMENT_LEN);
static void FixEdgeSegments(EdgeMap *map);

///-------------------------------------------------------------------------------
/// Predictive Edge Linking (PEL)
///
EdgeMap *PEL(unsigned char *edgeImg, int width, int height, int MIN_SEGMENT_LEN){
  // Close gaps of 1 pixel wide
//  FillGaps1(edgeImg, width, height);
  FillGaps2(edgeImg, width, height);

  // Convert the filled-up edge map to edge segments using 8 directional predictive edge linking
  EdgeMap *map = PELWalk8Dirs(edgeImg, width, height, 7); 

  // Extend the edge segments
  JoinNeighborEdgeSegments(map);

  // Thin down edge segments
  ThinEdgeSegments(map, MIN_SEGMENT_LEN);

  // Fix jitters of 1 pixel within an edge segment
  FixEdgeSegments(map);

  return map;
} //end-PEL

///======================================= STEP 1: FillGaps ======================================
///------------------------------------------------------------------------
/// Close gaps of 1 pixel wide between the end points of an edge map
///
static void FillGaps1(unsigned char *edgeImg, int width, int height){
  for (int i=1; i<height-1; i++){
    for (int j=1; j<width-1; j++){
      if (edgeImg[i*width+j] == 0) continue;

      int count = 0;
      if (edgeImg[(i-1)*width+j]) count++;
      if (edgeImg[(i+1)*width+j]) count++;
      if (edgeImg[i*width+j-1]) count++;
      if (edgeImg[i*width+j+1]) count++;

      if (edgeImg[(i-1)*width+j-1] && edgeImg[(i-1)*width+j] == 0 && edgeImg[i*width+j-1] == 0) count++;
      if (edgeImg[(i-1)*width+j+1] && edgeImg[(i-1)*width+j] == 0 && edgeImg[i*width+j+1] == 0) count++;
      if (edgeImg[(i+1)*width+j+1] && edgeImg[(i+1)*width+j] == 0 && edgeImg[i*width+j+1] == 0) count++;
      if (edgeImg[(i+1)*width+j-1] && edgeImg[(i+1)*width+j] == 0 && edgeImg[i*width+j-1] == 0) count++;

      if (count <= 1) edgeImg[i*width+j] = 128;           // Tip of an edge group pixel
    } //end-for
  } //end-for

  // Now use the endpoint information to join endpoints that are one pixel apart from each other
  for (int i=1; i<height-1; i++){
    for (int j=1; j<width-1; j++){
      if (edgeImg[i*width+j] != 128) continue;

      // Search for endpoints that are one pixel apart and connect them
      int r, c;

      r = i-1; c = j+2;
      if (r >= 0 && c < width && edgeImg[r*width+c] == 128){
        int index = (i-1)*width+j+1;

        edgeImg[index] = 255;
      } //end-if

      r = i; c = j+2;
      if (c < width && edgeImg[r*width+c] == 128){
        int index = i*width+j+1;

        edgeImg[index] = 255;
      } //end-if

      r = i+1; c = j+2;
      if (r <height && c < width && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j+1;

        edgeImg[index] = 255;
      } //end-if

#if 0
      // Right Diagonal
      r = i+2; c = j+2;
      if (r <height && c < width && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j+1;

        edgeImg[index] = 255;
      } //end-if
#endif

      r = i+2; c = j+1;
      if (r <height && c < width && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j+1;

        edgeImg[index] = 255;
      } //end-if

      r = i+2; c = j;
      if (r <height && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j;

        edgeImg[index] = 255;
      } //end-if

      r = i+2; c = j-1;
      if (c >= 0 && r <height && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j-1;

        edgeImg[index] = 255;
      } //end-if

#if 0
      // Left Diagonal
      r = i+2; c = j-2;
      if (c >= 0 && r <height && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j-1;

        edgeImg[index] = 255;
      } //end-if
#endif

      r = i+1; c = j-2;
      if (c >= 0 && r <height && edgeImg[r*width+c] == 128){
        int index = (i+1)*width+j-1;

        edgeImg[index] = 255;
      } //end-if

      edgeImg[i*width+j] = 255;
    } //end-for
  } //end-for
} //end-FillGaps1

///---------------------------------------------------------------------------------
/// Close gaps of 1 pixel wide: This joins the tip of an edge group to ANY neighbouring edgel
///
static void FillGaps2(unsigned char *edgeImg, int width, int height){
  for (int i=2; i<height-2; i++){
    for (int j=2; j<width-2; j++){
      if (edgeImg[i*width+j] != 255) continue;

      int count = 0;
      int loc = 1;
      if (edgeImg[(i-1)*width+j] == 255) count++;
      if (edgeImg[(i+1)*width+j] == 255){count++; loc = 2;}
      if (edgeImg[i*width+j-1] == 255){count++; loc = 3;}
      if (edgeImg[i*width+j+1] == 255){count++; loc = 4;}

      if (edgeImg[(i-1)*width+j-1] == 255 && edgeImg[(i-1)*width+j] != 255 && edgeImg[i*width+j-1] != 255){count++; loc = 5;}
      if (edgeImg[(i-1)*width+j+1] == 255 && edgeImg[(i-1)*width+j] != 255 && edgeImg[i*width+j+1] != 255){count++; loc = 6;}
      if (edgeImg[(i+1)*width+j+1] == 255 && edgeImg[(i+1)*width+j] != 255 && edgeImg[i*width+j+1] != 255){count++; loc = 7;}
      if (edgeImg[(i+1)*width+j-1] == 255 && edgeImg[(i+1)*width+j] != 255 && edgeImg[i*width+j-1] != 255){count++; loc = 8;}

      if (count == 0 || count > 1) continue;
  
      // Pixel at the tip of an edge group
      if (loc == 1){
        // Going Down
        // P
        // x
        if (edgeImg[(i+2)*width+j] == 255){edgeImg[(i+1)*width+j] = 128; continue;} // Down

        if (edgeImg[(i+2)*width+j+1] == 255 || edgeImg[(i+2)*width+j+2] == 255 || edgeImg[(i+1)*width+j+2] == 255){edgeImg[(i+1)*width+j+1] = 128; continue;} // Down-Right
        if (edgeImg[(i+2)*width+j-1] == 255 || edgeImg[(i+2)*width+j-2] == 255 || edgeImg[(i+1)*width+j-2] == 255){edgeImg[(i+1)*width+j-1] = 128; continue;} // Down-Left

      } else if (loc == 2){
        // Going Up
        // x
        // P
        if (edgeImg[(i-2)*width+j] == 255){edgeImg[(i-1)*width+j] = 128; continue;} // Up

        if (edgeImg[(i-2)*width+j+1] == 255 || edgeImg[(i-2)*width+j+2] == 255 || edgeImg[(i-1)*width+j+2] == 255){edgeImg[(i-1)*width+j+1] = 128; continue;} // Up-Right
        if (edgeImg[(i-2)*width+j-1] == 255 || edgeImg[(i-2)*width+j-2] == 255 || edgeImg[(i-1)*width+j-2] == 255){edgeImg[(i-1)*width+j-1] = 128; continue;} // Up-Left

      } else if (loc == 3){
        // Going Right
        // Px
        if (edgeImg[i*width+j+2] == 255){edgeImg[i*width+j+1] = 128; continue;} // Right

        if (edgeImg[(i-2)*width+j+1] == 255 || edgeImg[(i-2)*width+j+2] == 255 || edgeImg[(i-1)*width+j+2] == 255){edgeImg[(i-1)*width+j+1] = 128; continue;} // Up-Right
        if (edgeImg[(i+2)*width+j+1] == 255 || edgeImg[(i+2)*width+j+2] == 255 || edgeImg[(i+1)*width+j+2] == 255){edgeImg[(i+1)*width+j+1] = 128; continue;} // Down-Right

      } else if (loc == 4){
        // Going Left
        // xP
        if (edgeImg[i*width+j-2] == 255){edgeImg[i*width+j-1] = 128; continue;} // Left

        if (edgeImg[(i-2)*width+j-1] == 255 || edgeImg[(i-2)*width+j-2] == 255 || edgeImg[(i-1)*width+j-2] == 255){edgeImg[(i-1)*width+j-1] = 128; continue;} // Up-Left
        if (edgeImg[(i+2)*width+j-1] == 255 || edgeImg[(i+2)*width+j-2] == 255 || edgeImg[(i+1)*width+j-2] == 255){edgeImg[(i+1)*width+j-1] = 128; continue;} // Down-Left

      } else if (loc == 5){
        // Going Down-Right
        // P
        //  x
        if (edgeImg[(i+2)*width+j+1] == 255 || edgeImg[(i+2)*width+j+2] == 255 || edgeImg[(i+1)*width+j+2] == 255){edgeImg[(i+1)*width+j+1] = 128; continue;} // Down-Right

        if (edgeImg[i*width+j+2] == 255){edgeImg[i*width+j+1] = 128; continue;} // Down
        if (edgeImg[i*width+j+2] == 255){edgeImg[i*width+j+1] = 128; continue;} // Right

        if (edgeImg[(i+2)*width+j-1] == 255 || edgeImg[(i+2)*width+j-2] == 255 || edgeImg[(i+1)*width+j-2] == 255){edgeImg[(i+1)*width+j-1] = 128; continue;} // Down-Left
        if (edgeImg[(i-2)*width+j+1] == 255 || edgeImg[(i-2)*width+j+2] == 255 || edgeImg[(i-1)*width+j+2] == 255){edgeImg[(i-1)*width+j+1] = 128; continue;} // Up-Right

      } else if (loc == 6){
        // Going Down-Left
        //  P
        // x
        if (edgeImg[(i+2)*width+j-1] == 255 || edgeImg[(i+2)*width+j-2] == 255 || edgeImg[(i+1)*width+j-2] == 255){edgeImg[(i+1)*width+j-1] = 128; continue;} // Down-Left

        if (edgeImg[i*width+j+2] == 255){edgeImg[i*width+j+1] = 128; continue;} // Down
        if (edgeImg[i*width+j-2] == 255){edgeImg[i*width+j-1] = 128; continue;} // Left

        if (edgeImg[(i+2)*width+j+1] == 255 || edgeImg[(i+2)*width+j+2] == 255 || edgeImg[(i+1)*width+j+2] == 255){edgeImg[(i+1)*width+j+1] = 128; continue;} // Down-Right
        if (edgeImg[(i-2)*width+j-1] == 255 || edgeImg[(i-2)*width+j-2] == 255 || edgeImg[(i-1)*width+j-2] == 255){edgeImg[(i-1)*width+j-1] = 128; continue;} // Up-Left

      } else if (loc == 7){
        // Going Up-Left
        // x
        //  P
        if (edgeImg[(i-2)*width+j-1] == 255 || edgeImg[(i-2)*width+j-2] == 255 || edgeImg[(i-1)*width+j-2] == 255){edgeImg[(i-1)*width+j-1] = 128; continue;} // Up-Left

        if (edgeImg[(i-2)*width+j] == 255){edgeImg[(i-1)*width+j] = 128; continue;} // Up
        if (edgeImg[i*width+j-2] == 255){edgeImg[i*width+j-1] = 128; continue;} // Left

        if (edgeImg[(i-2)*width+j+1] == 255 || edgeImg[(i-2)*width+j+2] == 255 || edgeImg[(i-1)*width+j+2] == 255){edgeImg[(i-1)*width+j+1] = 128; continue;} // Up-Right
        if (edgeImg[(i+2)*width+j-1] == 255 || edgeImg[(i+2)*width+j-2] == 255 || edgeImg[(i+1)*width+j-2] == 255){edgeImg[(i+1)*width+j-1] = 128; continue;} // Down-Left

      } else { //if (loc == 8){
        // Going Up-Right
        //  x
        // P
        if (edgeImg[(i-2)*width+j+1] == 255 || edgeImg[(i-2)*width+j+2] == 255 || edgeImg[(i-1)*width+j+2] == 255){edgeImg[(i-1)*width+j+1] = 128; continue;} // Up-Right

        if (edgeImg[(i-2)*width+j] == 255){edgeImg[(i-1)*width+j] = 128; continue;} // Up
        if (edgeImg[i*width+j+2] == 255){edgeImg[i*width+j+1] = 128; continue;} // Right

        if (edgeImg[(i-2)*width+j-1] == 255 || edgeImg[(i-2)*width+j-2] == 255 || edgeImg[(i-1)*width+j-2] == 255){edgeImg[(i-1)*width+j-1] = 128; continue;} // Up-Left
        if (edgeImg[(i+2)*width+j+1] == 255 || edgeImg[(i+2)*width+j+2] == 255 || edgeImg[(i+1)*width+j+2] == 255){edgeImg[(i+1)*width+j+1] = 128; continue;} // Down-Right
      } //end-else 
    } //end-for
  } //end-for

  for (int i=0; i<width*height; i++) if (edgeImg[i] == 128) edgeImg[i] = 255;
} //end-FillGaps2


///======================================= Step 2: EdgeSegment Creation by 8 Directional Walk ======================================
#define UP_LEFT    1   // diagonal
#define UP         2   
#define UP_RIGHT   3
#define RIGHT      4
#define DOWN_RIGHT 5
#define DOWN       6
#define DOWN_LEFT  7
#define LEFT       8

///-----------------------------------------------------------------------------------------------
/// Next direction prediction engine (Use the last 8 directions to make a prediction for the next)
///
struct Queue {
#define QSIZE 8
  int Q[QSIZE];
  int noItems;
  int rear;

  Queue(){noItems = 0; rear = 0;}

   void Add(int dir){
    Q[rear++] = dir;
    if (rear >= QSIZE) rear = 0;
    if (noItems < QSIZE) noItems++;
  } //end-add

  int ComputeNextDir(int analysisDir){
    int C[2] = {0, 0};

    if (analysisDir == LEFT || analysisDir == RIGHT){
      // LEFT or RIGHT
      for (int i=0; i<noItems; i++){
        if (Q[i] == UP || Q[i] == DOWN) continue;

        if (Q[i] == LEFT || Q[i] == DOWN_LEFT || Q[i] == UP_LEFT) C[0]++;
        else                                                      C[1]++;
     } //end-for

      if (C[0] >= C[1]) return LEFT;
      else              return RIGHT;

    } else if (analysisDir == UP || analysisDir == DOWN){
      // UP or DOWN
      for (int i=0; i<noItems; i++){
        if (Q[i] == LEFT || Q[i] == RIGHT) continue;

        if (Q[i] == UP || Q[i] == UP_LEFT || Q[i] == UP_RIGHT) C[0]++;
        else                                                   C[1]++;
      } //end-for

      if (C[0] >= C[1]) return UP;
      else              return DOWN;

    } else if (analysisDir == UP_LEFT){
      // UP or LEFT
      for (int i=0; i<noItems; i++){
        if      (Q[i] == UP)   C[0]++;
        else if (Q[i] == LEFT) C[1]++;
      } //end-for

      if (C[0] >= C[1]) return UP;
      else              return LEFT;

    } else if (analysisDir == UP_RIGHT){
      // UP or RIGHT
      for (int i=0; i<noItems; i++){
        if      (Q[i] == UP)    C[0]++;
        else if (Q[i] == RIGHT) C[1]++;
      } //end-for

      if (C[0] >= C[1]) return UP;
      else              return RIGHT;

    } else if (analysisDir == DOWN_RIGHT){
      // DOWN or RIGHT
      for (int i=0; i<noItems; i++){
        if      (Q[i] == DOWN)  C[0]++;
        else if (Q[i] == RIGHT) C[1]++;
      } //end-for

      if (C[0] >= C[1]) return DOWN;
      else              return RIGHT;

    } else { //if (analysisDir == DOWN_LEFT){
      // DOWN or LEFT
      for (int i=0; i<noItems; i++){
        if      (Q[i] == DOWN)  C[0]++;
        else if (Q[i] == LEFT) C[1]++;
      } //end-for

      if (C[0] >= C[1]) return DOWN;
      else              return LEFT;
    } // end-else
  } //end-ComputeNextDir
};

///----------------------------------------------------------------------------------------------------
/// 8 Directional Walk with Prediction
///
static int Walk8Dirs(unsigned char *edgeImg, int width, int height, int r, int c, int dir, Pixel *pixels){
  Queue Q;

  int count = 0;

  while (1){
    edgeImg[r*width+c] = 0;

    if (r<=0 || r>=height-1) return count;
    if (c<=0 || c>=width-1) return count;

    pixels[count].r = r;
    pixels[count].c = c;
    count++;

    // Add the current direction to the Q
    Q.Add(dir);

    if        (dir == UP_LEFT){
      // Should we check UP or LEFT first?
      int nextDir = Q.ComputeNextDir(UP_LEFT);

      // Up-Left?
      if (edgeImg[(r-1)*width+c-1]){
        if (nextDir == UP){
          // Up?
          if (edgeImg[(r-1)*width+c]){
            pixels[count].r = r-1; pixels[count].c = c; count++;
            edgeImg[(r-1)*width+c] = 0;

          // Left?
          } else if (edgeImg[r*width+c-1]){
            pixels[count].r = r; pixels[count].c = c-1; count++;
            edgeImg[r*width+c-1] = 0;
          } //end-else

        } else {
          // Left?
          if (edgeImg[r*width+c-1]){
            pixels[count].r = r; pixels[count].c = c-1; count++;
            edgeImg[r*width+c-1] = 0;

          // Up?
          } else if (edgeImg[(r-1)*width+c]){
            pixels[count].r = r-1; pixels[count].c = c; count++;
            edgeImg[(r-1)*width+c] = 0;
          } //end-else
        } //end-else

        r--; c--; dir = UP_LEFT; continue;
      } // end-if

      if (nextDir == UP){
        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

      } else {
        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == UP){
      // Up
      if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

      // Should we check LEFT or RIGHT first?
      int nextDir = Q.ComputeNextDir(LEFT);

      if (nextDir == LEFT){
        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){
          if (edgeImg[r*width+c-1]){edgeImg[r*width+c-1] = 0; pixels[count].r = r; pixels[count].c = c-1; count++;}
          r--; c--; dir = UP_LEFT; continue;
        } //end-if

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){
          if (edgeImg[r*width+c+1]){edgeImg[r*width+c+1]= 0; pixels[count].r = r; pixels[count].c = c+1; count++;}
          r--; c++; dir = UP_RIGHT; continue;
        } //end-if

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

      } else {
        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){
          if (edgeImg[r*width+c+1]){edgeImg[r*width+c+1]= 0; pixels[count].r = r; pixels[count].c = c+1; count++;}
          r--; c++; dir = UP_RIGHT; continue;
        } //end-if

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){
          if (edgeImg[r*width+c-1]){edgeImg[r*width+c-1] = 0; pixels[count].r = r; pixels[count].c = c-1; count++;}
          r--; c--; dir = UP_LEFT; continue;
        } //end-if

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == UP_RIGHT){
      // Should we check UP or RIGHT first?
      int nextDir = Q.ComputeNextDir(UP_RIGHT);

      // Up-Right
      if (edgeImg[(r-1)*width+c+1]){
        if (nextDir == UP){
          // Up?
          if (edgeImg[(r-1)*width+c]){
            pixels[count].r = r-1; pixels[count].c = c; count++;
            edgeImg[(r-1)*width+c] = 0;

          // Right?
          } else if (edgeImg[r*width+c+1]){
            pixels[count].r = r; pixels[count].c = c+1; count++;
            edgeImg[r*width+c+1] = 0;
          } //end-else

        } else {
          // Right?
          if (edgeImg[r*width+c+1]){
            pixels[count].r = r; pixels[count].c = c+1; count++;
            edgeImg[r*width+c+1] = 0;

          // Up?
          } else if (edgeImg[(r-1)*width+c]){
            pixels[count].r = r-1; pixels[count].c = c; count++;
            edgeImg[(r-1)*width+c] = 0;
          } //end-else
        } //end-else

        r--; c++; dir = UP_RIGHT; continue;
      } // end-if

      if (nextDir == UP){
        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

      } else {
        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == RIGHT){
      // Right
      if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

      // Should we check UP or DOWN first?
      int nextDir = Q.ComputeNextDir(UP);

      if (nextDir == UP){
        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){
          if (edgeImg[(r-1)*width+c]){edgeImg[(r-1)*width+c]= 0; pixels[count].r = r-1; pixels[count].c = c; count++;}
          r--; c++; dir = UP_RIGHT; continue;
        } //end-if

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){
          if (edgeImg[(r+1)*width+c]){edgeImg[(r+1)*width+c]= 0; pixels[count].r = r+1; pixels[count].c = c; count++;}
          r++; c++; dir = DOWN_RIGHT; continue;
        } //end-if

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

      } else {
        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){
          if (edgeImg[(r+1)*width+c]){edgeImg[(r+1)*width+c]= 0; pixels[count].r = r+1; pixels[count].c = c; count++;}
          r++; c++; dir = DOWN_RIGHT; continue;
        } //end-if

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){
          if (edgeImg[(r-1)*width+c]){edgeImg[(r-1)*width+c]= 0; pixels[count].r = r-1; pixels[count].c = c; count++;}
          r--; c++; dir = UP_RIGHT; continue;
        } //end-if

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == DOWN_RIGHT){
      // Should we check DOWN or RIGHT first?
      int nextDir = Q.ComputeNextDir(DOWN_RIGHT);

      // Down-Right?
      if (edgeImg[(r+1)*width+c+1]){
        if (nextDir == DOWN){
          // Down?
          if (edgeImg[(r+1)*width+c]){
            pixels[count].r = r+1; pixels[count].c = c; count++;
            edgeImg[(r+1)*width+c] = 0;

          // Right?
          } else if (edgeImg[r*width+c+1]){
            pixels[count].r = r; pixels[count].c = c+1; count++;
            edgeImg[r*width+c+1] = 0;
          } //end-else

        } else {
          // Right?
          if (edgeImg[r*width+c+1]){
            pixels[count].r = r; pixels[count].c = c+1; count++;
            edgeImg[r*width+c+1] = 0;

          // Down?
          } else if (edgeImg[(r+1)*width+c]){
            pixels[count].r = r+1; pixels[count].c = c; count++;
            edgeImg[(r+1)*width+c] = 0;
          } //end-else
        } //end-else

        r++; c++; dir = DOWN_RIGHT; continue;
      } // end-if

      if (nextDir == DOWN){
        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

      } else {
        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){r++; c--; dir = DOWN_LEFT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == DOWN){
      // Down
      if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

      // Should we check LEFT or RIGHT first?
      int nextDir = Q.ComputeNextDir(LEFT);

      if (nextDir == LEFT){
        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){
          if (edgeImg[r*width+c-1]){edgeImg[r*width+c-1]= 0; pixels[count].r = r; pixels[count].c = c-1; count++;} 
          r++; c--; dir = DOWN_LEFT; continue;
        } //end-if

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){
          if (edgeImg[r*width+c+1]){edgeImg[r*width+c+1] = 0; pixels[count].r = r; pixels[count].c = c+1; count++;} 
          r++; c++; dir = DOWN_RIGHT; continue;
        } //end-if

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

      } else {
        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){
          if (edgeImg[r*width+c+1]){edgeImg[r*width+c+1]= 0; pixels[count].r = r; pixels[count].c = c+1; count++;} 
          r++; c++; dir = DOWN_RIGHT; continue;
        } //end-if

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){
          if (edgeImg[r*width+c-1]){edgeImg[r*width+c-1]= 0; pixels[count].r = r; pixels[count].c = c-1; count++;} 
          r++; c--; dir = DOWN_LEFT; continue;
        } //end-if

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}
      } //end-else

      // Nowhere to go
      return count;

    } else if (dir == DOWN_LEFT){
      // Should we check DOWN or LEFT first?
      int nextDir = Q.ComputeNextDir(DOWN_LEFT);

      // Down-Left?
      if (edgeImg[(r+1)*width+c-1]){
        if (nextDir == DOWN){
          // Down?
          if (edgeImg[(r+1)*width+c]){
            pixels[count].r = r+1; pixels[count].c = c; count++;
            edgeImg[(r+1)*width+c] = 0;

          // Left?
          } else if (edgeImg[r*width+c-1]){
            pixels[count].r = r; pixels[count].c = c-1; count++;
            edgeImg[r*width+c-1] = 0;
          } //end-else

        } else {
          // Left?
          if (edgeImg[r*width+c-1]){
            pixels[count].r = r; pixels[count].c = c-1; count++;
            edgeImg[r*width+c-1] = 0;

          // Down?
          } else if (edgeImg[(r+1)*width+c]){
            pixels[count].r = r+1; pixels[count].c = c; count++;
            edgeImg[(r+1)*width+c] = 0;
          } //end-else
        } //end-else

        r++; c--; dir = DOWN_LEFT; continue;
      } // end-if

      if (nextDir == DOWN){
        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

      } else {
        // Left
        if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){r--; c--; dir = UP_LEFT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Right
        if (edgeImg[r*width+c+1]){c++; dir = RIGHT; continue;}
      } //end-else
      // Nowhere to go
      return count;

    } else { // (dir == LEFT){
      // Left
      if (edgeImg[r*width+c-1]){c--; dir = LEFT; continue;}

      // Should we check UP or DOWN first?
      int nextDir = Q.ComputeNextDir(UP);

      if (nextDir == UP){
        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){
          if (edgeImg[(r-1)*width+c]){edgeImg[(r-1)*width+c]= 0; pixels[count].r = r-1; pixels[count].c = c; count++;}
          r--; c--; dir = UP_LEFT; continue;
        } //end-if

        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){
          if (edgeImg[(r+1)*width+c]){edgeImg[(r+1)*width+c] = 0; pixels[count].r = r+1; pixels[count].c = c; count++;}
          r++; c--; dir = DOWN_LEFT; continue;
        } //end-if

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

      } else {
        // Down-Left
        if (edgeImg[(r+1)*width+c-1]){
          if (edgeImg[(r+1)*width+c]){edgeImg[(r+1)*width+c] = 0; pixels[count].r = r+1; pixels[count].c = c; count++;}
          r++; c--; dir = DOWN_LEFT; continue;
        } //end-if

        // Up-Left
        if (edgeImg[(r-1)*width+c-1]){
          if (edgeImg[(r-1)*width+c]){edgeImg[(r-1)*width+c]= 0; pixels[count].r = r-1; pixels[count].c = c; count++;}
          r--; c--; dir = UP_LEFT; continue;
        } //end-if

        // Down
        if (edgeImg[(r+1)*width+c]){r++; dir = DOWN; continue;}

        // Up
        if (edgeImg[(r-1)*width+c]){r--; dir = UP; continue;}

        // Down-Right
        if (edgeImg[(r+1)*width+c+1]){r++; c++; dir = DOWN_RIGHT; continue;}

        // Up-Right
        if (edgeImg[(r-1)*width+c+1]){r--; c++; dir = UP_RIGHT; continue;}
      } //end-else

      // Nowhere to go
      return count;
    } // end-else
  } //end-while

  return count;
} //end-Walk8Dirs

///----------------------------------------------------------------------------------
/// Predictive edge walk using 8 directions
///
EdgeMap *PELWalk8Dirs(unsigned char *edgeImg, int width, int height, int MIN_SEGMENT_LEN){
  EdgeMap *map = new EdgeMap(width, height);
  Pixel *pixels =  new Pixel[width*height];

  int noSegments = 0;
  int totalLen = 0;

  // Go over the anchors in sorted order
  for (int i=1; i<height-1; i++){
    for (int j=1; j<width-1; j++){
      if (edgeImg[i*width+j] == 0) continue;

#if 0
      int dir1, dir2;
      dir1 = dir2 = -1;

      // 8 directions
      if      (edgeImg[i*width+j+1]){dir1 = RIGHT; dir2 = LEFT;}
      else if (edgeImg[(i+1)*width+j]){dir1 = DOWN; dir2 = UP;}

      else if (edgeImg[(i+1)*width+j-1]){dir1 = DOWN_LEFT; dir2 = UP_RIGHT;}
      else if (edgeImg[(i+1)*width+j+1]){dir1 = DOWN_RIGHT; dir2 = UP_LEFT;}

      // Skip single pixel edgels
      if (dir1 < 0){edgeImg[i*width+j] = 0; continue;}

      // Walk using 8 directions
      int len1 = Walk8Dirs(edgeImg, width, height, i, j, dir1, pixels);
      int len2 = Walk8Dirs(edgeImg, width, height, i, j, dir2, pixels+len1);

      if (len1+len2-1 < MIN_SEGMENT_LEN) continue;

      map->segments[noSegments].pixels = map->pixels+totalLen;
      int len = 0;
      for (int k=len1-1; k>=1; k--){
        map->segments[noSegments].pixels[len] = pixels[k];
        len++;
      } //end-for

      for (int k=len1; k<len1+len2; k++){
        map->segments[noSegments].pixels[len] = pixels[k];
        len++;
      } //end-for

      map->segments[noSegments].noPixels = len;
      noSegments++;
      totalLen += len;
    } //end-for
  } // end-for
#else
      int dir1, dir2;
      dir1 = dir2 = -1;

      // 8 directions
      if      (edgeImg[i*width+j+1]) dir1 = RIGHT;
      else if (edgeImg[(i+1)*width+j]) dir1 = DOWN;

      else if (edgeImg[(i+1)*width+j-1]) dir1 = DOWN_LEFT;
      else if (edgeImg[(i+1)*width+j+1]) dir1 = DOWN_RIGHT;

      // Skip single pixel edgels
      if (dir1 < 0){edgeImg[i*width+j] = 0; continue;}

      // Walk using 8 directions
      int len1 = Walk8Dirs(edgeImg, width, height, i, j, dir1, pixels);
    
      int sr, sc;
      if      (edgeImg[i*width+j+1]){dir2 = RIGHT; sr = i; sc = j+1;}
      else if (edgeImg[(i+1)*width+j]){dir2 = DOWN; sr = i+1; sc = j;}

      else if (edgeImg[(i+1)*width+j-1]){dir2 = DOWN_LEFT; sr = i+1; sc = j-1;}
      else if (edgeImg[(i+1)*width+j+1]){dir2 = DOWN_RIGHT; sr = i+1; sc = j+1;}

      int len2=0;
      if (dir2 > 0) len2 = Walk8Dirs(edgeImg, width, height, sr, sc, dir2, pixels+len1);

      if (len1+len2 < MIN_SEGMENT_LEN) continue;

      map->segments[noSegments].pixels = map->pixels+totalLen;
      int len = 0;
      for (int k=len1-1; k>=0; k--){
        map->segments[noSegments].pixels[len] = pixels[k];
        len++;
      } //end-for

      for (int k=len1; k<len1+len2; k++){
        map->segments[noSegments].pixels[len] = pixels[k];
        len++;
      } //end-for

      map->segments[noSegments].noPixels = len;
      noSegments++;
      totalLen += len;
    } //end-for
  } // end-for
#endif


  map->noSegments = noSegments;
  delete pixels;
  
  return map;
} // end-PELWalk8Dirs

///========================== Step 3: Join Edge Segments ======================================
///-------------------------------------------------------------------------------------------
/// Returns the joint points. 
///
static unsigned char *FindJointPoints(EdgeMap *map){
  int width = map->width;
  int height = map->height;

  unsigned char *joints = new unsigned char[width*height];
  memset(joints, 0, width*height);

  short *segments = new short[width*height];
  memset(segments, -1, sizeof(short)*width*height);

  for (int i=0; i<map->noSegments; i++){
    for (int j=0; j<map->segments[i].noPixels; j++){
      int r = map->segments[i].pixels[j].r;
      int c = map->segments[i].pixels[j].c;

      segments[r*width+c] = i;
    } //end-for
  } //end-for

  for (int i=0; i<map->noSegments; i++){
    for (int k=0; k<2; k++){
      int r, c;

      if (k==0){
        r = map->segments[i].pixels[0].r;
        c = map->segments[i].pixels[0].c;

      } else {
        r = map->segments[i].pixels[map->segments[i].noPixels-1].r;
        c = map->segments[i].pixels[map->segments[i].noPixels-1].c;
      } //end-else

      if (r <=0 || r>=height-1 || c<=0 || c>=width-1) continue;

      if      (segments[(r-1)*width+c] >= 0 && segments[(r-1)*width+c] != i) joints[(r-1)*width+c] = 255;  // up
      else if (segments[(r+1)*width+c] >= 0 && segments[(r+1)*width+c] != i) joints[(r+1)*width+c] = 255;  // down
      else if (segments[r*width+c-1] >= 0 && segments[r*width+c-1] != i) joints[r*width+c-1] = 255;  // left
      else if (segments[r*width+c+1] >= 0 && segments[r*width+c+1] != i) joints[r*width+c+1] = 255;  // right
      else if (segments[(r-1)*width+c-1] >= 0 && segments[(r-1)*width+c-1] != i) joints[(r-1)*width+c-1] = 255;  // up-left
      else if (segments[(r-1)*width+c+1] >= 0 && segments[(r-1)*width+c+1] != i) joints[(r-1)*width+c+1] = 255;  // up-right
      else if (segments[(r+1)*width+c+1] >= 0 && segments[(r+1)*width+c+1] != i) joints[(r+1)*width+c+1] = 255;  // down-right
      else if (segments[(r+1)*width+c-1] >= 0 && segments[(r+1)*width+c-1] != i) joints[(r+1)*width+c-1] = 255;  // down-left
    } //end-for
  } //end-for

  delete segments;
  return joints;
} //end-FindJointPoints

///---------------------------------------------------------------------
/// Clip from the tips of the edge segments if there is a neigboring segment
/// maxClipSize is the maximum # of pixels to clip from the tips of the edge segments
///
static void ClipEdgeSegments(EdgeMap *map, int maxClipSize=5){
  int width = map->width;
  int height = map->height;
    
  unsigned char *joints = FindJointPoints(map);

  for (int i=0; i<map->noSegments; i++){
    // The loopy segments should not be broken
    int fr = map->segments[i].pixels[0].r;
    int fc = map->segments[i].pixels[0].c;

    int lr = map->segments[i].pixels[map->segments[i].noPixels-1].r;
    int lc = map->segments[i].pixels[map->segments[i].noPixels-1].c;

    // Skip this segment if it forms a loop
    if (abs(fr-lr) <= 3 && abs(fc-lc) <= 3) continue;

    for (int k=0; k<map->segments[i].noPixels; k++){
      int r = map->segments[i].pixels[k].r;
      int c = map->segments[i].pixels[k].c;  

      if (joints[r*width+c]){
        if (k <= maxClipSize){
          map->segments[i].pixels += k;
          map->segments[i].noPixels -= k;

          joints[r*width+c] = 0;
        } //end-if

        break;
      } //end-if
    } //end-for

    for (int k=map->segments[i].noPixels-1; k>=0; k--){
      int r = map->segments[i].pixels[k].r;
      int c = map->segments[i].pixels[k].c;  

      if (joints[r*width+c]){
        if (map->segments[i].noPixels - k <= maxClipSize){
          map->segments[i].noPixels = k+1;

          joints[r*width+c] = 0;
        } //end-if

        break;
      } //end-if
    } //end-for

  } //end-for

  delete joints;
} //end-ClipEdgeSegments

///---------------------------------------------------------------------
/// Join edge segments whose endpoints are at most 2 pixels away from each other
///
static void JoinNeighborEdgeSegments(EdgeMap *map){
  // Clip the tips of the edge segments
  ClipEdgeSegments(map, 5);

  int width = map->width;
  int height = map->height;

  int *segments = new int[width*height];
  memset(segments, 0, sizeof(int)*width*height);

  // Mark the end of the segments on the "segments" array
  for (int i=0; i<map->noSegments; i++){
    int r, c;

    r = map->segments[i].pixels[0].r;
    c = map->segments[i].pixels[0].c;
    segments[r*width+c] = i+1;


    int index = map->segments[i].noPixels-1;
    r = map->segments[i].pixels[index].r;
    c = map->segments[i].pixels[index].c;
    segments[r*width+c] = i+1;
  } //end-for

  // Find the neighbors of each segment in the 2x2 neighborhood
  struct NN {
    bool taken;   // Is this segment already taken?
    int s, e;     // Neighbor from the start and end pixel
  };        

  NN *nn = new NN[map->noSegments];

  // Find the neighbors of each segment
  for (int i=0; i<map->noSegments; i++){
    int r, c;

    nn[i].taken = false;

    r = map->segments[i].pixels[0].r;
    c = map->segments[i].pixels[0].c;

    int neighbor = -1;
    int len = 0;
    for (int m=r-2; m<=r+2; m++){
      if (m < 0 || m >= height) continue;

      for (int n=c-2; n<=c+2; n++){
        if (n < 0 || n >= width) continue;
        if (segments[m*width+n] == 0) continue;
        if (segments[m*width+n] == i+1) continue;

        int s = segments[m*width+n]-1;
        if (map->segments[s].noPixels > len){neighbor=s; len = map->segments[s].noPixels;}
      } //end-for
    } //end-for
    nn[i].s = neighbor;

    int index = map->segments[i].noPixels-1;
    r = map->segments[i].pixels[index].r;
    c = map->segments[i].pixels[index].c;

    neighbor = -1;
    len = 0;
    for (int m=r-2; m<=r+2; m++){
      if (m < 0 || m >= height) continue;

      for (int n=c-2; n<=c+2; n++){
        if (n < 0 || n >= width) continue;
        if (segments[m*width+n] == 0) continue;
        if (segments[m*width+n] == i+1) continue;

        int s = segments[m*width+n]-1;
        if (map->segments[s].noPixels > len){neighbor=s; len = map->segments[s].noPixels;}
      } //end-for
    } //end-for

    nn[i].e = neighbor;
    if (nn[i].e == nn[i].s) nn[i].e = -1;
  } //end-for

  // Now join. Create a new edgemap for the joined edge segments
  int noSegments2 = 0;
  EdgeSegment *segments2 = new EdgeSegment[map->noSegments*4];
  Pixel *pix2 = map->segments[map->noSegments-1].pixels + map->segments[map->noSegments-1].noPixels;
  int *listBuffer = new int[map->noSegments*2];

  for (int i=0; i<map->noSegments; i++){
    // Already taken? Then skip.
    if (nn[i].taken) continue;

    // Segment with no neighbors? Just copy the pixels
    if (nn[i].s == -1 && nn[i].e == -1){
      nn[i].taken = true;

      segments2[noSegments2].pixels = map->segments[i].pixels;
      segments2[noSegments2].noPixels = map->segments[i].noPixels;
      pix2 += segments2[noSegments2].noPixels;
      noSegments2++;

      continue;
    } //end-if

    // At least 2 segments to join. First find the segments, and then join them
    int *list = listBuffer + map->noSegments;
    int listSize = 0;

    if (nn[i].s >= 0 || nn[i].e >= 0){
      int prev = i;

      list--;
      list[0] = prev;
      listSize++;
      nn[prev].taken = true; // Mark this segment as taken

      int curr = nn[i].s;
      while (curr >= 0 && nn[curr].taken == false){
        list--;
        list[0] = curr;
        listSize++;
        nn[curr].taken = true; // Mark this segment as taken

        if (prev == nn[curr].s){prev = curr; curr = nn[curr].e;}
        else {prev = curr; curr = nn[curr].s;}
      } //end-while

      // Walk from "e" to the next neighbors
      prev = i;
      curr = nn[i].e;
      while (curr >= 0 && nn[curr].taken == false){
        list[listSize] = curr;
        listSize++;
        nn[curr].taken = true; // Mark this segment as taken

        if (prev == nn[curr].s){prev = curr; curr = nn[curr].e;}
        else {prev = curr; curr = nn[curr].s;}
      } //end-while
    } //end-if

    // Now join the pixels of the segments properly
    int noPixels = 0;
    segments2[noSegments2].pixels = pix2;

    // Copy the pixels of the first segment in the list
    // This is junction (curr, next)
    int curr = list[0];
    int next = list[1];

    if (nn[curr].e == next){
      // Copy the pixels of the current segment in forward order
      for (int k=0; k<map->segments[curr].noPixels; k++){
        segments2[noSegments2].pixels[noPixels++] = map->segments[curr].pixels[k];
      } //end-for      

    } else {
      // Copy the pixels of the current segment in reverse order
      for (int k=map->segments[curr].noPixels-1; k>=0; k--){
        segments2[noSegments2].pixels[noPixels++] = map->segments[curr].pixels[k];
      } //end-for      
    } // end-else

    // Now copy the pixels of the rest of the segments
    for (int index = 1; index < listSize; index++){
      int prev = list[index]-1;
      curr = list[index];

      // This is junction (prev, curr)
      if (nn[curr].s == prev){
        // Copy the pixels of the current segment in forward order
        for (int k=0; k<map->segments[curr].noPixels; k++){
          segments2[noSegments2].pixels[noPixels++] = map->segments[curr].pixels[k];
        } //end-for      

      } else {
        // Copy the pixels of the current segment in reverse order
        for (int k=map->segments[curr].noPixels-1; k>=0; k--){
          segments2[noSegments2].pixels[noPixels++] = map->segments[curr].pixels[k];
        } //end-for      
      } // end-else
    } // end-while  

    segments2[noSegments2].noPixels = noPixels;
    pix2 += noPixels;
    noSegments2++;
  } //end-for

  map->noSegments = noSegments2;
  delete map->segments;
  map->segments = segments2;

  delete listBuffer;
  delete nn;
  delete segments;
} //end-JoinEdgeSegments

///============================= Step 4: ThinEdgeSegments ==================================
///-------------------------------------------------------------------------------------------
/// Thins down edge segment by removing superfulous pixels in the chain. For example:
///   xx       x 
///  xx   --> x
/// xx       x  
///
///
static void ThinEdgeSegments(EdgeMap *map, int MIN_SEGMENT_LEN){
  // Thin the edge segments
  int noSegments = 0;
  for (int i=0; i<map->noSegments; i++){
    int index = 0;

    for (int j=2; j<map->segments[i].noPixels; j++){
      int dx = abs(map->segments[i].pixels[index].c - map->segments[i].pixels[j].c);
      int dy = abs(map->segments[i].pixels[index].r - map->segments[i].pixels[j].r);
      
      if (dx >= 2 || dy >= 2){
//      if (dx+dy >= 2){
        map->segments[i].pixels[++index] = map->segments[i].pixels[j-1];
      } // end-if
    } //end-for

    // Copy the last pixel
    map->segments[i].pixels[++index] = map->segments[i].pixels[map->segments[i].noPixels-1];
    map->segments[i].noPixels = index+1;

    if (map->segments[i].noPixels >= MIN_SEGMENT_LEN) map->segments[noSegments++] = map->segments[i];
  } //end-for

  map->noSegments = noSegments;
} //end-ThinEdgeSegments

///============================= Step 5: FixEdgeSegments ==================================
///---------------------------------------------------------
/// Fix edge segments having one or two pixel fluctuations
/// An example one pixel problem getting fixed:
///  x
/// x x --> xxx
///
/// An example two pixel problem getting fixed:
///  xx
/// x  x --> xxxx
///
static void FixEdgeSegments(EdgeMap *map){
  /// First fix one pixel problems: There are four cases
  for (int i=0; i<map->noSegments; i++){
    int cp = map->segments[i].noPixels-2;  // Current pixel index
    int n2 = 0;  // next next pixel index

    while (n2 < map->segments[i].noPixels){
      int n1 = cp+1; // next pixel

      cp = cp % map->segments[i].noPixels; // Roll back to the beginning
      n1 = n1 % map->segments[i].noPixels; // Roll back to the beginning

      int r = map->segments[i].pixels[cp].r;
      int c = map->segments[i].pixels[cp].c;

      int r1 = map->segments[i].pixels[n1].r;
      int c1 = map->segments[i].pixels[n1].c;

      int r2 = map->segments[i].pixels[n2].r;
      int c2 = map->segments[i].pixels[n2].c;

      // 4 cases to fix
      if (r2 == r-2 && c2 == c){
        if (c1 != c){
          map->segments[i].pixels[n1].c = c;
        } //end-if

        cp = n2;
        n2 += 2;

      } else if (r2 == r+2 && c2 == c){
        if (c1 != c){
          map->segments[i].pixels[n1].c = c;
        } //end-if

        cp = n2;
        n2 += 2;

      } else if (r2 == r && c2 == c-2){
        if (r1 != r){
          map->segments[i].pixels[n1].r = r;
        } //end-if

        cp = n2;
        n2 += 2;

      } else if (r2 == r && c2 == c+2){
        if (r1 != r){
          map->segments[i].pixels[n1].r = r;
        } //end-if

        cp = n2;
        n2 += 2;

      } else {
        cp++;
        n2++;
      } //end-else
    } //end-while
  } // end-for
} //end-FixEdgeMap
