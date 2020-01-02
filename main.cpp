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

/******************************************************************************
 * Please cite the following paper: 
 * C. Akinlar, E. Chome, PEL: A Predictive Edge Linking Algorithm, Journal of Visual Communication and Image Representation, DOI: 10.1016/j.jvcir.2016.01.017 (2016).
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "Timer.h"
#include "EdgeMap.h"
#include "PEL.h"

/// Two functions to read/save PGM files
int ReadImagePGM(char *filename, char **pBuffer, int *pWidth, int *pHeight);
void SaveImagePGM(char *filename, char *buffer, int width, int height);

int main(){
  // Here is the test code
  int width, height;
  unsigned char *bem; 


//-------------------- FIG 8 images -----------
  char *str = (char *)"BEMs/lena.pgm";
//  char *str = (char *)"BEMs/chairs.pgm";
//  char *str = (char *)"BEMs/house.pgm";
//  char *str = (char *)"BEMs/circles.pgm";

//-------------------- FIG 11 images -----------
//  char *str = (char *)"BEMs/101085-gPb.pgm";
//  char *str = (char *)"BEMs/101087-gPb.pgm";
//  char *str = (char *)"BEMs/69015-gPb.pgm";
//  char *str = (char *)"BEMs/86000-gPb.pgm";
//  char *str = (char *)"BEMs/210088-gPb.pgm";

//-------------------- FIG 12 images -----------
//  char *str = (char *)"BEMs/1-fruits.pgm";
//  char *str = (char *)"BEMs/2-trees.pgm";
//  char *str = (char *)"BEMs/3-cells.pgm";
//  char *str = (char *)"BEMs/4-Diatom.pgm";
//  char *str = (char *)"BEMs/5-Diatom.pgm";


  if (ReadImagePGM(str, (char **)&bem, &width, &height) == 0){
    printf("Failed opening <%s>\n", str);
    return 1;
  } //end-if

  printf("Working on %dx%d image\n", width, height);

  //-------------------------------- ED Test ------------------------------------
  Timer timer;

  timer.Start();

  EdgeMap *map = PEL(bem, width, height, 8);

  timer.Stop();

  printf("PEL detects <%d> edge segments in <%4.2lf> ms\n\n", map->noSegments, timer.ElapsedTime());

  // This is how you access the pixels of the edge segments returned by ED
  memset(map->edgeImg, 0, width*height);
  for (int i=0; i<map->noSegments; i++){
    for (int j=0; j<map->segments[i].noPixels; j++){
      int r = map->segments[i].pixels[j].r;
      int c = map->segments[i].pixels[j].c;
      
      map->edgeImg[r*width+c] = 255;
    } //end-for
  } //end-for

  SaveImagePGM((char *)"PEL-Map.pgm", (char *)map->edgeImg, width, height);
  delete map;
  delete bem;

  return 0;
} //end-main

/******************************************************************************
* Function: ReadImagePGM
* Purpose: This function reads in an image in PGM format. The image can be
* read in from either a file or from standard input. The image is only read
* from standard input when infilename = NULL. Because the PGM format includes
* the number of columns and the number of rows in the image, these are read
* from the file. Memory to store the image is allocated in this function.
* All comments in the header are discarded in the process of reading the
* image. Upon failure, this function returns 0, upon sucess it returns 1.
******************************************************************************/
int ReadImagePGM(char *filename, char **pBuffer, int *pWidth, int *pHeight){
   FILE *fp;
   char buf[71];
   int width, height;

   if ((fp = fopen(filename, "rb")) == NULL){
     fprintf(stderr, "Error reading the file %s in ReadImagePGM().\n", filename);
     return(0);
   } //end-if

   /***************************************************************************
   * Verify that the image is in PGM format, read in the number of columns
   * and rows in the image and scan past all of the header information.
   ***************************************************************************/
   fgets(buf, 70, fp);
   bool P2 = false;
   bool P5 = false;

   if      (strncmp(buf, "P2", 2) == 0) P2 = true;
   else if (strncmp(buf, "P5", 2) == 0) P5 = true;

   if (P2 == false && P5 == false){
      fprintf(stderr, "The file %s is not in PGM format in ", filename);
      fprintf(stderr, "ReadImagePGM().\n");
      fclose(fp);
      return 0;
   } //end-if

   do {fgets(buf, 70, fp);} while (buf[0] == '#');  /* skip all comment lines */
   sscanf(buf, "%d %d", &width, &height);
   fgets(buf, 70, fp);  // Skip max value (255)

   *pWidth = width;
   *pHeight = height;

   /***************************************************************************
   * Allocate memory to store the image then read the image from the file.
   ***************************************************************************/
   if (((*pBuffer) = (char *) malloc((*pWidth)*(*pHeight))) == NULL){
      fprintf(stderr, "Memory allocation failure in ReadImagePGM().\n");
      fclose(fp);
      return(0);
   } //end-if  

   if (P2){
      int index=0;
      char *p = *pBuffer;
      int col = 0;
      int read = 0;

      while (1){
        int c;
        if (fscanf(fp, "%d", &c) < 1) break;
        read++;

        if (col < *pWidth) p[index++] = (unsigned char)c;

        col++;
        if (col == width) col = 0;
      } //end-while

      if (read != width*height){
        fprintf(stderr, "Error reading the image data in ReadImagePGM().\n");
        fclose(fp);
        free((*pBuffer));
        return(0);
      } //end-if

   } else if (P5){
      int index=0;
      char *p = *pBuffer;
      int col = 0;
      int read = 0;

      while (1){
        unsigned char c;
        if (fread(&c, 1, 1, fp) < 1) break;
        read++;

        if (col < *pWidth) p[index++] = c;

        col++;
        if (col == width) col = 0;
      } //end-while

     if (read != width*height){
        fprintf(stderr, "Error reading the image data in ReadImagePGM().\n");
        fclose(fp);
        free((*pBuffer));
        return(0);
     } //end-if
   } //end-else

   fclose(fp);
   return 1;
} //end-ReadPGMImage

///---------------------------------------------------------------------------------
/// Save a buffer as a .pgm image
///
void SaveImagePGM(char *filename, char *buffer, int width, int height){
  FILE *fp = fopen(filename, "wb");

  // .PGM header
  fprintf(fp, "P5\n");
  fprintf(fp, "# Some comment here!\n");
  fprintf(fp, "%d %d\n", width, height);
  fprintf(fp, "255\n");

  // Grayscale image
  fwrite(buffer, 1, width*height, fp);

  fclose( fp );
} //end-SaveImagePGM