#ifndef HWFCL_DMA_H
#define HWFCL_DMA_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>

#include "libcma.h"
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <time.h>

#define DMA0_ADDR 0x40400000
#define DMA1_ADDR 0x40410000

#define MM2S_CONTROL_REGISTER 0x00
#define MM2S_STATUS_REGISTER 0x04
#define MM2S_START_ADDRESS 0x18
#define MM2S_LENGTH 0x28

#define S2MM_CONTROL_REGISTER 0x30
#define S2MM_STATUS_REGISTER 0x34
#define S2MM_DESTINATION_ADDRESS 0x48
#define S2MM_LENGTH 0x58

#define HWFCL_ADDR 0x43C00000

#define INP_LENGTH 0x10
#define OUT_LENGTH 0x18

#define MAPPING_SIZE 0xFFFF

static int hw_loaded = 0;

static unsigned int* dma0_va;
static unsigned int* dma1_va;
static unsigned int* hwfcl_va;
static int fd;

static int allocated = 0;
float* input_stream;
float* partial_sum;

void alloc_buffers(unsigned int ks, unsigned int h, unsigned int w){
	unsigned int len = h*w;
	unsigned int inl = 3 + (ks*ks) + len;
	unsigned int sof = sizeof(float);
	input_stream = (float*)cma_alloc(inl*sof, 0);
	partial_sum = (float*)cma_alloc(len*sof, 0);
	allocated = 1;
}

void free_buffers(){
	cma_free(input_stream);
	cma_free(partial_sum);
	allocated = 0;
}

int map_hw_va(){
	printf("Attempting to mmap() hardware...\n");
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	
	dma0_va = mmap(NULL, MAPPING_SIZE,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, DMA0_ADDR);
	if(dma0_va == MAP_FAILED){ printf("Failed to map dma0\n"); return 1; }
	
	dma1_va = mmap(NULL, MAPPING_SIZE,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, DMA1_ADDR);
	if(dma0_va == MAP_FAILED){ printf("Failed to map dma1\n"); return 2; }
		
	hwfcl_va = mmap(NULL, MAPPING_SIZE,
		PROT_READ | PROT_WRITE, MAP_SHARED, fd, HWFCL_ADDR);
	if(dma0_va == MAP_FAILED){ printf("Failed to map hwfcl\n"); return 3; }
	
	return 0;
}

int unmap_hw_va(){
	//printf("Attempting to munmap() hardware...\n");
	printf("Attempting to close /dev/mem\n");
	int err;

	// Warnings here?
	// "Passing argument 1 of munmap discards volatile qualifier [...]"
	
	err = munmap(dma0_va, MAPPING_SIZE);
	if(err){ printf("Failed to unmap dma0\n"); return err;}

	err = munmap(dma1_va, MAPPING_SIZE);
	if(err){ printf("Failed to unmap dma1\n"); return err;}

	err = munmap(hwfcl_va, MAPPING_SIZE);
	if(err){ printf("Failed to unmap hwfcl\n"); return err;}
	
	err = close(fd);
	if(err){ printf("Failed to close /dev/mem (?)\n"); return err;}

	hw_loaded = 0;
	return 0;
}

void load_hw_py(){
	int pid = fork();
	if(!pid){
		char* py_args[4] = {"sudo", "python3", "ip_overlay.py", 0};
		execvp(py_args[0], py_args);
		perror("Executing ip_overlay.py");
	}
	waitpid(pid, NULL, 0);
}

unsigned int fcl_set(unsigned int* fcl_virtual_address, int offset, unsigned int value) {
	fcl_virtual_address[offset>>2] = value;
}

unsigned int dma_set(unsigned int* dma_virtual_address, int offset, unsigned int value) {
    dma_virtual_address[offset>>2] = value;
}

unsigned int dma_get(unsigned int* dma_virtual_address, int offset) {
    return dma_virtual_address[offset>>2];
}

