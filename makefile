# Copyright 2024 Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Luigi Giuffrida
# Date: 03/10/2024
# Description: Top-level makefile for conv1d accelerator.

# ----- CONFIGURATION ----- #

# Global configuration
ROOT_DIR			:= $(realpath .)
BUILD_DIR 			:= build

# RTL simulation
LOG_LEVEL			?= LOG_MEDIUM

# FUSESOC and Python values (default)
ifndef CONDA_DEFAULT_ENV
$(info USING VENV)
FUSESOC = ./.venv/bin/fusesoc
PYTHON  = ./.venv/bin/python
else
$(info USING MINICONDA $(CONDA_DEFAULT_ENV))
FUSESOC := $(shell which fusesoc)
PYTHON  := $(shell which python)
endif

APP_PARAMS 		?= --in_len 8 --in_ch 4 --k_len 3 --k_num 2 --stride 1 --padding 0

# ----- TARGETS ----- #

## Default target
.PHONY: all
all: lint format verilator-sim

## @section Verilator RTL simulation

## Build simulation model (do not launch simulation)
.PHONY: verilator-build
verilator-build:
	$(FUSESOC) run --no-export --target sim --tool verilator --build $(FUSESOC_FLAGS) polito:isa-lab:conv1d \
		$(FUSESOC_ARGS)

## Build and run simulation
.PHONY: verilator-sim
verilator-sim: | verilator-build
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) polito:isa-lab:conv1d \
		--log_level $(LOG_LEVEL)
		$(FUSESOC_ARGS)

## Run simulation
.PHONY: verilator-run
verilator-run:
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) polito:isa-lab:conv1d \
		--log_level $(LOG_LEVEL)
		$(FUSESOC_ARGS)

## Generate input data
.PHONY: gen-data
gen-data:
	$(MAKE) -C tb $(APP_PARAMS)

# Open waveform dump with GTKWave
.PHONY: waves
waves: $(BUILD_DIR)/sim-common/waves.fst | .check-gtkwave
	gtkwave -a tb/waves.gtkw $<

## @section Hardware generation

## Generate configuration registers
.PHONY: gen-registers
gen-registers: hw/control-reg/data/control_reg.hjson
	bash hw/control-reg/gen-control-reg.sh
	$(FUSESOC) run --no-export --target format polito:isa-lab:conv1d

## Perform static analysis with Verible
.PHONY: lint
lint:
	$(FUSESOC) run --no-export --target lint polito:isa-lab:conv1d

## Format generated code with Verible
.PHONY: format
format:
	$(FUSESOC) run --no-export --target format polito:isa-lab:conv1d

## @section Utilities

## Update vendored IPs
.PHONY: vendor-update
vendor-update:
	@echo "Updating vendored IPs..."
	find hw/vendor -mindepth 1 -maxdepth 1 -type f -name "*.vendor.hjson" -exec ./util/vendor.py -vU {} \;

# ----- HELPERS ----- #

## @section Helpers

# Check if GTKWave is available
.PHONY: .check-gtkwave
.check-gtkwave:
	@if [ ! `which gtkwave` ]; then \
	printf -- "### ERROR: 'gtkwave' is not in PATH.\n" >&2; \
	exit 1; fi

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) tb/data.h tb/data.c