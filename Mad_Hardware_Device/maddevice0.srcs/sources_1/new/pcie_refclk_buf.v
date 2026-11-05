// pcie_refclk_buf.v  (UltraScale+/GTE4)
// Input:  PCIe refclk from connector (typically 100 MHz differential)
// Output: sys_clk_gt = GT refclk (to PCIe IP sys_clk_gt)
//         sys_clk    = fabric clock derived from refclk (to PCIe IP sys_clk)

module pcie_refclk_buf (
    input  wire pcie_refclk_p,
    input  wire pcie_refclk_n,
    output wire sys_clk_gt,
    output wire sys_clk
);
    wire gt_refclk;
    wire gt_refclk_div2;

    // UltraScale+ GT refclk buffer
    IBUFDS_GTE4 ibufds_gte4_refclk (
        .I     (pcie_refclk_p),
        .IB    (pcie_refclk_n),
        .CEB   (1'b0),
        .O     (gt_refclk),
        .ODIV2 (gt_refclk_div2)
    );

    assign sys_clk_gt = gt_refclk;
    assign sys_clk    = gt_refclk_div2;

endmodule
