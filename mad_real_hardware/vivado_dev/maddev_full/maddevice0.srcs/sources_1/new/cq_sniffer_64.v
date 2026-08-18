// cq_sniffer_64.v
//
// Minimal PCIe CQ sniffer for UltraScale+ PCIe4 (pcie4_uscale_plus) in 64-bit AXI-stream mode.
// - Always asserts CQ TREADY (consumes all CQ traffic).
// - Captures first beat (tdata/tuser) of each incoming CQ TLP.
// - Pulses tlp_seen_pulse on each TLP start.
// - Pulses memwr_seen_pulse using a *heuristic* decode (refine after observing real headers).
//
// Notes:
// - Adds Xilinx interface attributes so IP Integrator can associate the AXIS bus with user_clk
//   and match the PCIe core's 125 MHz AXIS clock.
// - user_reset_n is active-low.
//
// Copyright: You
//

module cq_sniffer_64_v2 (
    // Clock/Reset
    (* X_INTERFACE_PARAMETER = "FREQ_HZ 125000000" *)
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 user_clk CLK" *)
    input  wire         user_clk,

    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 user_reset_n RST" *)
    input  wire         user_reset_n,   // active-low reset

    // Optional: gate by link-up (can tie to 1'b1)
    input  wire         user_lnk_up,

    // CQ from PCIe core (AXI-Stream)
    // Associate this bus with user_clk and user_reset_n and set 125 MHz to match the PCIe core.
    (* X_INTERFACE_PARAMETER = "ASSOCIATED_BUSIF m_axis_cq, ASSOCIATED_RESET user_reset_n, FREQ_HZ 125000000" *)
    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TDATA" *)
    input  wire [63:0]  m_axis_cq_tdata,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TKEEP"  *)
    input  wire [1:0]   m_axis_cq_tkeep,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TLAST"  *)
    input  wire         m_axis_cq_tlast,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TUSER"  *)
    input  wire [87:0]  m_axis_cq_tuser,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TVALID" *)
    input  wire         m_axis_cq_tvalid,

    (* X_INTERFACE_INFO = "xilinx.com:interface:axis:1.0 m_axis_cq TREADY" *)
    output wire         m_axis_cq_tready,

    // Debug outputs (probe with ILA)
    output reg          tlp_seen_pulse,
    output reg          memwr_seen_pulse,
    output reg  [63:0]  dbg_tdata0,
    output reg  [87:0]  dbg_tuser0
);

  // Always ready: swallow all CQ traffic so we can observe it
  assign m_axis_cq_tready = 1'b1;

  // Track start-of-packet / in-packet
  reg in_tlp;

  // Best-effort decode: CQ header packing varies by configuration.
  // We'll start with a heuristic and refine based on captured dbg_tdata0 patterns.
  function automatic is_likely_memwr;
    input [63:0] d;
    begin
      // Heuristic bucket on a "fmt/type-ish" field; replace once you confirm actual bit positions.
      case (d[30:24])
        7'h40, 7'h60, 7'h41, 7'h61: is_likely_memwr = 1'b1;
        default:                    is_likely_memwr = 1'b0;
      endcase
    end
  endfunction

  wire fire = m_axis_cq_tvalid && m_axis_cq_tready && user_lnk_up;

  always @(posedge user_clk) begin
    if (!user_reset_n) begin
      in_tlp           <= 1'b0;
      tlp_seen_pulse   <= 1'b0;
      memwr_seen_pulse <= 1'b0;
      dbg_tdata0 <= 64'd0;
      dbg_tuser0 <= 88'd0;
    end else begin
      // default deassert pulses (1-cycle)
      tlp_seen_pulse   <= 1'b0;
      memwr_seen_pulse <= 1'b0;

      if (fire) begin
        // Start of a new TLP = first beat observed when not currently inside a TLP
        if (!in_tlp) begin
          dbg_tdata0 <= m_axis_cq_tdata;
          dbg_tuser0 <= m_axis_cq_tuser;

          tlp_seen_pulse <= 1'b1;
          if (is_likely_memwr(m_axis_cq_tdata))
            memwr_seen_pulse <= 1'b1;
        end

        // Maintain in_tlp state until TLAST
        if (m_axis_cq_tlast)
          in_tlp <= 1'b0;
        else
          in_tlp <= 1'b1;
      end
    end
  end

endmodule