void dma_s2mm_status(unsigned int* dma_virtual_address, unsigned int id) {
    unsigned int status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    printf("Stream to memory-mapped status (0x%08x@0x%02x)\nDMA%d:",
    	status, S2MM_STATUS_REGISTER, id);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

void dma_mm2s_status(unsigned int* dma_virtual_address, unsigned int id) {
    unsigned int status = dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    printf("Memory-mapped to stream status (0x%08x@0x%02x)\nDMA%d:",
    	status, MM2S_STATUS_REGISTER, id);
    if (status & 0x00000001) printf(" halted"); else printf(" running");
    if (status & 0x00000002) printf(" idle");
    if (status & 0x00000008) printf(" SGIncld");
    if (status & 0x00000010) printf(" DMAIntErr");
    if (status & 0x00000020) printf(" DMASlvErr");
    if (status & 0x00000040) printf(" DMADecErr");
    if (status & 0x00000100) printf(" SGIntErr");
    if (status & 0x00000200) printf(" SGSlvErr");
    if (status & 0x00000400) printf(" SGDecErr");
    if (status & 0x00001000) printf(" IOC_Irq");
    if (status & 0x00002000) printf(" Dly_Irq");
    if (status & 0x00004000) printf(" Err_Irq");
    printf("\n");
}

int dma_mm2s_sync(unsigned int* dma_virtual_address) {
    unsigned int mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    while(!(mm2s_status & 1<<12) || !(mm2s_status & 1<<1) ){
        //dma_s2mm_status(dma_virtual_address, 0);
        //dma_mm2s_status(dma_virtual_address, 0);

        mm2s_status =  dma_get(dma_virtual_address, MM2S_STATUS_REGISTER);
    }
}

int dma_s2mm_sync(unsigned int* dma_virtual_address) {
    unsigned int s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    while(!(s2mm_status & 1<<12) || !(s2mm_status & 1<<1)){
        //dma_s2mm_status(dma_virtual_address, 0);
        //dma_mm2s_status(dma_virtual_address, 0);

        s2mm_status = dma_get(dma_virtual_address, S2MM_STATUS_REGISTER);
    }
}

void do_the_thing(
	float* inputs, float* weights, float* output,
	unsigned int ks, unsigned int h, unsigned int w
	//unsigned int n, unsigned int c
){

	// Load HW
	if(!hw_loaded){
		printf("Loading hardware...\n");
		load_hw_py();
		if(map_hw_va()){
			return;
		}
		hw_loaded = 1;
	}

	// Init control variables
	unsigned int out_len = h*w;
	unsigned int inp_len = 3 + (ks*ks) + out_len;
	unsigned int sof = sizeof(float);
	unsigned int ip = 0;
	unsigned int d = 0;
	unsigned int e = 0;
	unsigned int o = 0;

	// Allocate buffers
	//float* input_stream = (float*)cma_alloc(inp_len*sof, 0);
	//float* partial_sum = (float*)cma_alloc(out_len*sof, 0);

	if(!allocated){
		printf("Memory not allocated! Skipping...\n");
		return;
	}

	input_stream[ip++] = ks;
	input_stream[ip++] = h;
	input_stream[ip++] = w;

	// Fill buffers
	unsigned int i;
	for(i = 0; i < ks*ks; ++i) input_stream[ip++] = weights[e++];
	for(i = 0; i < out_len; ++i) input_stream[ip++] = inputs[d++];

	// For partial fraction testing
	for(i = 0; i < out_len; ++i) partial_sum[i] = output[i];

	// Normal
	//for(i = 0; i < out_len; ++i) partial_sum[i] = 0;

	// Start HWFCL
	//printf("Starting HWFCL IP\n");
	fcl_set(hwfcl_va, INP_LENGTH, inp_len);
	fcl_set(hwfcl_va, OUT_LENGTH, inp_len);
	hwfcl_va[0x00] = 0x81;

	// Start DMAs

	// Stop
	//printf("Stopping DMAs\n");
	dma_set(dma1_va, MM2S_CONTROL_REGISTER, 0);
	dma_set(dma0_va, MM2S_CONTROL_REGISTER, 0);
	dma_set(dma0_va, S2MM_CONTROL_REGISTER, 0);
	// Reset
	//printf("Resetting DMAs\n");
	dma_set(dma1_va, MM2S_CONTROL_REGISTER, 4);
	dma_set(dma0_va, MM2S_CONTROL_REGISTER, 4);
	dma_set(dma0_va, S2MM_CONTROL_REGISTER, 4);
	// Start (all interrupts masked)
	//printf("Starting DMAs\n");
	dma_set(dma1_va, MM2S_CONTROL_REGISTER, 0x0001);
	dma_set(dma0_va, MM2S_CONTROL_REGISTER, 0x0001);
	dma_set(dma0_va, S2MM_CONTROL_REGISTER, 0x0001);

	// Set addresses for DMA
	//printf("Setting buffer addresses\n");
	dma_set(dma1_va, MM2S_START_ADDRESS, cma_get_phy_addr((void*)input_stream));
	//dma_mm2s_status(dma1_va, 1);
	dma_set(dma0_va, MM2S_START_ADDRESS, cma_get_phy_addr((void*)partial_sum));
	//dma_mm2s_status(dma0_va, 0);
	dma_set(dma0_va, S2MM_DESTINATION_ADDRESS, cma_get_phy_addr((void*)partial_sum));
	//dma_s2mm_status(dma0_va, 0);

	// Set transfer lengths for DMA
	//printf("Setting transfer lengths\n");
	dma_set(dma0_va, S2MM_LENGTH, out_len*sof);
	//dma_s2mm_status(dma0_va, 0);
	dma_set(dma0_va, MM2S_LENGTH, out_len*sof);
	//dma_mm2s_status(dma0_va, 0);
	dma_set(dma1_va, MM2S_LENGTH, inp_len*sof);
	//dma_mm2s_status(dma1_va, 1);

	// Sync DMA
	//printf("Syncing DMAs\n");
	dma_mm2s_sync(dma1_va);
	dma_mm2s_sync(dma0_va);
	dma_s2mm_sync(dma0_va);

	// Transfer data from buffer to output
	//printf("Copying output buffer to output array\n");
	for(i = 0; i < out_len; ++i) output[o++] = partial_sum[i];

	// Stop HWFCL
	hwfcl_va[0x00] = 0x00;

	// Free buffers
	//cma_free(input_stream);
	//cma_free(partial_sum);
}

#endif
