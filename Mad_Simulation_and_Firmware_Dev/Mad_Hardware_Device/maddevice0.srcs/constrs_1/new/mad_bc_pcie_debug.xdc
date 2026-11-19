######################################################################
# Extra debug ILA for internal PCIe GT/PHY resetdone signal
######################################################################

create_debug_core u_ila_extra ila
set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_extra]
set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_extra]
set_property C_TRIGIN_EN false [get_debug_cores u_ila_extra]
set_property C_TRIGOUT_EN false [get_debug_cores u_ila_extra]
set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_extra]

connect_debug_port u_ila_extra/clk [get_nets {mad_bc_pcie_i/dbg_clk_wiz_clk_out1}]

set_property PROBE_TYPE DATA_AND_TRIGGER [get_debug_ports u_ila_extra/probe0]
set_property port_width 1 [get_debug_ports u_ila_extra/probe0]
connect_debug_port u_ila_extra/probe0 [get_nets {mad_bc_pcie_i/pcie4_uscale_plus_0/inst/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_top_i/diablo_gt.diablo_gt_phy_wrapper/phy_rst_i/resetdone_a__0}]
