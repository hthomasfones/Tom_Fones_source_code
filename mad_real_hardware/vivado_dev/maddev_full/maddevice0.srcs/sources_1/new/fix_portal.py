#!/usr/bin/env python3

from pathlib import Path
import shutil
import sys


def replace_once(text: str, old: str, new: str, description: str) -> str:
    count = text.count(old)

    if count != 1:
        raise RuntimeError(
            f"Could not safely perform: {description}\n"
            f"Expected exactly one matching section, but found {count}."
        )

    return text.replace(old, new, 1)


def main() -> None:
    if len(sys.argv) > 2:
        print("Usage: python3 fix_portal.py [pcie_device_portal.v]")
        sys.exit(1)

    source = (
        Path(sys.argv[1])
        if len(sys.argv) == 2
        else Path("pcie_device_portal.v")
    )

    if not source.is_file():
        print(f"File not found: {source}")
        sys.exit(1)

    backup = source.with_suffix(source.suffix + ".before_rc_fix")
    output = source.with_name(source.stem + "_fixed" + source.suffix)

    shutil.copy2(source, backup)
    text = source.read_text()

    # ------------------------------------------------------------
    # Add the five requester-completion wires to pcie_device_portal.
    # ------------------------------------------------------------
    old = """  (* mark_debug = "true" *) wire        portal_wr_busy;
  //
  wire [63:0] host_addr;"""

    new = """  (* mark_debug = "true" *) wire        portal_wr_busy;

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
  wire [63:0] host_addr;"""

    text = replace_once(
        text,
        old,
        new,
        "add requester-completion wires to pcie_device_portal",
    )

    # ------------------------------------------------------------
    # Connect the five outputs from pio_rx_engine to the portal.
    # ------------------------------------------------------------
    old = """    .dbg_sop(dbg_sop),
    .dbg_in_packet_q(dbg_in_packet_q),
    .dbg_rx_state(dbg_rx_state)
  );"""

    new = """    .dbg_sop(dbg_sop),
    .dbg_in_packet_q(dbg_in_packet_q),
    .dbg_rx_state(dbg_rx_state),

    .bufwr_rc_data(bufwr_rc_data),
    .bufwr_rc_keep(bufwr_rc_keep),
    .bufwr_rc_user(bufwr_rc_user),
    .bufwr_rc_valid(bufwr_rc_valid),
    .bufwr_rc_last(bufwr_rc_last)
  );"""

    text = replace_once(
        text,
        old,
        new,
        "connect requester-completion outputs to pcie_device_portal",
    )

    # ------------------------------------------------------------
    # Remove the second, conflicting set of output declarations.
    # ------------------------------------------------------------
    old = """  output wire                      dbg_sop,
  output wire                      dbg_in_packet_q,
  output wire              [7:0]   dbg_rx_state,
  output reg         bufwr_rc_valid,
  output reg [127:0] bufwr_rc_data,
  output reg         bufwr_rc_last,

  input                            wr_busy              // Memory Write Busy
);"""

    new = """  output wire                      dbg_sop,
  output wire                      dbg_in_packet_q,
  output wire              [7:0]   dbg_rx_state,

  input                            wr_busy              // Memory Write Busy
);"""

    text = replace_once(
        text,
        old,
        new,
        "remove duplicate pio_rx_engine output declarations",
    )

    # ------------------------------------------------------------
    # Remove the RC capture block from inside the parity-only area.
    # ------------------------------------------------------------
    capture_block = """  // ------------------------------------------------------------
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
      // Default: valid and last are pulses, not retained levels.
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

"""

    text = replace_once(
        text,
        capture_block,
        "",
        "remove RC capture from the parity-only section",
    )

    # ------------------------------------------------------------
    # Reinsert the capture block after the parity section so it is
    # present even though parity checking is disabled.
    # ------------------------------------------------------------
    old = """  end
  endgenerate

  generate
    //State machine for D-word Aligned Mode"""

    new = """  end
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
    //State machine for D-word Aligned Mode"""

    text = replace_once(
        text,
        old,
        new,
        "move RC capture after the parity section",
    )

    output.write_text(text)

    print()
    print("Correction completed successfully.")
    print(f"Original file:  {source}")
    print(f"Backup copy:   {backup}")
    print(f"Corrected file: {output}")
    print()
    print("Use the corrected file ending in _fixed.v.")


if __name__ == "__main__":
    main()
