######################################################################
# kcu116_pcie_quad224_x4.xdc
#
# Constraints for:
#   - mad_bc_pcie_wrapper
#   - KCU116
#   - PCIe Gen3 x4
#   - PCIe block location X0Y0
#   - GT Quad 224
#   - Dedicated debug clock on SYSCLK_300_P/N
######################################################################

######################################################################
# PCIe reset
######################################################################
set_property PACKAGE_PIN T19 [get_ports pcie_perstn]
set_property IOSTANDARD LVCMOS18 [get_ports pcie_perstn]

######################################################################
# PCIe reference clock for GT Quad 224 / GTYE4_COMMON_X0Y0
#
# Using MGTREFCLK0_224:
#   pcie_refclk_clk_p -> AB7  (MGTREFCLK0P_224)
#   pcie_refclk_clk_n -> AB6  (MGTREFCLK0N_224)
#
# Alternate pair, if needed later:
#   Y7 / Y6 = MGTREFCLK1P/N_224
######################################################################
set_property PACKAGE_PIN AB6 [get_ports pcie_refclk_clk_n]
set_property PACKAGE_PIN AB7 [get_ports pcie_refclk_clk_p]

# No IOSTANDARD on MGT refclk pins
create_clock -period 10.000 -name pcie_refclk [get_ports pcie_refclk_clk_p]

######################################################################
# PCIe x4 lanes for GT Quad 224
#
# Quad 224 channel mapping:
#
# Lane 0 / GTYE4_CHANNEL_X0Y0
#   RXP -> AF2   (MGTYRXP0_224)
#   RXN -> AF1   (MGTYRXN0_224)
#   TXP -> AF7   (MGTYTXP0_224)
#   TXN -> AF6   (MGTYTXN0_224)
#
# Lane 1 / GTYE4_CHANNEL_X0Y1
#   RXP -> AE4   (MGTYRXP1_224)
#   RXN -> AE3   (MGTYRXN1_224)
#   TXP -> AE9   (MGTYTXP1_224)
#   TXN -> AE8   (MGTYTXN1_224)
#
# Lane 2 / GTYE4_CHANNEL_X0Y2
#   RXP -> AD2   (MGTYRXP2_224)
#   RXN -> AD1   (MGTYRXN2_224)
#   TXP -> AD7   (MGTYTXP2_224)
#   TXN -> AD6   (MGTYTXN2_224)
#
# Lane 3 / GTYE4_CHANNEL_X0Y3
#   RXP -> AB2   (MGTYRXP3_224)
#   RXN -> AB1   (MGTYRXN3_224)
#   TXP -> AC5   (MGTYTXP3_224)
#   TXN -> AC4   (MGTYTXN3_224)
######################################################################

# Force PCIe GT channel placement to Quad 224
# Keep the same instance-index pattern used in your old XDC:
#   gen_gtye4_inst[3] -> lane 0
#   gen_gtye4_inst[2] -> lane 1
#   gen_gtye4_inst[1] -> lane 2
#   gen_gtye4_inst[0] -> lane 3

# set_property LOC GTYE4_CHANNEL_X0Y0 [get_cells {mad_bc_pcie_i/pcie4_uscale_plus_0/inst/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_top_i/diablo_gt.diablo_gt_phy_wrapper/gt_wizard.gtwizard_top_i/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_i/inst/gen_gtwizard_gtye4_top.mad_bc_pcie_pcie4_uscale_plus_0_1_gt_gtwizard_gtye4_inst/gen_gtwizard_gtye4.gen_channel_container[1].gen_enabled_channel.gtye4_channel_wrapper_inst/channel_inst/gtye4_channel_gen.gen_gtye4_channel_inst[3].GTYE4_CHANNEL_PRIM_INST}]
#set_property PACKAGE_PIN AF2 [get_ports {pci_exp_rxp[0]}]
#set_property PACKAGE_PIN AF1 [get_ports {pci_exp_rxn[0]}]
#set_property PACKAGE_PIN AF7 [get_ports {pci_exp_txp[0]}]
#set_property PACKAGE_PIN AF6 [get_ports {pci_exp_txn[0]}]

