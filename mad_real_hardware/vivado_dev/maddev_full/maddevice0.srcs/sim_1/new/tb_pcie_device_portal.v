`timescale 1ns / 1ps

module tb_pcie_device_portal(
);

reg user_clk;
reg reset_n;

reg [31:0] rc_base;

// 250 MHz clock = 4 ns period

reg  [127:0] m_axis_cq_tdata  = 128'd0;
reg          m_axis_cq_tlast  = 1'b0;
reg          m_axis_cq_tvalid = 1'b0;
reg  [87:0]  m_axis_cq_tuser  = 88'd0;
reg  [3:0]   m_axis_cq_tkeep  = 4'd0;
reg  [5:0]   pcie_cq_np_req_count = 6'd0;

reg  [127:0] m_axis_rc_tdata  = 128'd0;
reg          m_axis_rc_tlast  = 1'b0;
reg          m_axis_rc_tvalid = 1'b0;
reg  [3:0]   m_axis_rc_tkeep  = 4'd0;
reg  [74:0]  m_axis_rc_tuser  = 75'd0;

reg          s_axis_cc_tready = 1'b1;

reg          cfg_msg_received       = 1'b0;
reg  [4:0]   cfg_msg_received_type  = 5'd0;
reg  [7:0]   cfg_msg_data           = 8'd0;

reg          s_axis_rq_tready = 1'b1;
reg          wr_busy          = 1'b0;

integer init_i;
integer rc_i;


// -----------------------------------------------------------------------------
// Clock
// -----------------------------------------------------------------------------
initial begin
    user_clk = 1'b0;
    forever #2 user_clk = ~user_clk;
end


// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
initial begin
    reset_n = 1'b0;

    repeat (20)
        @(posedge user_clk);

    reset_n = 1'b1;
end


// -----------------------------------------------------------------------------
// SG-DMA H2D test
//
// Two descriptors:
//
// Descriptor 0 @ BAR0 0x58
//
//     HostAddr    = 00000001_00001000
//     DevDataOfst = 00000000
//     DmaCntl     = 80000000      H2D
//     DXBC        = 32 bytes
//     CDPP        = 00000000_00000078
//
// Descriptor 1 @ BAR0 0x78
//
//     HostAddr    = 00000001_00002000
//     DevDataOfst = 00000020
//     DmaCntl     = 80000000      H2D
//     DXBC        = 32 bytes
//     CDPP        = 00000000_00000001   END
//
// Host completion data:
//
// Descriptor 0:
//     C0000000 ... C0000007
//
// Descriptor 1:
//     D0000000 ... D0000007
//
// Expected final device memory:
//
// BAR4 DWORD 0..7:
//     C0000000 ... C0000007
//
// BAR4 DWORD 8..15:
//     D0000000 ... D0000007
//
// Expected accounting:
//     SG_TOTAL = 64
//     DTBC     = 64
//
// -----------------------------------------------------------------------------
initial begin

    wait (reset_n == 1'b1);
    @(posedge user_clk);


    // -------------------------------------------------------------------------
    // Disable ordinary cache-mode I/O.
    // -------------------------------------------------------------------------
    force dut.cache_xfer_enabled = 1'b0;


    // -------------------------------------------------------------------------
    // Clear destination BAR4 area.
    // -------------------------------------------------------------------------
    for (init_i = 0; init_i < 32; init_i = init_i + 1)
        dut.bar4_buf[init_i] = 32'h0000_0000;


    // -------------------------------------------------------------------------
    // Descriptor 0
    // -------------------------------------------------------------------------
    dut.regs_i.SgHostAddr[0]     = 64'h0000_0001_0000_1000;
    dut.regs_i.SgDevDataOfst[0]  = 32'h0000_0000;
    dut.regs_i.SgDmaCntl[0]      = 32'h8000_0000;  // bit 31 = 1 => H2D
    dut.regs_i.SgDXBC[0]         = 32'd32;
    dut.regs_i.SgReg20[0]        = 32'h0000_0000;
    dut.regs_i.SgCDPP[0]         = 64'h0000_0000_0000_0078;


    // -------------------------------------------------------------------------
    // Descriptor 1
    // -------------------------------------------------------------------------
    dut.regs_i.SgHostAddr[1]     = 64'h0000_0001_0000_2000;
    dut.regs_i.SgDevDataOfst[1]  = 32'h0000_0020;
    dut.regs_i.SgDmaCntl[1]      = 32'h8000_0000;  // H2D
    dut.regs_i.SgDXBC[1]         = 32'd32;
    dut.regs_i.SgReg20[1]        = 32'h0000_0000;
    dut.regs_i.SgCDPP[1]         = 64'h0000_0000_0000_0001;


    // -------------------------------------------------------------------------
    // BCDPP should reset to descriptor 0 at BAR0 offset 0x58.
    // -------------------------------------------------------------------------
    if (dut.bcdpp !== 64'h0000_0000_0000_0058) begin
        $display(
            "ERROR: unexpected BCDPP=%h expected=0000000000000058",
            dut.bcdpp
        );
        $finish;
    end


    // -------------------------------------------------------------------------
    // Enable chained DMA.
    // -------------------------------------------------------------------------
    force dut.chained_dma_enabled = 1'b1;

    @(posedge user_clk);


    // -------------------------------------------------------------------------
    // Issue DMA GO.
    // -------------------------------------------------------------------------
    force dut.dma_go_pulse = 1'b1;

    @(posedge user_clk);
    force dut.dma_go_pulse = 1'b0;


    // -------------------------------------------------------------------------
    // Wait until the complete two-descriptor chain finishes.
    // -------------------------------------------------------------------------
    wait (dut.sg_dma_complete == 1'b1);

    @(posedge user_clk);


    // -------------------------------------------------------------------------
    // Final result
    // -------------------------------------------------------------------------
    $display(
        "FINAL SG_STATE=%0d DESC=%0d SG_TOTAL=%0d DTBC=%0d DMA_COMPLETE=%b BAR4_C0=%h BAR4_C7=%h BAR4_D0=%h BAR4_D7=%h",
        dut.sg_state,
        dut.sg_desc_index,
        dut.sg_total_bytes,
        dut.regs_i.DTBC,
        dut.sg_dma_complete,
        dut.bar4_buf[0],
        dut.bar4_buf[7],
        dut.bar4_buf[8],
        dut.bar4_buf[15]
    );

    $finish;
end


// -----------------------------------------------------------------------------
// DUT
// -----------------------------------------------------------------------------
pcie_device_portal dut (
    .user_clk(user_clk),
    .reset_n(reset_n),

    .m_axis_cq_tdata(m_axis_cq_tdata),
    .m_axis_cq_tlast(m_axis_cq_tlast),
    .m_axis_cq_tvalid(m_axis_cq_tvalid),
    .m_axis_cq_tuser(m_axis_cq_tuser),
    .m_axis_cq_tkeep(m_axis_cq_tkeep),
    .pcie_cq_np_req_count(pcie_cq_np_req_count),

    .m_axis_rc_tdata(m_axis_rc_tdata),
    .m_axis_rc_tlast(m_axis_rc_tlast),
    .m_axis_rc_tvalid(m_axis_rc_tvalid),
    .m_axis_rc_tkeep(m_axis_rc_tkeep),
    .m_axis_rc_tuser(m_axis_rc_tuser),

    .s_axis_cc_tready(s_axis_cc_tready),

    .cfg_msg_received(cfg_msg_received),
    .cfg_msg_received_type(cfg_msg_received_type),
    .cfg_msg_data(cfg_msg_data),

    .s_axis_rq_tready(s_axis_rq_tready),

    .wr_busy(wr_busy)
);


// -----------------------------------------------------------------------------
// RC completion generator
//
// SG-DMA issues four 16-byte host Memory Read requests:
//
// Descriptor 0:
//     request 0 -> C0000000 ... C0000003
//     request 1 -> C0000004 ... C0000007
//
// Descriptor 1:
//     request 2 -> D0000000 ... D0000003
//     request 3 -> D0000004 ... D0000007
//
// Each Completion-with-Data is represented by:
//
// beat 0:
//     descriptor [95:0]
//     payload DWORD 0 [127:96]
//
// beat 1:
//     payload DWORDs 1..3
//
// -----------------------------------------------------------------------------
initial begin

    wait (reset_n == 1'b1);

    for (rc_i = 0; rc_i < 4; rc_i = rc_i + 1) begin

        // Select payload pattern by descriptor.
        if (rc_i < 2)
            rc_base = 32'hC000_0000 + (rc_i * 4);
        else
            rc_base = 32'hD000_0000 + ((rc_i - 2) * 4);


        // ---------------------------------------------------------
        // Wait until the H2D requester is waiting for host data.
        // BUFWR_SEQ_WAIT_RC = 3'd4.
        // ---------------------------------------------------------
        wait (dut.bufwr_seq_state == 3'd4);


        // ---------------------------------------------------------
        // First RC beat
        //
        // payload DWORD 0 appears in [127:96].
        // ---------------------------------------------------------
        @(negedge user_clk);

        m_axis_rc_tdata = {
            rc_base,
            96'h0
        };

        m_axis_rc_tkeep  = 4'hF;
        m_axis_rc_tvalid = 1'b1;
        m_axis_rc_tlast  = 1'b0;


        // ---------------------------------------------------------
        // Second RC beat
        //
        // payload DWORDs 1, 2, 3.
        // ---------------------------------------------------------
        @(negedge user_clk);

        m_axis_rc_tdata = {
            32'h0000_0000,
            rc_base + 32'd3,
            rc_base + 32'd2,
            rc_base + 32'd1
        };

        m_axis_rc_tkeep  = 4'h7;
        m_axis_rc_tvalid = 1'b1;
        m_axis_rc_tlast  = 1'b1;


        // ---------------------------------------------------------
        // Return RC interface to idle.
        // ---------------------------------------------------------
        @(negedge user_clk);

        m_axis_rc_tvalid = 1'b0;
        m_axis_rc_tlast  = 1'b0;
        m_axis_rc_tdata  = 128'd0;
        m_axis_rc_tkeep  = 4'd0;


        // Do not let the next iteration mistake this descriptor's
        // current WAIT_RC state for the next request.
        wait (dut.bufwr_seq_state != 3'd4);
    end
end


// -----------------------------------------------------------------------------
// Diagnostic display
// -----------------------------------------------------------------------------
always @(posedge user_clk) begin

    if (dut.dma_go_pulse ||
        dut.sg_state != 3'd0 ||
        dut.sg_h2d_start ||
        dut.sg_dma_complete ||
        dut.bufwr_sg_active ||
        dut.bufwr_rq_packet_done ||
        dut.bufwr_completion_received ||
        dut.bufwr_completion_stored) begin

        $display(
            "TIME=%0t SG_STATE=%0d DESC=%0d D2H_START=%b H2D_START=%b SG_ACTIVE=%b BUFWR_STATE=%0d REMAIN=%0d RQ_START=%b RQ_VALID=%b RQ_LAST=%b RQ_DONE=%b RC_VALID=%b RC_LAST=%b RC_RECV=%b STORED=%b HOST_ADDR=%h DEV_OFS=%h DXBC=%0d CDPP=%h H2D_DONE=%b SG_TOTAL=%0d DTBC=%0d BAR4_0=%h BAR4_7=%h BAR4_8=%h BAR4_15=%h",
            $time,
            dut.sg_state,
            dut.sg_desc_index,
            dut.sg_d2h_start,
            dut.sg_h2d_start,
            dut.bufwr_sg_active,
            dut.bufwr_seq_state,
            dut.bufwr_bytes_remaining,
            dut.bufwr_rq_start_q,
            dut.s_axis_rq_tvalid,
            dut.s_axis_rq_tlast,
            dut.bufwr_rq_packet_done,
            m_axis_rc_tvalid,
            m_axis_rc_tlast,
            dut.bufwr_completion_received,
            dut.bufwr_completion_stored,
            dut.bufwr_rq_host_addr,
            dut.sg_dev_data_ofst_q,
            dut.sg_dxbc_q,
            dut.sg_cdpp_q,
            dut.sg_h2d_complete,
            dut.sg_total_bytes,
            dut.regs_i.DTBC,
            dut.bar4_buf[0],
            dut.bar4_buf[7],
            dut.bar4_buf[8],
            dut.bar4_buf[15]
        );
    end
end

endmodule
