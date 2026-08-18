`timescale 1ps / 1ps

(* DowngradeIPIdentifiedWarnings = "yes" *)
module pcie_device_wrapper_V2 #(
  parameter        TCQ                            = 1,
  parameter [1:0]  AXISTEN_IF_WIDTH              = 2'b01,   // 128-bit
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
  parameter        PARITY_WIDTH                  = C_DATA_WIDTH / 8
)(
  // --------------------------------------------------------------------------
  // PCIe core side
  // --------------------------------------------------------------------------
  (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 user_clk CLK" *)
  //(* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF m_axis_cq:m_axis_rc:s_axis_cc, ASSOCIATED_RESET user_reset, FREQ_HZ 250000000" *)
  (* X_INTERFACE_PARAMETER =
   "ASSOCIATED_BUSIF m_axis_cq:m_axis_rc:s_axis_cc:s_axis_rq, ASSOCIATED_RESET user_reset, FREQ_HZ 250000000" *)
  input user_clk,
  
  (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 user_reset RST" *)
  (* X_INTERFACE_PARAMETER = "POLARITY ACTIVE_HIGH" *)
  input user_reset,
  
  input                            user_lnk_up,

  // CQ
  input      [C_DATA_WIDTH-1:0]    m_axis_cq_tdata,
  input                            m_axis_cq_tlast,
  input                            m_axis_cq_tvalid,
  input  [AXI4_CQ_TUSER_WIDTH-1:0] m_axis_cq_tuser,
  input        [KEEP_WIDTH-1:0]    m_axis_cq_tkeep,
  output                           m_axis_cq_tready,

  // RC
  input      [C_DATA_WIDTH-1:0]    m_axis_rc_tdata,
  input                            m_axis_rc_tlast,
  input                            m_axis_rc_tvalid,
  input        [KEEP_WIDTH-1:0]    m_axis_rc_tkeep,
  input  [AXI4_RC_TUSER_WIDTH-1:0] m_axis_rc_tuser,
  output                           m_axis_rc_tready,
  
  // CC
  output [C_DATA_WIDTH-1:0] s_axis_cc_tdata,
  output [KEEP_WIDTH-1:0]   s_axis_cc_tkeep,
  output                    s_axis_cc_tlast,
  input                     s_axis_cc_tready,
  output                    s_axis_cc_tvalid,
  output [32:0]             s_axis_cc_tuser,
 
  // NP request handshake
  input                     [5:0]  pcie_cq_np_req_count,
  output                    [1:0]  pcie_cq_np_req,

  // Message sideband
  input                            cfg_msg_received,
  input                     [4:0]  cfg_msg_received_type,
  input                     [7:0]  cfg_msg_received_data,
  
  
  // --------------------------------------------------------------------------
  // Decoded outputs
  // --------------------------------------------------------------------------
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
  output                   [12:0]  req_addr,
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
  output                   [10:0]  wr_addr,
  output                    [7:0]  wr_be,
  output                           wr_en,
  //
  output        dbg_regs_awvalid,
  output        dbg_regs_awready,
  output        dbg_regs_wvalid,
  output        dbg_regs_wready,
  output [3:0]  dbg_regs_wstrb,
  output        dbg_bufrd_go_pulse,
  output        dbg_dma_go_pulse,
  //
  output dbg_regs_arvalid,
  output dbg_regs_arready,
  output dbg_regs_rvalid,
  output dbg_regs_rready,
  output [31:0] dbg_regs_rdata,
  output dbg_tx_rd_en,
  output [ADDR_W-1:0] dbg_tx_rd_addr,
  output wire [MEM_W-1:0] dbg_tx_rd_data,
  output wire [15:0] dbg_regs_awaddr,
  output wire [15:0] dbg_regs_araddr,
  output wire [31:0] dbg_regs_wdata,
  //
  output        dbg_axiw_pending,
  output        dbg_portal_wr_busy,
  output        dbg_live_wr_en,
  output        dbg_sop,
  output        dbg_in_packet_q,
  output [7:0]  dbg_rx_state,
  output wire [7:0] dbg_rq_state,
  output wire       dbg_bufrd_start,
  //
  output wire [C_DATA_WIDTH-1:0] s_axis_rq_tdata,
  output wire [KEEP_WIDTH-1:0]   s_axis_rq_tkeep,
  output wire         s_axis_rq_tlast,
  output wire         s_axis_rq_tvalid,
  output wire [61:0]  s_axis_rq_tuser,
  input  wire         s_axis_rq_tready,
  //
  input          wr_busy
);

  // --------------------------------------------------------------------------
  // Local glue
  // --------------------------------------------------------------------------
  wire reset_n;
  wire [1:0] portal_pcie_cq_np_req;

  // TEMP DEBUG
  //assign m_axis_cq_tready = 1'b1;

  // Simple reset shaping
  assign reset_n = ~user_reset;

  // Vendor-style NP request mapping (CRITICAL)
  assign pcie_cq_np_req = portal_pcie_cq_np_req;
  
  wire portal_wr_en;
  
  wire       portal_dbg_regs_awvalid;
  wire       portal_dbg_regs_awready;
  wire       portal_dbg_regs_wvalid;
  wire       portal_dbg_regs_wready;
  wire [3:0] portal_dbg_regs_wstrb;
  wire       portal_dbg_bufrd_go_pulse;
  wire       portal_dbg_dma_go_pulse;
  wire       portal_dbg_axiw_pending;
  wire       portal_dbg_portal_wr_busy;
  wire       portal_dbg_live_wr_en;
  wire       portal_dbg_sop;
  wire       portal_dbg_in_packet_q;
  wire [7:0] portal_dbg_rx_state;

  // TEMP DEBUG ONLY: make ILA_1 trigger when CQ is accepted
  //assign wr_en = dbg_cq_fire;
  assign wr_en =  portal_wr_en;
  assign dbg_regs_awvalid   = portal_dbg_regs_awvalid;
  assign dbg_regs_awready   = portal_dbg_regs_awready;
  assign dbg_regs_wvalid    = portal_dbg_regs_wvalid;
  assign dbg_regs_wready    = portal_dbg_regs_wready;
  assign dbg_regs_wstrb     = portal_dbg_regs_wstrb;
  assign dbg_bufrd_go_pulse = portal_dbg_bufrd_go_pulse;
  assign dbg_dma_go_pulse   = portal_dbg_dma_go_pulse;
  //
  assign dbg_axiw_pending   = portal_dbg_axiw_pending;
  assign dbg_portal_wr_busy = portal_dbg_portal_wr_busy;
  assign dbg_live_wr_en     = portal_dbg_live_wr_en;
  assign dbg_sop            = portal_dbg_sop;
  assign dbg_in_packet_q    = portal_dbg_in_packet_q;
  assign dbg_rx_state       = portal_dbg_rx_state;
 
  // --------------------------------------------------------------------------
  // RX engine instance
  // --------------------------------------------------------------------------
  pcie_device_portal #(
    .TCQ                             ( TCQ                             ),
    .AXISTEN_IF_WIDTH               ( AXISTEN_IF_WIDTH               ),
    .AXISTEN_IF_CQ_ALIGNMENT_MODE   ( AXISTEN_IF_CQ_ALIGNMENT_MODE   ),
    .AXISTEN_IF_RC_ALIGNMENT_MODE   ( AXISTEN_IF_RC_ALIGNMENT_MODE   ),
    .AXISTEN_IF_RC_STRADDLE         ( AXISTEN_IF_RC_STRADDLE         ),
    .AXISTEN_IF_ENABLE_RX_MSG_INTFC ( AXISTEN_IF_ENABLE_RX_MSG_INTFC ),
    .AXISTEN_IF_CQ_PARITY_CHECK     ( AXISTEN_IF_CQ_PARITY_CHECK     ),
    .AXISTEN_IF_RC_PARITY_CHECK     ( AXISTEN_IF_RC_PARITY_CHECK     ),
    .AXISTEN_IF_ENABLE_MSG_ROUTE    ( AXISTEN_IF_ENABLE_MSG_ROUTE    ),
    .AXI4_CQ_TUSER_WIDTH            ( AXI4_CQ_TUSER_WIDTH            ),
    .AXI4_RC_TUSER_WIDTH            ( AXI4_RC_TUSER_WIDTH            ),
    .C_DATA_WIDTH                   ( C_DATA_WIDTH                   ),
    .ADDR_W                         ( ADDR_W                         ),
    .MEM_W                          ( MEM_W                          ),
    .BYTE_EN_W                      ( BYTE_EN_W                      ),
    .STRB_WIDTH                     ( STRB_WIDTH                     ),
    .KEEP_WIDTH                     ( KEEP_WIDTH                     ),
    .PARITY_WIDTH                   ( PARITY_WIDTH                   )
  ) portal_rx_i (
    .user_clk                       ( user_clk                       ),
    .reset_n                        ( reset_n                        ),

    .m_axis_cq_tdata                ( m_axis_cq_tdata                ),
    .m_axis_cq_tlast                ( m_axis_cq_tlast                ),
    .m_axis_cq_tvalid               ( m_axis_cq_tvalid               ),
    .m_axis_cq_tuser                ( m_axis_cq_tuser                ),
    .m_axis_cq_tkeep                ( m_axis_cq_tkeep                ),
    .pcie_cq_np_req_count           ( pcie_cq_np_req_count           ),
    .m_axis_cq_tready               ( m_axis_cq_tready               ),
    .pcie_cq_np_req                 ( portal_pcie_cq_np_req          ),

    .m_axis_rc_tdata                ( m_axis_rc_tdata                ),
    .m_axis_rc_tlast                ( m_axis_rc_tlast                ),
    .m_axis_rc_tvalid               ( m_axis_rc_tvalid               ),
    .m_axis_rc_tkeep                ( m_axis_rc_tkeep                ),
    .m_axis_rc_tuser                ( m_axis_rc_tuser                ),
    .m_axis_rc_tready               ( m_axis_rc_tready               ),
     //
    .s_axis_cc_tdata  (s_axis_cc_tdata),
    .s_axis_cc_tkeep  (s_axis_cc_tkeep),
    .s_axis_cc_tlast  (s_axis_cc_tlast),
    .s_axis_cc_tready (s_axis_cc_tready),
    .s_axis_cc_tvalid (s_axis_cc_tvalid),
    .s_axis_cc_tuser  (s_axis_cc_tuser),
    //
    .s_axis_rq_tdata  (s_axis_rq_tdata),
    .s_axis_rq_tkeep  (s_axis_rq_tkeep),
    .s_axis_rq_tlast  (s_axis_rq_tlast),
    .s_axis_rq_tvalid (s_axis_rq_tvalid),
    .s_axis_rq_tuser  (s_axis_rq_tuser),
    .s_axis_rq_tready (s_axis_rq_tready),
    //
    .cfg_msg_received               ( cfg_msg_received               ),
    .cfg_msg_received_type          ( cfg_msg_received_type          ),
    .cfg_msg_data                   ( cfg_msg_received_data          ),
     //
    .req_compl                      ( req_compl                      ),
    .req_compl_wd                   ( req_compl_wd                   ),
    .req_compl_ur                   ( req_compl_ur                   ),
    .compl_done                     ( compl_done                     ),

    .req_tc                         ( req_tc                         ),
    .req_attr                       ( req_attr                       ),
    .req_len                        ( req_len                        ),
    .req_rid                        ( req_rid                        ),
    .req_tag                        ( req_tag                        ),
    .req_be                         ( req_be                         ),
    .req_addr                       ( req_addr                       ),
    .req_at                         ( req_at                         ),

    .req_des_qword0                 ( req_des_qword0                 ),
    .req_des_qword1                 ( req_des_qword1                 ),
    .req_des_tph_present            ( req_des_tph_present            ),
    .req_des_tph_type               ( req_des_tph_type               ),
    .req_des_tph_st_tag             ( req_des_tph_st_tag             ),

    .req_mem_lock                   ( req_mem_lock                   ),
    .req_mem                        ( req_mem                        ),

    .wr_data                        ( wr_data                        ),
    .payload_len                    ( payload_len                    ),
    .wr_sop                         ( wr_sop                         ),
    .wr_eop                         ( wr_eop                         ),
    .wr_data_be                     ( wr_data_be                     ),
    .wr_addr                        ( wr_addr                        ),
    .wr_be                          ( wr_be                          ),
    .wr_en                          ( portal_wr_en                   ),
    .dbg_regs_awvalid               (portal_dbg_regs_awvalid),
    .dbg_regs_awready               (portal_dbg_regs_awready),
    .dbg_regs_wvalid                (portal_dbg_regs_wvalid),
    .dbg_regs_wready                (portal_dbg_regs_wready),
    .dbg_regs_wstrb                 (portal_dbg_regs_wstrb),
    .dbg_bufrd_go_pulse             (portal_dbg_bufrd_go_pulse),
    .dbg_dma_go_pulse               (portal_dbg_dma_go_pulse),
    //
    .dbg_regs_arvalid(dbg_regs_arvalid),
    .dbg_regs_arready(dbg_regs_arready),
    .dbg_regs_rvalid (dbg_regs_rvalid),
    .dbg_regs_rready(dbg_regs_rready),
    .dbg_regs_rdata  (dbg_regs_rdata),
    .dbg_tx_rd_en    (dbg_tx_rd_en),
    .dbg_tx_rd_addr  (dbg_tx_rd_addr),
    .dbg_tx_rd_data(dbg_tx_rd_data),
    .dbg_regs_awaddr(dbg_regs_awaddr),
    .dbg_regs_araddr(dbg_regs_araddr),
    .dbg_regs_wdata (dbg_regs_wdata),
    //
    .dbg_axiw_pending   (portal_dbg_axiw_pending),
    .dbg_portal_wr_busy (portal_dbg_portal_wr_busy),
    .dbg_live_wr_en     (portal_dbg_live_wr_en),
    .dbg_sop            (portal_dbg_sop),
    .dbg_in_packet_q    (portal_dbg_in_packet_q),
    .dbg_rx_state       (portal_dbg_rx_state),
    .dbg_rq_state          (dbg_rq_state),
    .dbg_bufrd_start (dbg_bufrd_start),
    //
    .wr_busy                        ( wr_busy                        )
  );
endmodule
