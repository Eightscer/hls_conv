#include "fcl.h"

inline int in_bounds(int x, int y, int w, int h){
	return (0 <= y && y < h && 0 <= x && x < w);
}

void hw_fcl(
	fstr& in_data, fstr& in_part, fstr& out_map,
	unsigned int in_len, unsigned int out_len
){
#pragma HLS INTERFACE axis register port=in_data
#pragma HLS INTERFACE axis register port=in_part
#pragma HLS INTERFACE axis register port=out_map
#pragma HLS INTERFACE s_axilite port=in_len bundle=CTRL
#pragma HLS INTERFACE s_axilite port=out_len bundle=CTRL
//#pragma HLS INTERFACE ap_ctrl_none port=return

	axis_t temp;
	temp = in_data.read();
	int ks = temp.data;		// Kernel size
	temp = in_data.read();
	int h = temp.data;			// Height of map
	temp = in_data.read();
	int w = temp.data;			// Width of map

	int y, x;					// Current coordinates for output map
	axis_t total;				// Multiply accumulate result

	// Special case when kernel is 1x1, no need for line buffer or window

	if(ks == 1){
		int i;
		int max = h*w;
		temp = in_data.read();
		float weight = temp.data;
		axis_t part;

		ksize1: for(i = 0; i < DIM; ++i){
		#pragma HLS LOOP_TRIPCOUNT max=50176
		#pragma HLS PIPELINE
			if(i < max-1){
				total = in_part.read();
				part = in_data.read();
				total.data += weight * part.data;
				out_map.write(total);
			} else {
				total = in_part.read();
				part = in_data.read();
				total.data += weight * part.data;
				out_map.write(total);
				break;
			}
		}

	} else {

		// Only other case is 3x3 window
		// No one uses 2x2 kernels for convolution lol
		
		float ker[KSIZE][KSIZE];	// Temporary array for kernel weights
		#pragma HLS ARRAY_PARTITION variable=ker complete dim=0

		int ky, kx;					// Coordinates for kernel

		float win[KSIZE][KSIZE];
		#pragma HLS ARRAY_PARTITION variable=win complete dim=0

		float buf[KSIZE][L_W + (2*KHALF)];
		#pragma HLS ARRAY_PARTITION variable=buf complete dim=1


		// Populate kernel
		pop_ker:for(ky = 0; ky < KSIZE; ++ky) {
			#pragma HLS UNROLL
			pop_ker_x:for(kx = 0; kx < KSIZE; ++kx) {
				#pragma HLS UNROLL
				axis_t r = in_data.read();
				ker[ky][kx] = r.data;
			}
		}

		// Initialize first values for buffer
		pop_buf_y: for(ky = -KHALF; ky <= KHALF; ++ky){
			pop_buf_x: for(kx = -KHALF; kx < KHALF + L_W; ++kx){
				//#pragma HLS PIPELINE
				if(kx >= w + KHALF) break;
				if(in_bounds(kx, ky, w, h)){
					axis_t r = in_data.read();
					buf[ky+KHALF][kx+KHALF] = r.data;
				} else {
					buf[ky+KHALF][kx+KHALF] = 0;
				}
			}
		}

		// Initialize window

		pop_win: for(ky = 0; ky < KSIZE; ++ky){
			#pragma HLS UNROLL
			pop_win_x: for(kx = 0; kx < KSIZE; ++kx){
				#pragma HLS UNROLL
				win[ky][kx] = buf[ky][kx];
			}
		}

		
		// For each pixel in current output map
		conv_y: for(y = 0; y < L_H; ++y){
			if(y >= h) break;
			conv_x: for(x = 0; x < L_W; ++x){
				//#pragma HLS PIPELINE
				if(x >= w) break;

				// Total sum of mult_accum for a pixel on all channels
				total = in_part.read();

				// Adjust window
				/*
				for(ky = 0; ky < KSIZE; ky++){
					for(kx = 0; kx < KSIZE; kx++){
						win[ky][kx] = buf[ky][kx+x];
					}
				}
				*/
				
				// For each weight in current window
				ma_y: for(ky = -KHALF; ky <= KHALF; ++ky){
					ma_x: for(kx = -KHALF; kx <= KHALF; ++kx){
						if(in_bounds(x+kx, y+ky, w, h)){
							// Multiply accumulate
							total.data += win[ky+KHALF][kx+KHALF] * ker[ky+KHALF][kx+KHALF];
							
						}
					}
				}

				// Add total sum to current output map
				out_map.write(total);

				// Shift window
				shift_win_y: for(ky = 0; ky < KSIZE; ++ky){
					shift_win_x: for(kx = 0; kx < KSIZE-1; ++kx){
						win[ky][kx] = win[ky][kx+1];
					}
				}
				shift_win_col: for(ky = 0; ky < KSIZE; ++ky)
					win[ky][KSIZE-1] = buf[ky][x+KSIZE];

				// Shift line buffer
				shift_buf: for(ky = 0; ky < KSIZE-1; ++ky){
					buf[ky][x] = buf[ky+1][x];
				}

				// Update buffer
				if(in_bounds(x-KHALF, y+KHALF+1, w, h)){
					axis_t r = in_data.read();
					buf[KSIZE-1][x] = r.data;
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
				update_buf_x: for(; x < w + (KHALF*2); ++x){
				#pragma HLS LOOP_TRIPCOUNT max=2
					update_buf_y: for(ky = 0; ky < KSIZE-1; ++ky){
						buf[ky][x] = buf[ky+1][x];
					}
		
					// Update buffer
					if(in_bounds(x-KHALF, y+KHALF+1, w, h)){
						axis_t r = in_data.read();
						buf[KSIZE-1][x] = r.data;
					} else {
						buf[KSIZE-1][x] = 0;				
					}
				}

				// Regenerate window
				regen_win_y: for(ky = 0; ky < KSIZE; ++ky){
					#pragma HLS UNROLL
					regen_win_x: for(kx = 0; kx < KSIZE; ++kx){
						#pragma HLS UNROLL
						win[ky][kx] = buf[ky][kx];
					}
				}
				
			} // End of conditional update
			
		} // End of y for loop
		
	} // End of ksize == 3 condition
	
}

