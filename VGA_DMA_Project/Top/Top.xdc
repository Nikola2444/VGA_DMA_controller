##LEDs
set_property -dict { PACKAGE_PIN M14   IOSTANDARD LVCMOS33 } [get_ports { led_o[0] }]; #IO_L23P_T3_35 Sch=LED0
set_property -dict { PACKAGE_PIN M15   IOSTANDARD LVCMOS33 } [get_ports { led_o[1] }]; #IO_L23N_T3_35 Sch=LED1
set_property -dict { PACKAGE_PIN G14   IOSTANDARD LVCMOS33 } [get_ports { led_o[2] }]; #IO_0_35=Sch=LED2
set_property -dict { PACKAGE_PIN D18   IOSTANDARD LVCMOS33 } [get_ports { led_o[3] }]; #IO_L3N_T0_DQS_AD1N_35 Sch=LED3

##Switches

set_property -dict { PACKAGE_PIN G15   IOSTANDARD LVCMOS33 } [get_ports { switch_i[0] }]; #IO_L19N_T3_VREF_35 Sch=SW0
set_property -dict { PACKAGE_PIN P15   IOSTANDARD LVCMOS33 } [get_ports { switch_i[1] }];  #IO_L24P_T3_34 Sch=SW1
set_property -dict { PACKAGE_PIN W13   IOSTANDARD LVCMOS33 } [get_ports { switch_i[2] }]; #IO_L4N_T0_34 Sch=SW2
set_property -dict { PACKAGE_PIN T16   IOSTANDARD LVCMOS33 } [get_ports { switch_i[3] }]; #IO_L9P_T1_DQS_34 Sch=SW3

##Buttons
set_property -dict { PACKAGE_PIN R18   IOSTANDARD LVCMOS33 } [get_ports { button_i[0] }]; #IO_L20N_T3_34 Sch=BTN0
set_property -dict { PACKAGE_PIN P16   IOSTANDARD LVCMOS33 } [get_ports { button_i[1] }]; #IO_L24N_T3_34 Sch=BTN1
set_property -dict { PACKAGE_PIN V16   IOSTANDARD LVCMOS33 } [get_ports { button_i[2] }]; #IO_L18P_T2_34 Sch=BTN2
set_property -dict { PACKAGE_PIN Y16   IOSTANDARD LVCMOS33 } [get_ports { button_i[3] }]; #IO_L7P_T1_34 Sch=BTN3

##VGA Connector
##IO_L7P_T1_AD2P_35
set_property PACKAGE_PIN M19 [get_ports {RGB_Out[11]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[11]}]

##IO_L9N_T1_DQS_AD3N_35
set_property PACKAGE_PIN L20 [get_ports {RGB_Out[12]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[12]}]

##IO_L17P_T2_AD5P_35
set_property PACKAGE_PIN J20 [get_ports {RGB_Out[13]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[13]}]

##IO_L18N_T2_AD13N_35
set_property PACKAGE_PIN G20 [get_ports {RGB_Out[14]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[14]}]

##IO_L15P_T2_DQS_AD12P_35
set_property PACKAGE_PIN F19 [get_ports {RGB_Out[15]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[15]}]

##IO_L14N_T2_AD4N_SRCC_35
set_property PACKAGE_PIN H18 [get_ports {RGB_Out[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[5]}]

##IO_L14P_T2_SRCC_34
set_property PACKAGE_PIN N20 [get_ports {RGB_Out[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[6]}]

##IO_L9P_T1_DQS_AD3P_35
set_property PACKAGE_PIN L19 [get_ports {RGB_Out[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[7]}]

##IO_L10N_T1_AD11N_35
set_property PACKAGE_PIN J19 [get_ports {RGB_Out[8]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[8]}]

##IO_L17N_T2_AD5N_35
set_property PACKAGE_PIN H20 [get_ports {RGB_Out[9]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[9]}]

##IO_L15N_T2_DQS_AD12N_35
set_property PACKAGE_PIN F20 [get_ports {RGB_Out[10]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[10]}]

##IO_L14N_T2_SRCC_34
set_property PACKAGE_PIN P20 [get_ports {RGB_Out[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[0]}]

##IO_L7N_T1_AD2N_35
set_property PACKAGE_PIN M20 [get_ports {RGB_Out[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[1]}]

##IO_L10P_T1_AD11P_35
set_property PACKAGE_PIN K19 [get_ports {RGB_Out[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[2]}]

##IO_L14P_T2_AD4P_SRCC_35
set_property PACKAGE_PIN J18 [get_ports {RGB_Out[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[3]}]

##IO_L18P_T2_AD13P_35
set_property PACKAGE_PIN G19 [get_ports {RGB_Out[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {RGB_Out[4]}]

##IO_L13N_T2_MRCC_34
set_property PACKAGE_PIN P19 [get_ports Hsync]
set_property IOSTANDARD LVCMOS33 [get_ports Hsync]

##IO_0_34
set_property PACKAGE_PIN R19 [get_ports Vsync]
set_property IOSTANDARD LVCMOS33 [get_ports Vsync]
