#process for getting script file directory
variable dispScriptFile [file normalize [info script]]
proc getScriptDirectory {} {
    variable dispScriptFile
    set scriptFolder [file dirname $dispScriptFile]
    return $scriptFolder
}

#change working directory to script file directory
cd [getScriptDirectory]
#set result directory
set resultDir .\/result
#set ip_repo_path to script dir
set ip_repo_path [getScriptDirectory]\/..\/
#redifine resultDir HERE if needed
#set resutDir C:\/User\/result

file mkdir $resultDir

# CONNECT SYSTEM
create_project VGA_DMA_controller $resultDir  -part xc7z010clg400-1 -force
set_property board_part digilentinc.com:zybo:part0:1.0 [current_project]
create_bd_design "VGA_DMA_controller"
update_compile_order -fileset sources_1
#add ip-s to main repo
set_property  ip_repo_paths  $ip_repo_path [current_project]
update_ip_catalog
#add and configure zynq
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0
endgroup

set_property -dict [list CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} CONFIG.PCW_USE_FABRIC_INTERRUPT {1} CONFIG.PCW_IRQ_F2P_INTR {1} CONFIG.PCW_PRESET_BANK1_VOLTAGE {LVCMOS 1.8V} CONFIG.PCW_QSPI_PERIPHERAL_ENABLE {1} CONFIG.PCW_QSPI_GRP_FBCLK_ENABLE {1} CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {1} CONFIG.PCW_ENET0_ENET0_IO {MIO 16 .. 27} CONFIG.PCW_ENET0_GRP_MDIO_ENABLE {1} CONFIG.PCW_ENET0_GRP_MDIO_IO {MIO 52 .. 53} CONFIG.PCW_SD0_PERIPHERAL_ENABLE {1} CONFIG.PCW_SD0_GRP_CD_ENABLE {1} CONFIG.PCW_SD0_GRP_CD_IO {MIO 47} CONFIG.PCW_SD0_GRP_WP_ENABLE {1} CONFIG.PCW_UART1_PERIPHERAL_ENABLE {1} CONFIG.PCW_TTC0_PERIPHERAL_ENABLE {1} CONFIG.PCW_USB0_PERIPHERAL_ENABLE {1} CONFIG.PCW_GPIO_MIO_GPIO_ENABLE {1} CONFIG.PCW_USE_S_AXI_HP0 {1} CONFIG.PCW_QSPI_GRP_SINGLE_SS_ENABLE {1}] [get_bd_cells processing_system7_0]

apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR" apply_board_preset "1" Master "Disable" Slave "Disable" }  [get_bd_cells processing_system7_0]

#add AXI Timer 
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0
endgroup
set_property -dict [list CONFIG.mode_64bit {1}] [get_bd_cells axi_timer_0]

#add LED_IP, output port and connect them
startgroup
create_bd_cell -type ip -vlnv FTN:user:myLed:1.0 myLed_0
endgroup

create_bd_port -dir O -from 3 -to 0 -type data led

startgroup
connect_bd_net [get_bd_ports led] [get_bd_pins myLed_0/led]
endgroup


#add VGA controller 
startgroup
create_bd_cell -type ip -vlnv FTN:user:VGA_DMA_IP:1.0 VGA_DMA_IP_0
endgroup

#create output ports and connect them
startgroup
create_bd_port -dir O -from 15 -to 0 -type data RGB_Out
connect_bd_net [get_bd_ports RGB_Out] [get_bd_pins VGA_DMA_IP_0/rgb]

create_bd_port -dir O -type data Hsync
connect_bd_net [get_bd_ports Hsync] [get_bd_pins VGA_DMA_IP_0/hsync]

create_bd_port -dir O -type data Vsync
connect_bd_net [get_bd_ports Vsync] [get_bd_pins VGA_DMA_IP_0/vsync]
endgroup

#add DMA, configure it, then connect it to VGA_IP
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_dma_0
endgroup
set_property -dict [list CONFIG.c_include_sg {0} CONFIG.c_sg_length_width {26} CONFIG.c_sg_include_stscntrl_strm {0} CONFIG.c_include_s2mm {0}] [get_bd_cells axi_dma_0]
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S] [get_bd_intf_pins VGA_DMA_IP_0/s00_axis]

#add concat, connect interrupts
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0
endgroup

connect_bd_net [get_bd_pins axi_timer_0/interrupt] [get_bd_pins xlconcat_0/In0]
connect_bd_net [get_bd_pins axi_dma_0/mm2s_introut] [get_bd_pins xlconcat_0/In1]
connect_bd_net [get_bd_pins xlconcat_0/dout] [get_bd_pins processing_system7_0/IRQ_F2P]

#dun block automation
startgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/axi_dma_0/M_AXI_MM2S} Slave {/processing_system7_0/S_AXI_HP0} intc_ip {Auto} master_apm {0}}  [get_bd_intf_pins processing_system7_0/S_AXI_HP0]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0} Slave {/axi_timer_0/S_AXI} intc_ip {New AXI Interconnect} master_apm {0}}  [get_bd_intf_pins axi_timer_0/S_AXI]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0} Slave {/myLed_0/s_axi} intc_ip {New AXI Interconnect} master_apm {0}}  [get_bd_intf_pins myLed_0/s_axi]

apply_bd_automation -rule xilinx.com:bd_rule:clkrst -config {Clk "/processing_system7_0/FCLK_CLK0 (100 MHz)" }  [get_bd_pins VGA_DMA_IP_0/s00_axis_aclk]

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config { Clk_master {Auto} Clk_slave {Auto} Clk_xbar {Auto} Master {/processing_system7_0/M_AXI_GP0} Slave {/axi_dma_0/S_AXI_LITE} intc_ip {New AXI Interconnect} master_apm {0}}  [get_bd_intf_pins axi_dma_0/S_AXI_LITE]
endgroup


#add constraint file
add_files -fileset constrs_1 -norecurse Top.xdc

#validating design
validate_bd_design
#Creating hdl wrapper
make_wrapper -files [get_files $resultDir/VGA_DMA_controller.srcs/sources_1/bd/VGA_DMA_controller/VGA_DMA_controller.bd] -top
add_files -norecurse $resultDir/VGA_DMA_controller.srcs/sources_1/bd/VGA_DMA_controller/hdl/VGA_DMA_controller_wrapper.v
#running synthesis and implementation
launch_runs impl_1 -to_step write_bitstream -jobs 4

#exporting hardware
wait_on_run impl_1
update_compile_order -fileset sources_1
file mkdir $resultDir/VGA_DMA_controller.sdk
file copy -force $resultDir/VGA_DMA_controller.runs/impl_1/VGA_DMA_controller_wrapper.sysdef $resultDir/VGA_DMA_controller.sdk/VGA_DMA_controller_wrapper.hdf
