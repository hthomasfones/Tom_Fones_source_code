//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/17/2026 01:37:47 PM
// Design Name: 
// Module Name: mad_device_regs
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

// mad_device_regs.sv
// Simple AXI4-Lite slave BAR0 register file for MADREGS snapshot region.

module mad_device_regs #(
    parameter ADDR_WIDTH = 16,
    parameter integer MAD_CACHE_NUM_SECTORS = 1,
    parameter integer MAD_SGDMA_MAX_SECTORS = 16,
    parameter integer MAD_SGDMA_MAX_ENTRIES = MAD_SGDMA_MAX_SECTORS

)(
    input  wire                  aclk,
    input  wire                  aresetn,

    input  wire [ADDR_WIDTH-1:0] s_axi_awaddr,
    input  wire                  s_axi_awvalid,
    output reg                   s_axi_awready,

    input  wire [31:0]           s_axi_wdata,
    input  wire [3:0]            s_axi_wstrb,
    input  wire                  s_axi_wvalid,
    output reg                   s_axi_wready,

    output reg  [1:0]            s_axi_bresp,
    output reg                   s_axi_bvalid,
    input  wire                  s_axi_bready,

    input  wire [ADDR_WIDTH-1:0] s_axi_araddr,
    input  wire                  s_axi_arvalid,
    output reg                   s_axi_arready,

    output reg  [31:0]           s_axi_rdata,
    output reg  [1:0]            s_axi_rresp,
    output reg                   s_axi_rvalid,
    input  wire                  s_axi_rready,

    output reg                   bufrd_go_pulse,
    output reg                   dma_go_pulse,
    

    // Buffered-read sequencer events from pcie_device_portal.
    // bufrd_advance pulses once for every accepted 16-byte RQ packet.
    // bufrd_io_complete pulses once after the final accepted packet.
    // Device-to-host buffered read events.
    input wire                  bufrd_advance,
    input wire                  bufrd_io_complete,
    
    // Device-to-host DMA sequencer events.
    input wire                  dma_busy,
    input wire                  dma_advance,
    input wire                  dma_complete,
    input wire                  bufrd_cache_io_complete,
    input wire                  bufrd_cache_first_use,
    input wire                  bufrd_cache_first_use_complete,
    input wire                  bufrd_cache_normal_complete,
    
    // Host-to-device buffered write events.
    input wire                   bufwr_advance,
    input wire                   bufwr_io_complete,
    
    input wire                   bufwr_cache_io_complete,
    input wire                   bufwr_cache_first_use,
    input wire                   bufwr_cache_first_use_complete,
    input wire                   bufwr_cache_normal_complete,
    
    // SG-DMA chain accounting events.
    input wire                  sg_dma_begin,
    input wire                  sg_dma_xfer_advance,
    input wire                  sg_dma_complete,
    input wire                  sg_dma_h2d,
    
    // SG-DMA descriptor interface to pcie_device_portal.
    input  wire [7:0]  sg_desc_index,
    //
    output wire [63:0] bcdpp,
    output wire [63:0] sg_host_addr,
    output wire [31:0] sg_dev_data_ofst,
    output wire [31:0] sg_dma_cntl,
    output wire [31:0] sg_dxbc,
    output wire [31:0] sg_reg20,
    output wire [63:0] sg_cdpp,
    
    output wire [63:0] host_addr,
    output wire [31:0] byte_indx_rd,
    output wire [31:0] byte_indx_wr,
    output wire [31:0] cache_indx_rd,
    output wire [31:0] cache_indx_wr,
    output wire [31:0] bufrd_transfer_length,
    output wire        cache_xfer_enabled,  
    output wire        chained_dma_enabled,
 
    input  wire        bufwr_cache_index_advance,
    output wire        int_enable_bufrd_input,
    output wire        int_enable_bufrd_output,
    output wire        read_cache_empty,
    output wire        write_cache_empty
);
    // ------------------------------------------------------------
    // SG-DMA descriptor interface
    // ------------------------------------------------------------
    assign bcdpp            = BCDPP;
    assign sg_host_addr     = SgHostAddr[sg_desc_index];
    assign sg_dev_data_ofst = SgDevDataOfst[sg_desc_index];
    assign sg_dma_cntl      = SgDmaCntl[sg_desc_index];
    assign sg_dxbc          = SgDXBC[sg_desc_index];
    assign sg_reg20         = SgReg20[sg_desc_index];
    assign sg_cdpp          = SgCDPP[sg_desc_index];
    
    assign host_addr             = HostPA;
    assign byte_indx_rd          = ByteIndxRd;
    assign byte_indx_wr          = ByteIndxWr;
    assign cache_xfer_enabled =
           ((Control & MAD_CONTROL_CACHE_XFER_BIT) != 32'h00000000);
    assign read_cache_empty =
        ((Status & MAD_STATUS_READ_CACHE_EMPTY_BIT) != 32'h00000000);

    assign write_cache_empty =
        ((Status & MAD_STATUS_WRITE_CACHE_EMPTY_BIT) != 32'h00000000);    
        
    assign chained_dma_enabled =
        ((Control & MAD_CONTROL_CHAINED_DMA_BIT) != 32'h00000000);   
           
    assign cache_indx_rd = CacheIndxRd;
    assign cache_indx_wr = CacheIndxWr;
    
    // Control[15:12] encodes one less than the requested I/O count.
    // IOSIZE_BYTES selects 16-byte units; otherwise the units are 512-byte blocks.
    wire [3:0] control_io_count =
        (Control & MAD_CONTROL_IO_COUNT_MASK) >> MAD_CONTROL_IO_COUNT_SHIFT;

    assign bufrd_transfer_length =
        ({28'd0, control_io_count} + 32'd1) <<
        (((Control & MAD_CONTROL_IOSIZE_BYTES_BIT) != 32'h0) ? 4 : 9);

    assign int_enable_bufrd_input = ((IntEnable & MAD_INT_BUFRD_INPUT_BIT) != 32'h0);
    assign int_enable_bufrd_output = ((IntEnable & MAD_INT_BUFRD_OUTPUT_BIT) != 32'h0);

    localparam [1:0] RESP_OKAY   = 2'b00;
    localparam [1:0] RESP_SLVERR = 2'b10;

    localparam [31:0] MAD_CONTROL_IOSIZE_BYTES_BIT = 32'h00000001;
    localparam [31:0] MAD_CONTROL_BUFRD_GO_BIT     = 32'h00000008;
    localparam [31:0] MAD_CONTROL_DMA_GO_BIT       = 32'h00000080;
    localparam [31:0] MAD_CONTROL_CACHE_XFER_BIT   = 32'h00000010;
    localparam [31:0] MAD_CONTROL_IO_COUNT_MASK    = 32'h0000F000;
    localparam integer MAD_CONTROL_IO_COUNT_SHIFT  = 12;
    localparam [31:0] MAD_CONTROL_CHAINED_DMA_BIT  = 32'h00000040;
 
    // DMA control/status bits.
    localparam [31:0] MAD_DMA_CNTL_BUSY_BIT = 32'h0000_0001;
    localparam [31:0] MAD_DMA_CNTL_DONE_BIT = 32'h0000_0002;
 
    // The first DMA implementation transfers one 512-byte block.
    localparam [31:0] MAD_DMA_TRANSFER_BYTES = 32'd512;
    localparam [31:0] MAD_DMA_PACKET_BYTES   = 32'd16;

    localparam [31:0] MAD_INT_STATUS_ALERT_BIT  = 32'h00008000;
    localparam [31:0] MAD_INT_BUFRD_INPUT_BIT   = 32'h00000001;
    localparam [31:0] MAD_INT_BUFRD_OUTPUT_BIT  = 32'h00000010;
    localparam [31:0] MAD_INT_DMA_INPUT_BIT     = 32'h00000002;
    localparam [31:0] MAD_INT_DMA_OUTPUT_BIT    = 32'h00000020;

    localparam [31:0] MAD_STATUS_READ_COUNT_MASK   = 32'h00000F00;
    localparam integer MAD_STATUS_READ_COUNT_SHIFT = 8;
    localparam [31:0] MAD_STATUS_WRITE_COUNT_MASK   = 32'h0000F000;
    localparam integer MAD_STATUS_WRITE_COUNT_SHIFT = 12;
    localparam [31:0] MAD_STATUS_READ_CACHE_EMPTY_BIT  = 32'h00000040;
    localparam [31:0] MAD_STATUS_WRITE_CACHE_EMPTY_BIT = 32'h00000080;
    localparam [31:0] MAD_STATUS_OVER_UNDER_ERR_BIT = 32'h00000020;
    localparam [31:0] MAD_STATUS_CACHE_INIT_MASK = 
                      (MAD_STATUS_READ_CACHE_EMPTY_BIT | MAD_STATUS_WRITE_CACHE_EMPTY_BIT);
    
    localparam        MAD_DIAG_PAGE_BIT = 12;
    localparam [31:0] DIAG_MAGIC   = 32'h4D414444;
    localparam [31:0] DIAG_VERSION = 32'h0001_0000;

    reg [31:0] MesgID;
    reg [31:0] Control;
    reg [31:0] Status;
    reg [31:0] IntEnable;
    reg [31:0] IntID;
    reg [31:0] IoTag;

    reg [31:0] PioCacheReadLen;
    reg [31:0] PioCacheWriteLen;
    reg [31:0] CacheIndxRd;
    reg [31:0] CacheIndxWr;
    reg [31:0] ByteIndxRd;
    reg [31:0] ByteIndxWr;
    reg [31:0] PowerState;
    reg [31:0] Devnum;

    reg [63:0] HostPA;
    reg [31:0] DevDataOfst;
    reg [31:0] DmaCntl;
    reg [31:0] DTBC;
    reg [31:0] Reg20;
    reg [63:0] BCDPP;
    
    // ------------------------------------------------------------
    // Chained / scatter-gather DMA descriptor storage.
    //
    // BAR0:
    //   0x58 + N*0x20
    //
    // Each descriptor contains eight 32-bit DWORDs:
    //
    //   +0x00 HostAddr[31:0]
    //   +0x04 HostAddr[63:32]
    //   +0x08 DevDataOfst
    //   +0x0C DmaCntl
    //   +0x10 DXBC
    //   +0x14 Reg20
    //   +0x18 CDPP[31:0]
    //   +0x1C CDPP[63:32]
    // ------------------------------------------------------------

    reg [63:0] SgHostAddr    [0:MAD_SGDMA_MAX_SECTORS-1];
    reg [31:0] SgDevDataOfst [0:MAD_SGDMA_MAX_SECTORS-1];
    reg [31:0] SgDmaCntl     [0:MAD_SGDMA_MAX_SECTORS-1];
    reg [31:0] SgDXBC        [0:MAD_SGDMA_MAX_SECTORS-1];
    reg [31:0] SgReg20       [0:MAD_SGDMA_MAX_SECTORS-1];
    reg [63:0] SgCDPP        [0:MAD_SGDMA_MAX_SECTORS-1];
    integer sg_i;

    reg [31:0] DiagFeatures;
    reg [31:0] DiagErrSticky;
    reg [31:0] DiagScratch;
    reg [31:0] DiagLtssm;
    reg [31:0] DiagLink;

    reg [ADDR_WIDTH-1:0] awaddr_latched;
    reg                  have_aw;

    reg prev_bufrd_go;
    reg prev_dma_go;
    reg prev_dma_busy;
    reg [31:0] next_control;
    reg [3:0] io_count_saved;

    function [31:0] apply_wstrb;
        input [31:0] oldv;
        input [31:0] newv;
        input [3:0]  strb;
        reg   [31:0] v;
        begin
            v = oldv;
            if (strb[0]) v[7:0]   = newv[7:0];
            if (strb[1]) v[15:8]  = newv[15:8];
            if (strb[2]) v[23:16] = newv[23:16];
            if (strb[3]) v[31:24] = newv[31:24];
            apply_wstrb = v;
        end
    endfunction

    wire aw_fire  = s_axi_awvalid && s_axi_awready;
    wire w_fire   = s_axi_wvalid  && s_axi_wready;
    wire do_write = !s_axi_bvalid && w_fire && (have_aw || aw_fire);

    wire [ADDR_WIDTH-1:0] write_addr =
        have_aw ? awaddr_latched : s_axi_awaddr;

    wire page_sel_wr = write_addr[MAD_DIAG_PAGE_BIT];
    
    wire [7:0] idx_wr = write_addr[9:2];

    wire page_sel_rd = s_axi_araddr[MAD_DIAG_PAGE_BIT];
    
    wire [7:0] idx_rd = s_axi_araddr[9:2];
    
    wire dma_begin = dma_busy && !prev_dma_busy;
    
    localparam [7:0] SG_FIRST_DWORD = 8'h16;   // BAR0 0x58 / 4

    wire [7:0] sg_wr_offset =
        idx_wr - SG_FIRST_DWORD;

    wire [7:0] sg_rd_offset =
        idx_rd - SG_FIRST_DWORD;

    wire [7:0] sg_wr_desc =
        sg_wr_offset >> 3;       // 8 DWORDs / descriptor

    wire [7:0] sg_rd_desc =
        sg_rd_offset >> 3;

    wire [2:0] sg_wr_field =
        sg_wr_offset[2:0];

    wire [2:0] sg_rd_field =
        sg_rd_offset[2:0];

    wire sg_wr_hit =
        (idx_wr >= SG_FIRST_DWORD) &&
        (sg_wr_desc < MAD_SGDMA_MAX_ENTRIES);

    wire sg_rd_hit =
        (idx_rd >= SG_FIRST_DWORD) &&
        (sg_rd_desc < MAD_SGDMA_MAX_ENTRIES);

    always @(posedge aclk) begin
        if (!aresetn) begin
            s_axi_awready <= 1'b1;
            s_axi_wready  <= 1'b1;
            s_axi_bvalid  <= 1'b0;
            s_axi_bresp   <= RESP_OKAY;

            s_axi_arready <= 1'b1;
            s_axi_rvalid  <= 1'b0;
            s_axi_rresp   <= RESP_OKAY;
            s_axi_rdata   <= 32'h0;

            have_aw        <= 1'b0;
            awaddr_latched <= {ADDR_WIDTH{1'b0}};

            MesgID    <= 32'h0;
            Control   <= 32'h0;
            Status    <= MAD_STATUS_CACHE_INIT_MASK;
            IntEnable <= 32'h0;
            IntID     <= 32'h0;
            IoTag     <= 32'h0;

            PioCacheReadLen  <= 32'h0;
            PioCacheWriteLen <= 32'h0;
            CacheIndxRd <= 32'h0;
            CacheIndxWr <= 32'h0;
            ByteIndxRd  <= 32'h0;
            ByteIndxWr  <= 32'h0;
            PowerState  <= 32'h0;
            Devnum      <= 32'h0;

            HostPA <= 64'h0;
            DevDataOfst <= 32'h0;
            DmaCntl <= 32'h0;
            DTBC    <= 32'h0;
            Reg20   <= 32'h0;
            BCDPP   <= 64'h0000_0000_0000_0058; 
            
            for (sg_i = 0; sg_i < MAD_SGDMA_MAX_ENTRIES; sg_i = sg_i + 1) begin
                SgHostAddr[sg_i]    <= 64'd0;
                SgDevDataOfst[sg_i] <= 32'd0;
                SgDmaCntl[sg_i]     <= 32'd0;
                SgDXBC[sg_i]        <= 32'd0;
                SgReg20[sg_i]       <= 32'd0;
                SgCDPP[sg_i]        <= 64'd0;
            end

            DiagFeatures  <= 32'h0000_0001;
            DiagErrSticky <= 32'h0;
            DiagScratch   <= 32'h0;
            DiagLtssm     <= 32'h0;
            DiagLink      <= 32'h0;

            prev_bufrd_go <= 1'b0;
            prev_dma_go   <= 1'b0;
            prev_dma_busy <= 1'b0;
            bufrd_go_pulse<= 1'b0;
            dma_go_pulse  <= 1'b0;
            io_count_saved <= 4'h0;

        end else begin
            bufrd_go_pulse <= 1'b0;
            dma_go_pulse   <= 1'b0;

            // Latch AW only when it is not consumed with W in the same cycle.
            if (aw_fire && !(w_fire && !s_axi_bvalid)) begin
                awaddr_latched <= s_axi_awaddr;
                have_aw <= 1'b1;
            end

            if (do_write) begin
                have_aw <= 1'b0;
                s_axi_bvalid <= 1'b1;
                s_axi_bresp  <= RESP_OKAY;

                next_control = Control;

                if (page_sel_wr == 1'b0) begin
                    case (idx_wr)
                        8'h00: MesgID <= apply_wstrb(MesgID, s_axi_wdata, s_axi_wstrb);

                        8'h01: begin
                            next_control = apply_wstrb(Control, s_axi_wdata, s_axi_wstrb);
                            Control <= next_control;
                        end

                        8'h02: begin
                            if (s_axi_wdata == 32'h0) Status <= MAD_STATUS_CACHE_INIT_MASK;
                            else Status <= apply_wstrb(Status, s_axi_wdata, s_axi_wstrb);
                        end

                        8'h03: IntEnable <= apply_wstrb(IntEnable, s_axi_wdata, s_axi_wstrb);

                        8'h04: begin
                            if (s_axi_wdata == 32'h0) IntID <= 32'h0;
                            else IntID <= apply_wstrb(IntID, s_axi_wdata, s_axi_wstrb);
                        end

                        8'h05: IoTag <= apply_wstrb(IoTag, s_axi_wdata, s_axi_wstrb);

                        8'h06: PioCacheReadLen  <= apply_wstrb(PioCacheReadLen,  s_axi_wdata, s_axi_wstrb);
                        8'h07: PioCacheWriteLen <= apply_wstrb(PioCacheWriteLen, s_axi_wdata, s_axi_wstrb);
                        8'h08: CacheIndxRd <= apply_wstrb(CacheIndxRd, s_axi_wdata, s_axi_wstrb);
                        8'h09: CacheIndxWr <= apply_wstrb(CacheIndxWr, s_axi_wdata, s_axi_wstrb);
                        8'h0A: ByteIndxRd  <= apply_wstrb(ByteIndxRd, s_axi_wdata, s_axi_wstrb);
                        8'h0B: ByteIndxWr  <= apply_wstrb(ByteIndxWr, s_axi_wdata, s_axi_wstrb);
                        8'h0C: PowerState  <= apply_wstrb(PowerState, s_axi_wdata, s_axi_wstrb);
                        8'h0D: Devnum      <= apply_wstrb(Devnum, s_axi_wdata, s_axi_wstrb);

                        8'h0E: HostPA[31:0]  <= apply_wstrb(HostPA[31:0],  s_axi_wdata, s_axi_wstrb);
                        8'h0F: HostPA[63:32] <= apply_wstrb(HostPA[63:32], s_axi_wdata, s_axi_wstrb);

                        8'h10: DevDataOfst <= apply_wstrb(DevDataOfst, s_axi_wdata, s_axi_wstrb);
                        8'h11: DmaCntl     <= apply_wstrb(DmaCntl,     s_axi_wdata, s_axi_wstrb);
                        8'h12: DTBC        <= apply_wstrb(DTBC,        s_axi_wdata, s_axi_wstrb);
                        8'h13: Reg20       <= apply_wstrb(Reg20,       s_axi_wdata, s_axi_wstrb);

                        
                        8'h14: begin
                               // BCDPP is hardware-defined and read-only.
                               // Accept the AXI write, but do not modify BCDPP.
                               //BCDPP[31:0]  <= apply_wstrb(BCDPP[31:0],  s_axi_wdata, s_axi_wstrb);
                               end

                        8'h15: begin
                               // BCDPP is hardware-defined and read-only.
                               // Accept the AXI write, but do not modify BCDPP.
                               //BCDPP[63:32] <= apply_wstrb(BCDPP[63:32], s_axi_wdata, s_axi_wstrb);
                               end

                        default: begin
                            if (sg_wr_hit) begin
                                case (sg_wr_field)
                                    // +0x00 HostAddr low
                                    3'd0:
                                        SgHostAddr[sg_wr_desc][31:0] <=
                                            apply_wstrb(SgHostAddr[sg_wr_desc][31:0],
                                                        s_axi_wdata, s_axi_wstrb);

                                    // +0x04 HostAddr high
                                    3'd1:
                                        SgHostAddr[sg_wr_desc][63:32] <=
                                            apply_wstrb(SgHostAddr[sg_wr_desc][63:32],
                                                        s_axi_wdata, s_axi_wstrb);

                                    // +0x08 DevDataOfst
                                    3'd2:
                                        SgDevDataOfst[sg_wr_desc] <=
                                            apply_wstrb(SgDevDataOfst[sg_wr_desc],
                                                        s_axi_wdata, s_axi_wstrb);

                                    // +0x0C DmaCntl
                                    3'd3:
                                        SgDmaCntl[sg_wr_desc] <=
                                            apply_wstrb(SgDmaCntl[sg_wr_desc],
                                                        s_axi_wdata, s_axi_wstrb);

                                    // +0x10 DXBC
                                   3'd4:
                                       SgDXBC[sg_wr_desc] <=
                                           apply_wstrb(SgDXBC[sg_wr_desc],
                                                       s_axi_wdata, s_axi_wstrb);

                                   // +0x14 Reg20
                                  3'd5:
                                      SgReg20[sg_wr_desc] <=
                                          apply_wstrb(SgReg20[sg_wr_desc],
                                                      s_axi_wdata, s_axi_wstrb);

                                  // +0x18 CDPP low
                                  3'd6:
                                      SgCDPP[sg_wr_desc][31:0] <=
                                          apply_wstrb(SgCDPP[sg_wr_desc][31:0],
                                                     s_axi_wdata, s_axi_wstrb);

                                  // +0x1C CDPP high
                                  3'd7:
                                      SgCDPP[sg_wr_desc][63:32] <=
                                          apply_wstrb(SgCDPP[sg_wr_desc][63:32],
                                                      s_axi_wdata, s_axi_wstrb);

                               endcase
                           end
                           else begin
                               s_axi_bresp <= RESP_SLVERR;
                           end
                        end
                    endcase
                end else begin
                    case (idx_wr)
                        8'h00: s_axi_bresp <= RESP_SLVERR;
                        8'h01: s_axi_bresp <= RESP_SLVERR;
                        8'h02: DiagFeatures <= apply_wstrb(DiagFeatures, s_axi_wdata, s_axi_wstrb);
                        8'h03: DiagErrSticky <= DiagErrSticky & ~apply_wstrb(32'h0, s_axi_wdata, s_axi_wstrb);
                        8'h04: DiagScratch <= apply_wstrb(DiagScratch, s_axi_wdata, s_axi_wstrb);
                        8'h05: DiagLtssm <= apply_wstrb(DiagLtssm, s_axi_wdata, s_axi_wstrb);
                        8'h06: DiagLink  <= apply_wstrb(DiagLink,  s_axi_wdata, s_axi_wstrb);
                        default: s_axi_bresp <= RESP_OKAY;
                    endcase
                end

                if (page_sel_wr == 1'b0) begin
                    if (!prev_bufrd_go && ((next_control & MAD_CONTROL_BUFRD_GO_BIT) != 32'h0)) begin
                        bufrd_go_pulse <= 1'b1;
                        // Capture the encoded count with the GO command.  Software may
                        // alter Control after launch without changing this operation.
                        io_count_saved <=
                            (next_control & MAD_CONTROL_IO_COUNT_MASK)
                            >> MAD_CONTROL_IO_COUNT_SHIFT;
                    end
                    if (!prev_dma_go && ((next_control & MAD_CONTROL_DMA_GO_BIT) != 32'h0))
                        dma_go_pulse <= 1'b1;

                    prev_bufrd_go <= ((next_control & MAD_CONTROL_BUFRD_GO_BIT) != 32'h0);
                    prev_dma_go   <= ((next_control & MAD_CONTROL_DMA_GO_BIT)   != 32'h0);
                end
            end

            // ------------------------------------------------------------
            // Buffered-read hardware progress/completion updates
            // ------------------------------------------------------------
            // ByteIndxRd advances after each accepted 16-byte requester
            // packet. HostPA intentionally remains constant.
            if (bufrd_advance && !cache_xfer_enabled)
                ByteIndxRd <= ByteIndxRd + 32'd16;
                
            // First successful read-cache fill makes the read cache valid.
            if (bufrd_cache_first_use_complete) begin
                Status <= Status & ~MAD_STATUS_READ_CACHE_EMPTY_BIT;
            end    
            
             // Host-to-device: advance the destination index after each
            // successfully received 16-byte completion payload.
            if (bufwr_advance && !cache_xfer_enabled)
                ByteIndxWr <= ByteIndxWr + 32'd16;
                
         
            
            // ------------------------------------------------------------
            // DMA hardware progress and completion
            // ------------------------------------------------------------

            // Retain the prior busy state so the first active clock can
            // initialize the DMA byte count.
            prev_dma_busy <= dma_busy;

           // While DMA is active, hardware owns the BUSY and DONE bits.
           // DONE remains cleared until the final requester packet.
          if (dma_busy) begin
              DmaCntl <= (DmaCntl | MAD_DMA_CNTL_BUSY_BIT) & ~MAD_DMA_CNTL_DONE_BIT;
          end

          // A newly started DMA operation initially has one complete
          // 512-byte block remaining.
          if (dma_begin) begin
              DTBC <= MAD_DMA_TRANSFER_BYTES;
          end else if (dma_advance) begin
             // Subtract one accepted 16-byte requester packet.
             if (DTBC > MAD_DMA_PACKET_BYTES)
                DTBC <= DTBC - MAD_DMA_PACKET_BYTES;
        else
            DTBC <= 32'd0;
        end

        // Completion has priority over the active-state assignments above.
        if (dma_complete) begin
            DmaCntl <= (DmaCntl & ~MAD_DMA_CNTL_BUSY_BIT) | MAD_DMA_CNTL_DONE_BIT;

            DTBC <= 32'd0;
        end      
        
        // ------------------------------------------------------------
        // SG-DMA transferred-byte accounting.
        //
        // DTBC is the hardware-reported number of bytes successfully
        // transferred by the entire SG-DMA chain.
        // ------------------------------------------------------------
        if (sg_dma_begin) begin
            DTBC <= 32'd0;
        end
        else if (sg_dma_xfer_advance) begin
            DTBC <= DTBC + MAD_DMA_PACKET_BYTES; 
        end
        
        if (sg_dma_complete) begin
            if (sg_dma_h2d)
                IntID <= IntID | MAD_INT_DMA_OUTPUT_BIT;
            else
                IntID <= IntID | MAD_INT_DMA_INPUT_BIT;
        end
             // Hardware completion takes priority over a simultaneous software
             // write to the corresponding cache-index register.
             // Device-to-host completion: copy the command's encoded count into
             // Status[11:8], preserving every other Status sub-field.
            if (bufrd_io_complete) begin
                Status <=
                    (Status & ~MAD_STATUS_READ_COUNT_MASK) |
                    ({28'd0, io_count_saved}
                        << MAD_STATUS_READ_COUNT_SHIFT);

                IntID <= IntID | MAD_INT_BUFRD_INPUT_BIT;
            end
            
            // Host-to-device completion: copy the encoded command count into
            // Status[15:12].  On first use of the write cache, also mark the
            // newly filled cache valid by clearing WRITE_CACHE_EMPTY.
            if (bufwr_io_complete) begin
                if (bufwr_cache_first_use_complete) begin
                    Status <= ((Status & ~MAD_STATUS_WRITE_COUNT_MASK) |
                              ({28'd0, io_count_saved} << MAD_STATUS_WRITE_COUNT_SHIFT))
                               & ~MAD_STATUS_WRITE_CACHE_EMPTY_BIT;
                end else begin
                    Status <= (Status & ~MAD_STATUS_WRITE_COUNT_MASK) |
                               ({28'd0, io_count_saved} << MAD_STATUS_WRITE_COUNT_SHIFT);
            end
            IntID <= IntID | MAD_INT_BUFRD_OUTPUT_BIT;
         end
            
            if (bufrd_cache_first_use_complete || bufrd_cache_normal_complete)
                CacheIndxRd <= CacheIndxRd + MAD_CACHE_NUM_SECTORS;

            if (bufwr_cache_normal_complete)
                CacheIndxWr <= CacheIndxWr + MAD_CACHE_NUM_SECTORS;
                
            if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid <= 1'b0;
                s_axi_bresp  <= RESP_OKAY;
            end

            if (s_axi_arvalid && s_axi_arready && !s_axi_rvalid) begin
                s_axi_rvalid <= 1'b1;
                s_axi_rresp  <= RESP_OKAY;

                if (page_sel_rd == 1'b0) begin
                    case (idx_rd)
                        8'h00: s_axi_rdata <= MesgID;
                        8'h01: s_axi_rdata <= Control;
                        8'h02: s_axi_rdata <= Status;
                        8'h03: s_axi_rdata <= IntEnable;
                        8'h04: s_axi_rdata <= IntID;
                        8'h05: s_axi_rdata <= IoTag;
                        8'h06: s_axi_rdata <= PioCacheReadLen;
                        8'h07: s_axi_rdata <= PioCacheWriteLen;
                        8'h08: s_axi_rdata <= CacheIndxRd;
                        8'h09: s_axi_rdata <= CacheIndxWr;
                        8'h0A: s_axi_rdata <= ByteIndxRd;
                        8'h0B: s_axi_rdata <= ByteIndxWr;
                        8'h0C: s_axi_rdata <= PowerState;
                        8'h0D: s_axi_rdata <= Devnum;
                        8'h0E: s_axi_rdata <= HostPA[31:0];
                        8'h0F: s_axi_rdata <= HostPA[63:32];
                        8'h10: s_axi_rdata <= DevDataOfst;
                        8'h11: s_axi_rdata <= DmaCntl;
                        8'h12: s_axi_rdata <= DTBC;
                        8'h13: s_axi_rdata <= Reg20;
                        8'h14: s_axi_rdata <= BCDPP[31:0];
                        8'h15: s_axi_rdata <= BCDPP[63:32];

                        //default: begin
                        //     s_axi_rdata <= 32'hDEAD_B000;
                        //     s_axi_rresp <= RESP_SLVERR;
                        //end
                        default: begin
                            if (sg_rd_hit) begin
                                case (sg_rd_field)

                                    3'd0:
                                    s_axi_rdata <= SgHostAddr[sg_rd_desc][31:0];

                                    3'd1:
                                    s_axi_rdata <= SgHostAddr[sg_rd_desc][63:32];

                                    3'd2:
                                    s_axi_rdata <= SgDevDataOfst[sg_rd_desc];

                                    3'd3:
                                    s_axi_rdata <= SgDmaCntl[sg_rd_desc];

                                    3'd4:
                                    s_axi_rdata <= SgDXBC[sg_rd_desc];

                                    3'd5:
                                    s_axi_rdata <= SgReg20[sg_rd_desc];

                                    3'd6:
                                    s_axi_rdata <= SgCDPP[sg_rd_desc][31:0];

                                    3'd7:
                                    s_axi_rdata <= SgCDPP[sg_rd_desc][63:32];
                               endcase
                            end
                   else begin
                       s_axi_rdata <= 32'hDEAD_B000;
                       s_axi_rresp <= RESP_SLVERR;
                   end
                end
                    endcase
                end else begin
                    case (idx_rd)
                        8'h00: s_axi_rdata <= DIAG_MAGIC;
                        8'h01: s_axi_rdata <= DIAG_VERSION;
                        8'h02: s_axi_rdata <= DiagFeatures;
                        8'h03: s_axi_rdata <= DiagErrSticky;
                        8'h04: s_axi_rdata <= DiagScratch;
                        8'h05: s_axi_rdata <= DiagLtssm;
                        8'h06: s_axi_rdata <= DiagLink;

                        default: begin
                            s_axi_rdata <= 32'h0;
                            s_axi_rresp <= RESP_OKAY;
                        end
                    endcase
                end
            end

            if (s_axi_rvalid && s_axi_rready) begin
                s_axi_rvalid <= 1'b0;
                s_axi_rresp  <= RESP_OKAY;
                s_axi_rdata  <= 32'h0;
            end
        end
    end

endmodule

