#ifndef HW_FCL
#define HW_FCL

// Potential hardware constants, based off of printf
#define KSIZE 3			// dimensions of filters
#define KHALF (((KSIZE) - 1) / 2)
#define L_C 3			// number of channels
#define L_N 16			// number of filters

#define N_W KSIZE*KSIZE*L_C

// number of parameters
#define L_P KSIZE*KSIZE*L_C*L_N
#define F_A KSIZE*KSIZE*L_C			// Filter area
#define K_A KSIZE*KSIZE				// Kernel window area

#define L_W 224			// input data width
#define L_H 224			// input data height

#define O_W 224			// output data width
#define O_H 224			// output data height

#define WRK O_H*O_W*KSIZE*KSIZE*L_C
#define INP L_C*L_H*L_W
#define O_S O_H*O_W*L_N
#define DIM L_H*L_W
#define D_O O_H*O_W

#define STRIDE 1
#define PAD 1

void hw_fcl(
	//float weights[KSIZE*KSIZE],
	//float in_ch[DIM],
	// in_data stores kernel size, width, height, weights, and channel data
	// Unused space are padded with zeroes
	float in_data[3+(KSIZE*KSIZE)+DIM],
	float in_part[DIM],
	float out_map[D_O]
);

#endif
