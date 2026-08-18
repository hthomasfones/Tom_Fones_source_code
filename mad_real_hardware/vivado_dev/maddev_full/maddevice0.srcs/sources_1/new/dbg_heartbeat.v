module dbg_heartbeat (
    input  wire clk,
    output wire [3:0] heartbeat
);

    reg [31:0] ctr = 32'd0;

    always @(posedge clk) begin
        ctr <= ctr + 1'b1;
    end

    assign heartbeat = ctr[13:10]; // 4 bits, increasing speed
endmodule
