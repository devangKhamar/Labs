/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Author: Devang Khamar
// Title: 8-bit (signed) booth multiplier.
// 
// Description: A 16-bit signed shift-add multiplier based on booth recoding logic. Design contains assertions that are 
//				parsed and verified by the Enhanced Bounded Model Checking tool.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// booth module:
//
// Top level File for Booth Multiplier. Implemented as a Finite State Machine with Datapath. Uses recodeLogic module to 
// recode multiplier bits into booth's notation at runtime. The recoding does not generate a new multiplier operand, but 
// instead generates signals that guide (multiplex) data flow on the data-path. This module also uses a 18-bit 
// adder/subtractor module.  
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


module booth(output [31:0] product,
		output busy,
		input [15:0] multiplier,
		input [15:0] multiplicand,
		input clk,
		input reset,
		input start_pulse);

	reg [15:0] m1, mplr,ma,mb;
	reg [15:0] m2, mpld ;
	reg [33:0] prod, product_reg; //34-bit to accomodate extra symbol
	reg [1:0] curr_state, next_state;
	reg init_reg, ireg;
	wire [2:0] window;
	reg status, stat_reg;
	reg [2:0]counter, nxt_cnt;
	
	//signals that probe required outputs
	
	wire [31:0] prod_inv,probe_1,probe_2,probe_3,probe_4;
	wire[15:0] inv_a, inv_b ;
	
	
	wire [17:0] op;
	wire skip, double, add_sub;
	wire [17:0] op_1,op_2;
	
	parameter start = 3'd0, add = 3'd1, shift = 3'd2, finish = 3'd3, min = 2'd0;
	
	
	// window samples and operand modification for the adder mdoule
	assign window = {mplr[1:0],init_reg};			// assign last two bits of multiplier to the window and an bit for previous MSB of window (initially zero).
	
	assign op_1 = product_reg[33:16];
	
	assign op_2 = double ? {mpld[15],mpld,1'b0} : {mpld[15],mpld[15],mpld}; //sign extend the multiplicand to make it 10-bits and double the value if needed.
	
	recodeLogic rd (add_sub, skip, double, window);
	add_subtract as1(op , op_1 , op_2, add_sub);
	
	always@(posedge clk, posedge reset)
		begin
			curr_state 	<= 	reset ? start 	: next_state;
			counter    	<= 	reset ? 3'd7	: nxt_cnt;
			mplr	   	<= 	reset ? 16'b0	: m1;
			mpld		<=	reset ? 16'b0	: m2;
			product_reg <=	reset ? 34'b0	: prod;
			stat_reg	<= 	reset ? 1'b0	: status;
			init_reg 	<=  reset ? 1'b0	: ireg;
			
			ma	   		<= 	reset ? 16'b0	: curr_state == start && status ? m1 : ma;
			mb			<=	reset ? 16'b0	: curr_state == start && status ? m2 : mb;
		end
		
	//FSMD datapath		
	always@*
		begin
		next_state = curr_state;
		nxt_cnt = counter; 
		
		m1 = mplr;
		m2 = mpld;
		prod = product_reg;
		ireg = init_reg;
		
		status = stat_reg;
		case(curr_state)
			start: begin
					if(start_pulse)											//if a start pulse recieved, begin computation.
						begin
							m1 = multiplier;
							m2 = multiplicand;
							status = 1'b1;									// set status bit to 1 => Module busy.
							prod = 34'b0;									// clear partial-product register.
							next_state = add;
							ireg = 1'b0;
						end
					else
						begin next_state = start; end
					end
			add:
				begin
					next_state 	= shift;
					prod [33:16]	= skip ? product_reg[33:16]: op;
				end
			shift:
				begin
					next_state 	= counter == min ? finish : add;							// a counter is used to keep track of partial-product cycles.
					prod 		= {product_reg[33], product_reg[33], product_reg[33:2]};	// right shift partial-product register by two.
					m1 			= {mplr[15], mplr[15], mplr[15:2]};							// right shift multiplier by two units.
					ireg 		=  mplr[1];													// overflow the multiplier bit to window.
					nxt_cnt 	= counter - 3'b1 ;
				end
			finish:
				begin
					status 		= 1'b0;
					nxt_cnt 	= 3'd7;
					next_state 	= start;
					m1 			= 16'b0;
					m2 			= 16'b0;
				end
			default:
				begin next_state = start; end
		endcase
		end
		
		assign product = product_reg[31:0];
		assign busy = stat_reg;	
		
		assign prod_inv = ~product + 32'b1;
		assign probe_1 = {16'b0,ma} * {16'b0,mb};
		assign probe_2 = {16'b0,mb} * {16'b0,~ma + 16'b1};
		assign probe_3 = {16'b0,ma} * {16'b0,~mb + 16'b1};
		assign probe_4 = {16'b0,inv_a} * {16'b0, inv_b};
		assign inv_a = ~ma + 16'b1;
		assign inv_b = ~mb + 16'b1;		
	
		//Check whether reset always goes to start state
		c1: assert property (reset |=> curr_state == start);
		
		//Check whether a start_pulse always sets the status_reg to 1
		c2: assert property (!reset && curr_state == 1 && start_pulse == 1 |=> stat_reg == 1);
		
		//if the multiplier recieves a start_pulse in start state only then will it begin compuation (move to add state)
		c3: assert property (!reset && stat_reg == 1 && curr_state == start |=> curr_state == add);
		
		//checks the most important property, whether or not multiplication is ALWAYS correct
		c4a: assert property (!reset && curr_state == finish && product_reg[33:32] == 2'b00 |->  (product == probe_1 || product == probe_4));
		c4b: assert property (!reset && curr_state == finish && product_reg[33:32] == 2'b11 |->	 (prod_inv == probe_2 || prod_inv == probe_3));
		
		//liveness properties:
		
		//if multiplier is initialized for computation, (curr_state == start && stat_reg == 1) then it will eventually finish
		l1: assert property (!reset && curr_state == start && stat_reg == 1 |=> ##[1:$] next_state == finish);
		//if multiplier is initialized for computation, (curr_state == start && stat_reg == 1) then it will eventually reach add state
		l2: assert property (!reset && curr_state == start && stat_reg == 1 |=> ##[1:$] next_state == add);
		//if multiplier is initialized for computation, (curr_state == start && stat_reg == 1) then it will eventually reach shift state
		l3: assert property (!reset && curr_state == start && stat_reg == 1 |=> ##[1:$] next_state == shift);
		
		//saftey property:
		
		//If system is not computing => counter must stay at reset value (3'd7)
		s1: assert property (!reset && !stat_reg |=> counter == 3'd7);
		
		
endmodule

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	add_subtract module:
//  
//	A simple 18-bit adder/!subtractor combinational circuit. It performs subtraction by computing two's complement of the operand and
//	feeds it to the adder. It recieves the directive to add/subtract from the recodeLogic block. Operand_1 is always the computed
// 	partial product, whereas operand_2 can be the multiplicand or it's doubled value.  
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

module add_subtract (output [17:0] result, input [17:0] operand_1, input [17:0] operand_2, input add_sub);
	
	wire [17:0]op_2;
	
	assign op_2 = add_sub ? ~operand_2 + 18'b1: operand_2;
	assign result = operand_1 + op_2;

	//checks whether circuit performs subtraction when add_sub is set to 1 (add_sub == add/!sub)
	c1: assert property (~add_sub |-> (result == operand_1 + operand_2));
	
	//checks whether circuit performs addition when add_sub is set to 0  
	c2: assert property ( add_sub |-> (result == operand_1 - operand_2));
	
endmodule

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 	recodeLogic module:
//  
//  This is a combinational circuit that performs booth recoding and tells the shift-add multiplier when to double the multiplicand,
//  whether to add or subtract the multiplicand from the partial product and when to skip addition. It takes as input three bits 
//  relative to the the previous, current and next bit positions of the multiplier, to compute these signals. 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

module recodeLogic(output add_sub,
					output skip,
					output double,
				input [2:0] window);
	//sign extension
	assign add_sub = window[2];
	assign skip = (window[2]&window[1]&window[0]) | (~(window[2] | window[1] | window[0]));
	assign double = (~window[2])&(window[1] & window[0]) | ((window[2]) & ((~window[1]) & (~window[0])));
	
	//safety property
	//ensures that if the current window requires doubling of input operand, a concurrent skip command is never set.	
	s1: assert property (double |-> !skip);
		
endmodule
