#include "fcl.h"

#include <stdio.h>

// HOW INPUT IS ORGANIZED
// Input images are a single stream of C maps of dimensions H*W. The data is
// read starting from the first map (C = 0) from the first row (H = 0) and
// continuing horizontally (W is incremented) before starting the next row.

// HOW WEIGHTS ARE ORGANIZED
// There are N kernels of dimensions C*K*K. Each kernel corresponds to an
// index N, which also denotes the number of output maps. It's assumed that
// like the input image, the data is stored in a stream starting with the
// filter for the first channel, first row, etc. before starting to read
// from the weights for the 2nd filter.

// HOW THE OUTPUT IS ORGANIZED
// In our case, the dimensions for the output maps are the same for the input
// maps, so there should be N output maps of dimensions H*W. Each pixel is
// assumed to be the sum of the values*weights for all C channels within the
// input image.

inline int in_bounds(int x, int y, int w, int h){
	return (0 <= y && y < h && 0 <= x && x < w);
}

void hw_fcl(
	//float weights[KSIZE*KSIZE],
	//float in_ch[DIM],
	float in_data[3+(KSIZE*KSIZE)+DIM],
	float in_part[DIM],
	float out_map[D_O]
){
	int i1 = 0;					// Stream counter for in_data
	int io = 0;					// Stream counter for out_map
	int ip = 0;					// Stream counter for in_part

	int ks = in_data[i1++];		// Kernel size
	int h = in_data[i1++];		// Height of map
	int w = in_data[i1++];		// Width of map

	int y, x;					// Current coordinates for output map
	float total;				// Multiply accumulate result

	// Special case when kernel is 1x1, no need for line buffer or window
	/*
	if(ks == 1){
		float weight = in_data[i1++];
		for(y = 0; y < h; ++y){
			for(x = 0; x < w; ++x){
				total = in_part[ip++];
				total += weight * in_data[i1++];
				out_map[io++] = total;
			}
		}
		return;
	}
	*/
	if(ks == 1){
		int i;
		int max = h*w;
		float weight = in_data[i1++];
		for(i = 0; i < DIM; ++i){
			if(i < max){
				total = in_part[ip++];
				total += weight * in_data[i1++];
				out_map[io++] = total;
			} else {
				break;
			}
		}
	} else {

		// Only other case is 3x3 window
		// No one uses 2x2 kernels for convolution lol
		
		float ker[KSIZE][KSIZE];			// Temporary array for kernel weights
		int ky, kx;							// Coordinates for kernel

		//int iw = 0;
		float win[KSIZE][KSIZE];
		float buf[KSIZE][L_W + (2*KHALF)];
		//float col[KSIZE];

		//float cmp[L_C][KSIZE][KSIZE];

		// Populate kernel. Likely able to be fully unrolled
		for(ky = 0; ky < KSIZE; ++ky)
			for(kx = 0; kx < KSIZE; ++kx)
				ker[ky][kx] = in_data[i1++];

		// Initialize first values for buffer
		for(ky = -KHALF; ky <= KHALF; ++ky){
			//for(kx = -KHALF; kx < KHALF + L_W; ++kx){
			for(kx = -KHALF; kx < KHALF + w; ++kx){
				if(in_bounds(kx, ky, w, h)){
					buf[ky+KHALF][kx+KHALF] = in_data[i1++];					
				} else {
					buf[ky+KHALF][kx+KHALF] = 0;
				}
			}
		}

		// Initialize window

		for(ky = 0; ky < KSIZE; ++ky){
			for(kx = 0; kx < KSIZE; ++kx){
				win[ky][kx] = buf[ky][kx];
			}
		}

		
		// For each pixel in current output map
		for(y = 0; y < L_H; ++y){
			if(y >= h) break;
			for(x = 0; x < L_W; ++x){
				if(x >= w) break;

				// Total sum of mult_accum for a pixel on all channels
				total = in_part[ip++];

				// Adjust window
				/*
				for(ky = 0; ky < KSIZE; ky++){
					for(kx = 0; kx < KSIZE; kx++){
						win[ky][kx] = buf[ky][kx+x];
					}
				}
				*/
				
				// For each weight in current window
				for(ky = -KHALF; ky <= KHALF; ++ky){
					for(kx = -KHALF; kx <= KHALF; ++kx){
						if(in_bounds(x+kx, y+ky, w, h)){
							// Multiply accumulate
							total += win[ky+KHALF][kx+KHALF] * ker[ky+KHALF][kx+KHALF];
							
						}
					}
				}

				// Add total sum to current output map
				out_map[io++] = total;
				//printf("i1: %d    i2: %d    i3: %d    iw: %d    io: %d\n", i1, i2, i3, iw, io);

				// Shift window
				for(ky = 0; ky < KSIZE; ++ky){
					for(kx = 0; kx < KSIZE-1; ++kx){
						win[ky][kx] = win[ky][kx+1];
					}
				}
				for(ky = 0; ky < KSIZE; ++ky)
					win[ky][KSIZE-1] = buf[ky][x+KSIZE];

				// Shift line buffer
				for(ky = 0; ky < KSIZE-1; ++ky){
					buf[ky][x] = buf[ky+1][x];
				}

				// Update buffer
				if(in_bounds(x-KHALF, y+KHALF+1, w, h)){
					buf[KSIZE-1][x] = in_data[i1++];
				} else {
					buf[KSIZE-1][x] = 0;				
				}

			} // End of x for loop

			// Don't update if we're already at the end
			if(y+KHALF < h){

				// Shift line buffer
				/*
				for(ky = 0; ky < KSIZE-1; ++ky){
					//for(kx = 0; kx < L_W + (2*KHALF); ++kx){
					for(kx = 0; kx < w + (2*KHALF); ++kx){
						buf[ky][kx] = buf[ky+1][kx];
					}
				}
				*/
				
				// Copy next row from input map
				//for(kx = -KHALF; kx < KHALF + L_W; ++kx){
				/*
				for(kx = -KHALF; kx < KHALF + w; ++kx){
					if(in_bounds(kx, y+KHALF+1, w, h)){
						buf[KSIZE-1][kx+KHALF] = in_data[i1++];					
					} else {
						buf[KSIZE-1][kx+KHALF] = 0;
					}
				}
				*/

				// Update rest of line buffer
				for(; x < w + (KHALF*2); ++x){
					for(ky = 0; ky < KSIZE-1; ++ky){
						buf[ky][x] = buf[ky+1][x];
					}
		
					// Update buffer
					if(in_bounds(x-KHALF, y+KHALF+1, w, h)){
						buf[KSIZE-1][x] = in_data[i1++];
					} else {
						buf[KSIZE-1][x] = 0;				
					}
				}

				// Regenerate window
				for(ky = 0; ky < KSIZE; ++ky){
					for(kx = 0; kx < KSIZE; ++kx){
						win[ky][kx] = buf[ky][kx];
					}
				}
				
			} // End of conditional update
			
		} // End of y for loop
		
	} // End of ksize == 3 condition
	
}

