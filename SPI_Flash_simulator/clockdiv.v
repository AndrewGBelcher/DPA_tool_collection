// Simple clock divider

module clock_div
(
	input clk_in,	// clock signal to be divided
	output clk_out	// divided clock signal out
);

reg[27:0] counter = 0;	
parameter DIVISOR = 2;

// The frequency of the output clk_out
//  = The frequency of the input clk_in divided by DIVISOR
// For example: Fclk_in = 50Mhz, if you want to get 1Hz signal to blink LEDs
// You will modify the DIVISOR parameter value to 28'd50.000.000
// Then the frequency of the output clk_out = 50Mhz/50.000.000 = 1Hz

always @(posedge clk_in)
begin
	counter <= counter + 1;

	if(counter >= (DIVISOR-1))
	begin
		counter <= 0;
	end
end

// clock out signal is high half of counters range
assign clk_out = (counter < DIVISOR/2) ? 0 : 1;
endmodule

