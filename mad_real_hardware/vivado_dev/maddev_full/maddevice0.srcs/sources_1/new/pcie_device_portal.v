//-----------------------------------------------------------------------------
//
// (c) Copyright 1995, 2007, 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of AMD, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// AMD, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND AMD HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) AMD shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or AMD had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// AMD products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of AMD products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//
//-----------------------------------------------------------------------------
//
// Project    : UltraScale+ FPGA PCI Express v4.0 Integrated Block
// File       : pio_rx_engine.v
// Version    : 1.3 
//-----------------------------------------------------------------------------
//
// Description: Local-Link Receive Unit.
//
//--------------------------------------------------------------------------------

`timescale 1ps/1ps

(* DowngradeIPIdentifiedWarnings = "yes" *)

//module pcie_device_portal #(
module pcie_device_portal #(
  parameter        TCQ                            = 1,
  parameter [1:0]  AXISTEN_IF_WIDTH              = 2'b01,
  parameter        AXISTEN_IF_CQ_ALIGNMENT_MODE  = "FALSE",
  parameter        AXISTEN_IF_RC_ALIGNMENT_MODE  = "FALSE",
  parameter        AXISTEN_IF_RC_STRADDLE        = 0,
  parameter        AXISTEN_IF_ENABLE_RX_MSG_INTFC= 0,
  parameter        AXISTEN_IF_CQ_PARITY_CHECK    = 0,
  parameter        AXISTEN_IF_RC_PARITY_CHECK    = 0,
  parameter [17:0] AXISTEN_IF_ENABLE_MSG_ROUTE   = 18'h2FFFF,
  parameter        AXI4_CQ_TUSER_WIDTH           = 88,
  parameter        AXI4_RC_TUSER_WIDTH           = 75,
  parameter        C_DATA_WIDTH                  = 128,
  parameter        ADDR_W                        = 5,
  parameter        MEM_W                         = 512,
  parameter        BYTE_EN_W                     = 64,
  parameter        STRB_WIDTH                    = C_DATA_WIDTH / 8,
  parameter        KEEP_WIDTH                    = C_DATA_WIDTH / 32,
  parameter        PARITY_WIDTH                  = C_DATA_WIDTH / 8,
  //
  parameter integer MAD_SECTOR_SHIFT              = 9,
  parameter integer MAD_SECTOR_SIZE               = (1 << MAD_SECTOR_SHIFT),
  parameter integer MAD_CACHE_NUM_SECTORS         = 1,
  parameter integer MAD_CACHE_SIZE                = 
                    (MAD_SECTOR_SIZE * MAD_CACHE_NUM_SECTORS),
  parameter integer  MAD_SGDMA_MAX_SECTORS       = 16,
  parameter integer BAR_OFFSET_WIDTH = 21,
  parameter integer REQ_ADDR_WIDTH   = BAR_OFFSET_WIDTH + 2,
  parameter integer WR_ADDR_WIDTH    = BAR_OFFSET_WIDTH
)(
  input                            user_clk,
  input                            reset_n,

  input      [C_DATA_WIDTH-1:0]    m_axis_cq_tdata,
  input                            m_axis_cq_tlast,
  input                            m_axis_cq_tvalid,
  input  [AXI4_CQ_TUSER_WIDTH-1:0] m_axis_cq_tuser,
  input        [KEEP_WIDTH-1:0]    m_axis_cq_tkeep,
  input                     [5:0]  pcie_cq_np_req_count,
  output                           m_axis_cq_tready,
  output                    [1:0]  pcie_cq_np_req,

  input      [C_DATA_WIDTH-1:0]    m_axis_rc_tdata,
  input                            m_axis_rc_tlast,
  input                            m_axis_rc_tvalid,
  input        [KEEP_WIDTH-1:0]    m_axis_rc_tkeep,
  input  [AXI4_RC_TUSER_WIDTH-1:0] m_axis_rc_tuser,
  output                           m_axis_rc_tready,

  output [C_DATA_WIDTH-1:0]        s_axis_cc_tdata,
  output [KEEP_WIDTH-1:0]          s_axis_cc_tkeep,
  output                           s_axis_cc_tlast,
  input                            s_axis_cc_tready,
  output                           s_axis_cc_tvalid,
  output [32:0]                    s_axis_cc_tuser,

  input                            cfg_msg_received,
  input                     [4:0]  cfg_msg_received_type,
  input                     [7:0]  cfg_msg_data,

  output                           req_compl,
  output                           req_compl_wd,
  output                           req_compl_ur,
  output                           compl_done,

  output                    [2:0]  req_tc,
  output                    [2:0]  req_attr,
  output                   [10:0]  req_len,
  output                   [15:0]  req_rid,
  output                    [7:0]  req_tag,
  output                    [7:0]  req_be,
  //output                   [12:0]  req_addr,
  output [REQ_ADDR_WIDTH-1:0]     req_addr,
  output                    [1:0]  req_at,

  output                   [63:0]  req_des_qword0,
  output                   [63:0]  req_des_qword1,
  output                           req_des_tph_present,
  output                    [1:0]  req_des_tph_type,
  output                    [7:0]  req_des_tph_st_tag,

  output                           req_mem_lock,
  output                           req_mem,

  output          [MEM_W-1:0]      wr_data,
  output                   [10:0]  payload_len,
  output                           wr_sop,
  output                           wr_eop,
  output         [BYTE_EN_W-1:0]   wr_data_be,
  //output                   [10:0]  wr_addr,
  output         [WR_ADDR_WIDTH-1:0]  wr_addr,
  output                    [7:0]  wr_be,
  output                           wr_en,
  output                           regs_awvalid,
  //
  output dbg_regs_awvalid,
  output dbg_regs_awready,
  output dbg_regs_wvalid,
  output dbg_regs_wready,
  output [3:0] dbg_regs_wstrb,
  output dbg_bufrd_go_pulse,
  output dbg_dma_go_pulse,
  output wire [7:0] dbg_rq_state,
  
  output wire        dbg_regs_arvalid,
  output wire        dbg_regs_arready,
  output wire        dbg_regs_rvalid,
  output wire        dbg_regs_rready,
  output wire [31:0] dbg_regs_rdata,
  output wire        dbg_tx_rd_en,
  (* mark_debug = "true" *) output wire [ADDR_W-1:0] dbg_tx_rd_addr,
  (* mark_debug = "true" *) output wire [MEM_W-1:0] dbg_tx_rd_data,
  
  output wire [15:0] dbg_regs_awaddr,
  output wire [15:0] dbg_regs_araddr,
  output wire [31:0] dbg_regs_wdata,
  output wire        dbg_axiw_pending,
  output wire        dbg_portal_wr_busy,
  output wire        dbg_live_wr_en,
  output wire        dbg_sop,
  output wire        dbg_in_packet_q,
  output wire [7:0]  dbg_rx_state,
  output wire        dbg_bufrd_start,

  // PCIe Requester Request interface
  output wire [C_DATA_WIDTH-1:0] s_axis_rq_tdata,
  output wire [KEEP_WIDTH-1:0]   s_axis_rq_tkeep,
  output wire                    s_axis_rq_tlast,
  output wire                    s_axis_rq_tvalid,
  output wire [61:0]             s_axis_rq_tuser,
  input  wire                    s_axis_rq_tready,
  //
  input             wr_busy
  );
      
  localparam integer MAD_SECTOR_SIZE_BYTES        = (1 << MAD_SECTOR_SHIFT);
  localparam integer RQ_PACKET_BYTES       = 16;
  localparam integer DMA_BLOCK_BYTES       = 512;
  
  localparam integer DMA_PACKETS_PER_BLOCK =
                     DMA_BLOCK_BYTES / RQ_PACKET_BYTES;  // 32
  localparam integer MAD_CACHE_SIZE_BYTES         = 
                     MAD_CACHE_NUM_SECTORS * MAD_SECTOR_SIZE_BYTES;
  localparam integer MAD_CACHE_SHIFT = $clog2(MAD_CACHE_SIZE);

  localparam integer MAD_READ_CACHE_OFFSET        = 0;
  localparam integer MAD_WRITE_CACHE_OFFSET       = MAD_CACHE_SIZE_BYTES;
  localparam integer MAD_TOTAL_CACHE_BYTES        = MAD_CACHE_SIZE_BYTES * 2;
  localparam integer MAD_CACHE_DWORDS             = MAD_TOTAL_CACHE_BYTES / 4;
  localparam integer MAD_CACHE_INDEX_WIDTH        = $clog2(MAD_CACHE_DWORDS);
  
  // ------------------------------------------------------------
  // Local BAR addressing
  //
  // BAR4 is 2 MiB:
  //   byte offsets require 21 bits: [20:0]
  //   DWORD offsets require 19 bits: [18:0]
  //
  // Internal request addresses prepend a 2-bit BAR-region code.
  // ------------------------------------------------------------
  //localparam integer BAR_OFFSET_WIDTH = 21;
  //localparam integer REQ_ADDR_WIDTH   = BAR_OFFSET_WIDTH + 2;  // 23
  //localparam integer WR_ADDR_WIDTH    = BAR_OFFSET_WIDTH;      // 21
    
  // ------------------------------------------------------------
  // AXI-Lite wires to BAR0 register block
  // ------------------------------------------------------------
  (* mark_debug = "true" *) wire [15:0] regs_awaddr;
  (* mark_debug = "true", keep = "true" *) wire regs_awready;

  (* mark_debug = "true" *) wire [31:0] regs_wdata;
  (* mark_debug = "true" *) wire [3:0]  regs_wstrb;
  (* mark_debug = "true" *) wire        regs_wvalid;

  (* mark_debug = "true", keep = "true" *) wire  regs_wready;

  wire [1:0]  regs_bresp;
  (* mark_debug = "true" *) wire        regs_bvalid;
  wire        regs_bready;

  wire [15:0] regs_araddr;
  wire        regs_arvalid;
  wire        regs_arready;

  wire [31:0] regs_rdata;
  wire [1:0]  regs_rresp;
  wire        regs_rvalid;
  wire        regs_rready;
  
  (* mark_debug = "true" *) wire        bufrd_go_pulse;
  (* mark_debug = "true" *) wire        dma_go_pulse;
  (* mark_debug = "true" *) wire        portal_wr_busy;

  // Accepted requester-completion beat from pio_rx_engine
  (* mark_debug = "true" *)
  wire [C_DATA_WIDTH-1:0] bufwr_rc_data;

  (* mark_debug = "true" *)
  wire [KEEP_WIDTH-1:0] bufwr_rc_keep;

  (* mark_debug = "true" *)
  wire [AXI4_RC_TUSER_WIDTH-1:0] bufwr_rc_user;

  (* mark_debug = "true" *)
  wire bufwr_rc_valid;

  (* mark_debug = "true" *)
  wire bufwr_rc_last;
  //
  
  // ------------------------------------------------------------
  // SG-DMA descriptor interface from mad_device_regs
  // ------------------------------------------------------------
   reg  [7:0]  sg_desc_index;

   wire [63:0] bcdpp;
   wire [63:0] sg_host_addr;
   wire [31:0] sg_dev_data_ofst;
   wire [31:0] sg_dma_cntl;
   wire [31:0] sg_dxbc;
   wire [31:0] sg_reg20;
   wire [63:0] sg_cdpp;
   wire        chained_dma_enabled;
  
  wire [63:0] host_addr;
  wire [31:0] byte_indx_rd;
  wire [31:0] byte_indx_wr;
  wire [31:0] bufrd_transfer_length;
  wire [31:0] cache_indx_rd;
  wire [31:0] cache_indx_wr;
  wire        int_enable_bufrd_input;
  wire        int_enable_bufrd_output;
  wire        cache_xfer_enabled;
  wire        read_cache_empty;
  wire        write_cache_empty;
  
  // The sequential controller below owns the buffered-read operation.
  wire bufrd_busy;
  wire bufwr_busy;
  
  wire dma_block_length_valid =
       (bufrd_transfer_length >= DMA_BLOCK_BYTES) &&
       (bufrd_transfer_length[8:0] == 9'd0);
 
  wire bufrd_start =
       bufrd_go_pulse &&
       int_enable_bufrd_input &&
       !bufrd_busy && !bufwr_busy;
    
  wire bufwr_start =
    bufrd_go_pulse &&
    int_enable_bufrd_output &&
    !bufrd_busy && !bufwr_busy;
    
  // DMA is a separate command from ordinary buffered I/O.
  // Initially support one device-to-host 512-byte block.
        
  wire dma_start =
      dma_go_pulse &&
      !chained_dma_enabled && dma_block_length_valid &&
      !bufrd_busy && !bufwr_busy;
  
  // ------------------------------------------------------------
  // SG-DMA descriptor walker
  //
  //   - starts at BCDPP
  //   - loads one BAR0 SG descriptor
  //   - dispatches D2H or H2D from DmaCntl[31]
  //   - waits for descriptor completion
  //   - follows CDPP to the next descriptor
  //   - terminates when CDPP == SG_CDPP_END
  // ------------------------------------------------------------

  localparam [2:0] SG_IDLE  = 3'd0;
  localparam [2:0] SG_LOAD  = 3'd1;
  localparam [2:0] SG_LATCH = 3'd2;
  localparam [2:0] SG_START = 3'd3;
  localparam [2:0] SG_WAIT  = 3'd4;
  localparam [2:0] SG_ERROR = 3'd5;
  localparam [2:0] SG_DONE  = 3'd6;
  
  localparam [63:0] SG_DESC_BASE = 64'h0000_0000_0000_0058;

  localparam [63:0] SG_CDPP_END = 64'h0000_0000_0000_0001;
  localparam [63:0] SG_DESC_LIMIT = 
                    SG_DESC_BASE + (MAD_SGDMA_MAX_SECTORS * 64'd32);
  
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

  // BAR0-relative offset of the descriptor selected by BCDPP.
  wire [63:0] sg_bcdpp_offset = bcdpp - SG_DESC_BASE;
  wire [63:0] sg_cdpp_offset = sg_cdpp_q - SG_DESC_BASE;
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

              // BCDPP must identify a 32-byte-aligned descriptor
              // beginning at or above BAR0 SG base 0x58.
              if ((bcdpp >= SG_DESC_BASE) && (bcdpp < SG_DESC_LIMIT) &&
                  (((bcdpp - SG_DESC_BASE) & 64'h0000_0000_0000_001F) == 64'd0)) begin

              // Each descriptor is 0x20 bytes.
              sg_desc_index <= sg_bcdpp_offset[12:5];
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
                else if ((sg_cdpp_q >= SG_DESC_BASE) && 
                         (sg_cdpp_q < SG_DESC_LIMIT) &&
                         (((sg_cdpp_q - SG_DESC_BASE) & 
                         64'h0000_0000_0000_001F) == 64'd0)) begin

                     sg_current_ptr <= sg_cdpp_q;

                     // Convert BAR0-relative CDPP into descriptor index.
                     sg_desc_index <= sg_cdpp_offset[12:5];

                     // Select and latch the next descriptor.
                     sg_state <= SG_LOAD;
                 end
        else begin
            sg_state <= SG_ERROR;
        end
    end

    // H2D completion will be added in the next stage.
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
  
 pio_rx_engine #(
    .TCQ(TCQ),
    .AXISTEN_IF_WIDTH(AXISTEN_IF_WIDTH),
    .AXISTEN_IF_CQ_ALIGNMENT_MODE(AXISTEN_IF_CQ_ALIGNMENT_MODE),
    .AXISTEN_IF_RC_ALIGNMENT_MODE(AXISTEN_IF_RC_ALIGNMENT_MODE),
    .AXISTEN_IF_RC_STRADDLE(AXISTEN_IF_RC_STRADDLE),
    .AXISTEN_IF_ENABLE_RX_MSG_INTFC(AXISTEN_IF_ENABLE_RX_MSG_INTFC),
    .AXISTEN_IF_CQ_PARITY_CHECK(AXISTEN_IF_CQ_PARITY_CHECK),
    .AXISTEN_IF_RC_PARITY_CHECK(AXISTEN_IF_RC_PARITY_CHECK),
    .AXISTEN_IF_ENABLE_MSG_ROUTE(AXISTEN_IF_ENABLE_MSG_ROUTE),
    .AXI4_CQ_TUSER_WIDTH(AXI4_CQ_TUSER_WIDTH),
    .AXI4_RC_TUSER_WIDTH(AXI4_RC_TUSER_WIDTH),
    .C_DATA_WIDTH(C_DATA_WIDTH),
    .ADDR_W(ADDR_W),
    .MEM_W(MEM_W),
    .BYTE_EN_W(BYTE_EN_W),
    .STRB_WIDTH(STRB_WIDTH),
    .KEEP_WIDTH(KEEP_WIDTH),
    .PARITY_WIDTH(PARITY_WIDTH),
    .REQ_ADDR_WIDTH(REQ_ADDR_WIDTH),
    .WR_ADDR_WIDTH (WR_ADDR_WIDTH)
  ) rx_i (
    .user_clk(user_clk),
    .reset_n(reset_n),

    .m_axis_cq_tdata(m_axis_cq_tdata),
    .m_axis_cq_tlast(m_axis_cq_tlast),
    .m_axis_cq_tvalid(m_axis_cq_tvalid),
    .m_axis_cq_tuser(m_axis_cq_tuser),
    .m_axis_cq_tkeep(m_axis_cq_tkeep),
    .pcie_cq_np_req_count(pcie_cq_np_req_count),
    .m_axis_cq_tready(m_axis_cq_tready),
    .pcie_cq_np_req(pcie_cq_np_req),

    .m_axis_rc_tdata(m_axis_rc_tdata),
    .m_axis_rc_tlast(m_axis_rc_tlast),
    .m_axis_rc_tvalid(m_axis_rc_tvalid),
    .m_axis_rc_tkeep(m_axis_rc_tkeep),
    .m_axis_rc_tuser(m_axis_rc_tuser),
    .m_axis_rc_tready(m_axis_rc_tready),

    .cfg_msg_received(cfg_msg_received),
    .cfg_msg_received_type(cfg_msg_received_type),
    .cfg_msg_data(cfg_msg_data),

    .req_compl(req_compl),
    .req_compl_wd(req_compl_wd),
    .req_compl_ur(req_compl_ur),
    .compl_done(compl_done),

    .req_tc(req_tc),
    .req_attr(req_attr),
    .req_len(req_len),
    .req_rid(req_rid),
    .req_tag(req_tag),
    .req_be(req_be),
    .req_addr(req_addr),
    .req_at(req_at),

    .req_des_qword0(req_des_qword0),
    .req_des_qword1(req_des_qword1),
    .req_des_tph_present(req_des_tph_present),
    .req_des_tph_type(req_des_tph_type),
    .req_des_tph_st_tag(req_des_tph_st_tag),

    .req_mem_lock(req_mem_lock),
    .req_mem(req_mem),

    .wr_data(wr_data),
    .payload_len(payload_len),
    .wr_sop(wr_sop),
    .wr_eop(wr_eop),
    .wr_data_be(wr_data_be),
    .wr_addr(wr_addr),
    .wr_be(wr_be),
    .wr_en(wr_en),
    .wr_busy(portal_wr_busy),

    .dbg_sop(dbg_sop),
    .dbg_in_packet_q(dbg_in_packet_q),
    .dbg_rx_state(dbg_rx_state),

    .bufwr_rc_data(bufwr_rc_data),
    .bufwr_rc_keep(bufwr_rc_keep),
    .bufwr_rc_user(bufwr_rc_user),
    .bufwr_rc_valid(bufwr_rc_valid),
    .bufwr_rc_last(bufwr_rc_last)
   );

    wire [ADDR_W-1:0] tx_rd_addr;
    wire [3:0]        tx_rd_be;
    wire              tx_rd_en;
    wire              tx_trn_sent;
    wire [MEM_W-1:0]  tx_rd_data;
    
    wire [1:0] wr_bar_region = wr_addr[20:19];
    wire [1:0] rd_bar_region = req_addr[22:21];
    //
    wire wr_to_bar0 = (wr_bar_region == 2'b00);
    wire rd_to_bar0 = (rd_bar_region == 2'b00);
    //
    wire wr_to_bar2 = (wr_bar_region == 2'b01);
    wire rd_to_bar2 = (rd_bar_region == 2'b01);
    //
    wire wr_to_bar4 = (wr_bar_region == 2'b10);
    wire rd_to_bar4 = (rd_bar_region == 2'b10);
    
    reg [31:0] regs_rdata_hold;
    always @(posedge user_clk) begin
        if (!reset_n)
            regs_rdata_hold <= 32'h0;
        else if (regs_rvalid)
            regs_rdata_hold <= regs_rdata;
    end
    
    reg [31:0] bar2_buf [0:MAD_CACHE_DWORDS-1];  // 256 DWORDs = 1024 bytes
    
   // CacheIndxRd/CacheIndxWr are cache-sector numbers.  Multiplication
   // by MAD_CACHE_SIZE converts each sector number to a BAR4 byte base.
   wire [31:0] cache_rd_bar4_base = cache_indx_rd * MAD_CACHE_SIZE;
   
   wire [31:0] cache_wr_bar4_base = cache_indx_wr * MAD_CACHE_SIZE;
    
   reg [31:0] bar4_buf [0:511];  
  
   reg [31:0]  bufrd_host_offset;
   reg         bufrd_sg_active;
   
    // The TX completion engine walks a long read by incrementing tx_rd_addr.
    // Use that address here so each 16-byte completion returns the next 4 DWORDs.
    
    // Current completion-read window is 512 bytes:
    // tx_rd_addr[4:0] selects 32 chunks of 16 bytes.
    wire [6:0] bar_rd_idx = {tx_rd_addr[4:0], 2'b00};
    
    wire [127:0] bar2_rd_16B =
      {
      bar2_buf[bar_rd_idx + 3], bar2_buf[bar_rd_idx + 2],
      bar2_buf[bar_rd_idx + 1], bar2_buf[bar_rd_idx + 0]
      };

    wire [127:0] bar4_rd_16B =
      {
      bar4_buf[bar_rd_idx + 3], bar4_buf[bar_rd_idx + 2],
      bar4_buf[bar_rd_idx + 1], bar4_buf[bar_rd_idx + 0]
      };
      
    wire [31:0] bufrd_bar4_byte_addr = bufrd_sg_active ? 
               (sg_dev_data_ofst_q + bufrd_host_offset) : byte_indx_rd;

    wire [8:0] bufrd_bar4_idx =
               {bufrd_bar4_byte_addr[10:4], 2'b00};
           
    // ByteIndxWr is a byte offset. Convert its 16-byte-aligned value
    // into the DWORD index used by the 128-entry BAR4 array.
    //wire [7:0] bufwr_bar4_idx = {byte_indx_wr[9:4], 2'b00};
    wire [31:0] bufwr_bar4_byte_addr = bufwr_sg_active ? 
               (sg_dev_data_ofst_q + bufwr_host_offset) : byte_indx_wr;
               
    wire [8:0] bufwr_bar4_idx =
               {bufwr_bar4_byte_addr[10:4], 2'b00};
               
    wire [127:0] bar4_bufrd_payload = {
        bar4_buf[bufrd_bar4_idx + 3],
        bar4_buf[bufrd_bar4_idx + 2],
        bar4_buf[bufrd_bar4_idx + 1],
        bar4_buf[bufrd_bar4_idx + 0]
    };

    wire bufrd_packet_done;
 
    // ------------------------------------------------------------
    // Buffered-read sequential RQ controller
    // ------------------------------------------------------------
    localparam [2:0] BUFRD_SEQ_IDLE        = 3'd0;
    localparam [2:0] BUFRD_SEQ_CACHE_FILL  = 3'd1;
    localparam [2:0] BUFRD_SEQ_CACHE_DONE  = 3'd2;
    localparam [2:0] BUFRD_SEQ_WAIT_RQ     = 3'd3;
    localparam [2:0] BUFRD_SEQ_RESTART     = 3'd4;
    localparam [2:0] BUFRD_SEQ_CACHE_STORE = 3'd5;

    reg [2:0]  bufrd_seq_state;
    reg        bufrd_rq_start_q;
    reg [31:0] bufrd_bytes_remaining;

    // Temporary offset into the current host DMA buffer.  Unlike
    // ByteIndxRd, this offset is reset at the beginning of every I/O.
    reg [31:0]  bufrd_cache_copy_offset;
    reg [31:0]  bufrd_cache_bar4_dword_base;
    reg [127:0] bufrd_copy_data_q;
    reg [31:0]  bufrd_copy_bar2_index_q;
    reg         bufrd_dma_active;
    
    // Local BAR4-to-BAR2 cache-fill indices.  The copy offset is a byte
    // offset; both memories are arrays of 32-bit DWORDs.
    wire [31:0] bufrd_copy_bar2_index =
        (MAD_READ_CACHE_OFFSET >> 2) + (bufrd_cache_copy_offset >> 2);
    
    wire [31:0] bufrd_copy_bar4_index =
        bufrd_cache_bar4_dword_base + (bufrd_cache_copy_offset >> 2);
        

    // HostPA is strictly the base address of the current host buffer.
    // Each accepted 16-byte RQ packet advances only this temporary offset.
    wire [63:0] bufrd_host_base =
       bufrd_sg_active ? sg_host_addr_q : host_addr;

    wire [63:0] bufrd_rq_host_addr =
        bufrd_host_base + {32'd0, bufrd_host_offset};     
        
    reg bufrd_cache_xfer_active;

    // Cache reads begin at BAR2 + 0. bufrd_host_offset advances
    // by 16 bytes for each accepted requester packet.
    wire [MAD_CACHE_INDEX_WIDTH-1:0] cache_read_dword_index =
                                     bufrd_host_offset[MAD_CACHE_INDEX_WIDTH+1:2];

    wire [127:0] cache_read_payload = {
        bar2_buf[cache_read_dword_index + 3],
        bar2_buf[cache_read_dword_index + 2],
        bar2_buf[cache_read_dword_index + 1],
        bar2_buf[cache_read_dword_index + 0]
    };

    wire [127:0] bufrd_payload =
        bufrd_cache_xfer_active ? cache_read_payload : bar4_bufrd_payload;

    // The programmed transfer length is always a multiple of 16 bytes:
    // byte mode selects (count + 1) * 16 and block mode selects
    // (count + 1) * 512.
    wire bufrd_length_valid =
        (bufrd_transfer_length >= RQ_PACKET_BYTES) &&
        (bufrd_transfer_length[3:0] == 4'd0);
        
    // One pulse for every accepted 16-byte device-to-host
    // requester packet, regardless of the command type.
    wire bufrd_packet_advance =
        (bufrd_seq_state == BUFRD_SEQ_WAIT_RQ) && bufrd_packet_done;

    // Ordinary buffered-read progress must not be reported for DMA.
    wire bufrd_advance = 
         bufrd_packet_advance && !bufrd_dma_active && !bufrd_sg_active;

    // DMA progress is reported separately.
    wire dma_advance = bufrd_packet_advance && bufrd_dma_active;

    // Final ordinary buffered-read packet.
    wire bufrd_io_complete = bufrd_packet_advance &&
                             !bufrd_dma_active && !bufrd_sg_active &&
                             !bufrd_cache_xfer_active && 
                             (bufrd_bytes_remaining == RQ_PACKET_BYTES);
    // Final DMA packet.
    wire dma_complete = dma_advance && (bufrd_bytes_remaining == RQ_PACKET_BYTES);
    wire dma_busy = bufrd_busy && bufrd_dma_active;
    wire sg_d2h_complete = bufrd_packet_advance &&
                           bufrd_sg_active && (bufrd_bytes_remaining == RQ_PACKET_BYTES);
    
    wire bufrd_io_busy =  bufrd_busy && !bufrd_dma_active && !bufrd_sg_active;
    assign bufrd_busy = (bufrd_seq_state != BUFRD_SEQ_IDLE);
   
     
    // First use of an empty read cache completes when BAR2 has been filled. 
    // No host transfer is issued for this priming operation.
    wire bufrd_cache_first_use_complete = (bufrd_seq_state == BUFRD_SEQ_CACHE_DONE) &&
                                          bufrd_cache_xfer_active && read_cache_empty;

    // A normal cached read completes after the current BAR2 cache has been 
    // returned to the host and BAR2 has been refilled 
    // from the selected BAR4 cache sector.
    wire bufrd_cache_normal_complete = 
         (bufrd_seq_state == BUFRD_SEQ_CACHE_DONE) &&
         bufrd_cache_xfer_active && !read_cache_empty;

    always @(posedge user_clk) begin
        if (!reset_n) begin
            bufrd_seq_state         <= BUFRD_SEQ_IDLE;
            bufrd_rq_start_q        <= 1'b0;
            bufrd_bytes_remaining   <= 32'd0;
            bufrd_host_offset       <= 32'd0;
            bufrd_cache_copy_offset <= 32'd0;
            bufrd_cache_bar4_dword_base <= 32'd0;
            bufrd_copy_data_q       <= 128'd0;
            bufrd_copy_bar2_index_q <= 32'd0;
            bufrd_cache_xfer_active <= 1'b0;
            bufrd_dma_active        <= 1'b0;
            bufrd_sg_active         <= 1'b0;
        end else begin
            // Requester start is a one-clock pulse.
            bufrd_rq_start_q <= 1'b0;

            case (bufrd_seq_state)
                BUFRD_SEQ_IDLE: begin
                    if (sg_d2h_start || dma_start || (bufrd_start && 
                         (cache_xfer_enabled || bufrd_length_valid))) begin

                           // Remember which command type started this operation.
                           bufrd_dma_active <= dma_start;
                           bufrd_sg_active  <= sg_d2h_start;
                           
                           // DMA initially transfers exactly one 512-byte block.
                           // Ordinary buffered I/O retains its programmed 16-byte
                           // granularity, and cache I/O retains MAD_CACHE_SIZE.
                           //if (dma_start)
                          //     bufrd_bytes_remaining <= DMA_BLOCK_BYTES;
                           //else
                           if (sg_d2h_start)
                               bufrd_bytes_remaining <= sg_dxbc_q;
                           else if (dma_start)
                               bufrd_bytes_remaining <= DMA_BLOCK_BYTES;
                           else
                               bufrd_bytes_remaining <= cache_xfer_enabled ?
                                                        MAD_CACHE_SIZE : bufrd_transfer_length;

                            // DMA is not a BAR2 cache operation.
                            bufrd_cache_xfer_active <= (dma_start || sg_d2h_start) ?
                                                       1'b0 : cache_xfer_enabled;
 
                            // Every new operation begins at the programmed HostPA.
                            bufrd_host_offset <= 32'd0;
                     
                            if (!dma_start && !sg_d2h_start && cache_xfer_enabled) begin
                                if (read_cache_empty) begin
                                    // First use: prime BAR2 from BAR4; return nothing to host.
                                    bufrd_cache_copy_offset <= 32'd0;
                                    bufrd_cache_bar4_dword_base <= cache_rd_bar4_base >> 2;
                                    bufrd_seq_state <= BUFRD_SEQ_CACHE_FILL;
                                end else begin
                                    // Normal cached read:
                                    // return CURRENT BAR2 cache to host first.
                                    bufrd_rq_start_q <= 1'b1;
                                    bufrd_seq_state  <= BUFRD_SEQ_WAIT_RQ;
                                end
                            end else begin
                                // Ordinary buffered read, DMA or SGDMA.
                                bufrd_rq_start_q <= 1'b1;
                                bufrd_seq_state  <= BUFRD_SEQ_WAIT_RQ;
                            end
                        end
                    end

                BUFRD_SEQ_CACHE_FILL: begin
                    // Stage 1: read one 16-byte group from BAR4 and register it.
                    bufrd_copy_data_q <= {bar4_buf[bufrd_copy_bar4_index + 3],
                                          bar4_buf[bufrd_copy_bar4_index + 2],
                                          bar4_buf[bufrd_copy_bar4_index + 1],
                                          bar4_buf[bufrd_copy_bar4_index + 0]};

                    // Preserve the destination index for the following store clock.
                    bufrd_copy_bar2_index_q <= bufrd_copy_bar2_index;
                    bufrd_seq_state <= BUFRD_SEQ_CACHE_STORE;
                end

                BUFRD_SEQ_CACHE_STORE: begin
                    // Stage 2: the memory-write block stores bufrd_copy_data_q
                    // into BAR2 on this clock.

                    if (bufrd_cache_copy_offset >= (MAD_CACHE_SIZE - 16)) begin
                         bufrd_cache_copy_offset <= 32'd0;
                         bufrd_seq_state         <= BUFRD_SEQ_CACHE_DONE;
                    end else begin
                        bufrd_cache_copy_offset <= bufrd_cache_copy_offset + 32'd16;
                        bufrd_seq_state <= BUFRD_SEQ_CACHE_FILL;
                    end
                end

                BUFRD_SEQ_CACHE_DONE: begin
                    // First use: BAR2 has just been primed; no host data was returned.
                    //
                    // Normal cached read:
                    //   the old BAR2 cache was already returned to the host,
                    //   and BAR2 has now been refilled from BAR4.
                    //
                    // Completion wires distinguish the two cases using
                    // read_cache_empty while CACHE_DONE is active.
                    bufrd_bytes_remaining    <= 32'd0;
                    bufrd_cache_xfer_active  <= 1'b0;
                    bufrd_dma_active         <= 1'b0;
                    bufrd_sg_active          <= 1'b0;
                    bufrd_seq_state          <= BUFRD_SEQ_IDLE;
                end

                BUFRD_SEQ_WAIT_RQ: begin
                    if (bufrd_packet_done) begin
                        if (bufrd_bytes_remaining > 32'd16) begin
                            bufrd_bytes_remaining <= 
                                bufrd_bytes_remaining - 32'd16;

                            // Advance the destination within this host buffer.
                            // mad_device_regs independently advances the
                            // persistent device ByteIndxRd by 16 bytes.
                            bufrd_host_offset <= bufrd_host_offset + 32'd16;
                            bufrd_seq_state <= BUFRD_SEQ_RESTART;
                        end else begin
                            if (bufrd_cache_xfer_active) begin
                                // Current BAR2 cache has now completely reached the host.
                                // Refill BAR2 from the sector selected by CacheIndxRd.
                                bufrd_bytes_remaining <= 32'd0;
                                bufrd_cache_copy_offset <= 32'd0;
                                bufrd_cache_bar4_dword_base <= cache_rd_bar4_base >> 2;
                                bufrd_seq_state <= BUFRD_SEQ_CACHE_FILL;
                            end else begin
                                bufrd_bytes_remaining <= 32'd0;
                                bufrd_dma_active      <= 1'b0;
                                bufrd_sg_active       <= 1'b0;
                                bufrd_seq_state       <= BUFRD_SEQ_IDLE;
                            end
                        end
                    end
                end

                BUFRD_SEQ_RESTART: begin
                    // ByteIndxRd and bufrd_host_offset were advanced on
                    // the preceding edge.  The next BAR4 payload and host
                    // destination address are now stable for tx_i.
                    bufrd_rq_start_q <= 1'b1;
                    bufrd_seq_state  <= BUFRD_SEQ_WAIT_RQ;
                end

                default: begin
                    bufrd_seq_state         <= BUFRD_SEQ_IDLE;
                    bufrd_rq_start_q        <= 1'b0;
                    bufrd_bytes_remaining   <= 32'd0;
                    bufrd_host_offset       <= 32'd0;
                    bufrd_cache_copy_offset <= 32'd0;
                    bufrd_cache_xfer_active <= 1'b0;
                    bufrd_dma_active        <= 1'b0;
                    bufrd_sg_active         <= 1'b0;
                end
            endcase
        end
    end
    
    // ------------------------------------------------------------
    // Buffered-write sequential H2D controller
    //
    // One 16-byte PCIe Memory Read request is outstanding at a
    // time. Returned Completion-with-Data payloads will later be
    // written into BAR4 at the current ByteIndxWr location.
    // ------------------------------------------------------------

    localparam [2:0] BUFWR_SEQ_IDLE            = 3'd0;
    localparam [2:0] BUFWR_SEQ_CACHE_WRITEBACK = 3'd1;
    localparam [2:0] BUFWR_SEQ_CACHE_DONE      = 3'd2;
    localparam [2:0] BUFWR_SEQ_WAIT_RQ         = 3'd3;
    localparam [2:0] BUFWR_SEQ_WAIT_RC         = 3'd4;
    localparam [2:0] BUFWR_SEQ_RESTART         = 3'd5;
    localparam [2:0] BUFWR_SEQ_CACHE_STORE     = 3'd6; // Store loaded 16B into BAR4

    reg [2:0]  bufwr_seq_state;
    reg        bufwr_rq_start_q;
    reg [31:0] bufwr_bytes_remaining;

    // Temporary offset into the current host DMA buffer.
    // HostPA remains the base address and is not modified.
    reg [31:0] bufwr_host_offset;
    reg [31:0] bufwr_cache_copy_offset;
    reg [31:0] bufwr_cache_bar4_dword_base;
    reg [127:0] bufwr_copy_data_q;
    reg [8:0]   bufwr_copy_bar4_index_q;
    reg         bufwr_sg_active;

    // Local BAR2-to-BAR4 write-cache indices.  The BAR2 source starts
    // at MAD_WRITE_CACHE_OFFSET; the BAR4 destination starts at the
    // selected cache-sector base latched when the operation begins.
    wire [31:0] bufwr_copy_bar2_index =
        (MAD_WRITE_CACHE_OFFSET >> 2) + (bufwr_cache_copy_offset >> 2);
        
    wire [31:0] bufwr_copy_bar4_index =
        bufwr_cache_bar4_dword_base + (bufwr_cache_copy_offset >> 2);

    // Address read from host memory by the next 16-byte Memory
    // Read request.
    wire [63:0] bufwr_host_base = bufwr_sg_active ? sg_host_addr_q : host_addr;
    wire [63:0] bufwr_rq_host_addr = bufwr_host_base + {32'd0, bufwr_host_offset};

    reg bufwr_cache_xfer_active;

    // Cache writes begin at BAR2 + MAD_CACHE_SIZE. bufwr_host_offset
    // advances by 16 bytes for each stored completion.
    wire [MAD_CACHE_INDEX_WIDTH-1:0] cache_write_dword_index =
        (MAD_WRITE_CACHE_OFFSET >> 2) +
        bufwr_host_offset[MAD_CACHE_INDEX_WIDTH+1:2];

    // The programmed length must contain one or more complete
    // 16-byte transfers.
    wire bufwr_length_valid =
        (bufrd_transfer_length >= 32'd16) &&
        (bufrd_transfer_length[3:0] == 4'd0);

    assign bufwr_busy =
        (bufwr_seq_state != BUFWR_SEQ_IDLE);

    // ------------------------------------------------------------
    // Temporary handshake placeholders
    //
    // Edit #3 will replace bufwr_rq_packet_done with the pulse
    // indicating that the Memory Read request was accepted on RQ.
    //
    // Edits #5 and #6 will replace bufwr_completion_stored with
    // the pulse indicating that the returned 16-byte RC payload
    // was successfully written into BAR4.
    // ------------------------------------------------------------

    //wire bufwr_rq_packet_done   = 1'b0;
    wire bufwr_rq_packet_done;
    reg  bufwr_completion_stored;

    // One pulse after each returned 16-byte completion has been
    // committed to BAR4. mad_device_regs uses this pulse to
    // advance the persistent ByteIndxWr by 16 bytes.
    wire bufwr_packet_advance = (bufwr_seq_state == BUFWR_SEQ_WAIT_RC) &&
                               bufwr_completion_stored;

    wire bufwr_advance = bufwr_packet_advance && !bufwr_sg_active;
   
    // One pulse when the completion just stored is the final
    // 16-byte packet of the programmed buffered-write transfer.
    wire bufwr_io_complete = bufwr_advance && (bufwr_bytes_remaining == 32'd16);
  
    wire sg_h2d_complete = bufwr_packet_advance && bufwr_sg_active &&
                           (bufwr_bytes_remaining == 32'd16);
                           
    // The first cached write fills the initially empty write cache.
    // It does not write an old cache sector back to BAR4.
    wire bufwr_cache_first_use_complete = bufwr_io_complete && 
          bufwr_cache_xfer_active && write_cache_empty;

    // A normal cached write first writes back the existing cache and
    // then replaces it with the incoming host data.
    wire bufwr_cache_normal_complete = 
         bufwr_io_complete && bufwr_cache_xfer_active && !write_cache_empty;       
   
    // ------------------------------------------------------------
    // H2D Requester Completion assembly
    //
    // A 16-byte Completion-with-Data occupies two beats on the
    // 128-bit, DWORD-aligned RC interface:
    //
    //   beat 0: 96-bit descriptor + payload DWORD 0
    //   beat 1: payload DWORDs 1, 2 and 3
    //
    // Step #4 only assembles and latches the returned payload.
    // BAR4 storage will be added in step #5.
    // ------------------------------------------------------------

    reg         bufwr_rc_in_packet;
    reg [127:0] bufwr_completion_payload;
    reg         bufwr_completion_received;
    
    wire sg_dma_xfer_advance = (bufrd_packet_advance && bufrd_sg_active) ||
                               (bufwr_packet_advance && bufwr_sg_active);

    // Optional diagnostic captures retained with the assembled data.
    reg [KEEP_WIDTH-1:0]         bufwr_completion_last_keep;
    reg [AXI4_RC_TUSER_WIDTH-1:0] bufwr_completion_user;
     
    always @(posedge user_clk) begin
        if (!reset_n) begin
            bufwr_rc_in_packet          <= 1'b0;
            bufwr_completion_payload    <= 128'd0;
            bufwr_completion_received   <= 1'b0;
            bufwr_completion_last_keep  <= {KEEP_WIDTH{1'b0}};
            bufwr_completion_user       <=
                {AXI4_RC_TUSER_WIDTH{1'b0}};
        end else begin
            // This indication is always a one-clock pulse.
            bufwr_completion_received <= 1'b0;

            // Only consume RC traffic while an H2D request is
            // waiting for its returned completion.
            if (bufwr_seq_state == BUFWR_SEQ_WAIT_RC) begin
                if (bufwr_rc_valid) begin

                    if (!bufwr_rc_in_packet) begin
                        // First RC beat:
                        //   data[95:0]   = completion descriptor
                        //   data[127:96] = returned payload DWORD 0
                        bufwr_completion_payload[31:0] <= bufwr_rc_data[127:96];

                        bufwr_completion_user <= bufwr_rc_user;

                        if (bufwr_rc_last) begin
                            // A normal four-DWORD completion should
                            // not end on its first 128-bit beat.
                            // Clear state defensively without claiming
                            // that a complete payload was received.
                            bufwr_rc_in_packet <= 1'b0;
                        end else begin
                            bufwr_rc_in_packet <= 1'b1;
                        end
                    end else begin
                        // Second RC beat:
                        //   data[95:0] = returned payload DWORDs 1..3
                        bufwr_completion_payload[127:32] <=
                            bufwr_rc_data[95:0];

                        bufwr_completion_last_keep <=
                            bufwr_rc_keep;

                        if (bufwr_rc_last) begin
                            bufwr_rc_in_packet        <= 1'b0;
                            bufwr_completion_received <= 1'b1;
                        end
                    end
                end
            end else begin
                // Abandon any partial RC packet when no H2D request
                // is awaiting completion.
                bufwr_rc_in_packet <= 1'b0;
            end
        end
    end 
     
    always @(posedge user_clk) begin
        if (!reset_n) begin
            bufwr_seq_state         <= BUFWR_SEQ_IDLE;
            bufwr_rq_start_q        <= 1'b0;
            bufwr_bytes_remaining   <= 32'd0;
            bufwr_host_offset       <= 32'd0;
            bufwr_cache_copy_offset <= 32'd0;
            bufwr_cache_bar4_dword_base <= 32'd0;
            bufwr_cache_xfer_active <= 1'b0;
            bufwr_copy_data_q       <= 128'd0;
            bufwr_copy_bar4_index_q <= 9'd0;
            bufwr_sg_active <= 1'b0;
        end else begin
            // Requester start is always a one-clock pulse.
            bufwr_rq_start_q <= 1'b0;

            case (bufwr_seq_state)

                BUFWR_SEQ_IDLE: begin
                    if (sg_h2d_start ||
                       (bufwr_start && (cache_xfer_enabled || bufwr_length_valid))) begin
                    //if (bufwr_start && (cache_xfer_enabled || bufwr_length_valid)) begin
                        // Latch the complete programmed transfer
                        // length. The first test will use 16 bytes.
                        //bufwr_bytes_remaining <= bufrd_transfer_length;
                        bufwr_sg_active <= sg_h2d_start;
                        //bufwr_bytes_remaining <= cache_xfer_enabled ? 
                        //                         MAD_CACHE_SIZE : bufrd_transfer_length;
                        if (sg_h2d_start)
                            bufwr_bytes_remaining <= sg_dxbc_q;
                        else
                            bufwr_bytes_remaining <= cache_xfer_enabled ?
                                                     MAD_CACHE_SIZE : bufrd_transfer_length;
                        bufwr_cache_xfer_active <= sg_h2d_start ? 1'b0 : cache_xfer_enabled;

                        // Every new I/O starts at HostPA.
                        bufwr_host_offset <= 32'd0;

                        if (!sg_h2d_start && cache_xfer_enabled) begin
                          if (write_cache_empty) begin
                            // There is no valid old write-cache sector to commit.
                            // Start filling the cache directly from host memory.
                            bufwr_cache_copy_offset <= 32'd0;
                            bufwr_rq_start_q        <= 1'b1;
                            bufwr_seq_state         <= BUFWR_SEQ_WAIT_RQ;
                         end else begin
                            // Commit the valid old BAR2 write cache to its selected
                            // cache-sized BAR4 sector before replacing it.
                            bufwr_cache_copy_offset <= 32'd0;
                            bufwr_cache_bar4_dword_base <= cache_wr_bar4_base >> 2;
                            bufwr_seq_state <= BUFWR_SEQ_CACHE_WRITEBACK;
                        end
                     end else begin
                           // Non-cache operation: request the first 16 host bytes.
                           bufwr_rq_start_q <= 1'b1;
                           bufwr_seq_state  <= BUFWR_SEQ_WAIT_RQ;
                     end
                   end
                end

                BUFWR_SEQ_CACHE_WRITEBACK: begin
                    // Pipeline stage 1:
                    // Select and capture one 16-byte group from BAR2.
                    bufwr_copy_data_q <= {bar2_buf[bufwr_copy_bar2_index + 3],
                                          bar2_buf[bufwr_copy_bar2_index + 2],
                                          bar2_buf[bufwr_copy_bar2_index + 1],
                                          bar2_buf[bufwr_copy_bar2_index + 0]};

                    // Capture the corresponding BAR4 DWORD destination.
                    bufwr_copy_bar4_index_q <= bufwr_copy_bar4_index[8:0];
                    bufwr_seq_state <= BUFWR_SEQ_CACHE_STORE;
                end

                BUFWR_SEQ_CACHE_STORE: begin
                    // Pipeline stage 2:
                    // The memory-write block stores bufwr_copy_data_q into BAR4 during this state.

                    if (bufwr_cache_copy_offset >= (MAD_CACHE_SIZE - 16)) begin
                        // The final 16-byte group is being stored on this edge.
                        bufwr_cache_copy_offset <= 32'd0;
                        bufwr_seq_state         <= BUFWR_SEQ_CACHE_DONE;
                    end else begin
                        bufwr_cache_copy_offset <=
                        bufwr_cache_copy_offset + 32'd16;
                        bufwr_seq_state <= BUFWR_SEQ_CACHE_WRITEBACK;
                    end
                end

                BUFWR_SEQ_CACHE_DONE: begin
                    // The final BAR4 writeback completed on the preceding
                    // edge.  It is now safe to replace the write cache.
                    bufwr_rq_start_q <= 1'b1;
                    bufwr_seq_state  <= BUFWR_SEQ_WAIT_RQ;
                end

                BUFWR_SEQ_WAIT_RQ: begin
                    // Wait until the Memory Read request has been
                    // accepted by the PCIe RQ interface.
                    if (bufwr_rq_packet_done) begin
                        bufwr_seq_state <= BUFWR_SEQ_WAIT_RC;
                    end
                end

                BUFWR_SEQ_WAIT_RC: begin
                    // Do not advance until the returned completion
                    // payload has actually been stored into BAR4.
                    if (bufwr_completion_stored) begin
                        if (bufwr_bytes_remaining > 32'd16) begin
                            bufwr_bytes_remaining <= bufwr_bytes_remaining - 32'd16;

                            // Advance within the current host buffer.
                            // mad_device_regs advances persistent
                            // ByteIndxWr on the same stored-completion
                            // pulse that moves this controller forward.
                            bufwr_host_offset <= bufwr_host_offset + 32'd16;

                            bufwr_seq_state <= BUFWR_SEQ_RESTART;
                        end else begin
                            bufwr_bytes_remaining <= 32'd0;
                            bufwr_sg_active       <= 1'b0;
                            bufwr_seq_state       <= BUFWR_SEQ_IDLE;
                        end
                    end
                end

                BUFWR_SEQ_RESTART: begin
                    // The host offset and ByteIndxWr will have been
                    // advanced before this state is entered. Issue
                    // the next 16-byte host Memory Read request.
                    bufwr_rq_start_q <= 1'b1;
                    bufwr_seq_state  <= BUFWR_SEQ_WAIT_RQ;
                end

                default: begin
                    bufwr_seq_state       <= BUFWR_SEQ_IDLE;
                    bufwr_rq_start_q      <= 1'b0;
                    bufwr_bytes_remaining <= 32'd0;
                    bufwr_host_offset     <= 32'd0;
                    bufwr_sg_active       <= 1'b0;
                end

            endcase
        end
    end
    
    //assign tx_rd_data =
    assign tx_rd_data = rd_to_bar0 ? 
           {MEM_W/32{regs_rdata_hold}} : rd_to_bar2 ? 
           {MEM_W/128{bar2_rd_16B}}    : rd_to_bar4 ? 
           {MEM_W/128{bar4_rd_16B}}    : {MEM_W/32{32'hEEEE_EEEE}};
           
    wire                    tx_cfg_msg_transmit;
    wire [2:0]              tx_cfg_msg_transmit_type;
    wire [31:0]             tx_cfg_msg_transmit_data;
    wire [2:0]              tx_cfg_fc_sel;
     
  pio_tx_engine #(
    .TCQ(TCQ),
    .AXISTEN_IF_WIDTH(AXISTEN_IF_WIDTH),
    .C_DATA_WIDTH(C_DATA_WIDTH),
    .MEM_W(MEM_W),
    .KEEP_WIDTH(KEEP_WIDTH),
    .REQ_ADDR_WIDTH(REQ_ADDR_WIDTH)
  ) tx_i (
    .user_clk(user_clk),
    .reset_n(reset_n),

    .s_axis_cc_tdata(s_axis_cc_tdata),
    .s_axis_cc_tkeep(s_axis_cc_tkeep),
    .s_axis_cc_tlast(s_axis_cc_tlast),
    .s_axis_cc_tready(s_axis_cc_tready),
    .s_axis_cc_tvalid(s_axis_cc_tvalid),
    .s_axis_cc_tuser(s_axis_cc_tuser),

    .req_compl(req_compl),
    .req_compl_wd(req_compl_wd),
    .req_compl_ur(req_compl_ur),
    .compl_done(compl_done),

    .req_tc(req_tc),
    .req_attr(req_attr),
    .req_len(req_len),
    .req_rid(req_rid),
    .req_tag(req_tag),
    .req_be(req_be),
    .req_addr(req_addr),
    .req_at(req_at),

    .req_des_qword0(req_des_qword0),
    .req_des_qword1(req_des_qword1),
    .req_des_tph_present(req_des_tph_present),
    .req_des_tph_type(req_des_tph_type),
    .req_des_tph_st_tag(req_des_tph_st_tag),

    .req_mem_lock(req_mem_lock),
    .req_mem(req_mem),
//    
    .payload_len(payload_len),
    .req_td(1'b0),
    .req_ep(1'b0),
    .completer_id(16'h0000),

    .rd_addr(tx_rd_addr),
    .rd_be(tx_rd_be),
    .rd_en(tx_rd_en),
    .trn_sent(tx_trn_sent),
    .rd_data(tx_rd_data),
    //
    .s_axis_rq_tdata(s_axis_rq_tdata),
    .s_axis_rq_tkeep(s_axis_rq_tkeep),
    .s_axis_rq_tlast(s_axis_rq_tlast),
    .s_axis_rq_tvalid(s_axis_rq_tvalid),
    .s_axis_rq_tuser(s_axis_rq_tuser),
    .s_axis_rq_tready(s_axis_rq_tready),

    .cfg_msg_transmit_done(1'b1),
    .cfg_msg_transmit(tx_cfg_msg_transmit),
    .cfg_msg_transmit_type(tx_cfg_msg_transmit_type),
    .cfg_msg_transmit_data(tx_cfg_msg_transmit_data),

    .pcie_rq_tag(6'd0),
    .pcie_rq_tag_vld(1'b0),
    .pcie_tfc_nph_av(2'd0),
    .pcie_tfc_npd_av(2'd0),
    .pcie_tfc_np_pl_empty(1'b1),
    .pcie_rq_seq_num(4'd0),
    .pcie_rq_seq_num_vld(1'b0),

    .cfg_fc_ph(8'd0),
    .cfg_fc_nph(8'd0),
    .cfg_fc_cplh(8'd0),
    .cfg_fc_pd(12'd0),
    .cfg_fc_npd(12'd0),
    .cfg_fc_cpld(12'd0),
    .cfg_fc_sel(tx_cfg_fc_sel),
    .gen_transaction(1'b0),
    //
    .bufrd_rq_start(bufrd_rq_start_q),
    .bufrd_rq_addr(bufrd_rq_host_addr),
    .bufrd_rq_data(bufrd_payload),
    .bufrd_packet_done(bufrd_packet_done),

    .bufwr_rq_start(bufwr_rq_start_q),
    .bufwr_rq_addr(bufwr_rq_host_addr),
    .bufwr_packet_done(bufwr_rq_packet_done),
    .dbg_rq_state(dbg_rq_state)
  );
    
  // ------------------------------------------------------------
  // TEMPORARY write-only CQ-to-AXI-Lite bridge for BAR0 regs
  // CC is still disabled, so reads are disabled.
  // ------------------------------------------------------------

  (* mark_debug = "true" *) reg axiw_pending;
  (* mark_debug = "true" *) wire live_wr_en_dbg = wr_en;

  // Hold the CQ RX engine in its write-wait state until the
  // BAR0 AXI-Lite write has been accepted.
  assign portal_wr_busy = axiw_pending || (wr_en && wr_to_bar0);
  
  reg [15:0] regs_awaddr_r;
  reg [31:0] regs_wdata_r;
  reg [3:0]  regs_wstrb_r;

  assign regs_awaddr           = regs_awaddr_r;
  assign regs_wdata            = regs_wdata_r;
  assign regs_wstrb            = regs_wstrb_r;
  assign regs_awvalid          = axiw_pending;
  assign regs_wvalid           = axiw_pending;
   
  always @(posedge user_clk) begin
    if (!reset_n) begin
      axiw_pending  <= 1'b0;
      regs_awaddr_r <= 16'd0;
      regs_wdata_r  <= 32'd0;
      regs_wstrb_r  <= 4'd0;
      bufwr_completion_stored <= 1'b0;
    end else begin
      // Default low so this indication is a one-clock pulse.
      bufwr_completion_stored <= 1'b0;

      if (!axiw_pending && wr_en && wr_to_bar0) begin
        regs_awaddr_r <= {2'b00, wr_addr[11:0], 2'b00};
        regs_wdata_r  <= wr_data[31:0];
        regs_wstrb_r  <= wr_data_be[3:0];
        axiw_pending  <= 1'b1;
      end else if (axiw_pending && regs_awready && regs_wready) begin
        axiw_pending <= 1'b0;
      end
      
      // Local cache fills have priority over host CQ writes into BAR2.
      if (bufrd_seq_state == BUFRD_SEQ_CACHE_STORE) begin
          bar2_buf[bufrd_copy_bar2_index_q + 0] <= bufrd_copy_data_q[31:0];
          bar2_buf[bufrd_copy_bar2_index_q + 1] <= bufrd_copy_data_q[63:32];
          bar2_buf[bufrd_copy_bar2_index_q + 2] <= bufrd_copy_data_q[95:64];
          bar2_buf[bufrd_copy_bar2_index_q + 3] <= bufrd_copy_data_q[127:96];
      end else if (wr_en && wr_to_bar2) begin
          bar2_buf[wr_addr[6:0] + 0] <= wr_data[31:0];
          bar2_buf[wr_addr[6:0] + 1] <= wr_data[63:32];
          bar2_buf[wr_addr[6:0] + 2] <= wr_data[95:64];
          bar2_buf[wr_addr[6:0] + 3] <= wr_data[127:96];
      end

      // Local write-cache writeback has priority over RC and CQ BAR4 writes.
      if (bufwr_seq_state == BUFWR_SEQ_CACHE_STORE) begin
           bar4_buf[bufwr_copy_bar4_index_q + 0] <= bufwr_copy_data_q[31:0];
           bar4_buf[bufwr_copy_bar4_index_q + 1] <= bufwr_copy_data_q[63:32];
           bar4_buf[bufwr_copy_bar4_index_q + 2] <= bufwr_copy_data_q[95:64];
           bar4_buf[bufwr_copy_bar4_index_q + 3] <= bufwr_copy_data_q[127:96];
      end else if (bufwr_completion_received) begin
        if (bufwr_cache_xfer_active) begin
          // Cache writes occupy BAR2 + MAD_CACHE_SIZE through
          // BAR2 + (2 * MAD_CACHE_SIZE) - 1.
          bar2_buf[cache_write_dword_index + 0] <=
              bufwr_completion_payload[31:0];
          bar2_buf[cache_write_dword_index + 1] <=
              bufwr_completion_payload[63:32];
          bar2_buf[cache_write_dword_index + 2] <=
              bufwr_completion_payload[95:64];
          bar2_buf[cache_write_dword_index + 3] <=
              bufwr_completion_payload[127:96];
        end else begin
          bar4_buf[bufwr_bar4_idx + 0] <=
              bufwr_completion_payload[31:0];
          bar4_buf[bufwr_bar4_idx + 1] <=
              bufwr_completion_payload[63:32];
          bar4_buf[bufwr_bar4_idx + 2] <=
              bufwr_completion_payload[95:64];
          bar4_buf[bufwr_bar4_idx + 3] <=
              bufwr_completion_payload[127:96];
        end

        bufwr_completion_stored <= 1'b1;
      end else if (wr_en && wr_to_bar4) begin
        bar4_buf[wr_addr[10:2] + 0] <= wr_data[31:0];
        bar4_buf[wr_addr[10:2] + 1] <= wr_data[63:32];
        bar4_buf[wr_addr[10:2] + 2] <= wr_data[95:64];
        bar4_buf[wr_addr[10:2] + 3] <= wr_data[127:96];
      end
    end
  end

  assign regs_bready  = 1'b1;
  //
  assign dbg_regs_awvalid   = regs_awvalid;
  assign dbg_regs_awready   = regs_awready;
  assign dbg_regs_wvalid    = regs_wvalid;
  assign dbg_regs_wready    = regs_wready;
  assign dbg_regs_wstrb     = regs_wstrb;
  assign dbg_bufrd_go_pulse = bufrd_go_pulse;
  assign dbg_dma_go_pulse   = dma_go_pulse;

  assign regs_araddr  = {2'b00, req_addr[13:0]};
  assign regs_arvalid = req_compl_wd && rd_to_bar0;
  assign regs_rready  = 1'b1;
  
  assign dbg_regs_arvalid = regs_arvalid;
  assign dbg_regs_arready = regs_arready;
  assign dbg_regs_rvalid  = regs_rvalid;
  assign dbg_regs_rready  = regs_rready;
  assign dbg_regs_rdata   = regs_rdata;

  assign dbg_tx_rd_en     = tx_rd_en;
  assign dbg_tx_rd_addr   = tx_rd_addr;
  assign dbg_tx_rd_data   = tx_rd_data;
  
  assign dbg_regs_awaddr = regs_awaddr;
  assign dbg_regs_araddr = regs_araddr;
  assign dbg_regs_wdata  = regs_wdata;

  assign dbg_axiw_pending   = axiw_pending;
  assign dbg_portal_wr_busy = portal_wr_busy;
  assign dbg_live_wr_en     = live_wr_en_dbg;
  assign dbg_bufrd_start = bufrd_start;
 
mad_device_regs #(
    .ADDR_WIDTH(16),
    .MAD_CACHE_NUM_SECTORS(MAD_CACHE_NUM_SECTORS),
    .MAD_SGDMA_MAX_SECTORS(MAD_SGDMA_MAX_SECTORS)
) regs_i (
    .aclk(user_clk),
    .aresetn(reset_n),

    .s_axi_awaddr(regs_awaddr),
    .s_axi_awvalid(regs_awvalid),
    .s_axi_awready(regs_awready),

    .s_axi_wdata(regs_wdata),
    .s_axi_wstrb(regs_wstrb),
    .s_axi_wvalid(regs_wvalid),
    .s_axi_wready(regs_wready),

    .s_axi_bresp(regs_bresp),
    .s_axi_bvalid(regs_bvalid),
    .s_axi_bready(regs_bready),

    .s_axi_araddr(regs_araddr),
    .s_axi_arvalid(regs_arvalid),
    .s_axi_arready(regs_arready),
 
    .s_axi_rdata(regs_rdata),
    .s_axi_rresp(regs_rresp),
    .s_axi_rvalid(regs_rvalid),
    .s_axi_rready(regs_rready),

    .bufrd_go_pulse(bufrd_go_pulse),
    .dma_go_pulse(dma_go_pulse),
    .dma_busy(dma_busy),
    .dma_advance(dma_advance),
    .dma_complete(dma_complete),

    .sg_dma_begin(sg_dma_begin),
    .sg_dma_xfer_advance(sg_dma_xfer_advance),
    .sg_dma_complete(sg_dma_complete),
    .sg_dma_h2d(sg_dma_h2d),

    .bufrd_advance(bufrd_advance),
    .bufrd_io_complete(bufrd_io_complete),
      
    .bufrd_cache_first_use_complete(bufrd_cache_first_use_complete),
    .bufrd_cache_normal_complete(bufrd_cache_normal_complete),

    .bufwr_advance(bufwr_advance),
    .bufwr_io_complete(bufwr_io_complete),

    .bufwr_cache_first_use_complete(bufwr_cache_first_use_complete),
    .bufwr_cache_normal_complete(bufwr_cache_normal_complete),
 
    // SG-DMA descriptor interface
    .sg_desc_index(sg_desc_index),

    .bcdpp(bcdpp),
    .sg_host_addr(sg_host_addr),
    .sg_dev_data_ofst(sg_dev_data_ofst),
    .sg_dma_cntl(sg_dma_cntl),
    .sg_dxbc(sg_dxbc),
    .sg_reg20(sg_reg20),
    .sg_cdpp(sg_cdpp),
    .chained_dma_enabled(chained_dma_enabled),
    
    .host_addr(host_addr),
    .byte_indx_rd(byte_indx_rd),
    .byte_indx_wr(byte_indx_wr),
    .bufrd_transfer_length(bufrd_transfer_length),
    .cache_indx_rd(cache_indx_rd),
    .cache_indx_wr(cache_indx_wr),
    .read_cache_empty(read_cache_empty),
    .write_cache_empty(write_cache_empty),
    .int_enable_bufrd_input(int_enable_bufrd_input),
    .int_enable_bufrd_output(int_enable_bufrd_output),
    .cache_xfer_enabled(cache_xfer_enabled)
);
endmodule

module pio_rx_engine  #(
  parameter        TCQ = 1,
  parameter [1:0]  AXISTEN_IF_WIDTH = 00,
  parameter        AXISTEN_IF_CQ_ALIGNMENT_MODE   = "FALSE",
  parameter        AXISTEN_IF_RC_ALIGNMENT_MODE   = "FALSE",
  parameter        AXISTEN_IF_RC_STRADDLE         = 0,
  parameter        AXISTEN_IF_ENABLE_RX_MSG_INTFC = 0,
  parameter        AXISTEN_IF_CQ_PARITY_CHECK     = 0,
  parameter        AXISTEN_IF_RC_PARITY_CHECK     = 0,
  parameter [17:0] AXISTEN_IF_ENABLE_MSG_ROUTE    = 18'h2FFFF,


  // Do not override parameters below this line
  //parameter C_DATA_WIDTH = (AXISTEN_IF_WIDTH[1]) ? 256 : (AXISTEN_IF_WIDTH[0])? 128 : 64,
  parameter       AXI4_CQ_TUSER_WIDTH = 88,
  parameter       AXI4_RC_TUSER_WIDTH = 75,
  parameter        C_DATA_WIDTH = 128,
  parameter ADDR_W       = 5,                   // Memory Depth based on the C_DATA_WIDTH
  parameter MEM_W        = 512,                 // Memory Depth based on the C_DATA_WIDTH
  parameter BYTE_EN_W    = 64,                  // Width of byte enable going to memory for write data

  parameter STRB_WIDTH   = C_DATA_WIDTH / 8,               // TSTRB width
  parameter KEEP_WIDTH   = C_DATA_WIDTH / 32,
  parameter PARITY_WIDTH = C_DATA_WIDTH / 8,               // TPARITY width
  parameter integer REQ_ADDR_WIDTH = 23,
  parameter integer WR_ADDR_WIDTH  = 21
) (

  (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 user_clk CLK" *)
  (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF m_axis_cq:m_axis_rc:s_axis_cc, ASSOCIATED_RESET reset_n, FREQ_HZ 250000000" *)
  input                            user_clk,
  
  input                            reset_n,

  // Completer Request Interface
  input        [C_DATA_WIDTH-1:0]    m_axis_cq_tdata,
  input                              m_axis_cq_tlast,
  input                              m_axis_cq_tvalid,
  input [AXI4_CQ_TUSER_WIDTH-1:0]    m_axis_cq_tuser,
  input          [KEEP_WIDTH-1:0]    m_axis_cq_tkeep,
  input                   [5:0]    pcie_cq_np_req_count,
  output reg                       m_axis_cq_tready,
  output reg [1:0]                 pcie_cq_np_req,
  
  // Requester Completion Interface
  input         [C_DATA_WIDTH-1:0]    m_axis_rc_tdata,
  input                               m_axis_rc_tlast,
  input                               m_axis_rc_tvalid,
  input           [KEEP_WIDTH-1:0]    m_axis_rc_tkeep,
  input  [AXI4_RC_TUSER_WIDTH-1:0]    m_axis_rc_tuser,
  output reg                          m_axis_rc_tready,

  //RX Message Interface
  input                            cfg_msg_received,
  input                   [4:0]    cfg_msg_received_type,
  input                   [7:0]    cfg_msg_data,
  
  // Memory Read data handshake with Completion
  // transmit unit. Transmit unit reponds to
  // req_compl assertion and responds with compl_done
  // assertion when a Completion w/ data is transmitted.

  output reg                       req_compl,
  output reg                       req_compl_wd,
  output reg                       req_compl_ur,
  input                            compl_done,

  output reg              [2:0]    req_tc,             // Memory Read TC
  output reg              [2:0]    req_attr,           // Memory Read Attribute
  output reg             [10:0]    req_len,            // Memory Read Length
  output reg             [15:0]    req_rid,            // Memory Read Requestor ID { 8'b0 (Bus no),
                                                    //                            3'b0 (Dev no),
                                                    //                            5'b0 (Func no)}
  output reg              [7:0]    req_tag,            // Memory Read Tag
  output reg              [7:0]    req_be,             // Memory Read Byte Enables
  //output reg             [12:0]    req_addr,           // Memory Read Address
  output reg              [REQ_ADDR_WIDTH-1:0]  req_addr,
  output reg              [1:0]    req_at,             // Address Translation

  // Outputs to the TX Block in case of an UR
  // Required to form the completions

  output reg             [63:0]    req_des_qword0,     // DWord0 and Dword1 of descriptor of the request
  output reg             [63:0]    req_des_qword1,     // DWord2 and Dword3 of descriptor of the request
  output reg                       req_des_tph_present,// TPH Present in the request
  output reg              [1:0]    req_des_tph_type,   // If TPH Present then TPH type
  output reg              [7:0]    req_des_tph_st_tag, // TPH Steering tag of the request

  //Output to Indicate that the Request was a Mem lock Read Req

  output reg                       req_mem_lock,
  output reg                       req_mem,
  
  // Accepted raw Requester Completion beat
  output reg [C_DATA_WIDTH-1:0]       bufwr_rc_data,
  output reg [KEEP_WIDTH-1:0]         bufwr_rc_keep,
  output reg [AXI4_RC_TUSER_WIDTH-1:0] bufwr_rc_user,
  output reg                          bufwr_rc_valid,
  output reg                          bufwr_rc_last,

  //Memory interface used to save 2 DW data received
  //on Memory Write 32 TLP. Data extracted from
  //inbound TLP is presented to the Endpoint memory
  //unit. Endpoint memory unit reacts to wr_en
  //assertion and asserts wr_busy when it is
  //processing written information.

  output reg         [MEM_W-1:0]    wr_data,            // Memory Write Data changed the memory width from 64 bit to generic mode
  output reg         [10:0]         payload_len,        // Transaction Payload Length Changed from 1 bit to 11 bit
  output reg                        wr_sop,             // SOP to EP Mem Controller
  output reg                        wr_eop,             // EOP to EP Mem Controller
  output reg    [BYTE_EN_W-1:0]     wr_data_be,         // Byte Enable to EP Mem Controller
  output reg             [WR_ADDR_WIDTH-1:0]    wr_addr,             // Memory Write Address
  output reg              [7:0]    wr_be,               // Memory Write Byte Enable
  output reg                       wr_en,               // Memory Write Enable

  output wire                      dbg_sop,
  output wire                      dbg_in_packet_q,
  output wire              [7:0]   dbg_rx_state,

  input                            wr_busy              // Memory Write Busy
);

  localparam PIO_RX_MEM_RD_FMT_TYPE    = 4'b0000;    // Memory Read
  localparam PIO_RX_MEM_WR_FMT_TYPE    = 4'b0001;    // Memory Write
  localparam PIO_RX_IO_RD_FMT_TYPE     = 4'b0010;    // IO Read
  localparam PIO_RX_IO_WR_FMT_TYPE     = 4'b0011;    // IO Write
  localparam PIO_RX_ATOP_FAA_FMT_TYPE  = 4'b0100;    // Fetch and ADD
  localparam PIO_RX_ATOP_UCS_FMT_TYPE  = 4'b0101;    // Unconditional SWAP
  localparam PIO_RX_ATOP_CAS_FMT_TYPE  = 4'b0110;    // Compare and SWAP
  localparam PIO_RX_MEM_LK_RD_FMT_TYPE = 4'b0111;    // Locked Read Request
  localparam PIO_RX_MSG_FMT_TYPE       = 4'b1100;    // MSG Transaction apart from Vendor Defined and ATS
  localparam PIO_RX_MSG_VD_FMT_TYPE    = 4'b1101;    // MSG Transaction apart from Vendor Defined and ATS
  localparam PIO_RX_MSG_ATS_FMT_TYPE   = 4'b1110;    // MSG Transaction apart from Vendor Defined and ATS

  localparam PIO_RX_RST_STATE          = 8'b00000000;
  localparam PIO_RX_WAIT_STATE         = 8'b00000001;
  localparam PIO_RX_64_QW1             = 8'b00000010;
  localparam PIO_RX_DATA               = 8'b00000011;
  localparam PIO_RX_DATA2              = 8'b00000100;
  localparam PIO_RX_DATA_WR            = 8'b00000101; // Added for supporting any payload length
  localparam PIO_RX_WRITE_COMMIT      = 8'b00000110; // One-cycle delayed local write pulse

  localparam BAR_ID_SELECT = (C_DATA_WIDTH == 64) ? 48 : 112;

  // Local Registers
  reg [3:0]          trn_type;

  reg [1:0]          region_select;
  reg [1:0]          bar_region_q;
  
  //
  (* mark_debug = "true" *) reg [7:0] rx_state;
   (* mark_debug = "true" *) wire sop;
  (* mark_debug = "true" *) reg  in_packet_q;

  reg [2:0]          data_start_loc;

  wire               io_bar_hit_n;
  wire               mem32_bar_hit_n;
  wire               mem64_bar_hit_n;
  wire               erom_bar_hit_n;
  wire [1:0]         bar_region;

  // Decode the BAR hit bits from the CQ descriptor early, before the RX generate block.
  // This avoids relying on declarations/assignments that appear after procedural code.
  assign io_bar_hit_n    = (m_axis_cq_tdata[BAR_ID_SELECT+:3] == 3'b010) ? 1'b0 : 1'b1; // BAR2
  assign mem64_bar_hit_n = (m_axis_cq_tdata[BAR_ID_SELECT+:3] == 3'b100) ? 1'b0 : 1'b1; // BAR4
  assign erom_bar_hit_n  = (m_axis_cq_tdata[BAR_ID_SELECT+:3] == 3'b110) ? 1'b0 : 1'b1;
  assign mem32_bar_hit_n = (m_axis_cq_tdata[BAR_ID_SELECT+:3] == 3'b000) ? 1'b0 : 1'b1; // BAR0

  assign bar_region =
        (!io_bar_hit_n)    ? 2'b01 :   // BAR2/resource2
        (!mem64_bar_hit_n) ? 2'b10 :   // BAR4/resource4
                             2'b00;    // BAR0/resource0

  reg [15:0]         req_snoop_latency;
  reg [15:0]         req_no_snoop_latency;
  reg [3:0]          req_obff_code;
  reg [7:0]          req_msg_code;
  reg [2:0]          req_msg_route;
  reg [15:0]         req_dst_id;
  reg [15:0]         req_vend_id;
  reg [31:0]         req_vend_hdr;
  reg [127:0]        req_tl_hdr;

  reg [C_DATA_WIDTH-1:0] m_axis_cq_tdata_q;
  reg [AXI4_CQ_TUSER_WIDTH-1:0] m_axis_cq_tuser_q;
  reg                m_axis_cq_tvalid_reg;
  reg [AXI4_CQ_TUSER_WIDTH-1:0] m_axis_cq_tuser_reg;

  reg     [31:0]                  m_axis_cq_tparity;
  reg     [31:0]                  m_axis_cq_tparity_q;
  reg     [KEEP_WIDTH-1:0]        m_axis_cq_tkeep_q;
  wire    [PARITY_WIDTH-1:0]      m_axis_cq_tparity_cal;
  reg     [PARITY_WIDTH-1:0]      m_axis_cq_tparity_cal_q;
  reg                             m_axis_cq_tvalid_q;
  wire                            parity_error;
  reg                             parity_error_latch;
  reg     [10:0] len_i; 
  
 // Generate a signal that indicates if we are currently receiving a packet.
 // This value is one clock cycle delayed from what is actually on the AXIS
 // data bus.

 always@(posedge user_clk)
  begin
    if(!reset_n)
      in_packet_q <= #   TCQ 1'b0;
    else if (m_axis_cq_tvalid && m_axis_cq_tready && m_axis_cq_tlast)
      in_packet_q <= #   TCQ 1'b0;
    else if (sop && m_axis_cq_tready)
      in_packet_q <= #   TCQ 1'b1;
  end

  assign sop = (rx_state == PIO_RX_RST_STATE) && m_axis_cq_tvalid;

  assign dbg_sop         = sop;
  assign dbg_in_packet_q = in_packet_q;
  assign dbg_rx_state    = rx_state;

  always @(posedge user_clk)
  begin
    if(!reset_n)
    begin
      m_axis_cq_tdata_q    <= #TCQ {C_DATA_WIDTH{1'b0}};
      m_axis_cq_tuser_reg  <= #TCQ {AXI4_CQ_TUSER_WIDTH{1'b0}};
    end
    else begin
      if(m_axis_cq_tvalid)
      begin
        m_axis_cq_tdata_q    <= #TCQ m_axis_cq_tdata;
	m_axis_cq_tuser_reg  <= #TCQ m_axis_cq_tuser;
      end
    end
  end
  
  generate
  if(AXISTEN_IF_CQ_PARITY_CHECK == 1) begin
    genvar a;
    for(a=0; a< STRB_WIDTH; a = a + 1) // Parity needs to be computed for every byte of data
    begin : parity_assign
        assign m_axis_cq_tparity_cal[a] = !(  m_axis_cq_tdata[(8*a)+ 0] ^ m_axis_cq_tdata[(8*a)+ 1]
                                            ^ m_axis_cq_tdata[(8*a)+ 2] ^ m_axis_cq_tdata[(8*a)+ 3]
                                            ^ m_axis_cq_tdata[(8*a)+ 4] ^ m_axis_cq_tdata[(8*a)+ 5]
                                            ^ m_axis_cq_tdata[(8*a)+ 6] ^ m_axis_cq_tdata[(8*a)+ 7]);
    end
    
    always @(posedge user_clk)
    begin
      if(!reset_n)
      begin
        m_axis_cq_tuser_q       <= #TCQ {AXI4_CQ_TUSER_WIDTH{1'd0}};
        m_axis_cq_tkeep_q       <= #TCQ {KEEP_WIDTH{1'd0}};
        m_axis_cq_tparity_cal_q <= #TCQ 'd0;
      end
      else begin
        if(m_axis_cq_tvalid)
        begin
          m_axis_cq_tuser_q    <= #TCQ m_axis_cq_tuser;
          m_axis_cq_tkeep_q    <= #TCQ m_axis_cq_tkeep;
          m_axis_cq_tparity_cal_q <= #TCQ m_axis_cq_tparity_cal;
        end
      end
    end

    always @(posedge user_clk)
    begin
      if(!reset_n)
        m_axis_cq_tvalid_q <= 'b0;
      else
        m_axis_cq_tvalid_q <= #TCQ m_axis_cq_tvalid;
    end

    always @(posedge user_clk)
    begin
      if(!reset_n)
        parity_error_latch <= 'd0;
      else if (m_axis_cq_tvalid_q)
        parity_error_latch <= parity_error_latch ? 1'b1 : parity_error;
    end

    if (C_DATA_WIDTH == 512) begin : pio_parity_512
      assign  parity_error  = m_axis_cq_tvalid_q && ((m_axis_cq_tkeep_q[15] && ((m_axis_cq_tparity_cal_q[63:60] != m_axis_cq_tparity[63:60]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[14] && ((m_axis_cq_tparity_cal_q[59:56] != m_axis_cq_tparity[59:56]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[13] && ((m_axis_cq_tparity_cal_q[55:52] != m_axis_cq_tparity[55:52]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[12] && ((m_axis_cq_tparity_cal_q[51:48] != m_axis_cq_tparity[51:48]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[11] && ((m_axis_cq_tparity_cal_q[47:44] != m_axis_cq_tparity[47:44]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[10] && ((m_axis_cq_tparity_cal_q[43:40] != m_axis_cq_tparity[43:40]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[9]  && ((m_axis_cq_tparity_cal_q[39:36] != m_axis_cq_tparity[39:36]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[8]  && ((m_axis_cq_tparity_cal_q[35:32] != m_axis_cq_tparity[35:32]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[7]  && ((m_axis_cq_tparity_cal_q[31:28] != m_axis_cq_tparity[31:28]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[6]  && ((m_axis_cq_tparity_cal_q[27:24] != m_axis_cq_tparity[27:24]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[5]  && ((m_axis_cq_tparity_cal_q[23:20] != m_axis_cq_tparity[23:20]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[4]  && ((m_axis_cq_tparity_cal_q[19:16] != m_axis_cq_tparity[19:16]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[3]  && ((m_axis_cq_tparity_cal_q[15:12] != m_axis_cq_tparity[15:12]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[2]  && ((m_axis_cq_tparity_cal_q[11:08] != m_axis_cq_tparity[11:08]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[1]  && ((m_axis_cq_tparity_cal_q[07:04] != m_axis_cq_tparity[07:04]) ? 1'b1 : 1'b0)) | 
                                                     (m_axis_cq_tkeep_q[0]  && ((m_axis_cq_tparity_cal_q[03:00] != m_axis_cq_tparity[03:00]) ? 1'b1 : 1'b0))); 
    end
    else if (C_DATA_WIDTH == 256) begin : pio_parity_256
      assign  parity_error  = m_axis_cq_tvalid_q && ((m_axis_cq_tparity_cal_q[31:0] != m_axis_cq_tparity[31:0]) ? 1'b1 : 1'b0);
    end
    else if (C_DATA_WIDTH == 128) begin : pio_parity_128
      assign  parity_error  = m_axis_cq_tvalid_q && ((m_axis_cq_tparity_cal_q[15:0] != m_axis_cq_tparity[15:0]) ? 1'b1 : 1'b0);
    end
    else if (C_DATA_WIDTH == 64) begin : pio_parity_64
      assign  parity_error  = m_axis_cq_tvalid_q && ((m_axis_cq_tparity_cal_q[7:0] != m_axis_cq_tparity[7:0]) ? 1'b1 : 1'b0);
    end
  
  end
  endgenerate

  // ------------------------------------------------------------
  // Raw Requester Completion capture
  //
  // bufwr_rc_valid is a one-clock pulse for every RC AXI-stream
  // beat actually accepted by this endpoint.
  // ------------------------------------------------------------
  always @(posedge user_clk) begin
    if (!reset_n) begin
      bufwr_rc_data  <= {C_DATA_WIDTH{1'b0}};
      bufwr_rc_keep  <= {KEEP_WIDTH{1'b0}};
      bufwr_rc_user  <= {AXI4_RC_TUSER_WIDTH{1'b0}};
      bufwr_rc_valid <= 1'b0;
      bufwr_rc_last  <= 1'b0;
    end else begin
      bufwr_rc_valid <= 1'b0;
      bufwr_rc_last  <= 1'b0;

      if (m_axis_rc_tvalid && m_axis_rc_tready) begin
        bufwr_rc_data  <= m_axis_rc_tdata;
        bufwr_rc_keep  <= m_axis_rc_tkeep;
        bufwr_rc_user  <= m_axis_rc_tuser;
        bufwr_rc_valid <= 1'b1;
        bufwr_rc_last  <= m_axis_rc_tlast;
      end
    end
  end

  generate
    //State machine for D-word Aligned Mode
    if (C_DATA_WIDTH == 64) begin : pio_rx_sm_64_dword_aligned
      reg [63:0]           desc_hdr_qw0;
      reg [7:0]            req_byte_enables;

      always@(posedge user_clk) begin
        if (!reset_n) begin

          desc_hdr_qw0        <= #TCQ 64'h0;
          m_axis_cq_tready    <= #TCQ 1'b0;
          m_axis_rc_tready    <= #TCQ 1'b1;
          pcie_cq_np_req      <= #TCQ 2'b01;

          req_compl           <= #TCQ 1'b0;
          req_compl_wd        <= #TCQ 1'b0;
          req_compl_ur        <= #TCQ 1'b0;

          req_tc              <= #TCQ 3'b0;
          req_attr            <= #TCQ 3'b0;
          req_len             <= #TCQ 11'b0;
          req_rid             <= #TCQ 16'b0;
          req_tag             <= #TCQ 8'b0;
          req_be              <= #TCQ 8'b0;
          req_addr            <= #TCQ 23'b0;
          req_at              <= #TCQ 2'b0;

          wr_be               <= #TCQ 8'b0;
          wr_addr             <= #TCQ 21'b0;
          wr_data             <= #TCQ 64'h0;
          wr_en               <= #TCQ 1'b0;
          payload_len         <= #TCQ 11'b0;
          data_start_loc      <= #TCQ 3'b0;

          rx_state               <= #TCQ PIO_RX_RST_STATE;
          trn_type            <= #TCQ 4'b0;

          req_snoop_latency   <= #TCQ 16'b0;
          req_no_snoop_latency<= #TCQ 16'b0;
          req_obff_code       <= #TCQ 4'b0;
          req_msg_code        <= #TCQ 8'b0;
          req_msg_route       <= #TCQ 3'b0;
          req_dst_id          <= #TCQ 16'b0;
          req_vend_id         <= #TCQ 16'b0;
          req_vend_hdr        <= #TCQ 32'b0;
          req_tl_hdr          <= #TCQ 128'b0;

          req_des_qword0      <= #TCQ 64'b0;
          req_des_qword1      <= #TCQ 64'b0;
          req_des_tph_present <= #TCQ 1'b0;
          req_des_tph_type    <= #TCQ 2'b0;
          req_des_tph_st_tag  <= #TCQ 8'b0;

          req_mem_lock        <= #TCQ 1'b0;
          req_mem             <= #TCQ 1'b0;
	  m_axis_cq_tparity   <= #TCQ 32'b0;
	  m_axis_cq_tparity_q <= #TCQ 32'b0;
		  
	  len_i               <= #TCQ 11'h0; 
          wr_sop              <= #TCQ 1'b0;  
          wr_eop              <= #TCQ 1'b0;
          wr_data_be          <= #TCQ 8'b0;  
        end
        else begin
          wr_en               <= #TCQ 1'b0;
          req_compl           <= #TCQ 1'b0;
	      m_axis_cq_tparity   <= #TCQ m_axis_cq_tuser[84:53];
	      m_axis_cq_tparity_q <= #TCQ m_axis_cq_tuser_q[84:53];

          case (rx_state)
            PIO_RX_RST_STATE : begin

              m_axis_cq_tready <= #TCQ 1'b1;
              m_axis_rc_tready <= #TCQ 1'b1;

              if (sop) begin
                desc_hdr_qw0     <= #TCQ m_axis_cq_tdata[63:0];
                req_byte_enables <= #TCQ m_axis_cq_tuser[7:0];
                rx_state            <= #TCQ PIO_RX_64_QW1;
				wr_sop           <= #TCQ m_axis_cq_tuser[40];
				wr_eop           <= #TCQ m_axis_cq_tlast;
              end
              else
                rx_state            <= #TCQ PIO_RX_RST_STATE;
            end // PIO_RX_RST_STATE

            PIO_RX_64_QW1 : begin
              if (m_axis_cq_tvalid) begin
                case (m_axis_cq_tdata[14:11])

                  PIO_RX_MEM_RD_FMT_TYPE : begin

                    trn_type            <= #TCQ m_axis_cq_tdata[14:11];
                    req_len             <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready    <= #TCQ 1'b0;
                    req_mem             <= #TCQ 1'b1;
                    rx_state               <= #TCQ PIO_RX_WAIT_STATE;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if(m_axis_cq_tdata[10:0] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      req_addr         <= #TCQ {region_select[1:0],desc_hdr_qw0[20:2], 2'b00};
                      req_at           <= #TCQ desc_hdr_qw0[1:0];
                      
		      payload_len      <= #TCQ m_axis_cq_tdata[10:0];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end  // PIO_RX_MEM_RD_FMT_TYPE

                  PIO_RX_MEM_WR_FMT_TYPE : begin
                    trn_type            <= #TCQ m_axis_cq_tdata[14:11];
                    req_len             <= #TCQ m_axis_cq_tdata[10:0];
                    req_mem             <= #TCQ 1'b0;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if(m_axis_cq_tdata[10:0] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      req_addr         <= #TCQ {region_select[1:0],desc_hdr_qw0[20:2], 2'b00};
                      req_at           <= #TCQ desc_hdr_qw0[1:0];

                      payload_len      <= #TCQ m_axis_cq_tdata[10:0];
                      wr_sop           <= #TCQ m_axis_cq_tuser[40];
	              wr_eop           <= #TCQ m_axis_cq_tlast;
                      data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE == "TRUE") ? {2'b0,m_axis_cq_tdata_q[2]} : 3'b0;
                      rx_state            <= #TCQ PIO_RX_DATA;
                    end
                    else begin // Payload length == 0
                      rx_state            <= #TCQ PIO_RX_RST_STATE;
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end // PIO_RX_MEM_WR_FMT_TYPE


                  PIO_RX_IO_RD_FMT_TYPE : begin

                    trn_type         <= #TCQ m_axis_cq_tdata[14:11];
                    req_len          <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[10:0] == 11'h001) || (m_axis_cq_tdata[10:0] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      req_addr         <= #TCQ {region_select[1:0],desc_hdr_qw0[20:2], 2'b00};
                      req_at           <= #TCQ desc_hdr_qw0[1:0];
                      if(m_axis_cq_tdata[10:0] == 11'h002)
                        payload_len    <= #TCQ 1'b1;
                      else
                        payload_len    <= #TCQ 1'b0;
                      end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end

                  end //PIO_RX_IO_RD_FMT_TYPE

                  PIO_RX_IO_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[14:11];
                    req_len          <= #TCQ m_axis_cq_tdata[10:0];
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[10:0] == 11'h001) || (m_axis_cq_tdata[10:0] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      req_addr         <= #TCQ {region_select[1:0],desc_hdr_qw0[20:2], 2'b00};
                      req_at           <= #TCQ desc_hdr_qw0[1:0];
                      if(m_axis_cq_tdata[10:0] == 11'h002)
                        payload_len    <=#TCQ 1'b1;
                      else
                        payload_len   <=#TCQ 1'b0;

                      data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE == "TRUE") ? {2'b0,m_axis_cq_tdata_q[2]} : 3'b0;
                      rx_state            <= #TCQ PIO_RX_DATA;

                    end
                    else begin // Payload > 2DWORDs
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      rx_state            <= #TCQ PIO_RX_RST_STATE;
                    end
                  end // PIO_RX_IO_WR_FMT_TYPE

                  PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[14:11];
                    req_len          <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[10:0] == 11'h001) || (m_axis_cq_tdata[10:0] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end // PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE

                  PIO_RX_MEM_LK_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[14:11];
                    req_len          <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_des_qword0      <= #TCQ desc_hdr_qw0[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[10:0] == 11'h001) || (m_axis_cq_tdata[10:0] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[59:57];
                      req_attr         <= #TCQ m_axis_cq_tdata[62:60];
                      req_rid          <= #TCQ m_axis_cq_tdata[31:16];
                      req_tag          <= #TCQ m_axis_cq_tdata[39:32];
                      req_be           <= #TCQ req_byte_enables;
                      req_mem_lock     <= #TCQ 1'b1;
                      req_addr         <= #TCQ {region_select[1:0],desc_hdr_qw0[20:2], 2'b00};
                      req_at           <= #TCQ desc_hdr_qw0[1:0];
                      if(m_axis_cq_tdata[10:0] == 11'h002)
                        payload_len    <=#TCQ 1'b1;
                      else
                        payload_len   <=#TCQ 1'b0;
                    end
                    else begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end //PIO_RX_MEM_LK_RD_FMT_TYPE

                  PIO_RX_MSG_FMT_TYPE : begin
                    req_snoop_latency    <= #TCQ desc_hdr_qw0[15:0];
                    req_no_snoop_latency <= #TCQ desc_hdr_qw0[31:16];
                    req_obff_code        <= #TCQ desc_hdr_qw0[35:32];
                    trn_type             <= #TCQ m_axis_cq_tdata[14:11];
                    req_len              <= #TCQ m_axis_cq_tdata[10:0];
                    req_mem              <= #TCQ 1'b0;
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[59:57];
                    req_attr             <= #TCQ m_axis_cq_tdata[62:60];
                    req_at               <= #TCQ desc_hdr_qw0[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[31:16];
                    req_tag              <= #TCQ m_axis_cq_tdata[39:32];
                    req_be               <= #TCQ req_byte_enables;
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_FMT_TYPE

                  PIO_RX_MSG_VD_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[14:11];
                    req_len              <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[59:57];
                    req_attr             <= #TCQ m_axis_cq_tdata[62:60];
                    req_rid              <= #TCQ m_axis_cq_tdata[31:16];
                    req_tag              <= #TCQ m_axis_cq_tdata[39:32];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    req_be               <= #TCQ req_byte_enables;
                    req_at               <= #TCQ desc_hdr_qw0[1:0];
                    req_dst_id           <= #TCQ desc_hdr_qw0[15:0];
                    req_vend_id          <= #TCQ desc_hdr_qw0[31:16];
                    req_vend_hdr         <= #TCQ desc_hdr_qw0[63:32];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_VD_FMT_TYPE

                  PIO_RX_MSG_ATS_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[14:11];
                    req_len              <= #TCQ m_axis_cq_tdata[10:0];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[59:57];
                    req_attr             <= #TCQ m_axis_cq_tdata[62:60];
                    req_rid              <= #TCQ m_axis_cq_tdata[31:16];
                    req_tag              <= #TCQ m_axis_cq_tdata[39:32];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    req_be               <= #TCQ req_byte_enables;
                    req_at               <= #TCQ desc_hdr_qw0[1:0];
                    req_tl_hdr[127:64]   <= #TCQ desc_hdr_qw0[63:0];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_ATS_FMT_TYPE

                  default : begin // other TLPs
                    rx_state        <= #TCQ PIO_RX_64_QW1;
                  end
                endcase // Req_Type
              end // m_axis_cq_tvalid
              else
                rx_state <= #TCQ PIO_RX_64_QW1;
            end // PIO_RX_64_QW1

            PIO_RX_DATA : begin
            if (AXISTEN_IF_CQ_ALIGNMENT_MODE == "TRUE") begin 
              if (m_axis_cq_tvalid)
              begin
                wr_addr          <= #TCQ req_addr[22:2];
                case (data_start_loc)
                  3'b000 : begin
                    wr_data          <= #TCQ payload_len ? m_axis_cq_tdata[63:0] : {32'h0, m_axis_cq_tdata[31:0]};
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[15:8] : { 4'h0, m_axis_cq_tuser[11:8]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b001 : begin
                    wr_data          <= #TCQ {32'h0, m_axis_cq_tdata[63:32]};
                    wr_be            <= #TCQ { 4'h0, m_axis_cq_tuser[15:12]};
                    wr_en            <= #TCQ payload_len ? 1'b0 : 1'b1;
                    rx_state            <= #TCQ payload_len ? PIO_RX_DATA2 : PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ payload_len ? 1'b1 : 1'b0;
                  end
                  default : begin
                    rx_state        <= #TCQ PIO_RX_DATA;
                  end
                endcase
              end // if (m_axis_cq_tvalid)
              else
                rx_state        <= #TCQ PIO_RX_DATA;
            end //Address Align mode 
			else begin
			  if (m_axis_cq_tvalid)
			  begin 
			    wr_addr    <= #TCQ req_addr[22:2];
                            wr_data    <= #TCQ m_axis_cq_tdata[63:0];
                            wr_data_be <= #TCQ m_axis_cq_tuser[15:8];
                            wr_be      <= #TCQ m_axis_cq_tuser[15:8];
                            wr_en      <= #TCQ 1'b0;
                            wr_sop     <= #TCQ m_axis_cq_tuser[40];
                            wr_eop     <= #TCQ m_axis_cq_tlast;

                            if (payload_len <= 2) begin
                              len_i            <= #TCQ 11'h0;
                              rx_state            <= #TCQ PIO_RX_WRITE_COMMIT;
                              m_axis_cq_tready <= #TCQ 1'b1;
                            end
                else begin 
                  len_i            <= #TCQ payload_len - 11'h2 ; 
                  rx_state            <= #TCQ PIO_RX_DATA_WR;					
                  m_axis_cq_tready <= #TCQ 1'b1;
			    end
			  end // if end m_axis_cq_tvalid
			  else 
			    rx_state        <= #TCQ PIO_RX_DATA;
			end
            end // PIO_RX_DATA

            PIO_RX_DATA2 : begin
              if (m_axis_cq_tvalid && m_axis_cq_tlast)
              begin
                  wr_data[63:32]   <= #TCQ m_axis_cq_tdata[31:0];
                  wr_be[7:4]       <= #TCQ m_axis_cq_tuser[11:8];
                  wr_en            <= #TCQ 1'b1;
                  m_axis_cq_tready <= #TCQ 1'b0;
                  rx_state            <= #TCQ PIO_RX_WAIT_STATE;
              end // if (m_axis_cq_tvalid)
              else
              rx_state        <= #TCQ PIO_RX_DATA2;
            end // PIO_RX_DATA2
			
	    PIO_RX_DATA_WR : begin 
               if (m_axis_cq_tvalid) begin 
                  if ((len_i-1)/2 == 0 ) begin // if len_i <= 2
                       case (len_i) 
                          1 : begin 
                                wr_data    <= #TCQ {32'b0, m_axis_cq_tdata[31:0]}; 
                                wr_data_be <= #TCQ {4'b0,m_axis_cq_tuser[11:8]};
                                len_i      <= #TCQ len_i -1 ; 
                              end
                          2 : begin 
                                wr_data <= #TCQ m_axis_cq_tdata[63:0];
                                wr_data_be <= #TCQ m_axis_cq_tuser[15:8]; 
                                len_i <= #TCQ len_i-2; 
                              end
                       endcase
					   
                       rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                       wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                       wr_en            <= #TCQ 1'b1;
                       m_axis_cq_tready <= #TCQ 1'b0;
                       wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                       wr_eop           <= #TCQ m_axis_cq_tlast; 

                 end // if len_i <= 2
                 else begin // if len_i > 2
                       wr_data          <= #TCQ m_axis_cq_tdata[63:0]; 
                       wr_data_be       <= #TCQ m_axis_cq_tuser[15:8];
                       len_i            <= #TCQ len_i-2; 
                       rx_state            <= #TCQ PIO_RX_DATA_WR;
                       wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                       wr_en            <= #TCQ 1'b1;
                       m_axis_cq_tready <= #TCQ 1'b1;
                       wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                       wr_eop           <= #TCQ m_axis_cq_tlast;                            
                 end
               end // m_axis_cq_tvalid 
               else
                   rx_state        <= #TCQ PIO_RX_DATA_WR;
            end //PIO_RX_DATA_WR

            PIO_RX_WAIT_STATE : begin
              wr_en         <= #TCQ 1'b0;
              req_compl     <= #TCQ 1'b0;
              req_compl_wd  <= #TCQ 1'b0;
			  wr_sop        <= #TCQ m_axis_cq_tuser[40]; 
              wr_eop        <= #TCQ m_axis_cq_tlast; 

              if ((trn_type == PIO_RX_MEM_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state            <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_LK_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if (((trn_type == PIO_RX_ATOP_FAA_FMT_TYPE) || (trn_type == PIO_RX_ATOP_UCS_FMT_TYPE) ||
                            (trn_type == PIO_RX_ATOP_CAS_FMT_TYPE)) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else
                rx_state        <= #TCQ PIO_RX_WAIT_STATE;
            end // PIO_RX_WAIT_STATE

            default : begin
              // default case stmt
              rx_state        <= #TCQ PIO_RX_RST_STATE;
            end // default
          endcase
        end // if reset_n
      end // always @ user_clk
    end // pio_rx_sm_64

    else if (C_DATA_WIDTH == 128) begin : pio_rx_sm_128_dword_aligned
      always@(posedge user_clk) begin
        if (!reset_n) begin
          m_axis_cq_tready    <= #TCQ 1'b0;
          m_axis_rc_tready    <= #TCQ 1'b1;
          pcie_cq_np_req      <= #TCQ 2'b01;

          req_compl           <= #TCQ 1'b0;
          req_compl_wd        <= #TCQ 1'b0;
          req_compl_ur        <= #TCQ 1'b0;

          req_tc              <= #TCQ 3'b0;
          req_attr            <= #TCQ 3'b0;
          req_len             <= #TCQ 11'b0;
          req_rid             <= #TCQ 16'b0;
          req_tag             <= #TCQ 8'b0;
          req_be              <= #TCQ 8'b0;
          req_addr            <= #TCQ 23'b0;
          bar_region_q        <= #TCQ 2'b00;
          req_at              <= #TCQ 2'b0;

          wr_be               <= #TCQ 8'b0;
          wr_addr             <= #TCQ 21'b0;
          wr_data             <= #TCQ 128'h0;
          wr_en               <= #TCQ 1'b0;
          payload_len         <= #TCQ 11'b0;
          data_start_loc      <= #TCQ 3'b0;

          rx_state            <= #TCQ PIO_RX_RST_STATE;
          trn_type            <= #TCQ 4'b0;

          req_snoop_latency   <= #TCQ 16'b0;
          req_no_snoop_latency<= #TCQ 16'b0;
          req_obff_code       <= #TCQ 4'b0;
          req_msg_code        <= #TCQ 8'b0;
          req_msg_route       <= #TCQ 3'b0;
          req_dst_id          <= #TCQ 16'b0;
          req_vend_id         <= #TCQ 16'b0;
          req_vend_hdr        <= #TCQ 32'b0;
          req_tl_hdr          <= #TCQ 128'b0;


          req_des_qword0      <= #TCQ 64'b0;
          req_des_qword1      <= #TCQ 64'b0;
          req_des_tph_present <= #TCQ 1'b0;
          req_des_tph_type    <= #TCQ 2'b0;
          req_des_tph_st_tag  <= #TCQ 8'b0;

          req_mem_lock        <= #TCQ 1'b0;
          req_mem             <= #TCQ 1'b0;
	      m_axis_cq_tparity   <= #TCQ 32'b0;
	      m_axis_cq_tparity_q <= #TCQ 32'b0;
		  
	  len_i               <= #TCQ 11'h0; 
          wr_sop              <= #TCQ 1'b0;  
          wr_eop              <= #TCQ 1'b0;
          wr_data_be          <= #TCQ 16'b0;  
        end
        else begin
          wr_en               <= #TCQ 1'b0;
          req_compl           <= #TCQ 1'b0;
	      m_axis_cq_tparity   <= #TCQ m_axis_cq_tuser[84:53];
	      m_axis_cq_tparity_q <= #TCQ m_axis_cq_tuser_q[84:53];

          case (rx_state)
            PIO_RX_RST_STATE : begin
              m_axis_cq_tready <= #TCQ 1'b1;
              m_axis_rc_tready <= #TCQ 1'b1;
              if (sop)
              begin
                case (m_axis_cq_tdata[78:75])
                  PIO_RX_MEM_RD_FMT_TYPE : begin

                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;

                    if(m_axis_cq_tdata[74:64] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      bar_region_q     <= #TCQ bar_region;
                      req_addr         <= #TCQ {bar_region, m_axis_cq_tdata[20:2], 2'b00};
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len      <= #TCQ m_axis_cq_tdata[74:64];
                    end
                    else begin
                      req_compl           <= #TCQ 1'b0;
                      req_compl_wd        <= #TCQ 1'b0;
                      req_compl_ur        <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    end
                  end  // PIO_RX_MEM_RD_FMT_TYPE

                  PIO_RX_MEM_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;  
                    wr_sop         <= #TCQ m_axis_cq_tuser[40];
		    wr_eop         <= #TCQ m_axis_cq_tlast;

                    if(m_axis_cq_tdata[74:64] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      bar_region_q     <= #TCQ bar_region;
                      req_addr         <= #TCQ {bar_region, m_axis_cq_tdata[20:2], 2'b00};
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
  		      payload_len      <= #TCQ m_axis_cq_tdata[74:64];
                      data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {1'b0,m_axis_cq_tdata[3:2]} : 3'b0;
                      rx_state            <= #TCQ PIO_RX_DATA;
                    end
                    else begin // Payload equals 0
                      rx_state            <= #TCQ PIO_RX_RST_STATE;
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    end
                  end // PIO_RX_MEM_WR_FMT_TYPE

                  PIO_RX_IO_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      if(m_axis_cq_tdata[74:64] == 11'h002)
                        payload_len    <= #TCQ 1'b1;
                      else
                        payload_len    <= #TCQ 1'b0;
                      end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    end
                  end //PIO_RX_IO_RD_FMT_TYPE

                  PIO_RX_IO_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      if(m_axis_cq_tdata[74:64] == 11'h002)
                        payload_len    <=#TCQ 1'b1;
                      else
                        payload_len   <=#TCQ 1'b0;

                      data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {1'b0,m_axis_cq_tdata[3:2]} : 3'b0;
                      rx_state            <= #TCQ PIO_RX_DATA;
                    end
                    else begin // Payload > 2DWORDs
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                      rx_state            <= #TCQ PIO_RX_RST_STATE;
                    end
                  end // PIO_RX_IO_WR_FMT_TYPE

                  PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    end
                  end // PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE

                  PIO_RX_MEM_LK_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_be           <= #TCQ m_axis_cq_tuser[7:0];
                      req_mem_lock     <= #TCQ 1'b1;
                      req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      if(m_axis_cq_tdata[74:64] == 11'h002)
                        payload_len    <=#TCQ 1'b1;
                      else
                        payload_len   <=#TCQ 1'b0;
                    end
                    else begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                      req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                      req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                      req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                      req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    end
                  end //PIO_RX_MEM_LK_RD_FMT_TYPE

                  PIO_RX_MSG_FMT_TYPE : begin
                    req_snoop_latency    <= #TCQ m_axis_cq_tdata[15:0];
                    req_no_snoop_latency <= #TCQ m_axis_cq_tdata[95:80];
                    req_obff_code        <= #TCQ m_axis_cq_tdata[35:32];
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem              <= #TCQ 1'b0;
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_FMT_TYPE

                  PIO_RX_MSG_VD_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_dst_id           <= #TCQ m_axis_cq_tdata[15:0];
                    req_vend_id          <= #TCQ m_axis_cq_tdata[95:80];
                    req_vend_hdr         <= #TCQ m_axis_cq_tdata[63:32];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_VD_FMT_TYPE

                  PIO_RX_MSG_ATS_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[47:40];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[50:48];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_tl_hdr[127:64]   <= #TCQ m_axis_cq_tdata[127:64];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_ATS_FMT_TYPE

                  default : begin // other TLPs
                    rx_state        <= #TCQ PIO_RX_RST_STATE;
                  end
                endcase // Req_Type
              end // m_axis_cq_tvalid
              else
                rx_state <= #TCQ PIO_RX_RST_STATE;
            end // PIO_RX_RST_STATE

            PIO_RX_DATA : begin
            if (AXISTEN_IF_CQ_ALIGNMENT_MODE == "TRUE") begin 
              if (m_axis_cq_tvalid)
              begin
                wr_addr <= #TCQ {bar_region_q, m_axis_cq_tdata[22:2]};
                case (data_start_loc)
                  3'b000 : begin
                    wr_data          <= #TCQ payload_len ? m_axis_cq_tdata[63:0] : {32'h0, m_axis_cq_tdata[31:0]};
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[15:8] : { 4'h0, m_axis_cq_tuser[11:8]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b001 : begin
                    wr_data          <= #TCQ payload_len ? m_axis_cq_tdata[95:32] : {32'h0, m_axis_cq_tdata[63:32]};
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[19:12] : { 4'h0, m_axis_cq_tuser[15:12]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b010 : begin
                    wr_data          <= #TCQ payload_len ? m_axis_cq_tdata[127:64] : {32'h0, m_axis_cq_tdata[95:64]};
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[23:16] : { 4'h0, m_axis_cq_tuser[19:16]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b011 : begin
                    wr_data          <= #TCQ {32'h0, m_axis_cq_tdata[127:96]};
                    wr_be            <= #TCQ { 4'h0, m_axis_cq_tuser[23:20]};
                    wr_en            <= #TCQ payload_len ? 1'b0 : 1'b1;
                    rx_state            <= #TCQ payload_len ? PIO_RX_DATA2 : PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ payload_len ? 1'b1 : 1'b0;
                  end
                  default : begin
                    rx_state        <= #TCQ PIO_RX_DATA;
                  end
                endcase
              end // if (m_axis_cq_tvalid)
              else
                rx_state        <= #TCQ PIO_RX_DATA;
			end	//address align mode end
			else begin
			  if (m_axis_cq_tvalid)
			  begin 
			     wr_addr   <= #TCQ req_addr[22:2];
			     wr_en     <= #TCQ 1'b0;
			     wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                             wr_eop           <= #TCQ m_axis_cq_tlast; 	
			  
			    if (payload_len < 5 ) begin    // Payload length is less than or equal to 4 D-Words 
				case (payload_len)
				    1 : begin 
				            wr_data <= #TCQ {96'b0, m_axis_cq_tdata[31:0]};
				            wr_data_be <= #TCQ {12'b0, m_axis_cq_tuser[11:8]}; 
				            wr_be      <= #TCQ {4'b0, m_axis_cq_tuser[11:8]};
				        end
				    2 : begin 
				            wr_data <= #TCQ {64'b0, m_axis_cq_tdata[63:0]};
				            wr_data_be <= #TCQ {8'b0, m_axis_cq_tuser[15:8]}; 
				            wr_be      <= #TCQ {4'b0, m_axis_cq_tuser[11:8]};
				        end
				    3 : begin 
				            wr_data <= #TCQ {32'b0, m_axis_cq_tdata[95:0]};
				            wr_data_be <= #TCQ {4'b0, m_axis_cq_tuser[19:8]}; 
				            wr_be      <= #TCQ {4'b0, m_axis_cq_tuser[11:8]};
				        end
				    4 : begin 
				            wr_data <= #TCQ m_axis_cq_tdata[127:0]; 
				            wr_data_be <= #TCQ m_axis_cq_tuser[23:8]; 
				            wr_be      <= #TCQ {4'b0, m_axis_cq_tuser[11:8]};
				        end
				    endcase
				    len_i            <= #TCQ 11'b0; 
				    rx_state            <= #TCQ PIO_RX_WRITE_COMMIT;
                                    m_axis_cq_tready <= #TCQ 1'b1;
		           end
                else begin 
                    len_i            <= #TCQ payload_len - 11'h4 ; 
		    rx_state            <= #TCQ PIO_RX_DATA_WR;					
                    m_axis_cq_tready <= #TCQ 1'b1;
		    end
		  end // if end m_axis_cq_tvalid
		  else 
		    rx_state        <= #TCQ PIO_RX_DATA;
		  end
                end // PIO_RX_DATA

            PIO_RX_WRITE_COMMIT : begin
              wr_en <= #TCQ 1'b1;
              m_axis_cq_tready <= #TCQ 1'b1;
              rx_state <= #TCQ PIO_RX_WAIT_STATE;
            end // PIO_RX_WRITE_COMMIT

            PIO_RX_DATA2 : begin
              if (m_axis_cq_tvalid && m_axis_cq_tlast)
              begin
                  wr_data[63:32]   <= #TCQ m_axis_cq_tdata[31:0];
                  wr_be[7:4]       <= #TCQ m_axis_cq_tuser[11:8];
                  wr_en            <= #TCQ 1'b1;
                  m_axis_cq_tready <= #TCQ 1'b0;
                  rx_state            <= #TCQ PIO_RX_WAIT_STATE;

              end // if (m_axis_cq_tvalid)
              else
              rx_state        <= #TCQ PIO_RX_DATA2;
            end // PIO_RX_DATA2
			
	    PIO_RX_DATA_WR : begin 
               if (m_axis_cq_tvalid) begin 
                  if ((len_i-1)/4 == 0 ) begin // if len_i <= 4
                       case (len_i) 
                          1 : begin 
                                wr_data    <= #TCQ {96'b0, m_axis_cq_tdata[31:0]}; 
                                wr_data_be <= #TCQ {12'b0,m_axis_cq_tuser[11:8]};
                                len_i      <= #TCQ len_i -1 ; 
                              end
                          2 : begin 
                                wr_data    <= #TCQ {64'b0, m_axis_cq_tdata[63:0]}; 
                                wr_data_be <= #TCQ {8'b0,m_axis_cq_tuser[15:8]};
                                len_i      <= #TCQ len_i-2; 
                              end
							  
			  3 : begin 
                                wr_data    <= #TCQ {32'b0, m_axis_cq_tdata[95:0]}; 
                                wr_data_be <= #TCQ {4'b0,m_axis_cq_tuser[19:8]};
                                len_i      <= #TCQ len_i-3; 
                              end
							  
						  4 : begin 
                                wr_data    <= #TCQ m_axis_cq_tdata[127:0]; 
                                wr_data_be <= #TCQ m_axis_cq_tuser[23:8];
                                len_i      <= #TCQ len_i-4; 
                              end
                       endcase
					   
                       rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                       wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                       wr_en            <= #TCQ 1'b1;
                       m_axis_cq_tready <= #TCQ 1'b0;
                       wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                       wr_eop           <= #TCQ m_axis_cq_tlast; 
                 end // if len_i <= 4
                 else begin // if len_i > 4
                       wr_data          <= #TCQ m_axis_cq_tdata[127:0]; 
                       wr_data_be       <= #TCQ m_axis_cq_tuser[23:8];
                       len_i            <= #TCQ len_i-4; 
                       rx_state            <= #TCQ PIO_RX_DATA_WR;
                       wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                       wr_en            <= #TCQ 1'b1;
                       m_axis_cq_tready <= #TCQ 1'b1;
                       wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                       wr_eop           <= #TCQ m_axis_cq_tlast;                            
                 end
               end // m_axis_cq_tvalid 
               else
               rx_state        <= #TCQ PIO_RX_DATA_WR;
            end //PIO_RX_DATA_WR

            PIO_RX_WAIT_STATE : begin
              wr_en        <= #TCQ 1'b0;
              req_compl    <= #TCQ 1'b0;
              req_compl_wd <= #TCQ 1'b0;

              wr_sop <= #TCQ 1'b0;
              wr_eop <= #TCQ 1'b0;

              if ((trn_type == PIO_RX_MEM_WR_FMT_TYPE) && (!wr_busy)) begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_WR_FMT_TYPE) && (!wr_busy)) begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_RD_FMT_TYPE) && (compl_done)) begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state <= #TCQ PIO_RX_RST_STATE;
              end else begin
                rx_state <= #TCQ PIO_RX_WAIT_STATE;
              end
            end // PIO_RX_WAIT_STATE

            default : begin
              // default case stmt
              rx_state        <= #TCQ PIO_RX_RST_STATE;
            end // default
          endcase
        end // if reset_n
      end // always @ user_clk
    end // pio_rx_sm_128

    else if (C_DATA_WIDTH == 256) begin : pio_rx_sm_256_dword_aligned
      always@(posedge user_clk) begin
        if(!reset_n) begin
          m_axis_cq_tready    <= #TCQ 1'b0;
          m_axis_rc_tready    <= #TCQ 1'b1;
          pcie_cq_np_req      <= #TCQ 2'b01;

          req_compl           <= #TCQ 1'b0;
          req_compl_wd        <= #TCQ 1'b0;
          req_compl_ur        <= #TCQ 1'b0;

          req_tc              <= #TCQ 3'b0;
          req_attr            <= #TCQ 3'b0;
          req_len             <= #TCQ 11'b0;
          req_rid             <= #TCQ 16'b0;
          req_tag             <= #TCQ 8'b0;
          req_be              <= #TCQ 8'b0;
          req_addr            <= #TCQ 23'b0;
          req_at              <= #TCQ 2'b0;

          wr_be               <= #TCQ 8'b0;
          wr_addr             <= #TCQ 21'b0;
          wr_data             <= #TCQ 256'h0;
          wr_en               <= #TCQ 1'b0;
          payload_len         <= #TCQ 11'b0;
          data_start_loc      <= #TCQ 3'b0;

          rx_state               <= #TCQ PIO_RX_RST_STATE;
          trn_type            <= #TCQ 4'b0;

          req_snoop_latency   <= #TCQ 16'b0;
          req_no_snoop_latency<= #TCQ 16'b0;
          req_obff_code       <= #TCQ 4'b0;
          req_msg_code        <= #TCQ 8'b0;
          req_msg_route       <= #TCQ 3'b0;
          req_dst_id          <= #TCQ 16'b0;
          req_vend_id         <= #TCQ 16'b0;
          req_vend_hdr        <= #TCQ 32'b0;
          req_tl_hdr          <= #TCQ 128'b0;

          req_des_qword0      <= #TCQ 64'b0;
          req_des_qword1      <= #TCQ 64'b0;
          req_des_tph_present <= #TCQ 1'b0;
          req_des_tph_type    <= #TCQ 2'b0;
          req_des_tph_st_tag  <= #TCQ 8'b0;

          req_mem_lock        <= #TCQ 1'b0;
          req_mem             <= #TCQ 1'b0;
	  m_axis_cq_tparity   <= #TCQ 32'b0;
	  m_axis_cq_tparity_q <= #TCQ 32'b0;
          len_i               <= #TCQ 11'h0; 
          wr_sop              <= #TCQ 1'b0;  
          wr_eop              <= #TCQ 1'b0;
          wr_data_be          <= #TCQ 32'b0;
        end
        else begin
          wr_en               <= #TCQ 1'b0;
          req_compl           <= #TCQ 1'b0;
	  m_axis_cq_tparity   <= #TCQ m_axis_cq_tuser[84:53];
	  m_axis_cq_tparity_q <= #TCQ m_axis_cq_tuser_q[84:53];

          case (rx_state)
            PIO_RX_RST_STATE : begin
              m_axis_cq_tready <= #TCQ 1'b1;
              m_axis_rc_tready <= #TCQ 1'b1;
              //req_compl_wd     <= #TCQ 1'b1;

              if (sop) begin //sop_if

                case( m_axis_cq_tdata[78:75] ) // Req_Type_fsm

                  PIO_RX_MEM_RD_FMT_TYPE : begin

                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ m_axis_cq_tuser[7:0];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    region_select    <= #TCQ 2'b00;
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if(m_axis_cq_tdata[74:64] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len      <= #TCQ m_axis_cq_tdata[74:64];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end  // PIO_RX_MEM_RD_FMT_TYPE

                  PIO_RX_MEM_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;
                    payload_len      <= #TCQ m_axis_cq_tdata[74:64];
                    wr_sop           <= #TCQ m_axis_cq_tuser[40]; 
                    wr_eop           <= #TCQ m_axis_cq_tlast; 
                    
                    if(AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE") begin // DWord Aligned Mode
                        if ( m_axis_cq_tdata[74:64] < 5 ) begin // DWORD Length less than 4 
                         case (m_axis_cq_tdata[74:64]) 
                              1 : begin wr_data <= #TCQ {224'b0, m_axis_cq_tdata[159:128]}; wr_data_be <= #TCQ {28'b0, m_axis_cq_tuser[27:24]}; end
                              2 : begin wr_data <= #TCQ {192'b0, m_axis_cq_tdata[191:128]}; wr_data_be <= #TCQ {24'b0, m_axis_cq_tuser[31:24]}; end
                              3 : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[223:128]}; wr_data_be <= #TCQ {20'b0, m_axis_cq_tuser[35:24]}; end
                              4 : begin wr_data <= #TCQ {128'b0, m_axis_cq_tdata[255:128]}; wr_data_be <= #TCQ {16'b0, m_axis_cq_tuser[39:24]}; end
                         endcase
                         len_i <= #TCQ 11'h0; 
                        end
                        else begin //DWORD Count is greater than 4 
                            wr_data    <= #TCQ {128'b0 , m_axis_cq_tdata[255:128]}; 
                            wr_data_be <= #TCQ {16'b0,m_axis_cq_tuser[39:24]}; 
                            len_i      <= #TCQ m_axis_cq_tdata[74:64] - 11'h4 ; 
                        end
                    end
			if ((m_axis_cq_tdata[74:64] < 11'h5 )) begin //if DWORD Count is less than or equal to 4
                           if (AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE" ) begin
                               rx_state            <= #TCQ PIO_RX_DATA;
                               req_addr         <= #TCQ {2'b00, m_axis_cq_tdata[20:2], 2'b00};
                               data_start_loc   <= #TCQ 3'b0;
                               m_axis_cq_tready <= #TCQ 1'b1;
                           end      
                           else begin // Address Aligned Mode
                               rx_state            <= #TCQ PIO_RX_DATA;
                               req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                               data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {m_axis_cq_tdata[4:2]} : 3'b0;
                           end
                      end                       
                      else begin 
                           if (AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE" ) begin //DWORD Aligned Mode 
                               rx_state            <= #TCQ PIO_RX_DATA_WR;
                               wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                               wr_en            <= #TCQ 1'b1;
                               wr_addr         <= #TCQ req_addr[22:2];
                               m_axis_cq_tready <= #TCQ 1'b1;
                           end 
                           else begin // Address Aligned Mode
                               rx_state            <= #TCQ PIO_RX_DATA;
                               req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                               data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {m_axis_cq_tdata[4:2]} : 3'b0;
                           end
                      end //if DWORD Count is more than 4
                   end  // PIO_RX_MEM_WR_FMT_TYPE

                  PIO_RX_IO_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ m_axis_cq_tuser[7:0];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len    <=#TCQ m_axis_cq_tdata[65];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end //PIO_RX_IO_RD_FMT_TYPE

                  PIO_RX_IO_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ m_axis_cq_tuser[7:0];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len   <=#TCQ m_axis_cq_tdata[65];
                      if(AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE") begin // DWord Aligned Mode
                        rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                        wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                        wr_en            <= #TCQ 1'b1;
                        wr_addr          <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2]};
                        m_axis_cq_tready <= #TCQ 1'b0;
                        if(m_axis_cq_tdata[74:64] == 11'h002) begin // 2DWord Payload
                          wr_data        <= #TCQ m_axis_cq_tdata[191:128];
                        end
                        else if (m_axis_cq_tdata[74:64] == 11'h001) begin // 1DW Payload
                          wr_data       <= #TCQ { 32'b0, m_axis_cq_tdata[159:128]};
                        end
                      end // DWord Aligned Mode
                      else begin // Address Aligned Mode
                        rx_state          <= #TCQ PIO_RX_DATA;
                        data_start_loc    <= #TCQ
                            (AXISTEN_IF_CQ_ALIGNMENT_MODE == "TRUE") ?
                            m_axis_cq_tdata_q[4:2] : 3'b0;
                      end
                    end // valid 1-DWORD or 2-DWORD payload
                    else begin
                      req_compl           <= #TCQ 1'b0;
                      req_compl_wd        <= #TCQ 1'b0;
                      req_compl_ur        <= #TCQ 1'b1;
                      rx_state           <= #TCQ PIO_RX_RST_STATE;
                    end
                  end // PIO_RX_IO_WR_FMT_TYPE

                  PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ m_axis_cq_tuser[7:0];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end // PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE

                  PIO_RX_MEM_LK_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ m_axis_cq_tuser[7:0];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_mem_lock     <= #TCQ 1'b1;
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len   <=#TCQ m_axis_cq_tdata[65];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end //PIO_RX_MEM_LK_RD_FMT_TYPE

                  PIO_RX_MSG_FMT_TYPE : begin
                    req_snoop_latency    <= #TCQ m_axis_cq_tdata[15:0];
                    req_no_snoop_latency <= #TCQ m_axis_cq_tdata[31:16];
                    req_obff_code        <= #TCQ m_axis_cq_tdata[35:32];
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem              <= #TCQ 1'b0;
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_FMT_TYPE

                  PIO_RX_MSG_VD_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    req_dst_id           <= #TCQ m_axis_cq_tdata[15:0];
                    req_vend_id          <= #TCQ m_axis_cq_tdata[31:16];
                    req_vend_hdr         <= #TCQ m_axis_cq_tdata[63:32];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_VD_FMT_TYPE

                  PIO_RX_MSG_ATS_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ m_axis_cq_tuser[7:0];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    req_tl_hdr           <= #TCQ m_axis_cq_tdata[127:0];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_ATS_FMT_TYPE

                  default : begin // other TLPs
                    rx_state        <= #TCQ PIO_RX_RST_STATE;
                  end
                endcase // Req_Type_fsm
              end //sop_if
            end // PIO_RX_RST_STATE

            PIO_RX_DATA : begin
              if (m_axis_cq_tvalid)
              begin
                wr_addr          <= #TCQ req_addr[22:2];
                case (data_start_loc)
                  3'b000 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[31:0] ;
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[63:32] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[15:8] : { 4'h0, m_axis_cq_tuser[11:8]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b001 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[63:32] ;
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[95:64] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[19:12] : { 4'h0, m_axis_cq_tuser[15:12]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b010 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[95:64] ;
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[127:96] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[23:16] : { 4'h0, m_axis_cq_tuser[19:16]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b011 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[127:96];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[159:128] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[27:20] : { 4'h0, m_axis_cq_tuser[23:20]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b100 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[159:128];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[191:160] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[31:24] : { 4'h0, m_axis_cq_tuser[27:24]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b101 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[191:160];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[223:192] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[35:28] : { 4'h0, m_axis_cq_tuser[31:28]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b110 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[223:192];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata[255:224] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser[39:32] : { 4'h0, m_axis_cq_tuser[35:32]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  3'b111 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata[255:224];
                    wr_data[63:32]   <= #TCQ 32'h0;
                    wr_be            <= #TCQ { 4'h0, m_axis_cq_tuser[39:36]};
                    wr_en            <= #TCQ payload_len ? 1'b0 : 1'b1;
                    rx_state            <= #TCQ payload_len ? PIO_RX_DATA2 : PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ payload_len ? 1'b1 : 1'b0;
                  end
                  default : begin
                    rx_state        <= #TCQ PIO_RX_DATA;
                  end
                endcase
              end // if (m_axis_cq_tvalid)
              else
                rx_state        <= #TCQ PIO_RX_DATA;
            end // PIO_RX_DATA

            PIO_RX_DATA2 : begin

              if (m_axis_cq_tvalid && m_axis_cq_tlast)
              begin
                  wr_data[63:32]   <= #TCQ m_axis_cq_tdata[31:0];
                  wr_be[7:4]       <= #TCQ m_axis_cq_tuser[11:8];
                  wr_en            <= #TCQ 1'b1;
                  m_axis_cq_tready <= #TCQ 1'b0;
                  rx_state            <= #TCQ PIO_RX_WAIT_STATE;
              end // if (m_axis_cq_tvalid)
              else
              rx_state        <= #TCQ PIO_RX_DATA2;
            end // PIO_RX_DATA2

            PIO_RX_DATA_WR : begin 
               if (m_axis_cq_tvalid) begin 
                  if ((len_i-1)/8 == 0 ) begin // if len_i <= 8 
                       case (len_i) 
                          1 : begin 
                                       wr_data <= #TCQ {224'b0, m_axis_cq_tdata[31:0]}; 
                                       wr_data_be <= #TCQ {28'b0,m_axis_cq_tuser[11:8]};
                                       len_i <= #TCQ len_i -1 ; 
                              end
                          2 : begin 
                                       wr_data <= #TCQ {192'b0, m_axis_cq_tdata[63:0]};
                                       wr_data_be <= #TCQ {24'b0,m_axis_cq_tuser[15:8]}; 
                                       len_i <= #TCQ len_i-2; 
                              end
                          3 : begin 
                                       wr_data <= #TCQ {160'b0, m_axis_cq_tdata[95:0]}; 
                                       wr_data_be <= #TCQ {20'b0,m_axis_cq_tuser[19:8]};
                                       len_i <= #TCQ len_i-3; 
                              end
                          4 : begin 
                                       wr_data <= #TCQ {128'b0, m_axis_cq_tdata[127:0]}; 
                                       wr_data_be <= #TCQ {16'b0,m_axis_cq_tuser[23:8]};
                                        len_i <= #TCQ len_i-4; 
                              end
                          5 : begin 
                                       wr_data <= #TCQ {96'b0, m_axis_cq_tdata[159:0]}; 
                                       wr_data_be <= #TCQ {12'b0,m_axis_cq_tuser[27:8]};
                                       len_i <= #TCQ len_i -5 ; 
                              end
                          6 : begin 
                                       wr_data <= #TCQ {64'b0, m_axis_cq_tdata[191:0]}; 
                                       wr_data_be <= #TCQ {8'b0,m_axis_cq_tuser[31:8]};
                                       len_i <= #TCQ len_i-6; 
                              end
                          7 : begin 
                                       wr_data <= #TCQ {32'b0, m_axis_cq_tdata[223:0]}; 
                                       wr_data_be <= #TCQ {4'b0,m_axis_cq_tuser[35:8]};
                                        len_i <= #TCQ len_i-7; 
                              end
                          8 : begin 
                                       wr_data <= #TCQ m_axis_cq_tdata[255:0]; 
                                       wr_data_be <= #TCQ m_axis_cq_tuser[39:8];
                                       len_i <= #TCQ len_i-8; 
                              end
                      endcase
                               rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                               wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                               wr_en            <= #TCQ 1'b1;
                               m_axis_cq_tready <= #TCQ 1'b0;
                               wr_sop          <= #TCQ m_axis_cq_tuser[40]; 
                               wr_eop           <= #TCQ m_axis_cq_tlast; 

                 end // if len_i <= 8 
                 else begin // if len_i > 8
                        wr_data          <= #TCQ m_axis_cq_tdata[255:0]; 
                        wr_data_be       <= #TCQ m_axis_cq_tuser[39:8];
                        len_i            <= #TCQ len_i-8; 
                        rx_state            <= #TCQ PIO_RX_DATA_WR;
                        wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                        wr_en            <= #TCQ 1'b1;
                        m_axis_cq_tready <= #TCQ 1'b1;
                        wr_sop          <= #TCQ m_axis_cq_tuser[40]; 
                        wr_eop           <= #TCQ m_axis_cq_tlast; 
                 end
               end // m_axis_cq_tvalid 
               else
               rx_state        <= #TCQ PIO_RX_DATA_WR;
            end //PIO_RX_DATA_WR

            PIO_RX_WAIT_STATE : begin
              wr_en      <= #TCQ 1'b0;
              req_compl  <= #TCQ 1'b0;
              req_compl_wd  <= #TCQ 1'b0;
              wr_sop          <= #TCQ m_axis_cq_tuser[40]; 
              wr_eop           <= #TCQ m_axis_cq_tlast; 

              if ((trn_type == PIO_RX_MEM_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_LK_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if (((trn_type == PIO_RX_ATOP_FAA_FMT_TYPE) || (trn_type == PIO_RX_ATOP_UCS_FMT_TYPE) ||
                            (trn_type == PIO_RX_ATOP_CAS_FMT_TYPE)) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else
                rx_state        <= #TCQ PIO_RX_WAIT_STATE;
            end // PIO_RX_WAIT_STATE
          endcase // state
        end // reset_n
      end // End of always Block
     end

    else begin : pio_rx_sm_512_dword_aligned
      always@(posedge user_clk) begin
        if(!reset_n) begin
          m_axis_cq_tready    <= #TCQ 1'b0;
          m_axis_rc_tready    <= #TCQ 1'b1;
          pcie_cq_np_req      <= #TCQ 2'b01;

          req_compl           <= #TCQ 1'b0;
          req_compl_wd        <= #TCQ 1'b0;
          req_compl_ur        <= #TCQ 1'b0;

          req_tc              <= #TCQ 3'b0;
          req_attr            <= #TCQ 3'b0;
          req_len             <= #TCQ 11'b0;
          req_rid             <= #TCQ 16'b0;
          req_tag             <= #TCQ 8'b0;
          req_be              <= #TCQ 8'b0;
          req_addr            <= #TCQ 23'b0;
          req_at              <= #TCQ 2'b0;

          wr_be               <= #TCQ 8'b0;
          wr_addr             <= #TCQ 21'b0;
          wr_data             <= #TCQ 512'h0;
          wr_en               <= #TCQ 1'b0;
          payload_len         <= #TCQ 1'b0;
          data_start_loc      <= #TCQ 3'b0;

          rx_state               <= #TCQ PIO_RX_RST_STATE;
          trn_type            <= #TCQ 4'b0;

          req_snoop_latency   <= #TCQ 16'b0;
          req_no_snoop_latency<= #TCQ 16'b0;
          req_obff_code       <= #TCQ 4'b0;
          req_msg_code        <= #TCQ 8'b0;
          req_msg_route       <= #TCQ 3'b0;
          req_dst_id          <= #TCQ 16'b0;
          req_vend_id         <= #TCQ 16'b0;
          req_vend_hdr        <= #TCQ 32'b0;
          req_tl_hdr          <= #TCQ 128'b0;

          req_des_qword0      <= #TCQ 64'b0;
          req_des_qword1      <= #TCQ 64'b0;
          req_des_tph_present <= #TCQ 1'b0;
          req_des_tph_type    <= #TCQ 2'b0;
          req_des_tph_st_tag  <= #TCQ 8'b0;

          req_mem_lock        <= #TCQ 1'b0;
          req_mem             <= #TCQ 1'b0;
	      m_axis_cq_tparity   <= #TCQ 64'b0;
	      m_axis_cq_tparity_q <= #TCQ 64'b0;
	      m_axis_cq_tvalid_reg <= #TCQ 1'b0;
		  
	  len_i               <= #TCQ 11'h0; 
          wr_sop              <= #TCQ 1'b0;  
          wr_eop              <= #TCQ 1'b0;
          wr_data_be          <= #TCQ 64'b0;
        end
        else begin
          wr_en               <= #TCQ 1'b0;
          req_compl           <= #TCQ 1'b0;
	      m_axis_cq_tparity   <= #TCQ m_axis_cq_tuser[182:119];
	      m_axis_cq_tparity_q <= #TCQ m_axis_cq_tuser_q[182:119];
	      m_axis_cq_tvalid_reg <= #TCQ m_axis_cq_tvalid;

          case (rx_state)
            PIO_RX_RST_STATE : begin
              m_axis_cq_tready <= #TCQ 1'b1;
              m_axis_rc_tready <= #TCQ 1'b1;
              //req_compl_wd     <= #TCQ 1'b1;

              if (sop) begin //sop_if
                case( m_axis_cq_tdata[78:75] ) // Req_Type_fsm
                  PIO_RX_MEM_RD_FMT_TYPE : begin

                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if(m_axis_cq_tdata[74:64] != 11'h000)
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len      <=#TCQ m_axis_cq_tdata[74:64];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end  // PIO_RX_MEM_RD_FMT_TYPE

                  PIO_RX_MEM_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;
					
                    wr_sop           <= #TCQ m_axis_cq_tuser[80]; 
                    wr_eop           <= #TCQ m_axis_cq_tlast; 
                    
           
                    if(AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE") begin // DWord Aligned Mode
					   payload_len      <= #TCQ m_axis_cq_tdata[74:64];
                      if(m_axis_cq_tdata[74:64] < 13 ) begin 
					     case (m_axis_cq_tdata[74:64]) 
                              1  : begin wr_data <= #TCQ {352'b0, m_axis_cq_tdata[159:128]}; wr_data_be <= #TCQ {60'b0, m_axis_cq_tuser[35:32]}; end
                              2  : begin wr_data <= #TCQ {320'b0, m_axis_cq_tdata[191:128]}; wr_data_be <= #TCQ {56'b0, m_axis_cq_tuser[39:32]}; end
                              3  : begin wr_data <= #TCQ {288'b0, m_axis_cq_tdata[223:128]}; wr_data_be <= #TCQ {52'b0, m_axis_cq_tuser[43:32]}; end
                              4  : begin wr_data <= #TCQ {256'b0, m_axis_cq_tdata[255:128]}; wr_data_be <= #TCQ {48'b0, m_axis_cq_tuser[47:32]}; end
							  5  : begin wr_data <= #TCQ {224'b0, m_axis_cq_tdata[287:128]}; wr_data_be <= #TCQ {44'b0, m_axis_cq_tuser[51:32]}; end
                              6  : begin wr_data <= #TCQ {192'b0, m_axis_cq_tdata[319:128]}; wr_data_be <= #TCQ {40'b0, m_axis_cq_tuser[55:32]}; end
                              7  : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[351:128]}; wr_data_be <= #TCQ {36'b0, m_axis_cq_tuser[59:32]}; end
                              8  : begin wr_data <= #TCQ {128'b0, m_axis_cq_tdata[383:128]}; wr_data_be <= #TCQ {32'b0, m_axis_cq_tuser[63:32]}; end
							  9  : begin wr_data <= #TCQ {224'b0, m_axis_cq_tdata[415:128]}; wr_data_be <= #TCQ {28'b0, m_axis_cq_tuser[67:32]}; end
                              10 : begin wr_data <= #TCQ {192'b0, m_axis_cq_tdata[447:128]}; wr_data_be <= #TCQ {24'b0, m_axis_cq_tuser[71:32]}; end
                              11 : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[479:128]}; wr_data_be <= #TCQ {20'b0, m_axis_cq_tuser[75:32]}; end
                              12 : begin wr_data <= #TCQ {128'b0, m_axis_cq_tdata[511:128]}; wr_data_be <= #TCQ {16'b0, m_axis_cq_tuser[79:32]}; end
                         endcase
                         len_i <= #TCQ 11'h0; 
						 
			 rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                         wr_be            <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                         wr_en            <= #TCQ 1'b1;
                         wr_addr          <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2]};
                         m_axis_cq_tready <= #TCQ 1'b0;
                      end
                      else begin 
					     wr_data          <= #TCQ {128'b0, m_axis_cq_tdata[511:128]}; 
						 wr_data_be       <= #TCQ {16'b0, m_axis_cq_tuser[79:32]};
						 len_i            <= #TCQ m_axis_cq_tdata[74:64] - 11'hC ;
						  
						 rx_state            <= #TCQ PIO_RX_DATA_WR;
                         wr_be            <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                         wr_en            <= #TCQ 1'b1;
                         wr_addr          <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2]};
                         m_axis_cq_tready <= #TCQ 1'b1;
		       end
		      end 
					
					else begin // Address Aligned mode 
					    if(m_axis_cq_tdata[74:64] == 11'h002) // 2DWord Payload
                           payload_len    <=#TCQ 1'b1;
                        else
                           payload_len   <=#TCQ 1'b0;
						   
					    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                           begin
						     rx_state            <= #TCQ PIO_RX_DATA;
                             req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                             data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {m_axis_cq_tdata[4:2]} : 3'b0;
						end
						else begin // Payload > 2DWORD
                            rx_state            <= #TCQ PIO_RX_RST_STATE;
                        end
		    end
                  end // PIO_RX_MEM_WR_FMT_TYPE

                  PIO_RX_IO_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len    <=#TCQ m_axis_cq_tdata[65];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end //PIO_RX_IO_RD_FMT_TYPE

                  PIO_RX_IO_WR_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len   <=#TCQ m_axis_cq_tdata[65];
                      if(AXISTEN_IF_CQ_ALIGNMENT_MODE == "FALSE") begin // DWord Aligned Mode
                        rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                        wr_be            <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                        wr_en            <= #TCQ 1'b1;
                        wr_addr          <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2]};
                        m_axis_cq_tready <= #TCQ 1'b0;
                        if(m_axis_cq_tdata[74:64] == 11'h002) begin // 2DWord Payload
                          wr_data        <= #TCQ m_axis_cq_tdata[191:128];
                        end
                        else if (m_axis_cq_tdata[74:64] == 11'h001) begin // 1DW Payload
                          wr_data       <= #TCQ { 32'b0, m_axis_cq_tdata[159:128]};
                        end
                      end // DWord Aligned Mode
                      else begin // Address Aligned Mode
                        rx_state            <= #TCQ PIO_RX_DATA;
                        data_start_loc   <= #TCQ (AXISTEN_IF_CQ_ALIGNMENT_MODE  == "TRUE") ? {m_axis_cq_tdata[4:2]} : 3'b0;
                      end
                    end
                    else begin // Payload > 2DWORDs
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                      rx_state            <= #TCQ PIO_RX_RST_STATE;
                    end
                  end // PIO_RX_IO_WR_FMT_TYPE

                  PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    req_mem          <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b0;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end // PIO_RX_ATOP_FAA_FMT_TYPE, PIO_RX_ATOP_UCS_FMT_TYPE, PIO_RX_ATOP_CAS_FMT_TYPE

                  PIO_RX_MEM_LK_RD_FMT_TYPE : begin
                    trn_type         <= #TCQ m_axis_cq_tdata[78:75];
                    req_len          <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready <= #TCQ 1'b0;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    req_be           <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_des_qword0      <= #TCQ m_axis_cq_tdata[63:0];
                    req_des_qword1      <= #TCQ m_axis_cq_tdata[127:64];
                    req_addr         <= #TCQ {region_select[1:0],m_axis_cq_tdata[20:2], 2'b00};
                    req_des_tph_present <= #TCQ m_axis_cq_tuser[42];
                    req_des_tph_type    <= #TCQ m_axis_cq_tuser[44:43];
                    req_des_tph_st_tag  <= #TCQ m_axis_cq_tuser[52:45];

                    if((m_axis_cq_tdata[74:64] == 11'h001) || (m_axis_cq_tdata[74:64] == 11'h002))
                    begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b1;
                      req_tc           <= #TCQ m_axis_cq_tdata[123:121];
                      req_attr         <= #TCQ m_axis_cq_tdata[126:124];
                      req_rid          <= #TCQ m_axis_cq_tdata[95:80];
                      req_tag          <= #TCQ m_axis_cq_tdata[103:96];
                      req_mem_lock     <= #TCQ 1'b1;
                      req_at           <= #TCQ m_axis_cq_tdata[1:0];
                      payload_len   <=#TCQ m_axis_cq_tdata[65];
                    end
                    else begin
                      req_compl        <= #TCQ 1'b1;
                      req_compl_wd     <= #TCQ 1'b0;
                      req_compl_ur     <= #TCQ 1'b1;
                    end
                  end //PIO_RX_MEM_LK_RD_FMT_TYPE

                  PIO_RX_MSG_FMT_TYPE : begin
                    req_snoop_latency    <= #TCQ m_axis_cq_tdata[15:0];
                    req_no_snoop_latency <= #TCQ m_axis_cq_tdata[31:16];
                    req_obff_code        <= #TCQ m_axis_cq_tdata[35:32];
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    req_mem              <= #TCQ 1'b0;
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_FMT_TYPE

                  PIO_RX_MSG_VD_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    req_dst_id           <= #TCQ m_axis_cq_tdata[15:0];
                    req_vend_id          <= #TCQ m_axis_cq_tdata[31:16];
                    req_vend_hdr         <= #TCQ m_axis_cq_tdata[63:32];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_VD_FMT_TYPE

                  PIO_RX_MSG_ATS_FMT_TYPE : begin
                    trn_type             <= #TCQ m_axis_cq_tdata[78:75];
                    req_len              <= #TCQ m_axis_cq_tdata[74:64];
                    m_axis_cq_tready     <= #TCQ 1'b0;
                    req_mem              <= #TCQ 1'b0;
                    req_tc               <= #TCQ m_axis_cq_tdata[123:121];
                    req_attr             <= #TCQ m_axis_cq_tdata[126:124];
                    req_at               <= #TCQ m_axis_cq_tdata[1:0];
                    req_rid              <= #TCQ m_axis_cq_tdata[95:80];
                    req_tag              <= #TCQ m_axis_cq_tdata[103:96];
                    req_be               <= #TCQ {m_axis_cq_tuser[11:8],m_axis_cq_tuser[3:0]};
                    req_msg_code         <= #TCQ m_axis_cq_tdata[111:104];
                    req_msg_route        <= #TCQ m_axis_cq_tdata[107:105];
                    req_tl_hdr           <= #TCQ m_axis_cq_tdata[127:0];
                    rx_state                <= #TCQ PIO_RX_RST_STATE;
                  end // PIO_RX_MSG_ATS_FMT_TYPE
                  default : begin // other TLPs
                    rx_state        <= #TCQ PIO_RX_RST_STATE;
                  end
                endcase // Req_Type_fsm
              end //sop_if
            end // PIO_RX_RST_STATE

            PIO_RX_DATA : begin
              if (m_axis_cq_tvalid || m_axis_cq_tvalid_reg)
              begin
                wr_addr          <= #TCQ req_addr[22:2];
                case (data_start_loc[1:0])
                  2'b00 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata_q[159:128];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata_q[191:160] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser_reg[39:32] : { 4'h0, m_axis_cq_tuser_reg[35:32]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  2'b01 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata_q[191:160];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata_q[223:192] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser_reg[43:36] : { 4'h0, m_axis_cq_tuser_reg[39:36]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  2'b10 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata_q[223:192];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata_q[255:224] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser_reg[47:40] : { 4'h0, m_axis_cq_tuser_reg[43:40]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  2'b11 : begin
                    wr_data[31:0]    <= #TCQ m_axis_cq_tdata_q[255:224];
                    wr_data[63:32]   <= #TCQ payload_len ? m_axis_cq_tdata_q[287:256] : 32'h0;
                    wr_be            <= #TCQ payload_len ? m_axis_cq_tuser_reg[51:44] : { 4'h0, m_axis_cq_tuser_reg[47:44]};
                    wr_en            <= #TCQ 1'b1;
                    rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                    m_axis_cq_tready <= #TCQ 1'b0;
                  end
                  default : begin
                    rx_state        <= #TCQ PIO_RX_DATA;
                  end
                endcase
              end // if (m_axis_cq_tvalid)
              else
                rx_state        <= #TCQ PIO_RX_DATA;
            end // PIO_RX_DATA
            PIO_RX_DATA2 : begin
              if (m_axis_cq_tvalid && m_axis_cq_tlast)
              begin
                  wr_data[63:16]   <= #TCQ m_axis_cq_tdata[31:0];
                  wr_be[7:4]       <= #TCQ m_axis_cq_tuser[11:8];
                  wr_en            <= #TCQ 1'b1;
                  m_axis_cq_tready <= #TCQ 1'b0;
                  rx_state            <= #TCQ PIO_RX_WAIT_STATE;
              end // if (m_axis_cq_tvalid)
              else
              rx_state        <= #TCQ PIO_RX_DATA2;
            end // PIO_RX_DATA2
	
	    PIO_RX_DATA_WR : begin 
               if (m_axis_cq_tvalid) begin 
                  if ((len_i-1)/16 == 0 ) begin // if len_i <= 16 
                       case (len_i) 
					          1  : begin wr_data <= #TCQ {352'b0, m_axis_cq_tdata[31:0]};    wr_data_be <= #TCQ {60'b0, m_axis_cq_tuser[19:16]}; end
                              2  : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[63:0]};    wr_data_be <= #TCQ {56'b0, m_axis_cq_tuser[23:16]}; end
                              3  : begin wr_data <= #TCQ {288'b0, m_axis_cq_tdata[95:0]};    wr_data_be <= #TCQ {52'b0, m_axis_cq_tuser[27:16]}; end
                              4  : begin wr_data <= #TCQ {256'b0, m_axis_cq_tdata[127:0]};   wr_data_be <= #TCQ {48'b0, m_axis_cq_tuser[31:16]}; end
							  5  : begin wr_data <= #TCQ {224'b0, m_axis_cq_tdata[159:0]};   wr_data_be <= #TCQ {44'b0, m_axis_cq_tuser[35:16]}; end
                              6  : begin wr_data <= #TCQ {192'b0, m_axis_cq_tdata[191:0]};   wr_data_be <= #TCQ {40'b0, m_axis_cq_tuser[39:16]}; end
                              7  : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[224:0]};   wr_data_be <= #TCQ {36'b0, m_axis_cq_tuser[43:16]}; end
                              8  : begin wr_data <= #TCQ {128'b0, m_axis_cq_tdata[255:0]};   wr_data_be <= #TCQ {32'b0, m_axis_cq_tuser[47:16]}; end
							  9  : begin wr_data <= #TCQ {224'b0, m_axis_cq_tdata[287:0]};   wr_data_be <= #TCQ {28'b0, m_axis_cq_tuser[51:16]}; end
                              10 : begin wr_data <= #TCQ {192'b0, m_axis_cq_tdata[319:0]};   wr_data_be <= #TCQ {24'b0, m_axis_cq_tuser[55:16]}; end
                              11 : begin wr_data <= #TCQ {160'b0, m_axis_cq_tdata[351:0]};   wr_data_be <= #TCQ {20'b0, m_axis_cq_tuser[59:16]}; end
                              12 : begin wr_data <= #TCQ {128'b0, m_axis_cq_tdata[383:0]};   wr_data_be <= #TCQ {16'b0, m_axis_cq_tuser[63:16]}; end
							  13 : begin wr_data <= #TCQ {96'b0, m_axis_cq_tdata[415:0]};    wr_data_be <= #TCQ {28'b0, m_axis_cq_tuser[67:16]}; end
                              14 : begin wr_data <= #TCQ {64'b0, m_axis_cq_tdata[447:0]};    wr_data_be <= #TCQ {24'b0, m_axis_cq_tuser[71:16]}; end
                              15 : begin wr_data <= #TCQ {16'b0, m_axis_cq_tdata[479:0]};    wr_data_be <= #TCQ {20'b0, m_axis_cq_tuser[75:16]}; end
                              16 : begin wr_data <= #TCQ m_axis_cq_tdata[511:0];           wr_data_be <= #TCQ {16'b0, m_axis_cq_tuser[79:16]}; end
                      endcase
			       len_i            <= #TCQ 11'h0; 
                               rx_state            <= #TCQ PIO_RX_WAIT_STATE;
                               wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                               wr_en            <= #TCQ 1'b1;
                               m_axis_cq_tready <= #TCQ 1'b0;
                               wr_sop           <= #TCQ m_axis_cq_tuser[80]; 
                               wr_eop           <= #TCQ m_axis_cq_tlast; 
                 end // if len_i <= 16
                 else begin // if len_i > 16
                        wr_data          <= #TCQ m_axis_cq_tdata[511:0]; 
                        wr_data_be       <= #TCQ m_axis_cq_tuser[79:16];
                        len_i            <= #TCQ len_i-16; 
                        rx_state            <= #TCQ PIO_RX_DATA_WR;
                        wr_be            <= #TCQ m_axis_cq_tuser[7:0];
                        wr_en            <= #TCQ 1'b1;
                        m_axis_cq_tready <= #TCQ 1'b1;
                        wr_sop           <= #TCQ m_axis_cq_tuser[80]; 
                        wr_eop           <= #TCQ m_axis_cq_tlast;                           
                 end
               end // m_axis_cq_tvalid 
               else
                 rx_state        <= #TCQ PIO_RX_DATA_WR;
            end //PIO_RX_DATA_WR

            PIO_RX_WAIT_STATE : begin
              wr_en      <= #TCQ 1'b0;
              req_compl  <= #TCQ 1'b0;
              req_compl_wd  <= #TCQ 1'b0;

              if ((trn_type == PIO_RX_MEM_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;

              end else if ((trn_type == PIO_RX_IO_WR_FMT_TYPE) && (!wr_busy))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_MEM_LK_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if ((trn_type == PIO_RX_IO_RD_FMT_TYPE) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else if (((trn_type == PIO_RX_ATOP_FAA_FMT_TYPE) || (trn_type == PIO_RX_ATOP_UCS_FMT_TYPE) ||
                            (trn_type == PIO_RX_ATOP_CAS_FMT_TYPE)) && (compl_done))
              begin
                m_axis_cq_tready <= #TCQ 1'b1;
                rx_state        <= #TCQ PIO_RX_RST_STATE;
              end else
                rx_state        <= #TCQ PIO_RX_WAIT_STATE;
            end // PIO_RX_WAIT_STATE
          endcase // state
        end // reset_n
      end // End of always Block
    end // pio_rx_sm_512
  endgenerate

  always @*
  begin
    case ({io_bar_hit_n, mem32_bar_hit_n, mem64_bar_hit_n, erom_bar_hit_n})

    /*  4'b0111 : begin
        region_select <= #TCQ 2'b00;    // Select IO region
      end
      4'b1011 : begin
        region_select <= #TCQ 2'b01;    // Select Mem32 region
      end
      4'b1101 : begin
        region_select <= #TCQ 2'b10;    // Select Mem64 region
      end
      4'b1110 : begin
        region_select <= #TCQ 2'b11;    // Select EROM region
      end */

      default : begin
        region_select <= #TCQ 2'b00;    // Error selection will select IO region
      end
    endcase
  end

  // synthesis translate_off
  reg  [8*20:1] state_ascii;
  always @(rx_state)
  begin
    case (rx_state)
      PIO_RX_RST_STATE              : state_ascii <= #TCQ "RX_RST_STATE";
      PIO_RX_WAIT_STATE             : state_ascii <= #TCQ "RX_WAIT_STATE";
      PIO_RX_64_QW1                 : state_ascii <= #TCQ "RX_64_QW1";
      PIO_RX_DATA                   : state_ascii <= #TCQ "RX_DATA";
      PIO_RX_DATA2                  : state_ascii <= #TCQ "RX_DATA2";
      default                       : state_ascii <= #TCQ "PIO STATE ERR";
    endcase
  end
  // synthesis translate_on
endmodule // pio_rx_engine

module pio_tx_engine    #(
  parameter       TCQ = 1,
  parameter [1:0] AXISTEN_IF_WIDTH = 00,
  parameter       AXI4_CC_TUSER_WIDTH = 33,
  parameter       AXI4_RQ_TUSER_WIDTH = 62,
  parameter       AXISTEN_IF_RQ_ALIGNMENT_MODE = "FALSE",
  parameter       AXISTEN_IF_CC_ALIGNMENT_MODE = "FALSE",
  parameter       AXISTEN_IF_ENABLE_CLIENT_TAG = 0,
  parameter       AXISTEN_IF_RQ_PARITY_CHECK   = 0,
  parameter       AXISTEN_IF_CC_PARITY_CHECK   = 0,

  //Do not modify the parameters below this line
  //parameter C_DATA_WIDTH = (AXISTEN_IF_WIDTH[1]) ? 256 : (AXISTEN_IF_WIDTH[0])? 128 : 64,
  parameter C_DATA_WIDTH = 512,
  parameter ADDR_W       = 5,                 // Memory Depth based on the C_DATA_WIDTH
  parameter MEM_W        = 256,                 // Memory Depth based on the C_DATA_WIDTH
  parameter BYTE_EN_W    = 32,               // Width of byte enable going to memory for write data
  
  parameter PARITY_WIDTH = C_DATA_WIDTH /8,
  parameter KEEP_WIDTH   = C_DATA_WIDTH /32,
  parameter STRB_WIDTH   = C_DATA_WIDTH / 8,
  parameter integer REQ_ADDR_WIDTH = 23
)(

  input                          user_clk,
  input                          reset_n,

  // AXI-S Completer Completion Interface
  output reg        [C_DATA_WIDTH-1:0]  s_axis_cc_tdata,
  output reg          [KEEP_WIDTH-1:0]  s_axis_cc_tkeep,
  output reg                            s_axis_cc_tlast,
  output reg                            s_axis_cc_tvalid,
  output     [AXI4_CC_TUSER_WIDTH-1:0]  s_axis_cc_tuser,
  input                                 s_axis_cc_tready,

  // AXI-S Requester Request Interface
  output reg        [C_DATA_WIDTH-1:0]  s_axis_rq_tdata,
  output reg          [KEEP_WIDTH-1:0]  s_axis_rq_tkeep,
  output reg                            s_axis_rq_tlast,
  output reg                            s_axis_rq_tvalid,
  output reg [AXI4_RQ_TUSER_WIDTH-1:0]  s_axis_rq_tuser,
  input                                 s_axis_rq_tready,

  // TX Message Interface
  input                          cfg_msg_transmit_done,
  output reg                     cfg_msg_transmit,
  output reg              [2:0]  cfg_msg_transmit_type,
  output reg             [31:0]  cfg_msg_transmit_data,

  //Tag availability and Flow control Information
  input                   [5:0]  pcie_rq_tag,
  input                          pcie_rq_tag_vld,
  input                   [1:0]  pcie_tfc_nph_av,
  input                   [1:0]  pcie_tfc_npd_av,
  input                          pcie_tfc_np_pl_empty,
  input                   [3:0]  pcie_rq_seq_num,
  input                          pcie_rq_seq_num_vld,

  //Cfg Flow Control Information
  input                   [7:0]  cfg_fc_ph,
  input                   [7:0]  cfg_fc_nph,
  input                   [7:0]  cfg_fc_cplh,
  input                  [11:0]  cfg_fc_pd,
  input                  [11:0]  cfg_fc_npd,
  input                  [11:0]  cfg_fc_cpld,
  output                   [2:0]  cfg_fc_sel,

  // PIO RX Engine Interface
  input                          req_compl,
  input                          req_compl_wd,
  input                          req_compl_ur,
 input                    [10:0] payload_len,
  output reg                     compl_done,
  input                   [2:0]  req_tc,
  input                          req_td,
  input                          req_ep,
  input                   [2:0]  req_attr,
  input                   [10:0]  req_len,
  input                  [15:0]  req_rid,
  input                   [7:0]  req_tag,
  input                   [7:0]  req_be,
  //input                  [12:0]  req_addr,
  input [REQ_ADDR_WIDTH-1:0]      req_addr,
  input                   [1:0]  req_at,

  input                  [15:0]  completer_id,

  // Inputs to the TX Block in case of an UR
  // Required to form the completions
  input                  [63:0]  req_des_qword0,
  input                  [63:0]  req_des_qword1,
  input                          req_des_tph_present,
  input                   [1:0]  req_des_tph_type,
  input                   [7:0]  req_des_tph_st_tag,

  //Indicate that the Request was a Mem lock Read Req
  input                          req_mem_lock,
  input                          req_mem,

  // PIO Memory Access Control Interface
  output reg       [ADDR_W-1:0]  rd_addr,
  output reg       [3:0]         rd_be,
  output reg                     rd_en,
  output reg                     trn_sent,
  input            [MEM_W-1:0]   rd_data,
  input  wire         gen_transaction,
  
  // Device-to-host: posted 16-byte Memory Write
  input  wire         bufrd_rq_start,
  input  wire [63:0]  bufrd_rq_addr,
  input  wire [127:0] bufrd_rq_data,
  output reg          bufrd_packet_done,

  // Host-to-device: non-posted 16-byte Memory Read
  input  wire         bufwr_rq_start,
  input  wire [63:0]  bufwr_rq_addr,
  output reg          bufwr_packet_done,

  output wire [7:0]   dbg_rq_state
 );

  localparam PIO_TX_RST_STATE                   = 4'b0000;
  localparam PIO_TX_COMPL_C1                    = 4'b0001;
  localparam PIO_TX_COMPL_C2                    = 4'b0010;
  localparam PIO_TX_COMPL_WD_C1                 = 4'b0011;
  localparam PIO_TX_COMPL_WD_C2                 = 4'b0100;
  localparam PIO_TX_COMPL_PYLD                  = 4'b0101;
  localparam PIO_TX_CPL_UR_C1                   = 4'b0110;
  localparam PIO_TX_CPL_UR_C2                   = 4'b0111;
  localparam PIO_TX_CPL_UR_C3                   = 4'b1000;
  localparam PIO_TX_CPL_UR_C4                   = 4'b1001;
  localparam PIO_TX_MRD_C1                      = 4'b1010;
  localparam PIO_TX_MRD_C2                      = 4'b1011;
  localparam PIO_TX_COMPL_WD_2DW                = 4'b1100;
  localparam PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1   = 4'b1101;
  localparam PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2   = 4'b1110;
  localparam PIO_TX_COMPL_WD_N_DW               = 4'b1111; // added for N-DW support

  // Local registers
  reg  [11:0]              byte_count_fbe;
  reg  [11:0]              byte_count_lbe;
//  wire [11:0]              byte_count; //currently not used
  reg  [06:0]              lower_addr;
  reg  [06:0]              lower_addr_q;
  reg  [06:0]              lower_addr_qq;
  reg  [6:0]               lower_addr_dw;  
  reg  [15:0]              tkeep;
  reg  [15:0]              tkeep_q;
  reg  [15:0]              tkeep_qq;
  reg                      req_compl_q;
  reg                      req_compl_qq;
  reg                      req_compl_wd_q;
  reg                      req_compl_wd_qq;
  reg                      req_compl_wd_qqq;
  reg                      req_compl_wd_qqqq; 
  reg                      req_compl_wd_qqqqq;
  reg                      req_compl_ur_q;
  reg                      req_compl_ur_qq;
  reg  [3:0]               rq_state;
  
  // Buffered requester commands, held stable for the full RQ packet.
  reg  [63:0]              bufrd_rq_addr_q;
  reg  [127:0]             bufrd_rq_data_q;
  reg  [63:0]              bufwr_rq_addr_q;

  // Selects the one-beat H2D Memory Read form of PIO_TX_MRD_C1.
  reg                      bufwr_rq_is_read_q;
  wire  [31:0]             s_axis_cc_tparity;
  wire  [31:0]             s_axis_rq_tparity;
  reg                      dword_count; // to count if its a 1DW or 2 DW transaction
  reg  [95:0]              rd_data_s1; // To Store the 1st rd_data in case of N-DW payload
  reg [3*C_DATA_WIDTH-1:0] rd_data_s0;
  reg  [10:0]              len_i; 
  reg  [31:0]              rd_data_reg; // To Store the 1st rd_data in case of 2DW payload
  reg [AXI4_CC_TUSER_WIDTH-1:0]  s_axis_cc_tuser_wo_parity;
  reg [12:0] byte_count; 
 // CFG func sel

  assign cfg_fc_sel         = 3'b0;
  assign dbg_rq_state       = {4'b0000, rq_state};

  // Present address and byte enable to memory module

always @ (posedge user_clk)begin
   if (!reset_n) begin
         rd_be    <=  #TCQ 4'b0;
   end
   else if(req_compl_wd) begin
        if(payload_len == 0) begin
         rd_be    <= #TCQ 4'h0;
        end
        else begin 
         rd_be    <= #TCQ req_be[3:0];
       end
    end
end

generate
// State machine to increment the read address to the memory 
  if (C_DATA_WIDTH == 64) begin : pio_tx_sm_64
    reg  rd_addr_state; 
    reg [10:0] rd_addr_rem;

        
    always @(s_axis_cc_tready) begin 
        rd_en = 1'b0; 
    if (s_axis_cc_tready) 
        rd_en = 1'b1; 
    else
        rd_en = 1'b0;   
    end

    always @(posedge user_clk or negedge reset_n) begin 
      if (!reset_n) begin 
	      rd_addr       <= #TCQ 8'b0; 
		  rd_addr_state <= #TCQ 1'b0; 
		  rd_addr_rem   <= #TCQ 11'b0; 
		  //rd_en         <= #TCQ 1'b0;
	  end 
	  else begin
          case (rd_addr_state) 
		        1'b0   : begin
                                if (req_compl_wd) begin
                                    rd_addr        <= #TCQ req_addr[10:3];
                                    //rd_en          <= #TCQ 1'b1; 
                                    rd_addr_state  <= #TCQ 1'b1; 
                                    case (req_addr[2])
									    1'b0   : rd_addr_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; 
										1'b1   : rd_addr_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1; 
                                    endcase			
                                end
                                else begin
                                    rd_addr        <= #TCQ rd_addr; 
                                    rd_addr_state  <= #TCQ 1'b0; 
                                    rd_addr_rem    <= #TCQ 11'b0; 
                                    //rd_en          <= #TCQ 1'b0; 									
                                end								    								
				         end // RST State end
				1'b1   : begin 
				if (s_axis_cc_tready) begin 
				                 if (rd_addr_rem == 0) begin 
								     rd_addr        <= #TCQ rd_addr; 
									 rd_addr_state  <= #TCQ 1'b0; 
									 rd_addr_rem    <= #TCQ 11'b0; 
									 //rd_en          <= #TCQ 1'b1; 
							     end 
								 else begin
								     if ((rd_addr_rem-1)/2 == 0 ) begin 
                                         rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b0; 
									     rd_addr_rem    <= #TCQ 11'b0;
									     //rd_en          <= #TCQ 1'b1; 
                                     end
                                     else begin
 									     rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b1; 
									     rd_addr_rem    <= #TCQ rd_addr_rem - 11'h2;
									     //rd_en          <= #TCQ 1'b1; 
                                     end									 
								 end
				end
				else begin 
                                        rd_addr        <= #TCQ rd_addr; //_prev; 
					rd_addr_state  <= #TCQ 1'b1; 
					rd_addr_rem    <= #TCQ rd_addr_rem; // _prev;
					//rd_en          <= #TCQ rd_en; 

				end 
						   end // Address Increment State end
          endcase		  
	  end
   end
//State machine to form the read data based on the offset 
    reg [1:0] rd_data_state;
    wire [63:0] temp_64_data_64; // debugging
    wire [63:0] temp_128_data_64; //debugging
    wire [63:0] temp_192_data_64; //debugging
    reg [10:0] dw_rem; 
    reg rd_data_ready; 
    localparam PIO_RD_DATA_RST  = 2'b00; 
	localparam PIO_RD_DATA_WAIT = 2'b01;  
	//debugging 
	assign temp_64_data_64  = rd_data_s0[63:0]; 
	assign temp_128_data_64 = rd_data_s0[127:64]; 
    assign temp_192_data_64 = rd_data_s0[191:127]; 
    //debugging
  always @(posedge user_clk or negedge reset_n) 
  begin
      if (!reset_n) begin 
	      rd_data_state <= #TCQ 3'b000; 
		  rd_data_s0    <= #TCQ 192'b0; 
		  rd_data_ready <= #TCQ 1'b0; 
		  dw_rem        <= #TCQ 11'b0; 
      end
	  else begin
          case (rd_data_state)
		  PIO_RD_DATA_RST : begin 
		                          rd_data_ready <= #TCQ 1'b0;
				                  if (req_compl_wd_qq) begin 
								      case (req_addr[2])
									        1'b0   : begin rd_data_s0[127:64]  <= #TCQ rd_data[63:0];   dw_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; end
											1'b1   : begin rd_data_s0[95:64]   <= #TCQ rd_data[63:32];  dw_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1; end						
									  endcase
									  if (s_axis_cc_tready) 
									      rd_data_state <= #TCQ PIO_RD_DATA_WAIT; 
									  else
                                          rd_data_state <= #TCQ PIO_RD_DATA_RST; 	              										
							      end
								  else begin 
								      rd_data_s0   <= #TCQ rd_data_s0;
									  rd_data_state <= #TCQ PIO_RD_DATA_RST; 
								  end
						    end //PIO_RD_DATA_RST 
		  PIO_RD_DATA_WAIT : begin
		                     if (s_axis_cc_tready) begin 
		                          rd_data_ready <= #TCQ 1'b1; 
		                          if (dw_rem == 0 ) begin
								      rd_data_s0    <= #TCQ {64'b0, rd_data_s0[191:64]};
									  rd_data_state  <= #TCQ PIO_RD_DATA_RST; 
								  end
								  else begin
								      case (req_addr[2])
									        1'b0   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[127:64]};  end
											1'b1   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[95:64]};  end									
									  endcase
									  if ((dw_rem -1)/2 == 0) 
                                          dw_rem        <= #TCQ 11'h0; 
                                      else 
									      dw_rem        <= #TCQ dw_rem - 11'h2; 
										  
                                      rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  								  
								  end  	
				      end
				      else begin 
                                          rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  
                                          rd_data_s0 <= #TCQ rd_data_s0; //_prev;
				      end 
		                     end //PIO_RD_DATA_WAIT 
							 
		  default : begin rd_data_s0 <= #TCQ rd_data_s0; rd_data_state <= #TCQ PIO_RD_DATA_RST; end
          endcase		  
	  end
	  
  end  
  
  end
  
  else if (C_DATA_WIDTH == 128) begin : pio_tx_sm_128
    reg  rd_addr_state; 
    reg [10:0] rd_addr_rem;
	
	always @(s_axis_cc_tready) begin 
        rd_en = 1'b0; 
    if (s_axis_cc_tready) 
        rd_en = 1'b1; 
    else
        rd_en = 1'b0;   
    end
	
    always @(posedge user_clk or negedge reset_n) begin 
      if (!reset_n) begin 
	      rd_addr       <= #TCQ 7'b0; 
		  rd_addr_state <= #TCQ 1'b0; 
		  rd_addr_rem   <= #TCQ 11'b0; 
		  //rd_en         <= #TCQ 1'b0; 
	  end 
	  else begin
          case (rd_addr_state) 
		        1'b0   : begin
                                if (req_compl_wd) begin
                                    rd_addr        <= #TCQ req_addr[10:4];
                                    //rd_en          <= #TCQ 1'b1; 
                                    rd_addr_state  <= #TCQ 1'b1; 
                                    case (req_addr[3:2])
									    2'b00  : rd_addr_rem <= #TCQ (payload_len < 5 ) ? 11'b0 : payload_len - 11'h4; 
										2'b01  : rd_addr_rem <= #TCQ (payload_len < 4 ) ? 11'b0 : payload_len - 11'h3; 
										2'b10  : rd_addr_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; 
										2'b11  : rd_addr_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1;
                                    endcase			
                                end
                                else begin
                                    rd_addr        <= #TCQ rd_addr; 
                                    rd_addr_state  <= #TCQ 1'b0; 
                                    rd_addr_rem    <= #TCQ 11'b0; 
                                    //rd_en          <= #TCQ 1'b0; 									
                                end								    								
				         end // RST State end
				1'b1   : begin 
				                 if (rd_addr_rem == 0) begin 
								     rd_addr        <= #TCQ rd_addr; 
									 rd_addr_state  <= #TCQ 1'b0; 
									 rd_addr_rem    <= #TCQ 11'b0; 
									 //rd_en          <= #TCQ 1'b1; 
							     end 
								 else begin
								     if ((rd_addr_rem-1)/4 == 0 ) begin 
                                         rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b0; 
									     rd_addr_rem    <= #TCQ 11'b0;
									     //rd_en          <= #TCQ 1'b1; 
                                     end
                                     else begin
 									     rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b1; 
									     rd_addr_rem    <= #TCQ rd_addr_rem - 11'h4;
									     //rd_en          <= #TCQ 1'b1; 
                                     end									 
								 end
						   end // Address Increment State end
		        
          endcase		  
	  end
   end
//State machine to form the read data based on the offset 
    reg [1:0] rd_data_state;
    wire [127:0] temp_128_data_128; // debugging
    wire [127:0] temp_256_data_128; //debugging
    wire [127:0] temp_384_data_128; //debugging
    reg [10:0] dw_rem; 
    reg rd_data_ready; 
    localparam PIO_RD_DATA_RST  = 2'b00; 
	localparam PIO_RD_DATA_WAIT = 2'b01;  
	//debugging 
	assign temp_128_data_128  = rd_data_s0[127:0]; 
	assign temp_256_data_128 = rd_data_s0[255:128]; 
    assign temp_384_data_128 = rd_data_s0[383:256]; 
    //debugging
  always @(posedge user_clk or negedge reset_n) 
  begin
      if (!reset_n) begin 
	      rd_data_state <= #TCQ 2'b00; 
		  rd_data_s0    <= #TCQ 384'b0; 
		  rd_data_ready <= #TCQ 1'b0; 
		  dw_rem        <= #TCQ 11'b0; 
      end
	  else begin
          case (rd_data_state)
		  PIO_RD_DATA_RST : begin 
		                          rd_data_ready <= #TCQ 1'b0;
				                  if (req_compl_wd_qq) begin 
								      case (req_addr[3:2])
									        2'b00   : begin rd_data_s0[255:128]  <= #TCQ rd_data[127:0];   dw_rem <= #TCQ (payload_len < 5 ) ? 11'b0 : payload_len - 11'h4; end
											2'b01   : begin rd_data_s0[223:128]  <= #TCQ rd_data[127:32];  dw_rem <= #TCQ (payload_len < 4 ) ? 11'b0 : payload_len - 11'h3; end	
                                            2'b10   : begin rd_data_s0[191:128]  <= #TCQ rd_data[127:64];  dw_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; end
											2'b11   : begin rd_data_s0[159:128]  <= #TCQ rd_data[127:96];  dw_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1; end															
									  endcase
									  if (s_axis_cc_tready) 
									      rd_data_state <= #TCQ PIO_RD_DATA_WAIT; 
									  else
                                          rd_data_state <= #TCQ PIO_RD_DATA_RST; 	              										
							      end
								  else begin 
								      rd_data_s0   <= #TCQ rd_data_s0;
									  rd_data_state <= #TCQ PIO_RD_DATA_RST; 
								  end
						    end //PIO_RD_DATA_RST 
		  PIO_RD_DATA_WAIT : begin
		                     if (s_axis_cc_tready) begin 
		                          rd_data_ready <= #TCQ 1'b1; 
		                          if (dw_rem == 0 ) begin
								      rd_data_s0    <= #TCQ {128'b0, rd_data_s0[383:128]};
									  rd_data_state  <= #TCQ PIO_RD_DATA_RST; 
								  end
								  else begin
								      case (req_addr[3:2])
									        2'b00   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[255:128]};  end
											2'b01   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[223:128]};  end	
                                            2'b10   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[191:128]};  end
											2'b11   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[159:128]};  end												
									  endcase
									  if ((dw_rem -1)/4 == 0) 
                                          dw_rem        <= #TCQ 11'h0; 
                                      else 
									      dw_rem        <= #TCQ dw_rem - 11'h4; 
										  
                                      rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  								  
								  end  	
						     end
                             else begin 
                                          rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  
                                          rd_data_s0 <= #TCQ rd_data_s0; //_prev;
				             end 								  
		                    end //PIO_RD_DATA_WAIT 
							 
		  default : begin rd_data_s0 <= #TCQ rd_data_s0; rd_data_state <= #TCQ PIO_RD_DATA_RST; end
          endcase		  
	  end
	  
  end  
  end
  
  else if (C_DATA_WIDTH == 256 ) begin : pio_tx_sm_256
    reg  rd_addr_state; 
    reg [10:0] rd_addr_rem;
	
	always @(s_axis_cc_tready) begin 
        rd_en = 1'b0; 
    if (s_axis_cc_tready) 
        rd_en = 1'b1; 
    else
        rd_en = 1'b0;   
    end
	
    always @(posedge user_clk or negedge reset_n) begin 
      if (!reset_n) begin 
	          rd_addr   <= #TCQ 6'b0; 
		  rd_addr_state <= #TCQ 1'b0; 
		  rd_addr_rem   <= #TCQ 11'b0; 
		  //rd_en         <= #TCQ 1'b0; 
	  end 
	  else begin
          case (rd_addr_state) 
		        1'b0   : begin
                                if (req_compl_wd) begin
                                    rd_addr        <= #TCQ req_addr[10:5];
                                    //rd_en          <= #TCQ 1'b1; 
                                    rd_addr_state  <= #TCQ 1'b1; 
                                    case (req_addr[4:2])
									    3'b000   : rd_addr_rem <= #TCQ (payload_len < 9 ) ? 11'b0 : payload_len - 11'h8; 
										3'b001   : rd_addr_rem <= #TCQ (payload_len < 8 ) ? 11'b0 : payload_len - 11'h7; 
										3'b010   : rd_addr_rem <= #TCQ (payload_len < 7 ) ? 11'b0 : payload_len - 11'h6; 
										3'b011   : rd_addr_rem <= #TCQ (payload_len < 6 ) ? 11'b0 : payload_len - 11'h5; 
										3'b100   : rd_addr_rem <= #TCQ (payload_len < 5 ) ? 11'b0 : payload_len - 11'h4; 
										3'b101   : rd_addr_rem <= #TCQ (payload_len < 4 ) ? 11'b0 : payload_len - 11'h3; 
										3'b110   : rd_addr_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; 
										3'b111   : rd_addr_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1; 
                                    endcase			
                                end
                                else begin
                                    rd_addr        <= #TCQ rd_addr; 
                                    rd_addr_state  <= #TCQ 2'b00; 
                                    rd_addr_rem    <= #TCQ 11'b0; 
                                    //rd_en          <= #TCQ 1'b0; 									
                                end								    								
				         end // RST State end
				1'b1   : begin 
				                 if (rd_addr_rem == 0) begin 
								     rd_addr        <= #TCQ rd_addr; 
									 rd_addr_state  <= #TCQ 1'b0; 
									 rd_addr_rem    <= #TCQ 11'b0; 
									 //rd_en          <= #TCQ 1'b1; 
							     end 
								 else begin
								     if ((rd_addr_rem-1)/8 == 0 ) begin 
                                         rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 2'b00; 
									     rd_addr_rem    <= #TCQ 11'b0;
									     //rd_en          <= #TCQ 1'b1; 
                                     end
                                     else begin
 									     rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 2'b01; 
									     rd_addr_rem    <= #TCQ rd_addr_rem - 11'h8;
									     //rd_en          <= #TCQ 1'b1; 
                                     end									 
								 end
						   end // Address Increment State end
		        
          endcase		  
	  end
   end
//State machine to form the read data based on the offset 
    reg [1:0] rd_data_state;
    wire [255:0] temp_256_data_256; // debugging
    wire [255:0] temp_512_data_256; //debugging
    wire [255:0] temp_768_data_256; //debugging
    reg [10:0] dw_rem; 
    reg rd_data_ready; 
    localparam PIO_RD_DATA_RST  = 2'b00; 
	localparam PIO_RD_DATA_WAIT = 2'b01;  
	//debugging 
	assign temp_256_data_256 = rd_data_s0[255:0]; 
	assign temp_512_data_256 = rd_data_s0[511:256]; 
    assign temp_768_data_256 = rd_data_s0[767:512]; 
    //debugging
  always @(posedge user_clk or negedge reset_n) 
  begin
      if (!reset_n) begin 
	      rd_data_state <= #TCQ 3'b000; 
		  rd_data_s0   <= #TCQ 768'b0; 
		  rd_data_ready <= #TCQ 1'b0; 
		  dw_rem        <= #TCQ 11'b0; 
      end
	  else begin
          case (rd_data_state)
		  PIO_RD_DATA_RST : begin 
		                          rd_data_ready <= #TCQ 1'b0;
				                  if (req_compl_wd_qq) begin 
								      case (req_addr[4:2])
									        3'b000   : begin rd_data_s0[511:256] <= #TCQ rd_data[255:0];   dw_rem <= #TCQ (payload_len < 9 ) ? 11'b0 : payload_len - 11'h8; end
											3'b001   : begin rd_data_s0[479:256] <= #TCQ rd_data[255:32];  dw_rem <= #TCQ (payload_len < 8 ) ? 11'b0 : payload_len - 11'h7; end
											3'b010   : begin rd_data_s0[447:256] <= #TCQ rd_data[255:64];  dw_rem <= #TCQ (payload_len < 7 ) ? 11'b0 : payload_len - 11'h6; end
											3'b011   : begin rd_data_s0[415:256] <= #TCQ rd_data[255:96];  dw_rem <= #TCQ (payload_len < 6 ) ? 11'b0 : payload_len - 11'h5; end
                                            3'b100   : begin rd_data_s0[383:256] <= #TCQ rd_data[255:128]; dw_rem <= #TCQ (payload_len < 5 ) ? 11'b0 : payload_len - 11'h4; end
											3'b101   : begin rd_data_s0[351:256] <= #TCQ rd_data[255:160]; dw_rem <= #TCQ (payload_len < 4 ) ? 11'b0 : payload_len - 11'h3; end
                                            3'b110   : begin rd_data_s0[319:256] <= #TCQ rd_data[255:192]; dw_rem <= #TCQ (payload_len < 3 ) ? 11'b0 : payload_len - 11'h2; end
											3'b111   : begin rd_data_s0[287:256] <= #TCQ rd_data[255:224]; dw_rem <= #TCQ (payload_len < 2 ) ? 11'b0 : payload_len - 11'h1; end											
									  endcase
									  if (s_axis_cc_tready) 
									      rd_data_state <= #TCQ PIO_RD_DATA_WAIT; 
									  else
                                          rd_data_state <= #TCQ PIO_RD_DATA_RST; 	              										
							      end
								  else begin 
								      rd_data_s0   <= #TCQ rd_data_s0;
									  rd_data_state <= #TCQ PIO_RD_DATA_RST; 
								  end
						    end //PIO_RD_DATA_RST 
		  PIO_RD_DATA_WAIT : begin
		                     if (s_axis_cc_tready) begin 
		                          rd_data_ready <= #TCQ 1'b1; 
		                          if (dw_rem == 0 ) begin
								      rd_data_s0    <= #TCQ {256'b0, rd_data_s0[767:256]};
									  rd_data_state  <= #TCQ PIO_RD_DATA_RST; 
								  end
								  else begin
								      case (req_addr[4:2])
									        3'b000   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[511:256]};  end
											3'b001   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[479:256]};  end
											3'b010   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[447:256]};  end
											3'b011   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[415:256]};  end
                                            3'b100   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[383:256]};  end
											3'b101   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[351:256]};  end
                                            3'b110   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[319:256]};  end
											3'b111   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[287:256]};  end											
									  endcase
									  if ((dw_rem -1)/8 == 0) 
                                          dw_rem        <= #TCQ 11'h0; 
                                      else 
									      dw_rem        <= #TCQ dw_rem - 11'h8; 
										  
                                      rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  								  
								  end  
                             end
                             else begin 
                                          rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  
                                          rd_data_s0 <= #TCQ rd_data_s0; //_prev;
				             end 									  
		                     end //PIO_RD_DATA_WAIT 
							 
		  default : begin rd_data_s0 <= #TCQ rd_data_s0; rd_data_state <= #TCQ PIO_RD_DATA_RST; end
          endcase		  
	  end
  end  
  end
  
  else begin // (C_DATA_WIDTH == 512 ) : pio_tx_sm_512
    reg  rd_addr_state; 
    reg [10:0] rd_addr_rem;
	
	always @(s_axis_cc_tready) begin 
        rd_en = 1'b0; 
    if (s_axis_cc_tready) 
        rd_en = 1'b1; 
    else
        rd_en = 1'b0;   
    end
	
    always @(posedge user_clk or negedge reset_n) begin 
      if (!reset_n) begin 
	      rd_addr       <= #TCQ 5'b0; 
		  rd_addr_state <= #TCQ 1'b0; 
		  rd_addr_rem   <= #TCQ 11'b0; 
		  //rd_en         <= #TCQ 1'b0; 
	  end 
	  else begin
          case (rd_addr_state) 
		        1'b0   : begin
                                if (req_compl_wd) begin
                                    rd_addr        <= #TCQ req_addr[10:6];
                                    //rd_en          <= #TCQ 1'b1; 
                                    rd_addr_state  <= #TCQ 1'b1; 
                                    case (req_addr[5:2])
									    4'b0000   : rd_addr_rem <= #TCQ (payload_len < 17 ) ? 11'b0 : payload_len - 11'h10; 
										4'b0001   : rd_addr_rem <= #TCQ (payload_len < 16 ) ? 11'b0 : payload_len - 11'hF; 
										4'b0010   : rd_addr_rem <= #TCQ (payload_len < 15 ) ? 11'b0 : payload_len - 11'hE; 
										4'b0011   : rd_addr_rem <= #TCQ (payload_len < 14 ) ? 11'b0 : payload_len - 11'hD; 
										4'b0100   : rd_addr_rem <= #TCQ (payload_len < 13 ) ? 11'b0 : payload_len - 11'hC; 
										4'b0101   : rd_addr_rem <= #TCQ (payload_len < 12 ) ? 11'b0 : payload_len - 11'hB; 
										4'b0110   : rd_addr_rem <= #TCQ (payload_len < 11 ) ? 11'b0 : payload_len - 11'hA; 
										4'b0111   : rd_addr_rem <= #TCQ (payload_len < 10 ) ? 11'b0 : payload_len - 11'h9; 
										4'b1000   : rd_addr_rem <= #TCQ (payload_len < 9 )  ? 11'b0 : payload_len - 11'h8; 
										4'b1001   : rd_addr_rem <= #TCQ (payload_len < 8 )  ? 11'b0 : payload_len - 11'h7; 
										4'b1010   : rd_addr_rem <= #TCQ (payload_len < 7 )  ? 11'b0 : payload_len - 11'h6; 
										4'b1011   : rd_addr_rem <= #TCQ (payload_len < 6 )  ? 11'b0 : payload_len - 11'h5; 
										4'b1100   : rd_addr_rem <= #TCQ (payload_len < 5 )  ? 11'b0 : payload_len - 11'h4; 
										4'b1101   : rd_addr_rem <= #TCQ (payload_len < 4 )  ? 11'b0 : payload_len - 11'h3; 
										4'b1110   : rd_addr_rem <= #TCQ (payload_len < 3 )  ? 11'b0 : payload_len - 11'h2; 
										4'b1111   : rd_addr_rem <= #TCQ (payload_len < 2 )  ? 11'b0 : payload_len - 11'h1; 
                                    endcase			
                                end
                                else begin
                                    rd_addr        <= #TCQ rd_addr; 
                                    rd_addr_state  <= #TCQ 2'b00; 
                                    rd_addr_rem    <= #TCQ 11'b0; 
                                    //rd_en          <= #TCQ 1'b0; 									
                                end								    								
				         end // RST State end
				1'b1   : begin 
				                 if (rd_addr_rem == 0) begin 
								     rd_addr        <= #TCQ rd_addr; 
									 rd_addr_state  <= #TCQ 1'b0; 
									 rd_addr_rem    <= #TCQ 11'b0; 
									 //rd_en          <= #TCQ 1'b1; 
							     end 
								 else begin
								     if ((rd_addr_rem-1)/16 == 0 ) begin 
                                         rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b0; 
									     rd_addr_rem    <= #TCQ 11'b0;
									     //rd_en          <= #TCQ 1'b1; 
                                     end
                                     else begin
 									     rd_addr        <= #TCQ rd_addr + 1; 
									     rd_addr_state  <= #TCQ 1'b1; 
									     rd_addr_rem    <= #TCQ rd_addr_rem - 11'h10;
									     //rd_en          <= #TCQ 1'b1; 
                                     end									 
								 end
						   end // Address Increment State end
          endcase		  
	  end
   end
//State machine to form the read data based on the offset 
    reg [1:0] rd_data_state;
    wire [511:0] temp_512_data; // debugging
    wire [511:0] temp_1024_data; //debugging
    wire [511:0] temp_1536_data; //debugging
    reg [10:0] dw_rem; 
    reg rd_data_ready; 
    localparam PIO_RD_DATA_RST  = 2'b00; 
	localparam PIO_RD_DATA_WAIT = 2'b01;  
	//debugging 
	assign temp_512_data  = rd_data_s0[511:0]; 
	assign temp_1024_data = rd_data_s0[1023:512]; 
    assign temp_1536_data = rd_data_s0[1535:1024]; 
    //debugging
	
  always @(posedge user_clk or negedge reset_n) 
  begin
      if (!reset_n) begin 
	      rd_data_state <= #TCQ 2'b00; 
		  rd_data_s0   <= #TCQ 1536'b0; 
		  rd_data_ready <= #TCQ 1'b0; 
		  dw_rem        <= #TCQ 11'b0; 
      end
	  else begin
          case (rd_data_state)
		  PIO_RD_DATA_RST : begin 
		                          rd_data_ready <= #TCQ 1'b0;
				                  if (req_compl_wd_qq) begin 
								      case (req_addr[5:2])
									        4'b0000   : begin rd_data_s0[1023:512] <= #TCQ rd_data[511:0];  dw_rem <= #TCQ (payload_len < 17 ) ? 11'b0 : payload_len - 11'h10; end
											4'b0001   : begin rd_data_s0[991:512] <= #TCQ rd_data[511:32];  dw_rem <= #TCQ (payload_len < 16 ) ? 11'b0 : payload_len - 11'hF; end
											4'b0010   : begin rd_data_s0[959:512] <= #TCQ rd_data[511:64];  dw_rem <= #TCQ (payload_len < 15 ) ? 11'b0 : payload_len - 11'hE; end
											4'b0011   : begin rd_data_s0[927:512] <= #TCQ rd_data[511:96];  dw_rem <= #TCQ (payload_len < 14 ) ? 11'b0 : payload_len - 11'hD; end
                                            4'b0100   : begin rd_data_s0[895:512] <= #TCQ rd_data[511:128]; dw_rem <= #TCQ (payload_len < 13 ) ? 11'b0 : payload_len - 11'hC; end
											4'b0101   : begin rd_data_s0[863:512] <= #TCQ rd_data[511:160]; dw_rem <= #TCQ (payload_len < 12 ) ? 11'b0 : payload_len - 11'hB; end
                                            4'b0110   : begin rd_data_s0[831:512] <= #TCQ rd_data[511:192]; dw_rem <= #TCQ (payload_len < 11 ) ? 11'b0 : payload_len - 11'hA; end
											4'b0111   : begin rd_data_s0[799:512] <= #TCQ rd_data[511:224]; dw_rem <= #TCQ (payload_len < 10 ) ? 11'b0 : payload_len - 11'h9; end			
                                            4'b1000   : begin rd_data_s0[767:512] <= #TCQ rd_data[511:256]; dw_rem <= #TCQ (payload_len < 9 )  ? 11'b0 : payload_len - 11'h8; end
											4'b1001   : begin rd_data_s0[735:512] <= #TCQ rd_data[511:288]; dw_rem <= #TCQ (payload_len < 8 )  ? 11'b0 : payload_len - 11'h7; end
											4'b1010   : begin rd_data_s0[703:512] <= #TCQ rd_data[511:320]; dw_rem <= #TCQ (payload_len < 7 )  ? 11'b0 : payload_len - 11'h6; end
											4'b1011   : begin rd_data_s0[671:512] <= #TCQ rd_data[511:352]; dw_rem <= #TCQ (payload_len < 6 )  ? 11'b0 : payload_len - 11'h5; end
                                            4'b1100   : begin rd_data_s0[639:512] <= #TCQ rd_data[511:384]; dw_rem <= #TCQ (payload_len < 5 )  ? 11'b0 : payload_len - 11'h4; end
											4'b1101   : begin rd_data_s0[607:512] <= #TCQ rd_data[511:416]; dw_rem <= #TCQ (payload_len < 4 )  ? 11'b0 : payload_len - 11'h3; end
                                            4'b1110   : begin rd_data_s0[575:512] <= #TCQ rd_data[511:448]; dw_rem <= #TCQ (payload_len < 3 )  ? 11'b0 : payload_len - 11'h2; end
											4'b1111   : begin rd_data_s0[543:512] <= #TCQ rd_data[511:480]; dw_rem <= #TCQ (payload_len < 2 )  ? 11'b0 : payload_len - 11'h1; end														
									  endcase
									  if (s_axis_cc_tready) 
									      rd_data_state <= #TCQ PIO_RD_DATA_WAIT; 
									  else
                                          rd_data_state <= #TCQ PIO_RD_DATA_RST; 	              										
							      end
								  else begin 
								      rd_data_s0   <= #TCQ rd_data_s0;
									  rd_data_state <= #TCQ PIO_RD_DATA_RST; 
								  end
						    end //PIO_RD_DATA_RST 
		  PIO_RD_DATA_WAIT : begin
		                     if (s_axis_cc_tready) begin 
		                          rd_data_ready <= #TCQ 1'b1; 
		                          if (dw_rem == 0 ) begin
								      rd_data_s0    <= #TCQ {512'b0, rd_data_s0[1535:512]};
									  rd_data_state  <= #TCQ PIO_RD_DATA_RST; 
								  end
								  else begin
								      case (req_addr[5:2])
									        4'b0000   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[1023:512]}; end
											4'b0001   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[991:512]};  end
											4'b0010   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[959:512]};  end
											4'b0011   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[927:512]};  end
                                            4'b0100   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[895:512]};  end
											4'b0101   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[863:512]};  end
                                            4'b0110   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[831:512]};  end
											4'b0111   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[799:512]};  end		
                                            4'b1000   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[767:512]};  end												
                                            4'b1001   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[735:512]};  end
											4'b1010   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[703:512]};  end
											4'b1011   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[671:512]};  end
											4'b1100   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[639:512]};  end
                                            4'b1101   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[607:512]};  end
											4'b1110   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[575:512]};  end
                                            4'b1111   : begin rd_data_s0 <= #TCQ {rd_data, rd_data_s0[543:512]};  end																						
									  endcase
									  if ((dw_rem -1)/16 == 0) 
                                          dw_rem        <= #TCQ 11'h0; 
                                      else 
									      dw_rem        <= #TCQ dw_rem - 11'h10; 
										  
                                      rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  								  
								  end   
                             end
                             else begin 
                                          rd_data_state <= #TCQ PIO_RD_DATA_WAIT;  
                                          rd_data_s0 <= #TCQ rd_data_s0; //_prev;
				             end 									  
		                    end //PIO_RD_DATA_WAIT 
							 
		  default : begin rd_data_s0 <= #TCQ rd_data_s0; rd_data_state <= #TCQ PIO_RD_DATA_RST; end
          endcase		  
	  end
  end  
  end
endgenerate

  // Calculate byte count based on byte enable

/* currently not used
  always @ (req_be) begin
     
    casex (req_be[3:0])

      4'b1xx1 : byte_count_fbe = 12'h004;
      4'b01x1 : byte_count_fbe = 12'h003;
      4'b1x10 : byte_count_fbe = 12'h003;
      4'b0011 : byte_count_fbe = 12'h002;
      4'b0110 : byte_count_fbe = 12'h002;
      4'b1100 : byte_count_fbe = 12'h002;
      4'b0001 : byte_count_fbe = 12'h001;
      4'b0010 : byte_count_fbe = 12'h001;
      4'b0100 : byte_count_fbe = 12'h001;
      4'b1000 : byte_count_fbe = 12'h001;
      4'b0000 : byte_count_fbe = 12'h001;
      default : byte_count_fbe = 12'h000;
    endcase

    casex (req_be[7:4])

      4'b1xx1 : byte_count_lbe = 12'h004;
      4'b01x1 : byte_count_lbe = 12'h003;
      4'b1x10 : byte_count_lbe = 12'h003;
      4'b0011 : byte_count_lbe = 12'h002;
      4'b0110 : byte_count_lbe = 12'h002;
      4'b1100 : byte_count_lbe = 12'h002;
      4'b0001 : byte_count_lbe = 12'h001;
      4'b0010 : byte_count_lbe = 12'h001;
      4'b0100 : byte_count_lbe = 12'h001;
      4'b1000 : byte_count_lbe = 12'h001;
      4'b0000 : byte_count_lbe = 12'h001;
      default : byte_count_lbe = 12'h000;

    endcase

  end
*/

  // Calculate the byte_count for 1DW or 2DW packets

//   assign byte_count = (payload_len == 1)? (byte_count_lbe + byte_count_fbe) : byte_count_fbe; //currently not used
  // Present address and byte enable to memory module
  
// calculating byte count based on first_be, last_be and payload length 
always @(req_be or payload_len) begin 

casex (req_be [7:0])

       8'b00001xx1   : byte_count = 13'h4; 
       8'b000001x1   : byte_count = 13'h3; 
       8'b00001x10   : byte_count = 13'h3; 
       8'b00000011   : byte_count = 13'h2; 
       8'b00000110   : byte_count = 13'h2; 
       8'b00001100   : byte_count = 13'h2; 
       8'b00000001   : byte_count = 13'h1; 
       8'b00000010   : byte_count = 13'h1; 
       8'b00000100   : byte_count = 13'h1; 
       8'b00001000   : byte_count = 13'h1; 
       8'b00000000   : byte_count = 13'h1; 
       8'b1xxxxxx1   : byte_count = (payload_len*4); 
       8'b01xxxxx1   : byte_count = (payload_len*4)-4'h1;
       8'b001xxxx1   : byte_count = (payload_len*4)-4'h2; 
       8'b0001xxx1   : byte_count = (payload_len*4)-4'h3; 
       8'b1xxxxx10   : byte_count = (payload_len*4)-4'h1; 
       8'b01xxxx10   : byte_count = (payload_len*4)-4'h2; 
       8'b001xxx10   : byte_count = (payload_len*4)-4'h3; 
       8'b0001xx10   : byte_count = (payload_len*4)-4'h4; 
       8'b1xxxx100   : byte_count = (payload_len*4)-4'h2; 
       8'b01xxx100   : byte_count = (payload_len*4)-4'h3; 
       8'b001xx100   : byte_count = (payload_len*4)-4'h4; 
       8'b0001x100   : byte_count = (payload_len*4)-4'h5; 
       8'b1xxx1000   : byte_count = (payload_len*4)-4'h3; 
       8'b01xx1000   : byte_count = (payload_len*4)-4'h4; 
       8'b001x1000   : byte_count = (payload_len*4)-4'h5; 
       8'b00011000   : byte_count = (payload_len*4)-4'h6;
       default       : byte_count = 13'h0; 
endcase
end

  // Calculate lower address based on  byte enable

  always @ (rd_be or req_addr or req_compl_wd_qqq) begin
    casex ({req_compl_wd_qqq, rd_be[3:0]})
        5'b1_0000 : lower_addr = {req_addr[6:2], 2'b00};
        5'b1_xxx1 : lower_addr = {req_addr[6:2], 2'b00};
        5'b1_xx10 : lower_addr = {req_addr[6:2], 2'b01};
        5'b1_x100 : lower_addr = {req_addr[6:2], 2'b10};
        5'b1_1000 : lower_addr = {req_addr[6:2], 2'b11};
        5'b0_xxxx : lower_addr = 8'h0;
    endcase
  end
  
  always @ (rd_be or req_addr) begin
     casex ({rd_be[3:0]})
          4'b0000 : lower_addr_dw = {req_addr[6:2], 2'b00};
          4'bxxx1 : lower_addr_dw = {req_addr[6:2], 2'b00};
          4'bxx10 : lower_addr_dw = {req_addr[6:2], 2'b01};
          4'bx100 : lower_addr_dw = {req_addr[6:2], 2'b10};
          4'b1000 : lower_addr_dw = {req_addr[6:2], 2'b11};
          4'bxxxx : lower_addr_dw = 8'h0;
      endcase
    end
  
  always @  (lower_addr) begin
    casex (lower_addr[4:2])
      3'b000 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h1 :16'h1; 
      3'b001 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h3 :16'h1; 
      3'b010 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h7 :16'h1; 
      3'b011 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'hf :16'h1; 
      3'b100 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h1f :16'h1; 
      3'b101 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h3f :16'h1; 
      3'b110 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'h7f :16'h1; 
      3'b111 : tkeep = (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" ) ? 16'hff :16'h1; 
    endcase
  end

  always @ (posedge user_clk)
  begin
    if (!reset_n) begin
      req_compl_q     <= #TCQ 1'b0;
      req_compl_qq    <= #TCQ 1'b0;
      req_compl_wd_q  <= #TCQ 1'b0;
      req_compl_wd_qq <= #TCQ 1'b0;
      req_compl_wd_qqq <= #TCQ 1'b0;
      req_compl_wd_qqqq <= #TCQ 1'b0; 
      req_compl_wd_qqqqq <= #TCQ 1'b0; 
      tkeep_q         <= #TCQ 16'h0F;
      req_compl_ur_q  <= #TCQ 1'b0;
      req_compl_ur_qq <= #TCQ 1'b0;
    end else begin
      lower_addr_q    <= #TCQ lower_addr;
      tkeep_q         <= #TCQ tkeep;
      tkeep_qq         <= #TCQ tkeep_q;
      lower_addr_qq   <= #TCQ lower_addr_q;
      req_compl_q     <= #TCQ req_compl;
      req_compl_qq    <= #TCQ req_compl_q;
      req_compl_wd_q  <= #TCQ req_compl_wd;
      req_compl_wd_qq <= #TCQ req_compl_wd_q;
      req_compl_wd_qqq <= #TCQ req_compl_wd_qq;
      req_compl_wd_qqqq <= #TCQ req_compl_wd_qqq; 
      req_compl_wd_qqqqq <= #TCQ req_compl_wd_qqqq;
      req_compl_ur_q  <= #TCQ req_compl_ur;
      req_compl_ur_qq <= #TCQ req_compl_ur_q;
    end
  end

  // Logic to compute the Parity of the CC and the RQ channel
  generate
  begin
    if(AXISTEN_IF_RQ_PARITY_CHECK == 1)
    begin
      genvar a;
      for(a=0; a< STRB_WIDTH; a = a + 1) // Parity needs to be computed for every byte of data
      begin : parity_assign
        assign s_axis_rq_tparity[a] = !(  s_axis_rq_tdata[(8*a)+ 0] ^ s_axis_rq_tdata[(8*a)+ 1]
                                 ^ s_axis_rq_tdata[(8*a)+ 2] ^ s_axis_rq_tdata[(8*a)+ 3]
                                 ^ s_axis_rq_tdata[(8*a)+ 4] ^ s_axis_rq_tdata[(8*a)+ 5]
                                 ^ s_axis_rq_tdata[(8*a)+ 6] ^ s_axis_rq_tdata[(8*a)+ 7]);

        assign s_axis_cc_tparity[a] = !(  s_axis_cc_tdata[(8*a)+ 0] ^ s_axis_cc_tdata[(8*a)+ 1]
                                 ^ s_axis_cc_tdata[(8*a)+ 2] ^ s_axis_cc_tdata[(8*a)+ 3]
                                 ^ s_axis_cc_tdata[(8*a)+ 4] ^ s_axis_cc_tdata[(8*a)+ 5]
                                 ^ s_axis_cc_tdata[(8*a)+ 6] ^ s_axis_cc_tdata[(8*a)+ 7]);
      end
    end else begin
      genvar b;
      for(b=0; b< STRB_WIDTH; b = b + 1) // Drive parity low if not enabled
      begin : parity_assign
        assign s_axis_rq_tparity[b] = {PARITY_WIDTH{1'b0}};
        assign s_axis_cc_tparity[b] = {PARITY_WIDTH{1'b0}};
      end
    end
        assign s_axis_rq_tparity[31:16] = 16'b0;
        assign s_axis_cc_tparity[31:16] = 16'b0;
  end
  endgenerate

  generate 
  if( AXISTEN_IF_WIDTH == 2'b11) // 512 -bit interface
  begin
    assign s_axis_cc_tuser   = {(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity : 64'b0), s_axis_cc_tuser_wo_parity[16:0]};
  end
  else
  begin
    assign s_axis_cc_tuser   = {(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity : 32'b0), s_axis_cc_tuser_wo_parity[0]};
  end
  endgenerate

  generate // 512 bit Interface
  if( AXISTEN_IF_WIDTH == 2'b11) // 512 -bit interface
  begin
   
    always @ ( posedge user_clk )
    begin
      if(!reset_n ) begin
        rq_state                   <= #TCQ PIO_TX_RST_STATE;
        rd_data_reg             <= #TCQ 32'b0;
        s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_cc_tlast         <= #TCQ 1'b0;
        s_axis_cc_tvalid        <= #TCQ 1'b0;
        s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_rq_tlast         <= #TCQ 1'b0;
        s_axis_rq_tvalid        <= #TCQ 1'b0;
        s_axis_cc_tuser_wo_parity <= #TCQ {AXI4_CC_TUSER_WIDTH{1'b0}};
        s_axis_rq_tuser         <= #TCQ {AXI4_RQ_TUSER_WIDTH{1'b0}};
        cfg_msg_transmit        <= #TCQ 1'b0;
        cfg_msg_transmit_type   <= #TCQ 3'b0;
        cfg_msg_transmit_data   <= #TCQ 32'b0;
        compl_done              <= #TCQ 1'b0;
        dword_count             <= #TCQ 1'b0;
        trn_sent                <= #TCQ 1'b0;
	len_i                   <= #TCQ 11'b0;
        rd_data_s1              <= #TCQ 96'b0; 
      end else begin // reset_else_block
            case (rq_state)
              PIO_TX_RST_STATE : begin  // Reset_State
                rq_state                  <= #TCQ PIO_TX_RST_STATE;
                s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b1}};
                s_axis_cc_tlast         <= #TCQ 1'b0;
                s_axis_cc_tvalid        <= #TCQ 1'b0;
                s_axis_cc_tuser_wo_parity <= #TCQ 81'b0;
                s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
                s_axis_rq_tlast         <= #TCQ 1'b0;
                s_axis_rq_tvalid        <= #TCQ 1'b0;
                s_axis_rq_tuser         <= #TCQ 60'b0;
                cfg_msg_transmit        <= #TCQ 1'b0;
                cfg_msg_transmit_type   <= #TCQ 3'b0;
                cfg_msg_transmit_data   <= #TCQ 32'b0;
                compl_done              <= #TCQ 1'b0;
                trn_sent                <= #TCQ 1'b0;
                dword_count             <= #TCQ 1'b0;
		len_i                   <= #TCQ payload_len; 

                if(req_compl) begin
                   rq_state <= #TCQ PIO_TX_COMPL_C1;
                end else if (req_compl_wd) begin
                   rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                end else if (req_compl_ur) begin
                   rq_state <= #TCQ PIO_TX_CPL_UR_C1;
                end else if (gen_transaction) begin
                   rq_state <= #TCQ PIO_TX_MRD_C1;
                end
              end // PIO_TX_RST_STATE

              PIO_TX_COMPL_C1 : begin // Completion Without Payload - Alignment doesnt matter
                                   // Sent in a Single Beat When Interface Width is 512 bit
                if(req_compl_qq) begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b1;
                  s_axis_cc_tkeep   <= #TCQ 8'h07;
                  s_axis_cc_tdata   <= #TCQ {256'b0,160'b0,        // Tied to 0 for 3DW completion descriptor
                                             1'b0,          // Force ECRC
                                             1'b0, req_attr,// 4- bits
                                             req_tc,        // 3- bits
                                             1'b0,          // Completer ID to control selection of Client
                                                            // Supplied Bus number
                                             8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                             8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                             req_tag,       // Select Client Tag or core's internal tag
                                             req_rid,       // Requester ID - 16 bits
                                             1'b0,          // Rsvd
                                             1'b0,          // Posioned completion
                                             3'b000,        // SuccessFull completion
                                             (req_mem ? (11'h1 + payload_len) : 11'b0),         // DWord Count 0 - IO Write completions
                                             2'b0,          // Rsvd
                                             1'b0,          // Locked Read Completion
                                             13'h0004,      // Byte Count
                                             6'b0,          // Rsvd
                                             req_at,        // Adress Type - 2 bits
                                             1'b0,          // Rsvd
                                             lower_addr};   // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                                1'b0,                    // Discontinue          
                                                4'b0000,                 // is_eop1_ptr
                                                4'b0000,                 // is_eop0_ptr
                                                2'b00,                   // is_eop[1:0]
                                                2'b00,                   // is_sop1_ptr[1:0]
                                                2'b00,                   // is_sop0_ptr[1:0]
                                                2'b00};                  // is_sop[1:0]

                  if(s_axis_cc_tready) begin
                    rq_state <= #TCQ PIO_TX_RST_STATE;
                    compl_done        <= #TCQ 1'b1;
                  end else begin
                    rq_state <= #TCQ PIO_TX_COMPL_C1;
                  end
                end
              end  //PIO_TX_COMPL
              PIO_TX_COMPL_WD_C1 : begin  // Completion With Payload
                                       // Possible Scenario's Payload can be 1 DW or 2 DW
                                       // Alignment can be either of Dword aligned or address aligned

// Support n-DW 
             // Requires three clock cycle to get the first rd_data from the BRAM 
             if (req_compl_wd_qqqq) begin
                 if(AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE") begin //DWORD_Aligned mode
					   s_axis_cc_tvalid  <= #TCQ 1'b1; 
					   s_axis_cc_tdata   <= #TCQ {rd_data_s0[415:0],       // 13 DW Read Data 
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag,       // Select Client Tag or core's internal tag
                                                 req_rid,       // Requester ID - 16 bits
                                                 1'b0,          // Rsvd
                                                 1'b0,          // Posioned completion
                                                 3'b000,        // SuccessFull completion
                                                 (req_mem ? (payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                                 2'b0,          // Rsvd
                                                 (req_mem_lock? 1'b1 : 1'b0),  // Locked Read Completion
                                                 byte_count,               //13'h0004,      // Byte Count
                                                 6'b0,          // Rsvd
                                                 req_at,        // Adress Type - 2 bits
                                                 1'b0,          // Rsvd
                                                 lower_addr_dw};   // Starting address of the mem byte - 7 bits
                      
					  s_axis_cc_tuser_wo_parity <= #TCQ {64'b0, // parity 64 bit -[80:17]
                                                         1'b0,                    // Discontinue          
                                                         4'b0000,                 // is_eop1_ptr
                                                         4'b0000,                 // is_eop0_ptr
                                                         2'b00,                   // is_eop[1:0]
                                                         2'b00,                   // is_sop1_ptr[1:0]
                                                         2'b00,                   // is_sop0_ptr[1:0]
                                                         2'b01};                  // is_sop[1:0]
					  if (s_axis_cc_tready) begin
					    if (len_i < 14 ) begin
					      case (len_i) 
					             1  : s_axis_cc_tkeep <= #TCQ 16'h000F; 
							     2  : s_axis_cc_tkeep <= #TCQ 16'h001F; 
							     3  : s_axis_cc_tkeep <= #TCQ 16'h003F; 
							     4  : s_axis_cc_tkeep <= #TCQ 16'h007F; 
							     5  : s_axis_cc_tkeep <= #TCQ 16'h00FF; 
								 6  : s_axis_cc_tkeep <= #TCQ 16'h01FF; 
							     7  : s_axis_cc_tkeep <= #TCQ 16'h03FF; 
							     8  : s_axis_cc_tkeep <= #TCQ 16'h07FF; 
							     9  : s_axis_cc_tkeep <= #TCQ 16'h0FFF; 
							    10  : s_axis_cc_tkeep <= #TCQ 16'h1FFF; 
					            11  : s_axis_cc_tkeep <= #TCQ 16'h3FFF; 
							    12  : s_axis_cc_tkeep <= #TCQ 16'h7FFF; 
							    13  : s_axis_cc_tkeep <= #TCQ 16'hFFFF;
					      endcase
						  s_axis_cc_tlast <= #TCQ 1'b1; 
                          rq_state           <= #TCQ PIO_TX_RST_STATE;		
                          rd_data_s1      <= #TCQ 96'h0; 	
                          len_i           <= #TCQ 11'b0; 	
                          compl_done      <= #TCQ 1'b1; 						  
			 end
			  else begin 
						  s_axis_cc_tkeep <= #TCQ 16'hFFFF; 
						  s_axis_cc_tlast <= #TCQ 1'b0;
						  rq_state           <= #TCQ PIO_TX_COMPL_WD_N_DW; 
						  rd_data_s1      <= #TCQ rd_data_s0[511:416];
                          len_i           <= #TCQ len_i - 11'hD; 	
                          compl_done      <= #TCQ 1'b0; 						  
						end
					  end
					 else begin 
					      rq_state           <= #TCQ PIO_TX_COMPL_WD_C1; 
				     end
			     end // DWORD Aligned mode end              
             end 
              end // PIO_TX_COMPL_WD
              PIO_TX_COMPL_PYLD : begin // FIXME : Completion with 1DW Payload in Address Aligned mode
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ tkeep_q;
                s_axis_cc_tdata[31:0]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b000) ? {rd_data} : ((AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE" ) ? rd_data : 32'b0);
                s_axis_cc_tdata[63:32]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b001) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[95:64]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b010) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[127:96]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b011) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[159:128]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b100) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[191:160]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b101) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[223:192]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b110) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[255:224]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b111) ? {rd_data} : {32'b0};

                s_axis_cc_tuser_wo_parity <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                                1'b0,                    // Discontinue          
                                                4'b0000,                 // is_eop1_ptr
                                                4'b0000,                 // is_eop0_ptr
                                                2'b00,                   // is_eop[1:0]
                                                2'b00,                   // is_sop1_ptr[1:0]
                                                2'b00,                   // is_sop0_ptr[1:0]
                                                2'b00};                  // is_sop[1:0]

                if(s_axis_cc_tready) begin
                  rq_state        <= #TCQ PIO_TX_RST_STATE;
                  compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_PYLD;
                end
              end // PIO_TX_COMPL_PYLD

              PIO_TX_COMPL_WD_2DW : begin // Completion with 2DW Payload in DWord Aligned mode
                                          // Requires 2 states to get the 2DW Payload

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 8'h1F;
                s_axis_cc_tdata   <= #TCQ {256'b0,96'b0,         // Tied to 0 for 3DW completion descriptor with 2DW Payload
                                           rd_data,       // 32 bit read data
                                           rd_data_reg,   // 32- bit read data
                                           1'b0,          // Force ECRC
                                           1'b0, req_attr,// 4- bits
                                           req_tc,        // 3- bits
                                           1'b0,          // Completer ID to control selection of Client
                                                          // Supplied Bus number
                                           8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                           8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                           req_tag,       // Select Client Tag or core's internal tag
                                           req_rid,       // Requester ID - 16 bits
                                           1'b0,          // Rsvd
                                           1'b0,          // Posioned completion
                                           3'b000,        // SuccessFull completion
                                           (req_mem ? (11'h1 + payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                           2'b0,          // Rsvd
                                           (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                           byte_count,                //13'h0004,      // Byte Count
                                           6'b0,          // Rsvd
                                           req_at,        // Adress Type - 2 bits
                                           1'b0,          // Rsvd
                                           lower_addr_q};   // Starting address of the mem byte - 7 bits
                s_axis_cc_tuser_wo_parity <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                                1'b0,                    // Discontinue          
                                                4'b0000,                 // is_eop1_ptr
                                                4'b0000,                 // is_eop0_ptr
                                                2'b00,                   // is_eop[1:0]
                                                2'b00,                   // is_sop1_ptr[1:0]
                                                2'b00,                   // is_sop0_ptr[1:0]
                                                2'b00};                  // is_sop[1:0]


                if(s_axis_cc_tready) begin
                  rq_state        <= #TCQ PIO_TX_RST_STATE;
                  compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW;
                  dword_count <= #TCQ 1'b1; // To increment the Read Address
                  rd_data_reg <= #TCQ rd_data; // store the current read data
                end
              end //  PIO_TX_COMPL_WD_2DW

              PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1 : begin 
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ (lower_addr_q[3:2]==2'b00)   ?  16'h003F :
                                          (lower_addr_q[3:2]==2'b01)   ?  16'h007F :
                                          (lower_addr_q[3:2]==2'b10)   ?  16'h00FF :
                                          /*(lower_addr_q[3:2]==2'b10) ?*/16'h01FF;

                s_axis_cc_tdata[511:128] <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b00)   ? {256'b0, 64'b0, rd_data,rd_data_reg} 
                                                :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b01)   ? {256'b0, 32'b0, rd_data,rd_data_reg, 32'b0} 
                                                :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b10)   ? {256'b0,        rd_data,rd_data_reg, 64'b0} 
                                                :/*(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b11)?*/{224'b0,        rd_data,rd_data_reg, 96'b0};
                s_axis_cc_tdata[127:0] <= #TCQ {32'b0,        // Tied to 0 for 3DW completion descriptor
                                           1'b0,          // Force ECRC
                                           1'b0, req_attr,// 4- bits
                                           req_tc,        // 3- bits
                                           1'b0,          // Completer ID to control selection of Client
                                                          // Supplied Bus number
                                           8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                           8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                           req_tag,       // Select Client Tag or core's internal tag
                                           req_rid,       // Requester ID - 16 bits
                                           1'b0,          // Rsvd
                                           1'b0,          // Posioned completion
                                           3'b000,        // SuccessFull completion
                                           (req_mem ? (11'h1 + payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                           2'b0,          // Rsvd
                                           (req_mem_lock? 1'b1 : 1'b0),      // Locked Read Completion
                                           byte_count,               //13'h0004,      // Byte Count
                                           6'b0,          // Rsvd
                                           req_at,        // Adress Type - 2 bits
                                           1'b0,          // Rsvd
                                           lower_addr_q};   // Starting address of the mem byte - 7 bits

                s_axis_cc_tuser_wo_parity <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                          1'b0,                    // Discontinue          
                                          4'b0000,                 // is_eop1_ptr
                                          4'b0000,                 // is_eop0_ptr
                                          2'b01,                   // is_eop[1:0]
                                          2'b00,                   // is_sop1_ptr[1:0]
                                          2'b00,                   // is_sop0_ptr[1:0]
                                          2'b01};                  // is_sop[1:0]

                dword_count       <= #TCQ 1'b0;
                if(s_axis_cc_tready) begin
                 rq_state        <= #TCQ PIO_TX_RST_STATE;
                 compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1;
                end // PIO_TX_COMPL_WD_2DW_ADDR_ALGN
              end

              PIO_TX_CPL_UR_C1 : begin // Completions with UR - Alignement mode matters here
                if (req_compl_ur_qq) begin
                     s_axis_cc_tvalid  <= #TCQ 1'b1;
                     s_axis_cc_tlast   <= #TCQ 1'b1;
                     s_axis_cc_tkeep   <= #TCQ 8'hFF;
                     s_axis_cc_tdata   <= #TCQ {256'b0,req_des_qword1, // 64 bits - Descriptor of the request 2 DW
                                                req_des_qword0, // 64 bits - Descriptor of the request 2 DW
                                                8'b0, // Rsvd
                                                req_des_tph_st_tag,   // TPH Steering tag - 8 bits
                                                5'b0,  // Rsvd
                                                req_des_tph_type,    // TPH type - 2 bits
                                                req_des_tph_present, // TPH present - 1 bit
                                                req_be,          // Request Byte enables - 8bits
                                                1'b0,          // Force ECRC
                                                1'b0, req_attr,// 4- bits
                                                req_tc,        // 3- bits
                                                1'b0,          // Completer ID to control selection of Client
                                                               // Supplied Bus number
                                                8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                req_tag,       // Select Client Tag or core's internal tag
                                                req_rid,       // Requester ID - 16 bits
                                                1'b0,          // Rsvd
                                                1'b0,          // Posioned completion
                                                3'b001,        // Completion Status - UR
                                                11'h005,       // DWord Count -55
                                                2'b0,          // Rsvd
                                                (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                                13'h0014,      // Byte Count - 20 bytes of Payload
                                                6'b0,          // Rsvd
                                                req_at,        // Adress Type - 2 bits
                                                1'b0,          // Rsvd
                                                lower_addr};   // Starting address of the mem byte - 7 bits

                     s_axis_cc_tuser_wo_parity <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                                1'b0,                    // Discontinue          
                                                4'b0000,                 // is_eop1_ptr
                                                4'b0000,                 // is_eop0_ptr
                                                2'b00,                   // is_eop[1:0]
                                                2'b00,                   // is_sop1_ptr[1:0]
                                                2'b00,                   // is_sop0_ptr[1:0]
                                                2'b00};                  // is_sop[1:0]

                     if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
                     end else begin
                       rq_state        <= #TCQ PIO_TX_CPL_UR_C1;
                     end
                end
              end // PIO_TX_CPL_UR

              PIO_TX_MRD_C1 : begin // Not used Memory Read Transaction - Alignment Doesnt Matter
                s_axis_rq_tvalid  <= #TCQ 1'b1;
                s_axis_rq_tlast   <= #TCQ 1'b1;
                s_axis_rq_tkeep   <= #TCQ 8'h0F;  // 4DW Descriptor For Memory Transaction Alone
                s_axis_rq_tdata   <= #TCQ {256'b0,128'b0,       // 4DW Unused
                                           1'b0,         // Force ECRC
                                           3'b000,       // Attributes
                                           3'b000,       // Traffic Class
                                           1'b0,         // RID Enable to use the Client supplied Bus/Device/Func No
                                           16'b0,        // Completer -ID, set only for Completers or ID based routing
                                           (AXISTEN_IF_ENABLE_CLIENT_TAG ?
                                           8'h00 : req_tag),  // Select Client Tag or core's internal tag
                                           8'h00,             // Req Bus No- used only when RID enable = 1
                                           8'h00,             // Req Dev/Func no - used only when RID enable = 1
                                           1'b0,              // Poisoned Req
                                           4'b0000,           // Req Type for MRd Req
                                           11'h001,           // DWORD Count
                                           62'h2AAA_BBBB_CCCC_DDDD, // Memory Read Address [62 bits]
                                           2'b00};             //AT -> 00- Untranslated Address

                s_axis_rq_tuser          <= #TCQ {(AXISTEN_IF_RQ_PARITY_CHECK ? s_axis_rq_tparity : 64'b0), // Parity
                                                  6'b101010,      // Seq Number 1
                                                  6'b101010,      // Seq Number 0
                                                  16'h0000,        // TPH Steering Tag
                                                  2'b0,         // TPH indirect Tag Enable
                                                  4'b0000,         // TPH Type
                                                  2'b00,         // TPH Present
                                                  1'b0,         // Discontinue
                                                  4'b0000, //eop1 ptr   
                                                  4'b0000, //eop0 ptr  
                                                  2'b01, //is EOP?  
                                                  2'b10, //sop1 ptr   
                                                  2'b00, //sop0 ptr   
                                                  2'b01,  //is SOP 
                                                  4'b0000,       // Byte Lane number in case of Address Aligned mode
                                                  8'h0,    // Last BE of the Read Data
                                                  8'hF }; // First BE of the Read Data

                if(s_axis_rq_tready) begin
                  rq_state <= #TCQ PIO_TX_RST_STATE;
                  trn_sent <= #TCQ 1'b1;
                end
                else
                  rq_state <= #TCQ PIO_TX_MRD_C1;
              end // PIO_TX_MRD
			  
	     PIO_TX_COMPL_WD_N_DW : begin
               if ((len_i-1)/16 == 0) begin 
                   s_axis_cc_tvalid  <= #TCQ 1'b1;
                   s_axis_cc_tlast   <= #TCQ 1'b1;
                   s_axis_cc_tuser_wo_parity <= #TCQ {64'b0, // parity 64 bit -[80:17]
                                                1'b0,                    // Discontinue          
                                                4'b0000,                 // is_eop1_ptr
                                                ((len_i==16) ? 4'b1111 : 
												(len_i==15) ? 4'b1110 : 
												(len_i==14) ? 4'b1101 : 
												(len_i==13) ? 4'b1100 : 
												(len_i==12) ? 4'b1011 : 
												(len_i==11) ? 4'b1010 : 
												(len_i==10) ? 4'b1001 :
												(len_i==9) ? 4'b1000 :
												(len_i==8) ? 4'b0111 :
												(len_i==7) ? 4'b0110 :
												(len_i==6) ? 4'b0101 :
												(len_i==5) ? 4'b0100 :
												(len_i==4) ? 4'b0011 : 
												(len_i==3) ? 4'b0010 :
                                                (len_i==2) ? 4'b0001 : 4'b0000 ), // is_eop0_ptr
                                                2'b01,                   // is_eop[1:0]
                                                2'b00,                   // is_sop1_ptr[1:0]
                                                2'b00,                   // is_sop0_ptr[1:0]
                                                2'b00};                  // is_sop[1:0]
                   case (len_i) 
                          1      : begin s_axis_cc_tdata <= #TCQ {480'b0, rd_data_s1[31:0]};
                                         s_axis_cc_tkeep <= #TCQ 16'h0001; 
                                   end
                          2      : begin s_axis_cc_tdata <= #TCQ {448'b0, rd_data_s1[63:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h0003; 
                                   end
                          3      : begin s_axis_cc_tdata <= #TCQ {416'b0, rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h0007; 
                                   end
                          4      : begin s_axis_cc_tdata <= #TCQ {384'b0, rd_data_s0[31:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h000F; 
                                   end
                          5      : begin s_axis_cc_tdata <= #TCQ {352'b0, rd_data_s0[63:0],rd_data_s1[95:0]};
                                         s_axis_cc_tkeep <= #TCQ 16'h001F; 
                                   end
                          6      : begin s_axis_cc_tdata <= #TCQ {320'b0, rd_data_s0[95:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h003F; 
                                   end
                          7      : begin s_axis_cc_tdata <= #TCQ {288'b0, rd_data_s0[127:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h007F; 
                                   end
                          8      : begin s_axis_cc_tdata <= #TCQ {256'b0,rd_data_s0[159:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h00FF; 
                                   end
			  9      : begin s_axis_cc_tdata <= #TCQ {224'b0,rd_data_s0[191:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h01FF; 
                                   end	
                         10      : begin s_axis_cc_tdata <= #TCQ {192'b0,rd_data_s0[223:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h03FF; 
                                   end
                         11      : begin s_axis_cc_tdata <= #TCQ {160'b0,rd_data_s0[255:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h07FF; 
                                   end		
                         12      : begin s_axis_cc_tdata <= #TCQ {128'b0,rd_data_s0[287:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h0FFF; 
                                   end	
                         13      : begin s_axis_cc_tdata <= #TCQ {96'b0,rd_data_s0[319:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h1FFF; 
                                   end	
                         14      : begin s_axis_cc_tdata <= #TCQ {64'b0,rd_data_s0[351:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h3FFF; 
                                   end	
                         15      : begin s_axis_cc_tdata <= #TCQ {32'b0,rd_data_s0[383:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'h7FFF; 
                                   end	
                         16      : begin s_axis_cc_tdata <= #TCQ {rd_data_s0[415:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 16'hFFFF; 
                                   end									   
                   endcase
                   len_i <= #TCQ 11'b0;
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
					   rd_data_s1   <= #TCQ rd_data_s0[511:416]; 
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end               
               end // len_i <= 16
               else begin 
                   s_axis_cc_tvalid            <= #TCQ 1'b1; 
                   s_axis_cc_tlast             <= #TCQ 1'b0; 
                   s_axis_cc_tuser_wo_parity   <= #TCQ {/*(AXISTEN_IF_CC_PARITY_CHECK ? s_axis_cc_tparity :*/ 64'b0, // parity 64 bit -[80:17]
                                                          1'b0,                    // Discontinue          
                                                          4'b0000,                 // is_eop1_ptr
                                                          4'b0000,                 // is_eop0_ptr
                                                          2'b00,                   // is_eop[1:0]
                                                          2'b00,                   // is_sop1_ptr[1:0]
                                                          2'b00,                   // is_sop0_ptr[1:0]
                                                          2'b00};                  // is_sop[1:0]
                   s_axis_cc_tkeep             <= #TCQ 16'hFFFF; 
                   s_axis_cc_tdata             <= #TCQ {rd_data_s0[415:0], rd_data_s1[95:0]}; 
                   rd_data_s1                  <= #TCQ rd_data_s0[511:416]; 
                   len_i                       <= #TCQ len_i - 11'h10; 
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW;
                       compl_done   <= #TCQ 1'b0;
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end     
               end    
           end //PIO_TX_COMPL_WD_N_DW
            endcase
          end // reset_else_block
      end // Always Block Ends
    end // If AXISTEN_IF_WIDTH = 512
  else if( AXISTEN_IF_WIDTH == 2'b10) // 256-bit interface
  begin
    always @ ( posedge user_clk )
    begin
      if(!reset_n ) begin
        rq_state                   <= #TCQ PIO_TX_RST_STATE;
        rd_data_reg             <= #TCQ 32'b0;
        s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_cc_tlast         <= #TCQ 1'b0;
        s_axis_cc_tvalid        <= #TCQ 1'b0;
        s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_rq_tlast         <= #TCQ 1'b0;
        s_axis_rq_tvalid        <= #TCQ 1'b0;
        s_axis_cc_tuser_wo_parity <= #TCQ {AXI4_CC_TUSER_WIDTH{1'b0}};
        s_axis_rq_tuser         <= #TCQ {AXI4_RQ_TUSER_WIDTH{1'b0}};
        cfg_msg_transmit        <= #TCQ 1'b0;
        cfg_msg_transmit_type   <= #TCQ 3'b0;
        cfg_msg_transmit_data   <= #TCQ 32'b0;
        compl_done              <= #TCQ 1'b0;
        dword_count             <= #TCQ 1'b0;
        trn_sent                <= #TCQ 1'b0;
        len_i                   <= #TCQ 11'b0;
        rd_data_s1              <= #TCQ 96'b0; 
      end else begin // reset_else_block
            case (rq_state)
              PIO_TX_RST_STATE : begin  // Reset_State
                rq_state                   <= #TCQ PIO_TX_RST_STATE;
                s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b1}};
                s_axis_cc_tlast         <= #TCQ 1'b0;
                s_axis_cc_tvalid        <= #TCQ 1'b0;
                s_axis_cc_tuser_wo_parity <= #TCQ 81'b0;
                s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
                s_axis_rq_tlast         <= #TCQ 1'b0;
                s_axis_rq_tvalid        <= #TCQ 1'b0;
                s_axis_rq_tuser         <= #TCQ 60'b0;
                cfg_msg_transmit        <= #TCQ 1'b0;
                cfg_msg_transmit_type   <= #TCQ 3'b0;
                cfg_msg_transmit_data   <= #TCQ 32'b0;
                compl_done              <= #TCQ 1'b0;
                trn_sent                <= #TCQ 1'b0;
                dword_count             <= #TCQ 1'b0;
                len_i                   <= #TCQ payload_len; 

                if(req_compl) begin
                   rq_state <= #TCQ PIO_TX_COMPL_C1;
                end else if (req_compl_wd) begin
                   rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                end else if (req_compl_ur) begin
                   rq_state <= #TCQ PIO_TX_CPL_UR_C1;
                end else if (gen_transaction) begin
                   rq_state <= #TCQ PIO_TX_MRD_C1;
                end
              end // PIO_TX_RST_STATE

              PIO_TX_COMPL_C1 : begin // Completion Without Payload - Alignment doesnt matter
                                   // Sent in a Single Beat When Interface Width is 256 bit
                if(req_compl_qq) begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b1;
                  s_axis_cc_tkeep   <= #TCQ 8'h07;
                  s_axis_cc_tdata   <= #TCQ {160'b0,        // Tied to 0 for 3DW completion descriptor
                                             1'b0,          // Force ECRC
                                             1'b0, req_attr,// 4- bits
                                             req_tc,        // 3- bits
                                             1'b0,          // Completer ID to control selection of Client
                                                            // Supplied Bus number
                                             8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                             8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                             req_tag,       // Select Client Tag or core's internal tag
                                             req_rid,       // Requester ID - 16 bits
                                             1'b0,          // Rsvd
                                             1'b0,          // Posioned completion
                                             3'b000,        // SuccessFull completion
                                             (req_mem ? (11'h1 + payload_len) : 11'b0),         // DWord Count 0 - IO Write completions
                                             2'b0,          // Rsvd
                                             1'b0,          // Locked Read Completion
                                             13'h0004,      // Byte Count
                                             6'b0,          // Rsvd
                                             req_at,        // Adress Type - 2 bits
                                             1'b0,          // Rsvd
                                             lower_addr};   // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state <= #TCQ PIO_TX_RST_STATE;
                    compl_done        <= #TCQ 1'b1;
                  end else begin
                    rq_state <= #TCQ PIO_TX_COMPL_C1;
                  end
                end
              end  //PIO_TX_COMPL

              PIO_TX_COMPL_WD_C1 : begin  // Completion With Payload
                                       // Alignment can be either of Dword aligned or address aligned
             // Support n-DW 
             // Requires three clock cycle to get the first rd_data from the BRAM 
             if (req_compl_wd_qqqq) begin
                 if(AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE") begin //DWORD_Aligned mode
					   s_axis_cc_tvalid  <= #TCQ 1'b1; 
					   s_axis_cc_tdata   <= #TCQ {rd_data_s0[159:0],       // 5- DW read data
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag,       // Select Client Tag or core's internal tag
                                                 req_rid,       // Requester ID - 16 bits
                                                 1'b0,          // Rsvd
                                                 1'b0,          // Posioned completion
                                                 3'b000,        // SuccessFull completion
                                                 (req_mem ? (payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                                 2'b0,          // Rsvd
                                                 (req_mem_lock? 1'b1 : 1'b0),  // Locked Read Completion
                                                byte_count,     // 13'h0004,      // Byte Count
                                                 6'b0,          // Rsvd
                                                 req_at,        // Adress Type - 2 bits
                                                 1'b0,          // Rsvd
                                                 lower_addr_dw};   // Starting address of the mem byte - 7 bits
                      s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
					  if (s_axis_cc_tready) begin
					    if (len_i < 6 ) begin
					      case (len_i) 
					             1  : s_axis_cc_tkeep <= #TCQ 8'h0F; 
							     2  : s_axis_cc_tkeep <= #TCQ 8'h1F; 
							     3  : s_axis_cc_tkeep <= #TCQ 8'h3F; 
							     4  : s_axis_cc_tkeep <= #TCQ 8'h7F; 
							     5  : s_axis_cc_tkeep <= #TCQ 8'hFF; 
					      endcase
						  s_axis_cc_tlast <= #TCQ 1'b1; 
                          rq_state           <= #TCQ PIO_TX_RST_STATE;		
                          rd_data_s1    <= #TCQ 96'h0; 	
                          len_i           <= #TCQ 11'b0; 	
                          compl_done      <= #TCQ 1'b1; 						  
				        end
						else begin 
						  s_axis_cc_tkeep <= #TCQ 8'hFF; 
						  s_axis_cc_tlast <= #TCQ 1'b0;
						  rq_state           <= #TCQ PIO_TX_COMPL_WD_N_DW; 
						  rd_data_s1    <= #TCQ rd_data_s0[255:160];
                          len_i           <= #TCQ len_i - 11'h5; 	
                          compl_done      <= #TCQ 1'b0; 						  
						end
					  end
					 else begin 
					      rq_state           <= #TCQ PIO_TX_COMPL_WD_C1; 
				     end
					 
			     end // DWORD Aligned mode end
                 
                 else begin //Address aligned mode
                      s_axis_cc_tvalid  <= #TCQ 1'b1;
                      s_axis_cc_tlast   <= #TCQ 1'b0;
                      s_axis_cc_tkeep   <= #TCQ 8'h07;
                      s_axis_cc_tdata   <= #TCQ {160'b0,        // Tied to 0 for 3DW completion descriptor
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag,       // Select Client Tag or core's internal tag
                                                 req_rid,       // Requester ID - 16 bits
                                                 1'b0,          // Rsvd
                                                 1'b0,          // Posioned completion
                                                 3'b000,        // SuccessFull completion
                                                 (req_mem ? (payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                                 2'b0,          // Rsvd
                                                 (req_mem_lock? 1'b1 : 1'b0),      // Locked Read Completion
                                                 byte_count,    //13'h0004,      // Byte Count
                                                 6'b0,          // Rsvd
                                                 req_at,        // Adress Type - 2 bits
                                                 1'b0,          // Rsvd
                                                 lower_addr_dw};   // Starting address of the mem byte - 7 bits
                      s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                      compl_done        <= #TCQ 1'b0;

                      if(s_axis_cc_tready) begin
                        rq_state <= #TCQ PIO_TX_COMPL_PYLD;
                      end else begin
                        rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                      end

                 end //Address aligned mode end
             end 
              end // PIO_TX_COMPL_WD

              PIO_TX_COMPL_PYLD : begin // Completion with 1DW Payload in Address Aligned mode

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ tkeep_q;
                s_axis_cc_tdata[31:0]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b000) ? {rd_data} : ((AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE" ) ? rd_data : 32'b0);
                s_axis_cc_tdata[63:32]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b001) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[95:64]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b010) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[127:96]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b011) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[159:128]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b100) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[191:160]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b101) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[223:192]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b110) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[255:224]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b111) ? {rd_data} : {32'b0};

                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state        <= #TCQ PIO_TX_RST_STATE;
                  compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_PYLD;
                end
              end // PIO_TX_COMPL_PYLD

              PIO_TX_COMPL_WD_2DW : begin // Completion with 2DW Payload in DWord Aligned mode
                                          // Requires 2 states to get the 2DW Payload

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 8'h1F;
                s_axis_cc_tdata   <= #TCQ {96'b0,         // Tied to 0 for 3DW completion descriptor with 2DW Payload
                                           rd_data,       // 32 bit read data
                                           rd_data_reg,   // 32- bit read data
                                           1'b0,          // Force ECRC
                                           1'b0, req_attr,// 4- bits
                                           req_tc,        // 3- bits
                                           1'b0,          // Completer ID to control selection of Client
                                                          // Supplied Bus number
                                           8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                           8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                           req_tag,       // Select Client Tag or core's internal tag
                                           req_rid,       // Requester ID - 16 bits
                                           1'b0,          // Rsvd
                                           1'b0,          // Posioned completion
                                           3'b000,        // SuccessFull completion
                                           (req_mem ? (11'h1 + payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                           2'b0,          // Rsvd
                                           (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                           13'h0004,      // Byte Count
                                           6'b0,          // Rsvd
                                           req_at,        // Adress Type - 2 bits
                                           1'b0,          // Rsvd
                                           lower_addr_q};   // Starting address of the mem byte - 7 bits
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state        <= #TCQ PIO_TX_RST_STATE;
                  compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW;
                  dword_count <= #TCQ 1'b1; // To increment the Read Address
                  rd_data_reg <= #TCQ rd_data; // store the current read data
                end

              end //  PIO_TX_COMPL_WD_2DW

              PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1 : begin // Completions with 2-DW Payload and Addr aligned mode
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ tkeep_q;
                s_axis_cc_tdata[255:0]     <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b000) ?  {192'b0, {rd_data,rd_data_reg}} 
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b001) ?  {160'b0, {rd_data,rd_data_reg}, 32'b0}
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b010) ?  {128'b0, {rd_data,rd_data_reg}, 64'b0} 
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b011) ?  { 96'b0, {rd_data,rd_data_reg}, 96'b0} 
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b100) ?  { 64'b0, {rd_data,rd_data_reg},128'b0} 
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b101) ?  { 32'b0, {rd_data,rd_data_reg},160'b0} 
                                                  :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b110) ?  {        {rd_data,rd_data_reg},192'b0} 
                                                  :/*(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[4:2]==3'b111) ?*/  {    {        rd_data_reg},224'b0}; 

                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                dword_count       <= #TCQ 1'b0;
                if(s_axis_cc_tready) begin
		   if(lower_addr_q[4:2]==3'b111)
		   begin
                     rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                     compl_done   <= #TCQ 1'b0;
                     s_axis_cc_tlast   <= #TCQ 1'b0;
		   end
		   else
		   begin
                     rq_state        <= #TCQ PIO_TX_RST_STATE;
                     compl_done   <= #TCQ 1'b1;
                     s_axis_cc_tlast   <= #TCQ 1'b1;
		   end
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1;
                end // PIO_TX_COMPL_WD_2DW_ADDR_ALGN
              end

              PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2 : begin // Completions with 2-DW Payload and Addr aligned mode
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 8'h01;
                s_axis_cc_tdata   <= #TCQ {224'b0, rd_data};

                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                dword_count       <= #TCQ 1'b0;
                if(s_axis_cc_tready) begin
                   rq_state        <= #TCQ PIO_TX_RST_STATE;
                   compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                end // PIO_TX_COMPL_WD_2DW_ADDR_ALGN
              end

              PIO_TX_CPL_UR_C1 : begin // Completions with UR - Alignement mode matters here
                if (req_compl_ur_qq) begin
                     s_axis_cc_tvalid  <= #TCQ 1'b1;
                     s_axis_cc_tlast   <= #TCQ 1'b1;
                     s_axis_cc_tkeep   <= #TCQ 8'hFF;
                     s_axis_cc_tdata   <= #TCQ {req_des_qword1, // 64 bits - Descriptor of the request 2 DW
                                                req_des_qword0, // 64 bits - Descriptor of the request 2 DW
                                                8'b0, // Rsvd
                                                req_des_tph_st_tag,   // TPH Steering tag - 8 bits
                                                5'b0,  // Rsvd
                                                req_des_tph_type,    // TPH type - 2 bits
                                                req_des_tph_present, // TPH present - 1 bit
                                                req_be,          // Request Byte enables - 8bits
                                                1'b0,          // Force ECRC
                                                1'b0, req_attr,// 4- bits
                                                req_tc,        // 3- bits
                                                1'b0,          // Completer ID to control selection of Client
                                                               // Supplied Bus number
                                                8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                req_tag,       // Select Client Tag or core's internal tag
                                                req_rid,       // Requester ID - 16 bits
                                                1'b0,          // Rsvd
                                                1'b0,          // Posioned completion
                                                3'b001,        // Completion Status - UR
                                                11'h005,       // DWord Count -55
                                                2'b0,          // Rsvd
                                                (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                                13'h0014,      // Byte Count - 20 bytes of Payload
                                                6'b0,          // Rsvd
                                                req_at,        // Adress Type - 2 bits
                                                1'b0,          // Rsvd
                                                lower_addr};   // Starting address of the mem byte - 7 bits
                     s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                     if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
                     end else begin
                       rq_state        <= #TCQ PIO_TX_CPL_UR_C1;
                     end
                end
              end // PIO_TX_CPL_UR

              PIO_TX_MRD_C1 : begin // Memory Read Transaction - Alignment Doesnt Matter
                s_axis_rq_tvalid  <= #TCQ 1'b1;
                s_axis_rq_tlast   <= #TCQ 1'b1;
                s_axis_rq_tkeep   <= #TCQ 8'h0F;  // 4DW Descriptor For Memory Transaction Alone
                s_axis_rq_tdata   <= #TCQ {128'b0,       // 4DW Unused
                                           1'b0,         // Force ECRC
                                           3'b000,       // Attributes
                                           3'b000,       // Traffic Class
                                           1'b0,         // RID Enable to use the Client supplied Bus/Device/Func No
                                           16'b0,        // Completer -ID, set only for Completers or ID based routing
                                           (AXISTEN_IF_ENABLE_CLIENT_TAG ?
                                           8'h00 : req_tag),  // Select Client Tag or core's internal tag
                                           8'h00,             // Req Bus No- used only when RID enable = 1
                                           8'h00,             // Req Dev/Func no - used only when RID enable = 1
                                           1'b0,              // Poisoned Req
                                           4'b0000,           // Req Type for MRd Req
                                           11'h001,           // DWORD Count
                                           62'h2AAA_BBBB_CCCC_DDDD, // Memory Read Address [62 bits]
                                           2'b00};             //AT -> 00- Untranslated Address

                s_axis_rq_tuser          <= #TCQ {(AXISTEN_IF_RQ_PARITY_CHECK ? s_axis_rq_tparity : 32'b0), // Parity
                                                  4'b1010,      // Seq Number
                                                  8'h00,        // TPH Steering Tag
                                                  1'b0,         // TPH indirect Tag Enable
                                                  2'b0,         // TPH Type
                                                  1'b0,         // TPH Present
                                                  1'b0,         // Discontinue
                                                  3'b000,       // Byte Lane number in case of Address Aligned mode
                                                  4'h0,    // Last BE of the Read Data
                                                  4'hF}; // First BE of the Read Data

                if(s_axis_rq_tready) begin
                  rq_state <= #TCQ PIO_TX_RST_STATE;
                  trn_sent <= #TCQ 1'b1;
                end
                else
                  rq_state <= #TCQ PIO_TX_MRD_C1;
              end // PIO_TX_MRD
             
           PIO_TX_COMPL_WD_N_DW : begin
               if ((len_i-1)/8 == 0) begin 
                   s_axis_cc_tvalid  <= #TCQ 1'b1;
                   s_axis_cc_tlast   <= #TCQ 1'b1;
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   case (len_i) 
                          1      : begin s_axis_cc_tdata <= #TCQ {224'b0, rd_data_s1[31:0]};
                                         s_axis_cc_tkeep <= #TCQ 8'h01; 
                                   end
                          2      : begin s_axis_cc_tdata <= #TCQ {192'b0, rd_data_s1[63:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'h03; 
                                   end
                          3      : begin s_axis_cc_tdata <= #TCQ {160'b0, rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'h07; 
                                   end
                          4      : begin s_axis_cc_tdata <= #TCQ {128'b0, rd_data_s0[31:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'h0F; 
                                   end
                          5      : begin s_axis_cc_tdata <= #TCQ {96'b0, rd_data_s0[63:0],rd_data_s1[95:0]};
                                         s_axis_cc_tkeep <= #TCQ 8'h1F; 
                                   end
                          6      : begin s_axis_cc_tdata <= #TCQ {64'b0, rd_data_s0[95:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'h3F; 
                                   end
                          7      : begin s_axis_cc_tdata <= #TCQ {32'b0, rd_data_s0[127:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'h7F; 
                                   end
                          8      : begin s_axis_cc_tdata <= #TCQ {rd_data_s0[159:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 8'hFF; 
                                   end
                   endcase
                   len_i <= #TCQ 11'b0;
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
					   rd_data_s1   <= #TCQ rd_data_s1; 
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end               
               end // len_i <= 8 
               else begin 
                   s_axis_cc_tvalid            <= #TCQ 1'b1; 
                   s_axis_cc_tlast             <= #TCQ 1'b0; 
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   s_axis_cc_tkeep             <= #TCQ 8'hFF; 
                   s_axis_cc_tdata             <= #TCQ {rd_data_s0[159:0], rd_data_s1[95:0]}; 
                   rd_data_s1                 <= #TCQ rd_data_s0[255:160]; 
                   len_i                       <= #TCQ len_i - 11'h8; 
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW;
                       compl_done   <= #TCQ 1'b0;
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end     
               end    
           end //PIO_TX_COMPL_WD_N_DW
            endcase
          end // reset_else_block
      end // Always Block Ends
    end // If AXISTEN_IF_WIDTH = 256

    else if( AXISTEN_IF_WIDTH == 2'b01) // 128-bit Interface
    begin
    always @ ( posedge user_clk )
    begin
      if(!reset_n ) begin
        rq_state                   <= #TCQ PIO_TX_RST_STATE;
        rd_data_reg             <= #TCQ 32'b0;
        s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_cc_tlast         <= #TCQ 1'b0;
        s_axis_cc_tvalid        <= #TCQ 1'b0;
        s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_rq_tlast         <= #TCQ 1'b0;
        s_axis_rq_tvalid        <= #TCQ 1'b0;
        s_axis_cc_tuser_wo_parity <= #TCQ {AXI4_CC_TUSER_WIDTH{1'b0}};
        s_axis_rq_tuser         <= #TCQ {AXI4_RQ_TUSER_WIDTH{1'b0}};
        cfg_msg_transmit        <= #TCQ 1'b0;
        cfg_msg_transmit_type   <= #TCQ 3'b0;
        cfg_msg_transmit_data   <= #TCQ 32'b0;
        compl_done              <= #TCQ 1'b0;
        dword_count             <= #TCQ 1'b0;
        trn_sent                <= #TCQ 1'b0;
		len_i                   <= #TCQ 11'b0; 
		rd_data_s1              <= #TCQ 96'b0;
        bufrd_rq_addr_q        <= #TCQ 64'b0;
        bufrd_rq_data_q        <= #TCQ 128'b0;
        bufrd_packet_done      <= #TCQ 1'b0;

        bufwr_rq_addr_q        <= #TCQ 64'b0;
        bufwr_rq_is_read_q     <= #TCQ 1'b0;
        bufwr_packet_done      <= #TCQ 1'b0;
      end else begin // reset_else_block
            // Default low; asserted for one cycle after the payload handshake.
           // Completion indications are one-clock pulses.
            bufrd_packet_done <= #TCQ 1'b0;
            bufwr_packet_done <= #TCQ 1'b0;
            case (rq_state)
              PIO_TX_RST_STATE : begin  // Reset_State
                rq_state                   <= #TCQ PIO_TX_RST_STATE;
                s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b1}};
                s_axis_cc_tlast         <= #TCQ 1'b0;
                s_axis_cc_tvalid        <= #TCQ 1'b0;
                s_axis_cc_tuser_wo_parity <= #TCQ 81'b0;
                s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
                s_axis_rq_tlast         <= #TCQ 1'b0;
                s_axis_rq_tvalid        <= #TCQ 1'b0;
                s_axis_rq_tuser         <= #TCQ 60'b0;
                cfg_msg_transmit        <= #TCQ 1'b0;
                cfg_msg_transmit_type   <= #TCQ 3'b0;
                cfg_msg_transmit_data   <= #TCQ 32'b0;
                compl_done              <= #TCQ 1'b0;
                trn_sent                <= #TCQ 1'b0;
                dword_count             <= #TCQ 1'b0;
		len_i                   <= #TCQ payload_len; 

                if(req_compl) begin
                   rq_state <= #TCQ PIO_TX_COMPL_C1;
                end else if (req_compl_wd) begin
                   rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                end else if (req_compl_ur) begin
                   rq_state <= #TCQ PIO_TX_CPL_UR_C1;
                end else if (bufrd_rq_start) begin
                    // D2H: posted Memory Write with a payload.
                    bufrd_rq_addr_q    <= #TCQ bufrd_rq_addr;
                    bufrd_rq_data_q    <= #TCQ bufrd_rq_data;
                    bufwr_rq_is_read_q <= #TCQ 1'b0;
                    rq_state           <= #TCQ PIO_TX_MRD_C1;

                 end else if (bufwr_rq_start) begin
                    // H2D: non-posted Memory Read without a payload.
                    bufwr_rq_addr_q    <= #TCQ bufwr_rq_addr;
                    bufwr_rq_is_read_q <= #TCQ 1'b1;
                    rq_state           <= #TCQ PIO_TX_MRD_C1;

                 end else if (gen_transaction) begin
                    bufwr_rq_is_read_q <= #TCQ 1'b0;
                    rq_state           <= #TCQ PIO_TX_MRD_C1;
                 end
              end // PIO_TX_RST_STATE

              PIO_TX_COMPL_C1 : begin // Completion Without Payload - Alignment doesnt matter
                                      // Sent in a Single Beat When Interface Width is 128 bit
                if(req_compl_qq) begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b1;
                  s_axis_cc_tkeep   <= #TCQ 4'h7;
                  s_axis_cc_tdata   <= #TCQ {32'b0,        // Tied to 0 for 3DW completion descriptor
                                             1'b0,          // Force ECRC
                                             1'b0, req_attr,// 4- bits
                                             req_tc,        // 3- bits
                                             1'b0,          // Completer ID to control selection of Client
                                                            // Supplied Bus number
                                             8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                             8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                             req_tag,       // Select Client Tag or core's internal tag
                                             req_rid,       // Requester ID - 16 bits
                                             1'b0,          // Rsvd
                                             1'b0,          // Posioned completion
                                             3'b000,        // SuccessFull completion
                                             (req_mem ? (11'h1 + payload_len) : 11'b0),         // DWord Count 0 - IO Write completions
                                             2'b0,          // Rsvd
                                             1'b0,          // Locked Read Completion
                                             13'h0004,      // Byte Count
                                             6'b0,          // Rsvd
                                             req_at,        // Adress Type - 2 bits
                                             1'b0,          // Rsvd
                                             lower_addr};   // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state <= #TCQ PIO_TX_RST_STATE;
                    compl_done        <= #TCQ 1'b1;
                  end else begin
                    rq_state <= #TCQ PIO_TX_COMPL_C1;
                  end
                end
              end  //PIO_TX_COMPL

              PIO_TX_COMPL_WD_C1 : begin  // Completion With Payload
                                          // Possible Scenario's Payload can be 1 DW or 2 DW
                                          // Alignment can be either of Dword aligned or address aligned
             // Support n-DW 
             // Requires three clock cycle to get the first rd_data from the BRAM 
             if (req_compl_wd_qqqq) begin
                 if(AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE") begin //DWORD_Aligned mode
					   s_axis_cc_tvalid  <= #TCQ 1'b1; 
					   s_axis_cc_tdata   <= #TCQ {rd_data_s0[31:0],       // 1- DW read data in first transaction
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag,       // Select Client Tag or core's internal tag
                                                 req_rid,       // Requester ID - 16 bits
                                                 1'b0,          // Rsvd
                                                 1'b0,          // Posioned completion
                                                 3'b000,        // SuccessFull completion
                                                 (req_mem ? (payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                                 2'b0,          // Rsvd
                                                 (req_mem_lock? 1'b1 : 1'b0),  // Locked Read Completion
                                                 byte_count,     //13'h0004,      // Byte Count
                                                 6'b0,          // Rsvd
                                                 req_at,        // Adress Type - 2 bits
                                                 1'b0,          // Rsvd
                                                 lower_addr_dw};   // Starting address of the mem byte - 7 bits
                      s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
					  if (s_axis_cc_tready) begin
					    if (len_i < 2) begin
					      s_axis_cc_tkeep <= #TCQ 4'hF; 
						  s_axis_cc_tlast <= #TCQ 1'b1; 
                          rq_state           <= #TCQ PIO_TX_RST_STATE;		
                          rd_data_s1      <= #TCQ 96'h0; 	
                          len_i           <= #TCQ 11'b0; 	
                          compl_done      <= #TCQ 1'b1; 						  
				        end
						else begin 
						  s_axis_cc_tkeep <= #TCQ 4'hF; 
						  s_axis_cc_tlast <= #TCQ 1'b0;
						  rq_state           <= #TCQ PIO_TX_COMPL_WD_N_DW; 
						  rd_data_s1      <= #TCQ rd_data_s0[127:32];
                          len_i           <= #TCQ len_i - 11'h1; 	
                          compl_done      <= #TCQ 1'b0; 						  
						end
					  end
					 else begin 
					      rq_state           <= #TCQ PIO_TX_COMPL_WD_N_DW; 
				     end
		         end // DWORD Aligned mode end
                 else begin //Address aligned mode
                      s_axis_cc_tvalid  <= #TCQ 1'b1;
                      s_axis_cc_tlast   <= #TCQ 1'b0;
                      s_axis_cc_tkeep   <= #TCQ 8'h07;
                      s_axis_cc_tdata   <= #TCQ {32'b0,        // Tied to 0 for 1DW completion descriptor
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag,       // Select Client Tag or core's internal tag
                                                 req_rid,       // Requester ID - 16 bits
                                                 1'b0,          // Rsvd
                                                 1'b0,          // Posioned completion
                                                 3'b000,        // SuccessFull completion
                                                 (req_mem ? (payload_len) : 11'b1),         // DWord Count 0 - IO Write completions
                                                 2'b0,          // Rsvd
                                                 (req_mem_lock? 1'b1 : 1'b0),      // Locked Read Completion
                                                 byte_count,     //13'h0004,      // Byte Count
                                                 6'b0,          // Rsvd
                                                 req_at,        // Adress Type - 2 bits
                                                 1'b0,          // Rsvd
                                                 lower_addr_dw};   // Starting address of the mem byte - 7 bits
                      s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                      compl_done        <= #TCQ 1'b0;

                      if(s_axis_cc_tready) begin
                        rq_state <= #TCQ PIO_TX_COMPL_PYLD;
                      end else begin
                        rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                      end
                 end //Address aligned mode end
             end 
              end // PIO_TX_COMPL_WD

              PIO_TX_COMPL_PYLD : begin // Completion with 1DW Payload in Address Aligned mode

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ (tkeep_q[7:0]&8'hF);
                s_axis_cc_tdata[31:0]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b00) ? {rd_data} : ((AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE" ) ? rd_data : 32'b0);
                s_axis_cc_tdata[63:32]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b01) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[95:64]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b10) ? {rd_data} : {32'b0};
                s_axis_cc_tdata[127:96]   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b11) ? {rd_data} : {32'b0};
                s_axis_cc_tuser_wo_parity <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state           <= #TCQ PIO_TX_RST_STATE;
                  compl_done      <= #TCQ 1'b1;
                end else begin
                  rq_state           <= #TCQ PIO_TX_COMPL_PYLD;
                end
              end // PIO_TX_COMPL_PYLD

              PIO_TX_COMPL_WD_2DW : begin // Completion with 2DW Payload in DWord Aligned mode
                                          // Requires 2 states to get the 2DW Payload

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ (tkeep_q[7:0]&8'hF);
                s_axis_cc_tdata[127:0]  <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b00) ? {64'b0,{rd_data,rd_data_reg}}
                                               :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b01) ? {32'b0,{rd_data,rd_data_reg},32'b0}
                                               :(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b10) ? {      {rd_data,rd_data_reg},64'b0}
                                               :/*(AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_q[3:2]==2'b11)?*/{    {        rd_data_reg},96'b0};
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
		   if(lower_addr_q[3:2]==2'b11)
		   begin
                     rq_state           <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                     compl_done      <= #TCQ 1'b0;
                     s_axis_cc_tlast <= #TCQ 1'b0;
		   end
		   else
		   begin
                     rq_state           <= #TCQ PIO_TX_RST_STATE;
                     compl_done      <= #TCQ 1'b1;
                     s_axis_cc_tlast <= #TCQ 1'b1;
		   end
                end else begin
                  rq_state           <= #TCQ PIO_TX_COMPL_WD_2DW;
                end
              end //  PIO_TX_COMPL_WD_2DW

              PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2 : begin // Completions with 2-DW Payload and Addr aligned mode

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 8'h01;
                s_axis_cc_tdata   <= #TCQ {96'b0, rd_data};

                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                dword_count       <= #TCQ 1'b0;
                if(s_axis_cc_tready) begin
                   rq_state        <= #TCQ PIO_TX_RST_STATE;
                   compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                end // PIO_TX_COMPL_WD_2DW_ADDR_ALGN
              end

              PIO_TX_CPL_UR_C1 : begin // Completions with UR - Alignement mode matters here
                if(req_compl_ur_qq) begin
                     s_axis_cc_tvalid  <= #TCQ 1'b1;
                     s_axis_cc_tlast   <= #TCQ 1'b1;
                     s_axis_cc_tkeep   <= #TCQ 4'hF;
                     compl_done        <= #TCQ 1'b0;
                     s_axis_cc_tdata   <= #TCQ {8'b0,                // Rsvd
                                                req_des_tph_st_tag,  // TPH Steering tag - 8 bits
                                                5'b0,                // Rsvd
                                                req_des_tph_type,    // TPH type - 2 bits
                                                req_des_tph_present, // TPH present - 1 bit
                                                req_be,              // Request Byte enables - 8bits

                                                1'b0,                // Force ECRC
                                                1'b0, req_attr,      // 4- bits
                                                req_tc,              // 3- bits
                                                1'b0,                // Completer ID to control selection of Client
                                                                     // Supplied Bus number
                                                8'h00,               // Completer Bus number - selected if Compl ID    = 1
                                                8'h00,               // Compl Dev / Func no - sel if Compl ID = 1
                                                req_tag,             // Select Client Tag or core's internal tag
                                                req_rid,             // Requester ID - 16 bits
                                                1'b0,                // Rsvd
                                                1'b0,                // Posioned completion
                                                3'b001,              // Completion Status - UR
                                                11'h005,             // DWord Count -55
                                                2'b0,                // Rsvd
                                                (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                                13'h0014,            // Byte Count - 20 bytes of Payload
                                                6'b0,                // Rsvd
                                                req_at,              // Adress Type - 2 bits
                                                1'b0,                // Rsvd
                                                lower_addr};   // Starting address of the mem byte - 7 bits
                     s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                     if (s_axis_cc_tready) begin
                       rq_state           <= #TCQ PIO_TX_CPL_UR_C2;
                     end else begin
                       rq_state           <= #TCQ PIO_TX_CPL_UR_C1;
                     end
                end
              end // PIO_TX_CPL_UR_C1
              PIO_TX_CPL_UR_C2 : begin // Completion for UR - Clock 2
                 s_axis_cc_tvalid  <= #TCQ 1'b1;
                 s_axis_cc_tlast   <= #TCQ 1'b1;
                 s_axis_cc_tkeep   <= #TCQ 4'hF;
                 s_axis_cc_tdata   <= #TCQ {req_des_qword1,      // 64 bits - Descriptor of the request 2 DW
                                            req_des_qword0};     // 64 bits - Descriptor of the request 2 DW};

                 s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                 if (s_axis_cc_tready) begin
                   rq_state           <= #TCQ PIO_TX_RST_STATE;
                   compl_done      <= #TCQ 1'b1;
                 end else begin
                   rq_state           <= #TCQ PIO_TX_CPL_UR_C2;
                 end
              end // PIO_TX_CPL_UR_PYLD_C1

              PIO_TX_MRD_C1 : begin
                if (bufwr_rq_is_read_q) begin
                  // ------------------------------------------------
                  // H2D: one-beat 16-byte Memory Read request
                  // ------------------------------------------------
                  s_axis_rq_tvalid <= #TCQ 1'b1;
                  s_axis_rq_tlast  <= #TCQ 1'b1;
                  s_axis_rq_tkeep  <= #TCQ 4'hF;

                  s_axis_rq_tdata <= #TCQ {
                                             1'b0,                   // Force ECRC
                                             3'b000,                 // Attributes
                                             3'b000,                 // Traffic Class
                                             1'b0,                   // RID Enable
                                             16'b0,                  // Requester ID supplied by core
                                             8'h00,                  // Tag: one outstanding request
                                             8'h00,                  // Requester Bus number
                                             8'h00,                  // Requester Device/Function
                                             1'b0,                   // Poisoned request
                                             4'b0000,                // Request Type: Memory Read
                                             11'h004,                // Four DWORDs = 16 bytes
                                             bufwr_rq_addr_q[63:2],  // Host physical source
                                             2'b00                   // Address Type: untranslated
                                           };

                  s_axis_rq_tuser <= #TCQ {
                                             34'b0,
                                             4'b0000,                // Sequence number
                                             8'h00,                  // TPH steering tag
                                             1'b0,                   // TPH indirect tag enable
                                             2'b00,                  // TPH type
                                             1'b0,                   // TPH present
                                             1'b0,                   // Discontinue
                                             3'b000,                 // Byte lane number
                                             4'hF,                   // Last DWORD byte enable
                                             4'hF                    // First DWORD byte enable
                                           };

                  if (s_axis_rq_tready) begin
                    rq_state          <= #TCQ PIO_TX_RST_STATE;
                    trn_sent          <= #TCQ 1'b1;
                    bufwr_packet_done <= #TCQ 1'b1;
                  end else begin
                    rq_state <= #TCQ PIO_TX_MRD_C1;
                  end

                end else begin
                  // ------------------------------------------------
                  // D2H: first beat of 16-byte Memory Write
                  // descriptor
                  // ------------------------------------------------
                  s_axis_rq_tvalid <= #TCQ 1'b1;
                  s_axis_rq_tlast  <= #TCQ 1'b0;
                  s_axis_rq_tkeep  <= #TCQ 4'hF;

                  s_axis_rq_tdata <= #TCQ {
                                             1'b0,                   // Force ECRC
                                             3'b000,                 // Attributes
                                             3'b000,                 // Traffic Class
                                             1'b0,                   // RID Enable
                                             16'b0,                  // Requester ID supplied by core
                                             8'h00,                  // Tag unused for posted write
                                             8'h00,                  // Requester Bus number
                                             8'h00,                  // Requester Device/Function
                                             1'b0,                   // Poisoned request
                                             4'b0001,                // Request Type: Memory Write
                                             11'h004,                // Four DWORDs = 16 bytes
                                             bufrd_rq_addr_q[63:2],  // Host physical destination
                                             2'b00                   // Address Type: untranslated
                                           };

                  s_axis_rq_tuser <= #TCQ {
                                             34'b0,
                                             4'b0000,                // Sequence number
                                             8'h00,                  // TPH steering tag
                                             1'b0,                   // TPH indirect tag enable
                                             2'b00,                  // TPH type
                                             1'b0,                   // TPH present
                                             1'b0,                   // Discontinue
                                             3'b000,                 // Byte lane number
                                             4'hF,                   // Last DWORD byte enable
                                             4'hF                    // First DWORD byte enable
                                           };

                  if (s_axis_rq_tready) begin
                    rq_state <= #TCQ PIO_TX_MRD_C2;
                  end else begin
                    rq_state <= #TCQ PIO_TX_MRD_C1;
                  end
                end
              end // PIO_TX_MRD_C1

              PIO_TX_MRD_C2 : begin // Buffered 16-byte Memory Write payload
                s_axis_rq_tvalid <= #TCQ 1'b1;
                s_axis_rq_tlast  <= #TCQ 1'b1;
                s_axis_rq_tkeep  <= #TCQ 4'hF;
                s_axis_rq_tdata  <= #TCQ bufrd_rq_data_q;
                s_axis_rq_tuser  <= #TCQ {AXI4_RQ_TUSER_WIDTH{1'b0}};

                if (s_axis_rq_tready) begin
                  rq_state          <= #TCQ PIO_TX_RST_STATE;
                  trn_sent       <= #TCQ 1'b1;
                  bufrd_packet_done  <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_MRD_C2;
                end
              end // PIO_TX_MRD_C2
  
			  PIO_TX_COMPL_WD_N_DW : begin
               if ((len_i-1)/4 == 0) begin 
                   s_axis_cc_tvalid  <= #TCQ 1'b1;
                   s_axis_cc_tlast   <= #TCQ 1'b1;
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   case (len_i) 
                          1      : begin s_axis_cc_tdata <= #TCQ {96'b0, rd_data_s1[31:0]};
                                         s_axis_cc_tkeep <= #TCQ 4'h1; 
                                   end
                          2      : begin s_axis_cc_tdata <= #TCQ {64'b0, rd_data_s1[63:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 4'h3; 
                                   end
                          3      : begin s_axis_cc_tdata <= #TCQ {32'b0, rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 4'h7; 
                                   end
                          4      : begin s_axis_cc_tdata <= #TCQ {rd_data_s0[31:0],rd_data_s1[95:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 4'hF; 
                                   end
                   endcase
                   len_i <= #TCQ 11'b0;
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
					   rd_data_s1   <= #TCQ rd_data_s1; 
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end               
               end // len_i <= 4
               else begin 
                   s_axis_cc_tvalid            <= #TCQ 1'b1; 
                   s_axis_cc_tlast             <= #TCQ 1'b0; 
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   s_axis_cc_tkeep             <= #TCQ 4'hF; 
                   s_axis_cc_tdata             <= #TCQ {rd_data_s0[31:0], rd_data_s1[95:0]}; 
                   rd_data_s1                  <= #TCQ rd_data_s0[127:32]; 
                   len_i                       <= #TCQ len_i - 11'h4; 
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW;
                       compl_done   <= #TCQ 1'b0;
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end     
               end    
           end //PIO_TX_COMPL_WD_N_DW
            endcase
          end // reset_else_block
      end // Always Block Ends
    end // If AXISTEN_IF_WIDTH = 128

    else
    begin // 64 Bit Interface
    always @ ( posedge user_clk )
    begin
      if(!reset_n ) begin
        rq_state                   <= #TCQ PIO_TX_RST_STATE;
        rd_data_reg             <= #TCQ 32'b0;
        s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_cc_tlast         <= #TCQ 1'b0;
        s_axis_cc_tvalid        <= #TCQ 1'b0;
        s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
        s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
        s_axis_rq_tlast         <= #TCQ 1'b0;
        s_axis_rq_tvalid        <= #TCQ 1'b0;
        s_axis_cc_tuser_wo_parity <= #TCQ {AXI4_CC_TUSER_WIDTH{1'b0}};
        s_axis_rq_tuser         <= #TCQ {AXI4_RQ_TUSER_WIDTH{1'b0}};
        cfg_msg_transmit        <= #TCQ 1'b0;
        cfg_msg_transmit_type   <= #TCQ 3'b0;
        cfg_msg_transmit_data   <= #TCQ 32'b0;
        compl_done              <= #TCQ 1'b0;
        dword_count             <= #TCQ 1'b0;
        trn_sent                <= #TCQ 1'b0;
	len_i                   <= #TCQ 11'b0; 
	rd_data_s1              <= #TCQ 96'b0;

      end else begin // reset_else_block
            case (rq_state)
              PIO_TX_RST_STATE : begin  // Reset_State
                rq_state                   <= #TCQ PIO_TX_RST_STATE;
                s_axis_cc_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_cc_tkeep         <= #TCQ {KEEP_WIDTH{1'b1}};
                s_axis_cc_tlast         <= #TCQ 1'b0;
                s_axis_cc_tvalid        <= #TCQ 1'b0;
                s_axis_cc_tuser_wo_parity <= #TCQ 81'b0;
                s_axis_rq_tdata         <= #TCQ {C_DATA_WIDTH{1'b0}};
                s_axis_rq_tkeep         <= #TCQ {KEEP_WIDTH{1'b0}};
                s_axis_rq_tlast         <= #TCQ 1'b0;
                s_axis_rq_tvalid        <= #TCQ 1'b0;
                s_axis_rq_tuser         <= #TCQ 60'b0;
                cfg_msg_transmit        <= #TCQ 1'b0;
                cfg_msg_transmit_type   <= #TCQ 3'b0;
                cfg_msg_transmit_data   <= #TCQ 32'b0;
                compl_done              <= #TCQ 1'b0;
                trn_sent                <= #TCQ 1'b0;
                dword_count             <= #TCQ 1'b0;
		len_i                   <= #TCQ payload_len; 

                if(req_compl) begin
                   rq_state <= #TCQ PIO_TX_COMPL_C1;
                end else if (req_compl_wd) begin
                   rq_state <= #TCQ PIO_TX_COMPL_WD_C1;
                end else if (req_compl_ur) begin
                   rq_state <= #TCQ PIO_TX_CPL_UR_C1;
                end else if (gen_transaction) begin
                   rq_state <= #TCQ PIO_TX_MRD_C1;
                end
              end // PIO_TX_RST_STATE

              PIO_TX_COMPL_C1 : begin // Completion Without Payload - Alignment doesnt matter
                                   // Sent in a Single Beat When Interface Width is 128 bit
                if(req_compl_qq)
                begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b0;
                  s_axis_cc_tkeep   <= #TCQ 2'h3;
                  compl_done        <= #TCQ 1'b0;
                  s_axis_cc_tdata   <= #TCQ {req_rid,       // Requester ID - 16 bits
                                             1'b0,          // Rsvd
                                             1'b0,          // Posioned completion
                                             3'b000,        // SuccessFull completion
                                             (req_mem ? (11'h1 + payload_len) : 11'b0),         // DWord Count 0 - IO Write completions
                                             2'b0,          // Rsvd
                                             1'b0,          // Locked Read Completion
                                             13'h0004,      // Byte Count
                                             6'b0,          // Rsvd
                                             req_at,        // Adress Type - 2 bits
                                             1'b0,          // Rsvd
                                             lower_addr};   // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state           <= #TCQ PIO_TX_COMPL_C2;
                  end else begin
                    rq_state           <= #TCQ PIO_TX_COMPL_C1;
                  end
                end
              end  //PIO_TX_COMPL

              PIO_TX_COMPL_C2 : begin // Completion Without Payload - Alignment doesnt matter
                                      // Sent in a Two Beats When Interface Width is 64 bit
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b1;
                  s_axis_cc_tkeep   <= #TCQ 2'h1;
                  s_axis_cc_tdata   <= #TCQ {32'b0,         // Tied to 0 for 3DW completion descriptor
                                             1'b0,          // Force ECRC
                                             1'b0, req_attr,// 4- bits
                                             req_tc,        // 3- bits
                                             1'b0,          // Completer ID to control selection of Client
                                                            // Supplied Bus number
                                             8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                             8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                             req_tag};      // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state           <= #TCQ PIO_TX_RST_STATE;
                    compl_done      <= #TCQ 1'b1;
                  end else begin
                    rq_state           <= #TCQ PIO_TX_COMPL_C2;
                  end
              end  //PIO_TX_COMPL

              PIO_TX_COMPL_WD_C1 : begin  // Completion With Payload
                                          // Possible Scenario's Payload can be 1 DW or 2 DW
                                          // Alignment can be either of Dword aligned or address aligned
                if(req_compl_wd_qqq)
                begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b0;
                  s_axis_cc_tkeep   <= #TCQ 2'h3;
                  compl_done        <= #TCQ 1'b0;
                  if(AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE") begin // DWORD_aligned_Mode
                    s_axis_cc_tdata   <= #TCQ {req_rid,                                   // Requester ID - 16 bits
                                             1'b0,                                      // Rsvd
                                             1'b0,                                      // Posioned completion
                                             3'b000,                                    // SuccessFull completion
                                             (req_mem ? payload_len: 11'b1),            // DWord Count 0 - IO Write completions
                                             2'b0,                                      // Rsvd
                                             (req_mem_lock? 1'b1 : 1'b0),               // Locked Read Completion
                                             byte_count,                                //13'h0004,// Byte Count
                                             6'b0,                                      // Rsvd
                                             req_at,                                    // Adress Type - 2 bits
                                             1'b0,                                      // Rsvd
                                             lower_addr_dw};                               // Starting address of the mem byte - 7 bits
				  end
                  else begin //Address Align Mode 
                    s_axis_cc_tdata   <= #TCQ {req_rid,                                   // Requester ID - 16 bits
                                             1'b0,                                      // Rsvd
                                             1'b0,                                      // Posioned completion
                                             3'b000,                                    // SuccessFull completion
                                             (req_mem ? (1'b1 + payload_len): 11'b1),            // DWord Count 0 - IO Write completions
                                             2'b0,                                      // Rsvd
                                             (req_mem_lock? 1'b1 : 1'b0),               // Locked Read Completion
                                             byte_count,                                //13'h0004,// Byte Count
                                             6'b0,                                      // Rsvd
                                             req_at,                                    // Adress Type - 2 bits
                                             1'b0,                                      // Rsvd
                                             lower_addr_dw};                               // Starting address of the mem byte - 7 bits
                  end			 
									
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state      <= #TCQ PIO_TX_COMPL_WD_C2;
                    dword_count <= #TCQ (payload_len != 0 ) ? 1'b1 : 1'b0;    // To increment the Read Address
                  end else begin
                    rq_state      <= #TCQ PIO_TX_COMPL_WD_C1;
                  end
                end
              end // PIO_TX_COMPL_WD

              PIO_TX_COMPL_WD_C2 : begin  // Completion With Payload
                                          // Possible Scenario's Payload can be 1 DW or 2 DW
                                          // Alignment can be either of Dword aligned or address aligned

                  if(AXISTEN_IF_CC_ALIGNMENT_MODE == "FALSE") begin // DWORD_aligned_Mode
                    if(s_axis_cc_tready) begin

                      s_axis_cc_tvalid  <= #TCQ 1'b1;
                      s_axis_cc_tkeep   <= #TCQ 2'h3;
                      s_axis_cc_tdata   <= #TCQ {rd_data_s0[31:0],       // 32- bit read data
                                                 1'b0,          // Force ECRC
                                                 1'b0, req_attr,// 4- bits
                                                 req_tc,        // 3- bits
                                                 1'b0,          // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                                 8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                                 8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                                 req_tag};      // Starting address of the mem byte - 7 bits
                      s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                       if(len_i < 2) // 1DW_packet - Requires just one cycle to get the data rd_data from the BRAM.
                        begin
                          rq_state             <= #TCQ PIO_TX_RST_STATE;
                          s_axis_cc_tlast   <= #TCQ 1'b1;
                          compl_done        <= #TCQ 1'b1;
						  len_i             <= #TCQ 11'b0;
                        end else begin
                          s_axis_cc_tlast   <= #TCQ 1'b0;
                          rq_state             <= #TCQ PIO_TX_COMPL_WD_N_DW;
						  len_i             <= #TCQ payload_len - 11'b1;
						  rd_data_s1        <= #TCQ rd_data_s0[63:32];
                        end
                      end else begin
                        rq_state <= #TCQ PIO_TX_COMPL_WD_C2;
                      end
                end        //DWORD_aligned_Mode
                else begin // Addr_aligned_mode
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b0;
                  s_axis_cc_tkeep   <= #TCQ 2'h3;
                  s_axis_cc_tdata   <= #TCQ {1'b0,          // Force ECRC
                                             1'b0, req_attr,// 4- bits
                                             req_tc,        // 3- bits
                                             1'b0,          // Completer ID to control selection of Client
                                                            // Supplied Bus number
                                             8'h00,         // Completer Bus number - selected if Compl ID    = 1
                                             8'h00,         // Compl Dev / Func no - sel if Compl ID = 1
                                             req_tag};      // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                  compl_done        <= #TCQ 1'b0;

                  if(s_axis_cc_tready) begin
                    if(payload_len == 0) begin // 1DW_packet - Requires just one cycle to get the data rd_data from the BRAM.
                      rq_state         <= #TCQ PIO_TX_COMPL_PYLD;
                    end else begin
                      rq_state         <= #TCQ PIO_TX_COMPL_WD_2DW;
                      dword_count   <= #TCQ 1'b1;    // To increment the Read Address
                      rd_data_reg   <= #TCQ rd_data; // store the current read data
                    end
                  end else begin
                      rq_state         <= #TCQ PIO_TX_COMPL_WD_C2;
                  end
              end    // Addr_aligned_mode
            end // PIO_TX_COMPL_WD

              PIO_TX_COMPL_PYLD : begin // Completion with 1DW Payload in Address Aligned mode

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ tkeep_qq[1:0]&2'h3;
                s_axis_cc_tdata   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_qq[2]) ? {rd_data,32'b0} : {32'b0, rd_data};
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state           <= #TCQ PIO_TX_RST_STATE;
                  compl_done      <= #TCQ 1'b1;
                end else begin
                  rq_state           <= #TCQ PIO_TX_COMPL_PYLD;
                end
              end // PIO_TX_COMPL_PYLD

              PIO_TX_COMPL_WD_2DW : begin // Completion with 2DW Payload in DWord Aligned mode
                                          // Requires 2 states to get the 2DW Payload

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 2'h3;
                s_axis_cc_tdata   <= #TCQ (AXISTEN_IF_CC_ALIGNMENT_MODE == "TRUE" && lower_addr_qq[2]) ? {rd_data_reg,32'b0} : {rd_data,rd_data_reg};
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
		   if(lower_addr_qq[2])
		   begin
                     rq_state           <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                     compl_done      <= #TCQ 1'b0;
                     s_axis_cc_tlast <= #TCQ 1'b0;
		   end
		   else
		   begin
                     rq_state           <= #TCQ PIO_TX_RST_STATE;
                     compl_done      <= #TCQ 1'b1;
                     s_axis_cc_tlast <= #TCQ 1'b1;
		   end
                end else begin
                  rq_state           <= #TCQ PIO_TX_COMPL_WD_2DW;
                end

              end //  PIO_TX_COMPL_WD_2DW
              PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2 : begin // Completions with 2-DW Payload and Addr aligned mode

                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 8'h01;
                s_axis_cc_tdata   <= #TCQ {32'b0, rd_data};

                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                dword_count       <= #TCQ 1'b0;
                if(s_axis_cc_tready) begin
                   rq_state        <= #TCQ PIO_TX_RST_STATE;
                   compl_done   <= #TCQ 1'b1;
                end else begin
                  rq_state <= #TCQ PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2;
                end // PIO_TX_COMPL_WD_2DW_ADDR_ALGN
              end

              PIO_TX_CPL_UR_C1 : begin // Completions with UR - Beat 1
                if(req_compl_ur_qq) begin
                  s_axis_cc_tvalid  <= #TCQ 1'b1;
                  s_axis_cc_tlast   <= #TCQ 1'b1;
                  s_axis_cc_tkeep   <= #TCQ 2'h3;
                  compl_done        <= #TCQ 1'b0;
                  s_axis_cc_tdata   <= #TCQ {req_rid,             // Requester ID - 16 bits
                                             1'b0,                // Rsvd
                                             1'b0,                // Posioned completion
                                             3'b001,              // Completion Status - UR
                                             11'h005,             // DWord Count -55
                                             2'b0,                // Rsvd
                                             (req_mem_lock? 1'b1 : 1'b0),   // Locked Read Completion
                                             13'h0014,            // Byte Count - 20 bytes of Payload
                                             6'b0,                // Rsvd
                                             req_at,              // Adress Type - 2 bits
                                             1'b0,                // Rsvd
                                             lower_addr};   // Starting address of the mem byte - 7 bits
                  s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                  if(s_axis_cc_tready) begin
                    rq_state           <= #TCQ PIO_TX_CPL_UR_C2;
                  end else begin
                    rq_state           <= #TCQ PIO_TX_CPL_UR_C1;
                  end
                end
              end // PIO_TX_CPL_UR_C1

              PIO_TX_CPL_UR_C2 : begin // Completions with UR - Beat 2
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 2'h3;
                compl_done        <= #TCQ 1'b0;
                s_axis_cc_tdata   <= #TCQ {8'b0,                // Rsvd
                                           req_des_tph_st_tag,  // TPH Steering tag - 8 bits
                                           5'b0,                // Rsvd
                                           req_des_tph_type,    // TPH type - 2 bits
                                           req_des_tph_present, // TPH present - 1 bit
                                           req_be,              // Request Byte enables - 8bits

                                           1'b0,                // Force ECRC
                                           1'b0, req_attr,      // 4- bits
                                           req_tc,              // 3- bits
                                           1'b0,                // Completer ID to control selection of Client
                                                                // Supplied Bus number
                                           8'h00,               // Completer Bus number - selected if Compl ID    = 1
                                           8'h00,               // Compl Dev / Func no - sel if Compl ID = 1
                                           req_tag};            // Select Client Tag or core's internal tag
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state           <= #TCQ PIO_TX_CPL_UR_C3;
                end else begin
                  rq_state           <= #TCQ PIO_TX_CPL_UR_C2;
                end
              end // PIO_TX_CPL_UR_C2

              PIO_TX_CPL_UR_C3 : begin // Completions with UR - Beat 3
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 2'h3;
                compl_done        <= #TCQ 1'b0;
                s_axis_cc_tdata   <= #TCQ req_des_qword0;      // 64 bits - Descriptor of the request 2 DW
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state           <= #TCQ PIO_TX_CPL_UR_C4;
                end else begin
                  rq_state           <= #TCQ PIO_TX_CPL_UR_C3;
                end
              end // PIO_TX_CPL_UR_C3

              PIO_TX_CPL_UR_C4 : begin // Completions with UR - Beat 4
                s_axis_cc_tvalid  <= #TCQ 1'b1;
                s_axis_cc_tlast   <= #TCQ 1'b1;
                s_axis_cc_tkeep   <= #TCQ 2'h3;
                s_axis_cc_tdata   <= #TCQ req_des_qword1;      // 64 bits - Descriptor of the request 2 DW
                s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};

                if(s_axis_cc_tready) begin
                  rq_state           <= #TCQ PIO_TX_RST_STATE;
                  compl_done      <= #TCQ 1'b1;
                end else begin
                  rq_state           <= #TCQ PIO_TX_CPL_UR_C4;
                end
              end // PIO_TX_CPL_UR_C4

              PIO_TX_MRD_C1 : begin // Memory Read Transaction - Alignment Doesnt Matter

                s_axis_rq_tvalid  <= #TCQ 1'b1;
                s_axis_rq_tlast   <= #TCQ 1'b0;
                s_axis_rq_tkeep   <= #TCQ 2'h3;  // 2DW Descriptor For Memory Transaction Alone
                trn_sent          <= #TCQ 1'b0;
                s_axis_rq_tdata   <= #TCQ {62'h2AAA_BBBB_CCCC_DDDD, // Memory Read Address [62 bits]
                                           2'b00};             //AT -> 00- Untranslated Address

                s_axis_rq_tuser          <= #TCQ {(AXISTEN_IF_RQ_PARITY_CHECK ? s_axis_rq_tparity : 32'b0), // Parity
                                                  4'b1010,      // Seq Number
                                                  8'h00,        // TPH Steering Tag
                                                  1'b0,         // TPH indirect Tag Enable
                                                  2'b0,         // TPH Type
                                                  1'b0,         // TPH Present
                                                  1'b0,         // Discontinue
                                                  3'b000,       // Byte Lane number in case of Address Aligned mode
                                                  4'h0,    // Last BE of the Read Data
                                                  4'hF}; // First BE of the Read Data

                if(s_axis_rq_tready) begin
                  rq_state <= #TCQ PIO_TX_MRD_C2;
                end else begin
                  rq_state <= #TCQ PIO_TX_MRD_C1;
                end
              end // PIO_TX_MRD

              PIO_TX_MRD_C2 : begin // Memory Read Transaction - Alignment Doesnt Matter
                s_axis_rq_tvalid  <= #TCQ 1'b1;
                s_axis_rq_tlast   <= #TCQ 1'b1;
                s_axis_rq_tkeep   <= #TCQ 2'h3;               // 2DW Descriptor For Memory Transaction Alone
                s_axis_rq_tdata   <= #TCQ {1'b0,              // Force ECRC
                                           3'b000,            // Attributes
                                           3'b000,            // Traffic Class
                                           1'b0,              // RID Enable to use the Client supplied Bus/Device/Func No
                                           16'b0,             // Completer -ID, set only for Completers or ID based routing
                                           (AXISTEN_IF_ENABLE_CLIENT_TAG ?
                                           8'h00 : req_tag),  // Select Client Tag or core's internal tag
                                           8'h00,             // Req Bus No- used only when RID enable = 1
                                           8'h00,             // Req Dev/Func no - used only when RID enable = 1
                                           1'b0,              // Poisoned Req
                                           4'b0000,           // Req Type for MRd Req
                                           11'h001};          // DWORD Count

                s_axis_rq_tuser   <= #TCQ 60'b0;

                if(s_axis_rq_tready) begin
                  rq_state           <= #TCQ PIO_TX_RST_STATE;
                  trn_sent        <= #TCQ 1'b1;
                end else begin
                  rq_state           <= #TCQ PIO_TX_MRD_C2;
                end
              end // PIO_TX_MRD
			  
			  PIO_TX_COMPL_WD_N_DW : begin
			  if (s_axis_cc_tready) begin 
               if ((len_i-1)/2 == 0) begin 
                   s_axis_cc_tvalid  <= #TCQ 1'b1;
                   s_axis_cc_tlast   <= #TCQ 1'b1;
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   case (len_i) 
                          1      : begin s_axis_cc_tdata <= #TCQ {32'b0, rd_data_s1[31:0]};
                                         s_axis_cc_tkeep <= #TCQ 2'h1; 
                                   end
                          2      : begin s_axis_cc_tdata <= #TCQ {rd_data_s0[31:0],rd_data_s1[31:0]}; 
                                         s_axis_cc_tkeep <= #TCQ 2'h3; 
                                   end
                   endcase
                   len_i <= #TCQ 11'b0;
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_RST_STATE;
                       compl_done   <= #TCQ 1'b1;
					   rd_data_s1   <= #TCQ rd_data_s1; 
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end               
               end // len_i <= 2
               else begin 
                   s_axis_cc_tvalid            <= #TCQ 1'b1; 
                   s_axis_cc_tlast             <= #TCQ 1'b0; 
                   s_axis_cc_tuser_wo_parity   <= #TCQ {32'b0,1'b0};
                   s_axis_cc_tkeep             <= #TCQ 2'h3; 
                   s_axis_cc_tdata             <= #TCQ {rd_data_s0[31:0], rd_data_s1[31:0]}; 
                   rd_data_s1                  <= #TCQ rd_data_s0[63:32]; 
                   len_i                       <= #TCQ len_i - 11'h2; 
                   if(s_axis_cc_tready) begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW;
                       compl_done   <= #TCQ 1'b0;
                   end else begin
                       rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
                   end     
               end    
	       end else begin 
                     rq_state        <= #TCQ PIO_TX_COMPL_WD_N_DW; 
	       end
           end //PIO_TX_COMPL_WD_N_DW
            endcase
          end // reset_else_block
      end // If AXISTEN_IF_WIDTH = 64
    end
  endgenerate

  // synthesis translate_off
  reg  [8*20:1] state_ascii;
  always @(rq_state)
  begin
    case (rq_state)
      PIO_TX_RST_STATE                    : state_ascii <= #TCQ "TX_RST_STATE";
      PIO_TX_COMPL_C1                     : state_ascii <= #TCQ "TX_COMPL_C1";
      PIO_TX_COMPL_C2                     : state_ascii <= #TCQ "TX_COMPL_C2";
      PIO_TX_COMPL_WD_C1                  : state_ascii <= #TCQ "TX_COMPL_WD_C1";
      PIO_TX_COMPL_WD_C2                  : state_ascii <= #TCQ "TX_COMPL_WD_C2";
      PIO_TX_COMPL_PYLD                   : state_ascii <= #TCQ "TX_COMPL_PYLD";
      PIO_TX_CPL_UR_C1                    : state_ascii <= #TCQ "TX_CPL_UR_C1";
      PIO_TX_CPL_UR_C2                    : state_ascii <= #TCQ "TX_CPL_UR_C2";
      PIO_TX_CPL_UR_C3                    : state_ascii <= #TCQ "TX_CPL_UR_C3";
      PIO_TX_CPL_UR_C4                    : state_ascii <= #TCQ "TX_CPL_UR_C4";
      PIO_TX_MRD_C1                       : state_ascii <= #TCQ "TX_MRD_C1";
      PIO_TX_MRD_C2                       : state_ascii <= #TCQ "TX_MRD_C2";
      PIO_TX_COMPL_WD_2DW                 : state_ascii <= #TCQ "TX_COMPL_WD_2DW";
      PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C1    : state_ascii <= #TCQ "TX_COMPL_WD_2DW_ADDR_ALGN_C1";
      PIO_TX_COMPL_WD_2DW_ADDR_ALGN_C2    : state_ascii <= #TCQ "TX_COMPL_WD_2DW_ADDR_ALGN_C2";
      default                             : state_ascii <= #TCQ "PIO STATE ERR";
    endcase
  end
  // synthesis translate_on

endmodule // pio_tx_engine
