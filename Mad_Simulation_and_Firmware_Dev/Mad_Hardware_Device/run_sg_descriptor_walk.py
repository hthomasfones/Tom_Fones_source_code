#!/usr/bin/env python3
"""Extract production SG walker verbatim and exercise three descriptors in XSim."""
import pathlib, re, subprocess, sys
src = pathlib.Path(sys.argv[1] if len(sys.argv)>1 else 'maddevice0.srcs/sources_1/new/pcie_device_portal.v')
text = src.read_text()
start = text.index('  localparam [2:0] SG_IDLE')
end = text.index(' pio_rx_engine #(', start)
block = text[start:end]
# Extract the last always block only: the source region is the walker itself.
# Preserve the production implementation, not a rewritten state machine.
block = block[:block.rfind('\n  end')+6]
if not all(x in block for x in ('SG_WAIT:', 'SG_DONE:', 'sg_desc_index <= sg_cdpp_offset[12:5]')):
    raise SystemExit('SG walker structure changed; refusing to generate a stale test')
header = r'''`timescale 1ns/1ps
module tb_portal_sg_descriptor_walk;
  reg user_clk = 0;
  always #5 user_clk = ~user_clk;
  reg reset_n = 0;
  reg dma_go_pulse = 0;
  reg chained_dma_enabled = 0;
  reg sg_d2h_complete = 0, sg_h2d_complete = 0;
  localparam integer MAD_SGDMA_MAX_SECTORS = 64;
  localparam [63:0] TB_BAR0_BASE = 64'h0000_0000_8540_8000;
  localparam [63:0] TB_BCDPP = TB_BAR0_BASE + 64'h60;
  reg [63:0] bcdpp = 0;
  reg [63:0] SgHostAddr[0:2], SgCDPP[0:2];
  reg [31:0] SgDevDataOfst[0:2], SgDmaCntl[0:2], SgDXBC[0:2], SgReg20[0:2];
  wire [63:0] sg_host_addr = SgHostAddr[sg_desc_index];
  wire [31:0] sg_dev_data_ofst = SgDevDataOfst[sg_desc_index];
  wire [31:0] sg_dma_cntl = SgDmaCntl[sg_desc_index];
  wire [31:0] sg_dxbc = SgDXBC[sg_desc_index];
  wire [31:0] sg_reg20 = SgReg20[sg_desc_index];
  wire [63:0] sg_cdpp = SgCDPP[sg_desc_index];
  integer starts, completions, errors, cycles, expected_index;
  reg expected_h2d;
'''
footer = r'''
  task tick;
    begin @(posedge user_clk); #1; end
  endtask
  task check;
    input condition;
    input [255:0] message;
    begin if (condition !== 1'b1) begin
      $display("FAIL: %0s at t=%0t state=%0d idx=%0d", message, $time, sg_state, sg_desc_index);
      errors = errors + 1; $finish;
    end end
  endtask
  task run_chain;
    input h2d;
    integer j, k;
    begin
      expected_h2d = h2d;
      starts = 0; completions = 0;
      // This models driver programming of BCDPP and the BAR0 descriptor array.
      bcdpp = TB_BCDPP;
      for (j=0; j<3; j=j+1) begin
        SgHostAddr[j] = 64'h0000_0001_0000_0000 + (j*64'h1000);
        SgDevDataOfst[j] = j*512;
        SgDmaCntl[j] = h2d ? 32'h8000_0000 : 32'h0;
        SgDXBC[j] = 512;
        SgReg20[j] = 0;
        SgCDPP[j] = (j == 2) ? 64'd1 : (TB_BCDPP + (j+1)*32);
      end
      chained_dma_enabled = 1;
      @(negedge user_clk); dma_go_pulse = 1;
      @(negedge user_clk); dma_go_pulse = 0;
      for (j=0; j<3; j=j+1) begin
        k=0;
        while (!(sg_state == SG_WAIT && sg_desc_index == j &&
                 (h2d ? sg_h2d_start : sg_d2h_start))) begin
          tick(); k=k+1;
          check(k < 40, "timeout waiting for descriptor start");
          check(sg_state != SG_ERROR, "unexpected SG_ERROR");
        end
        check(sg_host_addr_q == SgHostAddr[j], "host address mismatch");
        check(sg_dev_data_ofst_q == SgDevDataOfst[j], "device offset mismatch");
        check(sg_dxbc_q == 512, "DXBC mismatch");
        check(sg_cdpp_q == SgCDPP[j], "CDPP mismatch");
        check((h2d ? !sg_d2h_start : !sg_h2d_start), "wrong DMA direction");
        starts=starts+1;
        // Hold WAIT for five clocks; it must not advance without completion.
        repeat (5) begin
          tick();
          check(sg_state == SG_WAIT && sg_desc_index == j, "advanced without completion");
        end
        @(negedge user_clk);
        if (h2d) sg_h2d_complete = 1;
        else sg_d2h_complete = 1;
        tick();
        @(negedge user_clk);
        sg_h2d_complete = 0; sg_d2h_complete = 0;
        completions=completions+1;
        check(sg_total_bytes == (j+1)*512, "byte count mismatch");
      end
      k=0;
      while (!sg_dma_complete) begin
        tick(); k=k+1;
        check(k < 20, "timeout waiting for SG completion");
        check(sg_state != SG_ERROR, "SG_ERROR after final descriptor");
      end
      check(sg_state == SG_IDLE, "did not return to SG_IDLE");
      check(starts == 3 && completions == 3, "incorrect descriptor count");
      check(sg_total_bytes == 1536, "incorrect final byte total");
      tick();
      check(!sg_dma_complete, "completion pulse longer than one cycle");
      $display("PASS: %0s three-descriptor walk; bytes=%0d", h2d ? "H2D" : "D2H", sg_total_bytes);
      chained_dma_enabled = 0;
      repeat (3) tick();
    end
  endtask
  initial begin
    errors=0;
    repeat (5) tick();
    check(bcdpp == 0, "BCDPP model must reset to zero");
    @(negedge user_clk); reset_n = 1;
    tick();
    run_chain(0);
    run_chain(1);
    $display("PASS: all SG descriptor walker checks");
    $finish;
  end
  initial begin #100000; $display("FAIL: global timeout"); $finish; end
endmodule
'''
out=pathlib.Path('tb_portal_sg_descriptor_walk.v')
out.write_text(header+block+'\n'+footer)
print('Generated',out,'from production walker in',src)
for cmd in (['xvlog',str(out)], ['xelab','tb_portal_sg_descriptor_walk','-s','sg_walk_sim'], ['xsim','sg_walk_sim','-runall']):
    print('+',' '.join(cmd),flush=True)
    subprocess.run(cmd,check=True)
