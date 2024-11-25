// Copyright 2024 Politecnico di Torino.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 2.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-2.0. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// File: conv1d.cpp
// Author(s):
//   Luigi Giuffrida
// Date: 08/11/2024
// Description: TB for the OBI CONV1D accelerator

#include <iostream>
#include <getopt.h>
#include <random>
#include <time.h>

// Verilator libraries
#include <verilated.h>
#include <verilated_fst_c.h>

// DUT header
#include "Vconv1d_tb_wrapper.h"
#include "conv1d_control_reg.h"

// Testbench components
#include "tb_macros.hh"
#include "tb_components.hh"

// Defines
// -------
#define FST_FILENAME "logs/waves.fst"
#define END_OF_RESET_TIME 5
#define MAX_SIM_CYCLES 2e6
#define MAX_SIM_TIME (MAX_SIM_CYCLES * 2)
#define WATCHDOG_TIMEOUT 100 // cycles to wait for a program step to complete
#define END_OF_TEST_TIMEOUT 10 // cycles between done assertion and simulation end
#define RUN_CYCLES 500

// Generate clock and reset
void clkGen(Vconv1d_tb_wrapper *dut);
void rstDut(Vconv1d_tb_wrapper *dut, vluint64_t sim_time);

// Generate OBI transactions
ObiReqTx *genObiWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t be);
ObiReqTx *genObiReadReqTx(const vluint32_t addr_offs);
RegReqTx *genRegWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t wstrb);
RegReqTx *genRegReadReqTx(const vluint32_t addr_offs);

// Run a number of cycles
void runCycles(unsigned int ncycles, Vconv1d_tb_wrapper *dut, uint8_t gen_waves, VerilatedFstC *trace);

// Global variables
vluint64_t sim_cycles = 0;
TbLogger logger;    // testbench logger

