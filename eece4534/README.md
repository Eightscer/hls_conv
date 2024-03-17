# Convolution Accelerator #

## EECE 4534 - Spring 2022 ##

## Developer: Oscar Kellner ##

### Overview ###

A convolution neural network (CNN or ConvNet) is a type of neural network that excels in processing multidimensional input data, particularly images. CNNs are often used in computer vision applications.

A CNN consists of multiple convolution and pooling layers that apply filters and 'convolve' the data. In a single convolution layer, filters (or kernels) are applied to each input channel through matrix multiplication, the output of which is sent as the input to the following layer (often a pooling layer).

The goal of this project is to implement a hardware-based accelerator for a forward convolution layer through the use of High Level Synthesis (HLS) using Vivado HLS, then compare the change in performance to the original software solution.

The framework used for this project is Darknet, which is an open-source neural network framework written in C, making it ideal for adaptation for HLS.

The scope of the project will start small; the inputs will initially be constrained to single channel (grayscale) 256x256 images with simple image filters. If time allows it, multi-channel inputs (colored images) with larger dimensions as well as greater exploration of optimization choices will be considered.

## Roadmap ##

1. Isolate the 'heart' of the `forward_convolutional_layer` function into its own independent function, later to be used for HLS, and verify that it still works by reincorporating it into the Darknet framework by modifying the source code.
2. Successfully synthesize the isolated function in Vivado HLS, making modifications if necessary. Describe bounds for the input data (e.g. 224 x 224 input image) and add AXI-Stream and DMA interfaces. Verify that the synthesized function works by interacting with it via the PYNQ libraries in Python.
3. Find how to interface with the generated hardware via a userspace driver by using a simple example (e.g. a function synthesized in HLS that adds 2 numbers and stores the result).
4. If successful with #3, integrate the synthesized accelerator into a working userspace driver.
5. Modify the Darknet source once again to interface with the new userspace driver, and verify that it outputs the same results as the unmodified version.
