# 1D multichannel convolution accelerator

This repository contains the code for a 1D multichannel convolution accelerator.

## Description

You should design and implement the accelerator as described in the assignment.

To guide you in the implementation, we provide you with a set of files that you should use as a starting point. You can find these files in the following directories:

- `hw/` contains the source files for the accelerator.
- `sw/` contains the C code with the status and control registers of the accelerator.
- `tb/` contains the testbench for the accelerator.
- `util/` contains the vendor script to get the dependencies.

In the files provided, you will find some TODOs that you should complete to implement the accelerator.

## Makefile description

The Makefile provided in the root directory of the repository contains the following targets:

- `verilator-build` compiles the testbench and the accelerator HDL code using __Verilator__.
- `verilator-run` compiles and runs the testbench using __Verilator__.
- `verilator-run` runs the testbench using __Verilator__.
- `gen-data` generates the input data for the accelerator.
- `waves` opens *gtkwave* to visualize the waveforms.
- `gen-registers` generates the register map for the accelerator.
- `lint` runs the linter on the accelerator code.
- `format` formats the accelerator code.
- `vendor-update` updates the vendor dependencies.
- `clean` removes all the generated files.

## Running the simulation

You can decide how to verify the correctness of the accelerator. You can use the provided testbench or write your own.

> __Hint:__ The provided testbench simulates the OBI protocol, so you can provide the input data to the accelerator by issuing write transactions to the accelerator memory and configure the accelerator by writing to the status and control registers.

You can also use the provided input data or generate your own. To generate the input data, you can use the `gen-data` target, it will call the makefile in the `tb/` directory to call the `datagen.py` script, it accepts the following arguments:

- `--in_ch` to specify the number of input channels.
- `--in_len` to specify the length of the input data.
- `--k_len` to specify the length of the kernel.
- `--k_num` to specify the number of filters.
- `--stride` to specify the stride of the convolution.
- `--padding` to specify the padding of the convolution.
- `--seed` to specify the seed for the random number generator (can be usefull to use always the same seed and get deterministic numbers).


`datagen.py` will generate a file with the input data and the golden result in the `tb/` directory. You can include the file in your __Verilator__ testbench to read the input data and compare the result. 

## Once you are done with the implementation

Use Vendor to import the accelerator into the provided platform and integrate it with the provided software.

