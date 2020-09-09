// Author: Andrew Belcher
//
// Description: SPI flash hardware emulation for Differential Power Analysis based attacks.
// Outputs a differntial clock pair signal for an ARM m3 device
// Uses a pattern on UART for patching a section of an IPL header within a SPI flash image
// Features a manual push button for driving a reset hammer on the target board for testing
//
// *Quad SPI mode not supported*


// Top level module
module spi_sim 
#( 
	parameter WIDTH = 3, // Width of the register required
	parameter N = 1,// We will divide by 12 for example in this case
	parameter PATCH_OFFSET = 16'h4230
)

(
	input  clk,			// system clock
	input  push_button,	// reset button to be output as the hammer
	output tru_clk, 	// primary clock signal output for differntial pair clock signal for target
	output comp_clk,	// complementary clock signal output for differntial pair clock signal for target
	output trigger_out,	// output trigger for oscilloscope capture
	output reset_out,		// reset hammer for the target device
	output tx_out,		// UART tx for host software capturing traces
	input  rx_in,		// UART rx for host software capturing traces, for updating internal flash(window)

	// SPI signals
	input  spi_reset,	
	input  spi_cs,
	input  spi_clk,
	input  spi_sio0,
	output spi_sio1
);

reg [30:0]count = 0;
reg [WIDTH-1:0]div_value;
wire [WIDTH-1:0]next;
reg div_clk;


// output buffer for differntial clock pair
OBUFDS 
#(
	.IOSTANDARD("lvds_25")
) 

// output instantiation of the output buffer 
OBUFDS_inst 
(
	.O(tru_clk),
	.OB(comp_clk),
	.I(div_clk)	// turn divided system clock into differntial clock signal
);

wire reset_sig; 		// output of reset signal cleaned
reg reset_state = 1;	// state for holding reset hammer
reg trigger_set = 1;	// state for holding trigger signal

// serial
reg	serial_reset = 1;	// reset signal for uart modules
reg	[7:0]tx_data_out;	// uart reg tx
wire [7:0]rx_data_in;	// uart reg rx
reg [7:0]rx_data;		// rx buffer register for state control
reg tx_send = 0;		// send control signal for tx
wire rx_done_sig;		// uart done signal
wire tx_done_sig;		// uart done signal
reg	rx_done;			// uart done state
reg tx_done;			// uart done state
reg recv_bytes = 0;		// 

// spi
wire [7:0]spi_rx_data;
reg spi_tx_strobe;
wire spi_rx_strobe;
reg [7:0] flash_data;
reg [31:0]read_addr;
reg spi_cs_buf;
reg spi_cs_prev;
reg spi_cs_sync;
reg read_in_progress;
reg [12:0]bytes;
reg [3:0]sig_cnt = 0;
reg sig_checked = 0;
reg ipl_read = 0;

// patch window
reg [127:0]patch;
reg [7:0]patch0;
reg [7:0]patch1;
reg [7:0]patch2;
reg [7:0]patch3;
reg [7:0]patch4;
reg [7:0]patch5;
reg [7:0]patch6;
reg [7:0]patch7;
reg [7:0]patch8;
reg [7:0]patch9;
reg [7:0]patch10;
reg [7:0]patch11;
reg [7:0]patch12;
reg [7:0]patch13;
reg [7:0]patch14;
reg [7:0]patch15;
reg [33:0]patch_index = 0;