int main(int argc, char *argv[])
{
    // Define command-line options
    const option longopts[] = {
        {"log_level", required_argument, NULL, 'l'},
        {"gen_waves", required_argument, NULL, 'w'},
        {"seed", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    // Process command-line options
    // ----------------------------
    int opt; // current option
    int prg_seed = time(NULL);
    bool gen_waves = true;
    while ((opt = getopt_long(argc, argv, "l:w:", longopts, NULL)) >= 0)
    {
        switch (opt)
        {
        case 'l': // set the log level
            logger.setLogLvl(optarg);
            TB_CONFIG("Log level set to %s", optarg);
            break;
        case 'w': // generate waves
            if (!strcmp(optarg, "true")) {
                gen_waves = 1;
                TB_CONFIG("Waves enabled");
            }
            else {
                gen_waves = 0;
                TB_CONFIG("Waves disabled");
            }
            break;
        case 's': // set the seed
            prg_seed = atoi(optarg);
            TB_CONFIG("Seed set to %d", prg_seed);
            break;
        default:
            TB_ERR("ERROR: unrecognised option %c.\n", opt);
            exit(EXIT_FAILURE);
        }
    }

    // Create Verilator simulation context
    VerilatedContext *cntx = new VerilatedContext;

    // Pass simulation context to the logger
    logger.setSimContext(cntx);

    if (gen_waves)
    {
        Verilated::mkdir("logs");
        cntx->traceEverOn(true);
    }

    // Instantiate DUT
    Vconv1d_tb_wrapper *dut = new Vconv1d_tb_wrapper(cntx);

    // Set the file to store the waveforms in
    VerilatedFstC *trace = NULL;
    if (gen_waves)
    {
        trace = new VerilatedFstC;
        dut->trace(trace, 10);
        trace->open(FST_FILENAME);
    }

    // TB components
    Drv *drv = new Drv(dut);
    Scb *scb = new Scb();
    ReqMonitor *reqMon = new ReqMonitor(dut, scb);
    RspMonitor *rspMon = new RspMonitor(dut, scb);

    // Initialize PRG
    srand(prg_seed);

    // Simulation program
    // ------------------
    // TODO: Define simulation program variables
    ObiReqTx *obi_req = NULL;
    ObiReqTx *obi_check_req = NULL;
    vluint32_t obi_rdata = 0;
    vluint32_t obi_check_data = 0;
    bool obi_accepted = 0;

    RegReqTx *reg_req = NULL;
    RegReqTx *reg_check_req = NULL;
    vluint32_t reg_rdata = 0;
    vluint32_t reg_check_data = 0;
    bool reg_accepted = 0;

    bool irq_received = 0;

    bool end_of_test = false;
    unsigned int exit_timer = 0;
    unsigned int watchdog = 0;
    unsigned int prev_step_cnt = 0;
    unsigned int step_cnt = 0;

    TB_LOG(LOG_LOW, "Starting simulation...");
    while (!cntx->gotFinish() && cntx->time() < MAX_SIM_TIME)
    {
        // Generate clock and reset
        rstDut(dut, cntx->time());
        clkGen(dut);

        // Evaluate simulation step
        dut->eval();

        if (dut->clk_i == 1 && cntx->time() > END_OF_RESET_TIME)
        {
            
            // TODO: implement the simulation program

            
            // Drive DUT inputs
            drv->drive(obi_req, reg_req);
            delete obi_req;
            delete reg_req;
            obi_req = NULL;
            reg_req = NULL;

            // Update input signals
            dut->eval();

            // Monitor DUT signals
            reqMon->monitor();
            rspMon->monitor();
            
            irq_received = rspMon->irq();
            obi_accepted = reqMon->acceptedObi();
            reg_accepted = reqMon->acceptedReg();

            // Schedule checks
            if (reg_accepted) {
                reg_rdata = rspMon->getRegData();
                if (reg_check_req) scb->scheduleRegCheck(reg_check_data);
            }
            if (rspMon->isDataReadyObi()) obi_rdata = rspMon->getObiData();
            if (obi_accepted && obi_check_req) scb->scheduleObiCheck(obi_check_data);

            // Trigger scheduled checks
            if (scb->checkData() != 0) end_of_test = true;

            // Check for exit conditions
            if (prev_step_cnt != step_cnt) watchdog = 0;
            else watchdog++;
            if (watchdog > WATCHDOG_TIMEOUT) {
                TB_WARN("Watchdog timeout reached: terminating simulation.");
                scb->notifyError();
                break;
            }
            prev_step_cnt = step_cnt;
            if (end_of_test)
            {
                if (exit_timer++ == END_OF_TEST_TIMEOUT) {
                    TB_LOG(LOG_MEDIUM, "End of simulation reached: terminating.");
                    break;
                }
            }
        }

        // Dump waveforms and advance simulation time
        if (gen_waves) trace->dump(cntx->time());
        if (dut->clk_i == 1) sim_cycles++;
        cntx->timeInc(1);
    }

    // Simulation complete
    dut->final();

    // Print simulation summary
    if (scb->getErrNum() > 0)
    {
        TB_ERR("CHECKS FAILED > errors: %u/%u", scb->getErrNum(), scb->getTxNum());
        if (gen_waves) trace->close();
        exit(EXIT_SUCCESS);
    }
    else if (!scb->isDone())
    {
        TB_ERR("CHECKS PENDING > errors: %u/%u", scb->getErrNum(), scb->getTxNum());
        if (gen_waves) trace->close();
        exit(EXIT_SUCCESS);
    }
    TB_SUCCESS(LOG_LOW, "CHECKS PASSED > errors: %u (checked %u transactions)", scb->getErrNum(), scb->getTxNum());

    // Clean up and exit
    if (gen_waves) trace->close();
    delete dut;
    delete cntx;
    delete obi_req;
    delete reg_req;

    return 0;
}

void clkGen(Vconv1d_tb_wrapper *dut)
{
    dut->clk_i ^= 1;
}

void rstDut(Vconv1d_tb_wrapper *dut, vluint64_t sim_time)
{
    dut->rst_ni = 1;
    if (sim_time > 1 && sim_time < END_OF_RESET_TIME)
    {
        dut->rst_ni = 0;
    }
}

void runCycles(unsigned int ncycles, Vconv1d_tb_wrapper *dut, uint8_t gen_waves, VerilatedFstC *trace)
{
    VerilatedContext *cntx = dut->contextp();
    for (unsigned int i = 0; i < (2 * ncycles); i++)
    {
        // Generate clock
        clkGen(dut);

        // Evaluate the DUT
        dut->eval();

        // Save waveforms
        if (gen_waves)
            trace->dump(cntx->time());
        if (dut->clk_i == 1)
            sim_cycles++;
        cntx->timeInc(1);
    }
}

// Issue write OBI transaction
ObiReqTx *genObiWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t be)
{
    ObiReqTx *req = new ObiReqTx;

    // OBI write request
    req->obi_req.req = 1;
    req->obi_req.we = 1;
    req->obi_req.be = be;
    req->obi_req.addr = addr_offs;
    req->obi_req.wdata = wdata;

    return req;
}

// Issue read OBI transaction
ObiReqTx *genObiReadReqTx(const vluint32_t addr_offs)
{
    ObiReqTx *req = new ObiReqTx;

    // OBI read request
    req->obi_req.req = 1;
    req->obi_req.we = 0;
    req->obi_req.be = 0xf;
    req->obi_req.addr = addr_offs;
    req->obi_req.wdata = 0;

    return req;
}

// Issue write register interface transaction
RegReqTx *genRegWriteReqTx(const vluint32_t addr_offs, const vluint32_t wdata, vluint8_t wstrb)
{
    RegReqTx *req = new RegReqTx;

    // OBI write request
    req->reg_req.valid = 1;
    req->reg_req.write = 1;
    req->reg_req.wstrb = wstrb;
    req->reg_req.addr = addr_offs;
    req->reg_req.wdata = wdata;

    return req;
}

// Issue read register interface transaction
RegReqTx *genRegReadReqTx(const vluint32_t addr_offs)
{
    RegReqTx *req = new RegReqTx;

    // OBI read request
    req->reg_req.valid = 1;
    req->reg_req.write = 0;
    req->reg_req.wstrb = 0xf;
    req->reg_req.addr = addr_offs;
    req->reg_req.wdata = 0;

    return req;
}
