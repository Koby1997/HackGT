`timescale 1ns / 1ps
/*
 * This software is Copyright (c) 2018 Denis Burykin
 * [denis_burykin yahoo com], [denis-burykin2014 yandex ru]
 * and it is hereby released to the general public under the following terms:
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 */

module inpkt_config #(
	parameter SUBTYPE1_WIDTH = 32
	)(
	input CLK,
	input [7:0] din,
	input wr_en, pkt_end,
	output full,

	output reg [SUBTYPE1_WIDTH-1:0] dout1 = 0,
	// unsupported subtype, bad length for this subtype
	output reg err = 0
	);

	assign full = 0;

	// Data length in bytes for subtype 1
	localparam SUBTYPE1_N_BYTES = SUBTYPE1_WIDTH[2:0] == 0
		? SUBTYPE1_WIDTH / 8 : (SUBTYPE1_WIDTH + 1) / 8;

	reg [`MSB(SUBTYPE1_N_BYTES) :0] cnt = 0;
	
	localparam STATE_SUBTYPE = 0,
				STATE_DATA1 = 1;

	(* FSM_EXTRACT="true" *)
	reg [1:0] state = STATE_SUBTYPE;

	always @(posedge CLK) if (~err & wr_en)
		case(state)
		STATE_SUBTYPE: begin
			if (din != 1)
				err <= 1;
			state <= STATE_DATA1;
		end
		
		// Subtype 1: SUBTYPE1_WIDTH-bit value
		STATE_DATA1: if (pkt_end) begin
			if (cnt != SUBTYPE1_N_BYTES)
				err <= 1;
			cnt <= 0;
			state <= STATE_SUBTYPE;
		end
		else begin
			dout1 [8*cnt +:8] <= din;
			cnt <= cnt + 1'b1;
			if (cnt == SUBTYPE1_N_BYTES)
				err <= 1;
			state <= STATE_DATA1;
		end

		endcase

endmodule
