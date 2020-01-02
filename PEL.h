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
#ifndef _PEL_H_
#define _PEL_H_

// Link edges and return an edgemap (Predictive edge linking)
EdgeMap *PEL(unsigned char *edgeImg, int width, int height, int MIN_SEGMENT_LEN=10);

#endif