// map flash image to block rom for simulation
(* ram_style = "block" *) reg [7:0] flash_rom[0:16'h43ff];
initial $readmemb("flash.mif", flash_rom,0,16'h43ff);


// button signal debouncer
debounce_reset debounce_reset
(
	.clk		(clk),
	.noisy		(push_button),
	.clean		(reset_sig)
);

// uart rx module
uart_rx	uart_rx_inst
(
	.clk_s		(clk),		
	.rstn_s		(serial_reset),
	.iDATA		(rx_in),
	.oDATA		(rx_data_in),
	.oDONE		(rx_done_sig)
);

// uart tx module
uart_tx	uart_tx_inst
(
	.clk_s		(clk),
	.rstn_s		(serial_reset),
	.iSEND		(tx_send),
	.iDATA		(tx_data_out),
	.oDATA		(tx_out),
	.oFINISH	(tx_done_sig)	
);

// spi io module 
spi_device spiflash(
	.mclk(clk),
	//.reset(spi_reset),
	.spi_cs(spi_cs),
	.spi_clk(spi_clk),
	.spi_mosi(spi_sio0),
	.spi_miso_out(spi_sio1),
	.spi_tx_strobe(spi_tx_strobe),
	.spi_tx_data(flash_data),
	.spi_rx_data(spi_rx_data),
	//s.tx_done(tx_done_sig),
	.spi_rx_strobe(spi_rx_strobe)
	);




always@(posedge clk )
begin

	spi_cs_buf <= spi_cs;
	spi_cs_prev <= spi_cs_buf;
	spi_cs_sync <= spi_cs_prev;


	rx_done <= rx_done_sig;
	rx_data <= rx_data_in;
	
	// trigger capture at end of ipl read in
	if(read_addr > 32'h4DE70)
	begin
		trigger_set <= 0;		
	end
	else
	begin
		trigger_set <= 1;
	end
	
	
	if(!spi_cs_sync && spi_cs_prev)
	begin
		bytes <= 0;
		read_in_progress <= 0;
		read_addr <= 0;
		spi_tx_strobe <= 0;
	end
	
	
	if(spi_cs_sync && !spi_cs_prev)
	begin
		read_in_progress <= 0;
		//spi_tx_strobe <= 0;
	end
	
	
	if(spi_rx_strobe)
	begin
		if(spi_rx_data == 5)
		begin
			flash_data <= 8'h40;
			spi_tx_strobe <= 1;
		end
		
		else if(sig_checked == 1)
		begin
			if(sig_cnt < 2)
			begin
				case(sig_cnt)
				0: flash_data <= 8'h20;
				1: flash_data <= 8'h19;
				endcase
				sig_cnt = sig_cnt + 1;
				spi_tx_strobe <= 1;
			end
			
			else
			begin
				sig_checked = 0;
				spi_tx_strobe <= 0;
			end
		end
		
		else if(bytes == 0 && spi_rx_data == 3)
		begin
			read_in_progress <= 1;
		end
		
		
		else if(bytes <= 1 && read_in_progress == 1)
		begin
			case(bytes)
			0:	read_addr[7:0] <= spi_rx_data;
			1:	read_addr[15:8] <= spi_rx_data;
			//2:	read_addr[23:16] <= spi_rx_data;
			endcase
			bytes <= bytes + 1;
		end
		
				
		else if(bytes > 1 && read_in_progress == 1)
		begin
			if(read_addr >= PATCH_OFFSET)
			begin
				ipl_read = 1;

				if(read_addr <= PATCH_OFFSET + 15)
				begin
					case(read_addr)
						PATCH_OFFSET: flash_data <= patch0;
						PATCH_OFFSET + 1: flash_data <= patch1;
						PATCH_OFFSET + 2: flash_data <= patch2;
						PATCH_OFFSET + 3: flash_data <= patch3;
						PATCH_OFFSET + 4: flash_data <= patch4;
						PATCH_OFFSET + 5: flash_data <= patch5;
						PATCH_OFFSET + 6: flash_data <= patch6;
						PATCH_OFFSET + 7: flash_data <= patch7;
						PATCH_OFFSET + 8: flash_data <= patch8;
						PATCH_OFFSET + 9: flash_data <= patch9;
						PATCH_OFFSET + 10: flash_data <= patch10;
						PATCH_OFFSET + 11: flash_data <= patch11;
						PATCH_OFFSET + 12: flash_data <= patch12;
						PATCH_OFFSET + 13: flash_data <= patch13;
						PATCH_OFFSET + 14: flash_data <= patch14;
						PATCH_OFFSET + 15: flash_data <= patch15;			
					endcase				

					
					spi_tx_strobe <= 1;
					read_addr <= read_addr + 1;				
				end
				else
				begin
					flash_data <= flash_rom[read_addr[23:0]];
					spi_tx_strobe <= 1;
					read_addr <= read_addr + 1;
					//bytes <= bytes + 1;
				end
			end
		
			else
			begin
				flash_data <= flash_rom[read_addr[23:0]];
				spi_tx_strobe <= 1;
				read_addr <= read_addr + 1;
				//bytes <= bytes + 1;
			end
		end

		
		
		else if(spi_rx_data == 8'h9f)//sig check
		begin

		
			sig_cnt = 0;
			flash_data <= 8'hc2;//0xc2		
			spi_tx_strobe <= 1;
			sig_checked = 1;
		end
		
		else
		begin
				spi_tx_strobe <= 0;
		end
	end
	
	// start of patch data update
	if(rx_data == 8'h3a && recv_bytes == 0)// when : is recieved
	begin
			patch_index = 0;
			recv_bytes = 1;
	end

	// for each byte in the uart data for patch
	if(rx_data && recv_bytes == 1) // when we are exepecting patch bytes
	begin
			
			case(patch_index)
				0:;
				1: patch0 <= rx_data;
				2: patch1 <= rx_data;
				3: patch2 <= rx_data;
				4: patch3 <= rx_data;
				5: patch4 <= rx_data;
				6: patch5 <= rx_data;
				7: patch6 <= rx_data;
				8: patch7 <= rx_data;
				9: patch8 <= rx_data;
				10: patch9 <= rx_data;
				11: patch10 <= rx_data;
				12: patch11 <= rx_data;
				13: patch12 <= rx_data;
				14: patch13 <= rx_data;
				15: patch14 <= rx_data;
				16: patch15 <= rx_data;
			endcase				
			patch_index = patch_index + 1;
				//recv_bytes <= 0;
	end

	// reset count after receiving all 0x10 bytes for the patch data
	if(patch_index > 16)
	begin
		recv_bytes = 0;
	end
	
	// if rx byte is ! set count to 0 to reset the target/load flash
	if(rx_data == 8'h21 && recv_bytes == 0) 
	begin
	//	patch_index = patch_index + 1;
	//	tx_send <= 1;
		count = 0;
	end
	

	// & byte to reset patch index for audit
	if(rx_data == 8'h26 && recv_bytes == 0) // reset patch index = &
	begin
		patch_index = 0;
	end	

	// useful for debug, print the patch via cmd *, increments every detection
	if(rx_data == 8'h2a && recv_bytes == 0)
	begin
		case(patch_index)
			0: tx_data_out <= patch0;
			1: tx_data_out <= patch1;
			2: tx_data_out <= patch2;
			3: tx_data_out <= patch3;
			4: tx_data_out <= patch4;
			5: tx_data_out <= patch5;
			6: tx_data_out <= patch6;
			7: tx_data_out <= patch7;
			8: tx_data_out <= patch8;
			9: tx_data_out <= patch9;
			10: tx_data_out <= patch10;
			11: tx_data_out <= patch11;
			12: tx_data_out <= patch12;
			13: tx_data_out <= patch13;
			14: tx_data_out <= patch14;
			15: tx_data_out <= patch15;
		endcase				
		patch_index = patch_index + 1;		
		tx_send <= 1;
	end		
		

	else
	begin
		tx_send <= 0;
	end



	// reset state control
	if(reset_sig == 1)
	begin
		ipl_read = 0;
		count = 0;
	end

	if(reset_sig == 0)
	begin
		count = count +1;
	end
		
	if(count > 1109999*5)
	begin
		reset_state <= 1;
	end
	
	// make count max
	if (count > 31'h7FFFFFFF)
	begin
		count = 31'h7FFFFFFF;
	end

	if(count < 20000) 
	begin
		reset_state <= 0;
	end
end


// drive clock
always@(posedge clk)
begin

	if (next == 2)// how many cycles to run before inverting clock signal/ that means full clk = half of expected
 	begin
	     div_value <= 0; // reset to 0
	     div_clk <= ~div_clk; // invert signal
	end
	
	else 
	begin
		div_value <= next; // increment
	end
end

assign next = div_value + 1; // increment
assign rst_btn = reset_state;
assign trigger_out = !trigger_set ;


endmodule