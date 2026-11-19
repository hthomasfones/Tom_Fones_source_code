`timescale 1ns/1ps

module tb_mad_device_mem;

    reg clk = 1'b0;
    always #5 clk = ~clk;

    reg          wr_en   = 1'b0;
    reg [15:0]   wr_addr = 16'd0;
    reg [127:0]  wr_data = 128'd0;

    reg          rd_en   = 1'b0;
    reg [15:0]   rd_addr = 16'd0;
    wire [127:0] rd_data;

    localparam [127:0] TEST_PATTERN =
        128'h00112233445566778899AABBCCDDEEFF;

    mad_device_mem dut (
        .clk(clk),
        .wr_en(wr_en),
        .wr_addr(wr_addr),
        .wr_data(wr_data),
        .rd_en(rd_en),
        .rd_addr(rd_addr),
        .rd_data(rd_data)
    );
    
    // Monitor UltraRAM output after every rising clock edge.
    always @(posedge clk) begin
        #1;
        $display("T=%0t rd_en=%b rd_addr=%h rd_data=%032h",
                 $time, rd_en, rd_addr, rd_data);
    end

    initial begin
        $display("Starting UltraRAM read latency test");

        // Write known data to address 0x0010.
        @(negedge clk);
        wr_addr = 16'h0010;
        wr_data = TEST_PATTERN;
        wr_en   = 1'b1;

        @(negedge clk);
        wr_en = 1'b0;

        // Issue one read request.
        @(negedge clk);
        rd_addr = 16'h0010;
        rd_en   = 1'b1;

        @(negedge clk);
        rd_en = 1'b0;

        repeat (5) begin
            @(posedge clk);
            #1;
            $display("Time=%0t rd_data=%032h",
                     $time, rd_data);
        end

        if (rd_data === TEST_PATTERN)
            $display("PASS: Read data matches");
        else
            $display("FAIL: Read data mismatch");

        $finish;
    end

endmodule
