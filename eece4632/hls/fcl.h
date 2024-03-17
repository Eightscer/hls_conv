#ifndef HW_FCL
#define HW_FCL

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

// Potential hardware constants, based off of printf
#define KSIZE 3			// dimensions of filters
#define KHALF (((KSIZE) - 1) / 2)

#define L_W 224			// input data width
#define L_H 224			// input data height

#define DIM L_H*L_W

struct axis_t{
	float data;
	ap_uint<1> TLAST;
};

typedef hls::stream<axis_t> fstr;

// in_data stores ksize, height, width, weights, channel data
void hw_fcl(
	fstr& in_data, fstr& in_part, fstr& out_map,
	unsigned int in_len, unsigned int out_len
);

#endif
