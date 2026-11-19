`timescale 1ns/1ps
module tb_portal_sg_descriptor_walk;
  reg user_clk = 0;
  always #5 user_clk = ~user_clk;
  reg reset_n = 0;
  reg dma_go_pulse = 0;
  reg chained_dma_enabled = 0;
  reg sg_d2h_complete = 0, sg_h2d_complete = 0;
  reg [7:0] sg_desc_index;
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
  localparam [2:0] SG_IDLE  = 3'd0;
  localparam [2:0] SG_LOAD  = 3'd1;
  localparam [2:0] SG_LATCH = 3'd2;
  localparam [2:0] SG_START = 3'd3;
  localparam [2:0] SG_WAIT  = 3'd4;
  localparam [2:0] SG_ERROR = 3'd5;
  localparam [2:0] SG_DONE  = 3'd6;
  
  localparam [63:0] SG_CDPP_END = 64'h0000_0000_0000_0001;
    
  reg [2:0] sg_state;

  reg [63:0] sg_host_addr_q;
  reg [31:0] sg_dev_data_ofst_q;
  reg [31:0] sg_dma_cntl_q;
  reg [31:0] sg_dxbc_q;
  reg [31:0] sg_reg20_q;
  reg [63:0] sg_cdpp_q;

  reg [63:0] sg_current_ptr;
  reg [31:0] sg_total_bytes;
  
  reg        sg_dma_begin;
  reg        sg_d2h_start;
  reg        sg_h2d_start;
  reg        sg_dma_complete;

  // BCDPP is the host-programmed address of descriptor 0.
  // Each following SG descriptor occupies 32 bytes.
  // Convert CDPP offset relative to BCDPP into descriptor index.
  wire [63:0] sg_desc_limit = bcdpp + (MAD_SGDMA_MAX_SECTORS * 64'd32);

  wire [63:0] sg_cdpp_offset = sg_cdpp_q - bcdpp;
  wire        sg_descriptor_complete = sg_dma_cntl_q[31] ? 
                                       sg_h2d_complete : sg_d2h_complete;
  wire sg_dma_h2d;
  assign sg_dma_h2d = sg_dma_cntl_q[31];                                       
  
  always @(posedge user_clk) begin
    if (!reset_n) begin
      sg_state           <= SG_IDLE;
      sg_desc_index      <= 8'd0;

      sg_host_addr_q     <= 64'd0;
      sg_dev_data_ofst_q <= 32'd0;
      sg_dma_cntl_q      <= 32'd0;
      sg_dxbc_q          <= 32'd0;
      sg_reg20_q         <= 32'd0;
      sg_cdpp_q          <= 64'd0;

      sg_current_ptr     <= 64'd0;
      sg_total_bytes     <= 32'd0;
      sg_d2h_start       <= 1'b0;
      sg_h2d_start       <= 1'b0;
      sg_dma_begin       <= 1'b0;
      sg_dma_complete    <= 1'b0;
    end
    else begin
        sg_d2h_start    <= 1'b0;
        sg_h2d_start    <= 1'b0;
        sg_dma_begin    <= 1'b0;
        sg_dma_complete <= 1'b0;
        case (sg_state)
        SG_IDLE: begin

          if (dma_go_pulse && chained_dma_enabled) begin
              sg_dma_begin <= 1'b1; 
              sg_current_ptr <= bcdpp;
              sg_total_bytes <= 32'd0;

          // BCDPP identifies descriptor 0 of the host-programmed
          // SG descriptor array.
          if ((bcdpp != 64'd0) && (bcdpp != SG_CDPP_END)) begin
              sg_desc_index <= 8'd0;
              sg_state      <= SG_LOAD;
          end
          else begin
              sg_state <= SG_ERROR;
          end
        end
      end  

        SG_LOAD: begin
          // sg_desc_index changed on the preceding clock.
          // Give the descriptor outputs one full cycle to settle.
          sg_state <= SG_LATCH;
        end

        SG_LATCH: begin
          sg_host_addr_q     <= sg_host_addr;
          sg_dev_data_ofst_q <= sg_dev_data_ofst;
          sg_dma_cntl_q      <= sg_dma_cntl;
          sg_dxbc_q          <= sg_dxbc;
          sg_reg20_q         <= sg_reg20;
          sg_cdpp_q          <= sg_cdpp;

          sg_state           <= SG_START;
        end

        SG_START: begin
            // DmaCntl bit 31 selects host-to-device.
            if (sg_dma_cntl_q[31])
                sg_h2d_start <= 1'b1;
            else
                sg_d2h_start <= 1'b1;

            sg_state <= SG_WAIT;
         end

        SG_WAIT: begin
            // D2H SG descriptor completed.
            //if (!sg_dma_cntl_q[31] && sg_d2h_complete) begin
            if (sg_descriptor_complete) begin
                // Account for this completed descriptor.
                sg_total_bytes <= sg_total_bytes + sg_dxbc_q;

                // CDPP == 1 marks the final descriptor.
                if (sg_cdpp_q == SG_CDPP_END) begin
                   sg_state <= SG_DONE;
                end

                // Otherwise CDPP must point to another valid,
                // 32-byte-aligned descriptor within the BAR0 SG array.
                else if ((sg_cdpp_q >= bcdpp) && (sg_cdpp_q < sg_desc_limit) &&
                        (((sg_cdpp_q - bcdpp) & 64'h0000_0000_0000_001F) == 64'd0)) begin
                     sg_current_ptr <= sg_cdpp_q;

                     // Convert CDPP offset relative to BCDPP into descriptor index.
                     sg_desc_index <= sg_cdpp_offset[12:5];

                     // Select and latch the next descriptor.
                     sg_state <= SG_LOAD;
                 end
                 else begin
                     sg_state <= SG_ERROR;
                 end
             end
         end
        SG_DONE: begin
             // sg_total_bytes now contains the sum of all successfully
             // completed descriptors in this chain.
             //
             // Later this state will update DTBC/status/interrupt.
             sg_dma_complete <= 1'b1;
             sg_state        <= SG_IDLE;
        end
        
        SG_ERROR: begin
          // Hold here for the first implementation.
          // Later this state will set device status / interrupt.
          sg_state <= SG_ERROR;
        end

        default: begin
          sg_state <= SG_IDLE;
        end

      endcase
    end
  end

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
