// Copyright 2022 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: conv1d_obi_to_sram_gnt.sv
// Author: Michele Caon
// Date: 06/12/2022
// Description: OBI to SRAM interface

module conv1d_obi_to_sram_gnt #(
  // OBI request type, expected to contain:
  //    logic           req     > request
  //    logic           we      > write enable
  //    logic [BEW-1:0] be      > byte enable
  //    logic  [AW-1:0] addr    > target address
  //    logic  [DW-1:0] wdata   > data to write
  parameter type obi_req_t = conv1d_obi_pkg::obi_req_t,
  // OBI response type, expected to contain:
  //    logic           gnt     > request accepted
  //    logic           rvalid  > read data is valid
  //    logic  [DW-1:0] rdata   > read data
  parameter type obi_resp_t = conv1d_obi_pkg::obi_resp_t,
  // SRAM request type, expected to contain:
  //    logic           req     > request
  //    logic           we      > write enable
  //    logic [BEW-1:0] be      > byte enable
  //    logic  [AW-1:0] addr    > target address
  //    logic  [DW-1:0] wdata   > data to write
  parameter type sram_req_t = conv1d_sram_pkg::sram_req_t,
  // SRAM response type, expected to contain:
  //    logic  [DW-1:0] rdata   > read data
  parameter type sram_rsp_t = conv1d_sram_pkg::sram_rsp_t,
  parameter int unsigned DELAY = 'd1  // SRAM read delay
) (
  input logic clk_i,
  input logic rst_ni,

  // OBI interface
  input  obi_req_t  obi_req_i,  // OBI bus request
  output obi_resp_t obi_rsp_o,  // OBI bus response

  // SRAM interface
  output sram_req_t sram_req_o,  // SRAM request
  input  logic      sram_gnt_i,  // SRAM grant
  input  sram_rsp_t sram_rsp_i   // SRAM response
);
  // INTERNAL SIGNALS
  // ----------------
  logic obi_rvalid[DELAY+1];

  // OBI rvalid delay chain
  // ----------------------
  // The OBI rvalid signal is asserted when the memory produces the output
  // data, that is a number of clock cycles equal to the memory latency after
  // the input request is accepted (i.e., OBI gnt is asserted).
  // NOTE: OBI expects the rvalid signal to be asserted for each request,
  //       including store request for which no data is provided by the slave.
  assign obi_rvalid[0] = obi_req_i.req;
  generate
    for (genvar i = 1; unsigned'(i) <= DELAY; i++) begin : gen_rvalid_delay
      always_ff @(posedge clk_i or negedge rst_ni) begin
        if (!rst_ni) begin
          obi_rvalid[i] <= 1'b0;
        end else begin
          obi_rvalid[i] <= obi_rvalid[i-1];
        end
      end
    end
  endgenerate

  // OUTPUT EVALUATION
  // -----------------

  // OBI request to SRAM request
  assign sram_req_o.req   = obi_req_i.req;
  assign sram_req_o.we    = obi_req_i.we;
  assign sram_req_o.be    = obi_req_i.be;
  assign sram_req_o.addr  = obi_req_i.addr;
  assign sram_req_o.wdata = obi_req_i.wdata;

  // SRAM response to OBI response
  assign obi_rsp_o.gnt    = sram_gnt_i;
  assign obi_rsp_o.rvalid = obi_rvalid[DELAY];
  assign obi_rsp_o.rdata  = sram_rsp_i.rdata;
endmodule
