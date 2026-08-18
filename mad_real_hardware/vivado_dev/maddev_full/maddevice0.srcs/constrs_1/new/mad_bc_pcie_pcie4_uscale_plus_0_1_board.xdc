##-----------------------------------------------------------------------------
##
## Project    : UltraScale+ FPGA PCI Express v4.0 Integrated Block
## File       : ip_pcie4_uscale_plus_impl_x0y0.xdc
## Version    : 1.3
##
## Minimal implementation-time constraints for custom endpoint design
##
##-----------------------------------------------------------------------------

# sys_reset is asynchronous into the PCIe hard block
set_false_path -from [get_pins sys_reset]

###############################################################################
# TIMING Exceptions - MCP
###############################################################################

# Multi-cycle paths for AXIS pins of the PCIe hard block
set PCIE4MACRO mad_bc_pcie_pcie4_uscale_plus_0_1_pcie_4_0_pipe_inst/pcie_4_0_e4_inst
set PCIE4MACROPINS  [get_pins "$PCIE4MACRO/*AXIS*"]

set PCIE4MACROINPINS [get_pins $PCIE4MACROPINS -filter DIRECTION==IN]
set_multicycle_path -setup 2 -end   -through $PCIE4MACROINPINS
set_multicycle_path -hold  1 -end   -through $PCIE4MACROINPINS

set PCIE4MACROOUTPINS [get_pins $PCIE4MACROPINS -filter DIRECTION==OUT]
set_multicycle_path -setup 2 -start -through $PCIE4MACROOUTPINS
set_multicycle_path -hold  1 -start -through $PCIE4MACROOUTPINS

# Multi-cycle paths for selected user/config pins
set PCIE4INST mad_bc_pcie_pcie4_uscale_plus_0_1_pcie_4_0_pipe_inst/pcie_4_0_e4_inst
set USERPINS  [get_pins "$PCIE4INST/CFG* $PCIE4INST/CONF* $PCIE4INST/PCIECQNPREQ* $PCIE4INST/PCIERQTAG* $PCIE4INST/PCIETFC* $PCIE4INST/USERSPARE*"]

set USERINPINS [get_pins $USERPINS -filter DIRECTION==IN]
set_multicycle_path -setup 2 -end   -through $USERINPINS
set_multicycle_path -hold  1 -end   -through $USERINPINS

set USEROUTPINS [get_pins $USERPINS -filter DIRECTION==OUT]
set_multicycle_path -setup 2 -start -through $USEROUTPINS
set_multicycle_path -hold  1 -start -through $USEROUTPINS

# Special case for CFGPHYLINKDOWN
set USERPINS1  [get_pins "$PCIE4INST/CFGPHYLINKDOWN"]
set USERINPINS1 [get_pins $USERPINS1 -filter DIRECTION==IN]
set_multicycle_path -setup 4 -end   -through $USERINPINS1
set_multicycle_path -hold  3 -end   -through $USERINPINS1

set USEROUTPINS1 [get_pins $USERPINS1 -filter DIRECTION==OUT]
set_multicycle_path -setup 4 -start -through $USEROUTPINS1
set_multicycle_path -hold  3 -start -through $USEROUTPINS1