# set_property LOC GTYE4_CHANNEL_X0Y1 [get_cells {mad_bc_pcie_i/pcie4_uscale_plus_0/inst/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_top_i/#diablo_gt.diablo_gt_phy_wrapper/gt_wizard.gtwizard_top_i/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_i/inst/#gen_gtwizard_gtye4_top.mad_bc_pcie_pcie4_uscale_plus_0_1_gt_gtwizard_gtye4_inst/#gen_gtwizard_gtye4.gen_channel_container[1].gen_enabled_channel.gtye4_channel_wrapper_inst/channel_inst/#gtye4_channel_gen.gen_gtye4_channel_inst[2].GTYE4_CHANNEL_PRIM_INST}]
#set_property PACKAGE_PIN AE4 [get_ports {pci_exp_rxp[1]}]
#set_property PACKAGE_PIN AE3 [get_ports {pci_exp_rxn[1]}]
#set_property PACKAGE_PIN AE9 [get_ports {pci_exp_txp[1]}]
#set_property PACKAGE_PIN AE8 [get_ports {pci_exp_txn[1]}]

# set_property LOC GTYE4_CHANNEL_X0Y2 [get_c# ells {mad_bc_pcie_i/pcie4_uscale_plus_0/inst/#mad_bc_pcie_pcie4_uscale_plus_0_1_gt_top_i/diablo_gt.diablo_gt_phy_wrapper/gt_wizard.gtwizard_top_i/#mad_bc_pcie_pcie4_uscale_plus_0_1_gt_i/inst/gen_gtwizard_gtye4_top.mad_bc_pcie_pcie4_uscale_plus_0_1_gt_gtwizard_gtye4_inst/#gen_gtwizard_gtye4.gen_channel_container[1].gen_enabled_channel.gtye4_channel_wrapper_inst/channel_inst/#gtye4_channel_gen.gen_gtye4_channel_inst[1].GTYE4_CHANNEL_PRIM_INST}]
#set_property PACKAGE_PIN AD2 [get_ports {pci_exp_rxp[2]}]
#set_property PACKAGE_PIN AD1 [get_ports {pci_exp_rxn[2]}]
#set_property PACKAGE_PIN AD7 [get_ports {pci_exp_txp[2]}]
#set_property PACKAGE_PIN AD6 [get_ports {pci_exp_txn[2]}]

# set_property LOC GTYE4_CHANNEL_X0Y3 [get_cells {mad_bc_pcie_i/pcie4_uscale_plus_0/inst/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_top_i/#diablo_gt.diablo_gt_phy_wrapper/gt_wizard.gtwizard_top_i/mad_bc_pcie_pcie4_uscale_plus_0_1_gt_i/inst/#gen_gtwizard_gtye4_top.mad_bc_pcie_pcie4_uscale_plus_0_1_gt_gtwizard_gtye4_inst/#gen_gtwizard_gtye4.gen_channel_container[1].gen_enabled_channel.gtye4_channel_wrapper_inst/channel_inst/#gtye4_channel_gen.gen_gtye4_channel_inst[0].GTYE4_CHANNEL_PRIM_INST}]
#set_property PACKAGE_PIN AB2 [get_ports {pci_exp_rxp[3]}]
#set_property PACKAGE_PIN AB1 [get_ports {pci_exp_rxn[3]}]
#set_property PACKAGE_PIN AC5 [get_ports {pci_exp_txp[3]}]
#set_property PACKAGE_PIN AC4 [get_ports {pci_exp_txn[3]}]

# No IOSTANDARD constraints on GT TX/RX pins

######################################################################
# Dedicated debug clock input
# KCU116 SYSCLK_300_P/N
#   SYSCLK_300_P -> K22
#   SYSCLK_300_N -> K23
######################################################################
set_property PACKAGE_PIN K22 [get_ports dbg_clk_p]
set_property PACKAGE_PIN K23 [get_ports dbg_clk_n]
set_property IOSTANDARD LVDS [get_ports dbg_clk_p]
set_property IOSTANDARD LVDS [get_ports dbg_clk_n]

create_clock -period 3.333 -name dbg_clk [get_ports dbg_clk_p]

######################################################################
# Debug hub properties
######################################################################
# set_property C_CLK_INPUT_FREQ_HZ 100000000 [get_debug_cores dbg_hub]
# set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hu# # b]
# set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
# connect_debug_port dbg_hub/clk [get_nets {clk}]


set_property C_CLK_INPUT_FREQ_HZ 100000000 [get_debug_cores dbg_hub]
set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
connect_debug_port dbg_hub/clk [get_nets mad_bc_pcie_i/dbg_clk_wiz/inst/clk_out1_mad_bc_pcie_dbg_clk_wiz_0]
