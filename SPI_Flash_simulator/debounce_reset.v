// debouncer for a push button

module debounce_reset 
(
	input clk, 	// the system clock
	input noisy,	// the button signal to clean 
	output clean	// the cleaned output
);

parameter NDELAY = 650000; // 650000 * 20ns(50MHz) = 13ms
parameter NBITS = 20; // enough depth to fit decimal 650000 and under

reg [NBITS-1:0] count;	// the counter value
reg xnew;
reg cleansig;

always @(posedge clk)
begin
	
	// state has changed so start debouncing
	if (noisy != xnew) 
	begin 
		xnew <= noisy;
		count <= 0; 
	end
	
	// output the state of noisy after debounce delay
	else if (count == NDELAY)
	begin
		cleansig <= xnew;
	end

	// not at max, keep counting
	else 
	begin
		count <= count + 1;
	end
end

// wire out the clean signal
assign clean = cleansig;
endmodule
