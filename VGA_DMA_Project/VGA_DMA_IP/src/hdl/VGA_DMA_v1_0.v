
`timescale 1 ns / 1 ps

	module VGA_DMA_v1_0 #
	(
		// Users to add parameters here

		// User parameters ends
		// Do not modify the parameters beyond this line

		// Parameters of Axi Slave Bus Interface S00_AXIS
		parameter integer C_S00_AXIS_TDATA_WIDTH	= 32
	)
	(
		// Users to add ports here
        output wire hsync,
        output wire vsync,
        output wire [15 : 0]rgb, 
		// User ports ends
		// Do not modify the ports beyond this line

        
		// Ports of Axi Slave Bus Interface S00_AXIS
		input wire  s00_axis_aclk,
		input wire  s00_axis_aresetn,
		output wire  s00_axis_tready,
		input wire [C_S00_AXIS_TDATA_WIDTH-1 : 0] s00_axis_tdata,
		input wire [(C_S00_AXIS_TDATA_WIDTH/8)-1 : 0] s00_axis_tstrb,
		input wire  s00_axis_tlast,
		input wire  s00_axis_tvalid
	);
// Instantiation of Axi Bus Interface S00_AXIS
    wire [15:0] VGA_SDATA;
    wire VGA_SREADY;
    wire VGA_SVALID;
	VGA_DMA_AXIS # ( 
		.C_S_AXIS_TDATA_WIDTH(C_S00_AXIS_TDATA_WIDTH)
	) VGA_DMA_AXIS_inst (
		.S_AXIS_ACLK(s00_axis_aclk),
		.S_AXIS_ARESETN(s00_axis_aresetn),
		.S_AXIS_TREADY(s00_axis_tready),
		.S_AXIS_TDATA(s00_axis_tdata),
		.S_AXIS_TSTRB(s00_axis_tstrb),
		.S_AXIS_TLAST(s00_axis_tlast),
		.S_AXIS_TVALID(s00_axis_tvalid),
		.VGA_SREADY(VGA_SREADY),
		.VGA_SVALID(VGA_SVALID),
		.VGA_SDATA(VGA_SDATA)
	);
	
	VGA #(
	     )
	     VGA_inst(.clk(s00_axis_aclk),
	              .reset(s00_axis_aresetn),
	              .rgb(rgb),
	              .sdata(VGA_SDATA),
	              .svalid(VGA_SVALID),
	              .sready(VGA_SREADY),
	              .hsync(hsync),
	              .vsync(vsync));
	

	// Add user logic here

	// User logic ends

	endmodule
