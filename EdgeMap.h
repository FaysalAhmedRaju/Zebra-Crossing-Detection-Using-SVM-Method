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
#ifndef _EDGE_MAP_H_
#define _EDGE_MAP_H_

#include <memory.h>

enum GradientOperator {PREWITT_OPERATOR=101, SOBEL_OPERATOR=102, SCHARR_OPERATOR=103};

struct Pixel {int r, c;};

struct EdgeSegment {
  Pixel *pixels;       // Pointer to the pixels array
  int noPixels;        // # of pixels in the edge map
};

struct EdgeMap {
public:
  int width, height;        // Width & height of the image
  unsigned char *edgeImg;   // BW edge map


  Pixel *pixels;            // Edge map in edge segment form
  EdgeSegment *segments;     
  int noSegments;
      
public:
  // constructor
  EdgeMap(int w, int h){
    width = w;
    height = h;

    edgeImg = new unsigned char[width*height];

    pixels = new Pixel[width*height];
    segments = new EdgeSegment[width*height];
    noSegments = 0;
  } //end-EdgeMap

  // Destructor
  ~EdgeMap(){
    delete edgeImg;
    delete pixels;
    delete segments;
  } //end-~EdgeMap


  void ConvertEdgeSegments2EdgeImg(){
    memset(edgeImg, 0, width*height);

    for (int i=0; i<noSegments; i++){
      for (int j=0; j<segments[i].noPixels; j++){
        int r = segments[i].pixels[j].r;
        int c = segments[i].pixels[j].c;

        edgeImg[r*width+c] = 255;
      } //end-for
    } //end-for
  } //end-ConvertEdgeSegments2EdgeImg
};


#endif