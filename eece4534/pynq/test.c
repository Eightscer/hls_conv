#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "dma.h"
#include "fcl_test.h"

int main(){
	clock_t t_start, t_end;
	float t_totalhw, t_totalsw;

	printf("Testing single-weight\n");
	unsigned int ks = 1, h = 4, w = 4;
	unsigned int channels = 1, filters = 1;
	unsigned int len = h*w;
	float* inputs = (float*)malloc(sizeof(float)*len);
	float* output = (float*)malloc(sizeof(float)*len);
	float* weights = (float*)malloc(sizeof(float)*ks*ks);

	unsigned int i;
	weights[0] = 0.5;
	for(i = 0; i < len; ++i){
		inputs[i] = 0.25;
		output[i] = 0.625;
	}

	printf("Running.\n");
	alloc_buffers(ks, h, w);
	t_start = clock();
	do_the_thing(inputs, weights, output, ks, h, w);
	t_end = clock();
	free_buffers();
	t_totalhw = (float)(t_end - t_start) / CLOCKS_PER_SEC;

	printf("Output:\n");
	for(i = 0; i < len; ++i){
		printf("%f ", output[i]);
	}
	printf("\nTime taken: %.10f seconds.\n\n", t_totalhw);

	free(inputs);
	free(weights);
	free(output);

	printf("Testing layer 1 of tiny.cfg tiny.weights data/dog.jpg\n");
	ks = 3; h = 224; w = 224; channels = 3; filters = 16; len = h*w;
	inputs = (float*)malloc(sizeof(float)*len*channels);
	output = (float*)calloc(len*filters, sizeof(float));
	weights = (float*)malloc(sizeof(float)*ks*ks*channels*filters);

	int err = 0;
	printf("Loading inputs...\n");
	FILE* fi = fopen("inputs.dat", "r");
	if(!fi){ printf("Could not open inputs.dat.\n"); err = 1; }
	fread(inputs, sizeof(float)*len*channels, 1, fi);
	fclose(fi);
	if(err){ free(inputs); free(weights); return err; }

	printf("Loading weights...\n");
	FILE* fw = fopen("weights.dat", "r");
	if(!fw){ printf("Could not open weights.dat.\n"); err = 1; }
	fread(weights, sizeof(float)*ks*ks*channels*filters, 1, fw);
	fclose(fw);
	if(err){ free(inputs); free(weights); return err; }

	alloc_buffers(ks, h, w);

	unsigned int c, n;
	printf("Running.\n");
	t_start = clock();
	for(n = 0; n < filters; ++n){
		//printf("Filter %d...\n", n);
		for(c = 0; c < channels; ++c){
			do_the_thing(inputs + (len*c), weights + ((n*ks*ks*channels) + (ks*ks*c)),
				output + (len*n), ks, h, w);
		}
	}
	t_end = clock();

	//free(inputs);
	//free(weights);
	free_buffers();

	t_totalhw = (float)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("Time taken: %.10f seconds.\n\n", t_totalhw);

	printf("Comparing output to expected output...");
	FILE* fo = fopen("exp_output.dat", "r");
	if(!fo){ printf("Could not open exp_output.dat.\n"); return 1; }

	uint32_t ex;
	uint32_t discrp = 0;
	float accerr = 0;
	float temp;
	for(ex = 0; ex < len*filters; ++ex){
		fread(&temp, sizeof(float), 1, fo);
		if(temp != output[ex]){
			discrp++;
			accerr += (temp - output[ex]);
		}
	}

	fclose(fo);
	free(output);

	printf(" Done!\nNumber of discrepancies: %d\n", discrp);
	printf("Average error: %.20f\n", accerr / discrp);

	printf("\nTesting layer 1 on software\nLoading data...\n");
	output = (float*)calloc(len*filters, sizeof(float));

	float* in_data = (float*)malloc((3+(ks*ks)+len)*sizeof(float));
	float* in_part = (float*)calloc(len, sizeof(float));

	in_data[0] = ks; in_data[1] = h; in_data[2] = w;

	printf("Running.\n");
	t_start = clock();
	for(n = 0; n < filters; ++n){
		//printf("Filter %d...\n", n);
		memset(in_part, 0, len*sizeof(float));
		for(c = 0; c < channels; ++c){
			memcpy(in_data+3, weights+(n*channels*ks*ks)+(c*ks*ks), ks*ks*sizeof(float));
			memcpy(in_data+3+(ks*ks), inputs+(c*len), len*sizeof(float));
			hw_fcl(in_data, in_part, in_part);
		}
		memcpy(output+(n*len), in_part, len*sizeof(float));
	}
	t_end = clock();

	t_totalsw = (float)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("Time taken: %.10f seconds.\n\n", t_totalsw);

	printf("Comparing output to expected output...");
	fo = fopen("exp_output.dat", "r");
	if(!fo){ printf("Could not open exp_output.dat.\n"); return 1; }

	discrp = 0; accerr = 0;
	for(ex = 0; ex < len*filters; ++ex){
		fread(&temp, sizeof(float), 1, fo);
		if(temp != output[ex]){
			discrp++;
			accerr += (temp - output[ex]);
		}
	}

	fclose(fo);
	free(output);

	printf(" Done!\nNumber of discrepancies: %d\n", discrp);
	printf("Average error: %.20f\n", accerr / discrp);

	free(inputs);
	free(weights);
	unmap_hw_va();

	printf("\n\nHardware was %.10f times faster than software.\n",
		t_totalsw / t_totalhw);
	
	return 0;
}
