`timescale 1ns / 1ps

//////////////////////////////////////////////////////////////////////////////////
// Company: HTF Consulting
// Engineer: Tom Fones
// 
// Create Date: 01/01/2027
// Design Name: 
// Module Name: mad_device_mem.v
// Project Name: MAd_Hardware_Device
// Target Devices: Kintex: xilinx.com:kcu116:part0:1.5
// Tool Versions: 
// Description: Prototype PCIe endpoint device
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// mad_device_mem
//
// 1 MiB device backing store.
//
// Organization:
//     65536 words x 128 bits
//     16 bytes per word
//     1,048,576 bytes total
//
// Port A: 128-bit write port
// Port B: 128-bit synchronous read port
//
// Both ports use the same clock.
// ----------------------------------------------------------------------------

module mad_device_mem
(
    input  wire         clk,

    // Write port
    input  wire         wr_en,
    input  wire [16:0]  wr_addr,
    input  wire [127:0] wr_data,

    // Read port
    input  wire         rd_en,
    input  wire [16:0]  rd_addr,
    output wire [127:0] rd_data
);

    // No ECC is being used, so these status outputs are intentionally unused.
    wire dbiterrb_unused;
    wire sbiterrb_unused;

    xpm_memory_sdpram #(
        .ADDR_WIDTH_A(17),
        .ADDR_WIDTH_B(17),
        .AUTO_SLEEP_TIME(0),

        // One write-enable bit controls the entire 128-bit word.
        .BYTE_WRITE_WIDTH_A(128),

        .CASCADE_HEIGHT(0),
        .CLOCKING_MODE("common_clock"),
        .ECC_BIT_RANGE("7:0"),
        .ECC_MODE("no_ecc"),
        .ECC_TYPE("none"),
        .IGNORE_INIT_SYNTH(0),
        .MEMORY_INIT_FILE("none"),
        .MEMORY_INIT_PARAM("0"),
        .MEMORY_OPTIMIZATION("true"),

        // Explicitly request UltraRAM.
        .MEMORY_PRIMITIVE("ultra"),

        // MEMORY_SIZE is specified in bits:
        // 2 MiB = 2,097,152 bytes = 16,777,216 bits.
        .MEMORY_SIZE(16777216),

        .MESSAGE_CONTROL(0),
        .RAM_DECOMP("auto"),
        .READ_DATA_WIDTH_B(128),

        // Keep the latency from the Vivado 2023.2 template for this
        // first synthesis experiment.
        .READ_LATENCY_B(2),

        .READ_RESET_VALUE_B("0"),
        .RST_MODE_A("SYNC"),
        .RST_MODE_B("SYNC"),
        .SIM_ASSERT_CHK(0),
        .USE_EMBEDDED_CONSTRAINT(0),
        .USE_MEM_INIT(1),
        .USE_MEM_INIT_MMI(0),
        .WAKEUP_TIME("disable_sleep"),
        .WRITE_DATA_WIDTH_A(128),
        //.WRITE_MODE_B("no_change"),
        .WRITE_MODE_B("read_first"),
        .WRITE_PROTECT(1)
    )
    xpm_memory_sdpram_inst
    (
        .dbiterrb(dbiterrb_unused),
        .doutb(rd_data),
        .sbiterrb(sbiterrb_unused),

        .addra(wr_addr),
        .addrb(rd_addr),

        .clka(clk),

        // CLOCKING_MODE="common_clock", so clkb is unused.
        .clkb(1'b0),

        .dina(wr_data),
        .ena(wr_en),
        .enb(rd_en),

        // ECC disabled.
        .injectdbiterra(1'b0),
        .injectsbiterra(1'b0),

        // Always allow the final read-data register to update.
        .regceb(1'b1),

        // We don't need to reset the RAM contents or output register.
        .rstb(1'b0),

        // Dynamic sleep disabled.
        .sleep(1'b0),

        // BYTE_WRITE_WIDTH_A == WRITE_DATA_WIDTH_A, therefore wea is
        // one bit wide and performs a complete 128-bit-word write.
        .wea(wr_en)
    );

endmodule
