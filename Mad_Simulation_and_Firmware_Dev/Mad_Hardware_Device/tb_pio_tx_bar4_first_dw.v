`timescale 1ns/1ps
// Focused test of the actual pio_tx_engine inside pcie_device_portal.v.
// No PCIe hard IP or UltraRAM model: BAR4 data_valid/data are injected
// with a representative delay after req_compl_wd.
module tb_pio_tx_bar4_first_dw;
    reg clk = 0;
    always #2 clk = ~clk;  // 250 MHz
    reg reset_n = 0;
    reg req_compl_wd = 0;
    reg bar4_pio_data_valid = 0;
    reg s_axis_cc_tready = 1;
    reg [511:0] rd_data = 0;
    reg [22:0] req_addr = 23'h400000; // BAR4 region, DWORD offset 0
    wire [127:0] s_axis_cc_tdata;
    wire [3:0] s_axis_cc_tkeep;
    wire s_axis_cc_tlast, s_axis_cc_tvalid, compl_done;
    wire [32:0] s_axis_cc_tuser;
    integer cycles, accepted, completed;
    integer errors = 0;
    reg [127:0] held_data;
    reg [3:0] held_keep;
    reg held_last;
    reg [32:0] held_user;
    reg checking_stall;
    reg [31:0] expected_dw;

    pio_tx_engine #(
        .AXISTEN_IF_WIDTH(2'b01),
        .C_DATA_WIDTH(128),
        .MEM_W(512),
        .ADDR_W(5),
        .BYTE_EN_W(64),
        .REQ_ADDR_WIDTH(23),
        .TCQ(1)
    ) dut (
        .user_clk(clk), .reset_n(reset_n),
        .s_axis_cc_tdata(s_axis_cc_tdata),
        .s_axis_cc_tkeep(s_axis_cc_tkeep),
        .s_axis_cc_tlast(s_axis_cc_tlast),
        .s_axis_cc_tvalid(s_axis_cc_tvalid),
        .s_axis_cc_tuser(s_axis_cc_tuser),
        .s_axis_cc_tready(s_axis_cc_tready),
        .s_axis_rq_tready(1'b1),
        .cfg_msg_transmit_done(1'b0),
        .pcie_rq_tag(6'd0), .pcie_rq_tag_vld(1'b0),
        .pcie_tfc_nph_av(2'd0), .pcie_tfc_npd_av(2'd0),
        .pcie_tfc_np_pl_empty(1'b1),
        .pcie_rq_seq_num(4'd0), .pcie_rq_seq_num_vld(1'b0),
        .cfg_fc_ph(8'd0), .cfg_fc_nph(8'd0), .cfg_fc_cplh(8'd0),
        .cfg_fc_pd(12'd0), .cfg_fc_npd(12'd0), .cfg_fc_cpld(12'd0),
        .req_compl(1'b0), .req_compl_wd(req_compl_wd),
        .req_compl_ur(1'b0), .payload_len(11'd1),
        .compl_done(compl_done),
        .req_tc(3'd0), .req_td(1'b0), .req_ep(1'b0),
        .req_attr(3'd0), .req_len(11'd1),
        .req_rid(16'h1234), .req_tag(8'h56),
        .req_be(8'h0f), .req_addr(req_addr), .req_at(2'b00),
        .completer_id(16'd0),
        .req_des_qword0(64'd0), .req_des_qword1(64'd0),
        .req_des_tph_present(1'b0), .req_des_tph_type(2'd0),
        .req_des_tph_st_tag(8'd0),
        .req_mem_lock(1'b0), .req_mem(1'b1),
        .rd_data(rd_data),
        .bar4_pio_read(1'b1),
        .bar4_pio_data_valid(bar4_pio_data_valid),
        .gen_transaction(1'b0),
        .bufrd_rq_start(1'b0), .bufrd_rq_addr(64'd0),
        .bufrd_rq_data(128'd0),
        .bufwr_rq_start(1'b0), .bufwr_rq_addr(64'd0)
    );

    always @(posedge clk) begin
        if (!reset_n) begin
            accepted <= 0;
            completed <= 0;
        end else begin
            if (s_axis_cc_tvalid && s_axis_cc_tready) begin
                accepted <= accepted + 1;
                $display("CC accepted t=%0t data=%032h keep=%h last=%b", $time,
                         s_axis_cc_tdata, s_axis_cc_tkeep, s_axis_cc_tlast);
                if (!s_axis_cc_tlast || s_axis_cc_tkeep !== 4'hf ||
                    s_axis_cc_tdata[127:96] !== expected_dw) begin
                    $display("FAIL: unexpected first-DWORD completion");
                    errors = errors + 1;
                    $display("TEST ABORT: completion failure");
                    $finish;
                end
            end
            if (compl_done) completed <= completed + 1;
            if (checking_stall && !s_axis_cc_tready) begin
                if (!s_axis_cc_tvalid || s_axis_cc_tdata !== held_data ||
                    s_axis_cc_tkeep !== held_keep ||
                    s_axis_cc_tlast !== held_last ||
                    s_axis_cc_tuser !== held_user) begin
                    $display("FAIL: CC outputs changed during backpressure");
                    errors = errors + 1;
                    $display("TEST ABORT: completion failure");
                    $finish;
                end
            end
        end
    end

    task tick;
        begin @(posedge clk); #1; end
    endtask

    task run_case;
        input integer stall;
        input [31:0] value;
        integer n;
        begin
            reset_n = 0;
            req_compl_wd = 0;
            bar4_pio_data_valid = 0;
            checking_stall = 0;
            s_axis_cc_tready = stall ? 1'b0 : 1'b1;
            rd_data = 0;
            expected_dw = value;
            repeat (5) tick();
            @(negedge clk); reset_n = 1;
            repeat (3) tick();
            @(negedge clk); req_compl_wd = 1;
            tick();
            @(negedge clk); req_compl_wd = 0;
            // Data must NOT appear before UltraRAM reports valid.
            repeat (7) begin
                tick();
                if (s_axis_cc_tvalid) begin
                    $display("FAIL: completion emitted before BAR4 data_valid");
                    errors = errors + 1;
                    $display("TEST ABORT: completion failure");
                    $finish;
                end
            end
            @(negedge clk);
            rd_data = {4{{96'h0,value}}};
            bar4_pio_data_valid = 1;
            // Wait for registered first CC beat.
            n = 0;
            while (!s_axis_cc_tvalid && n < 30) begin tick(); n = n + 1; end
            if (!s_axis_cc_tvalid) begin
                $display("FAIL: CC valid timeout");
                errors = errors + 1;
                $display("TEST ABORT: assertion failure");
                $finish;
            end
            if (stall) begin
                held_data = s_axis_cc_tdata;
                held_keep = s_axis_cc_tkeep;
                held_last = s_axis_cc_tlast;
                held_user = s_axis_cc_tuser;
                checking_stall = 1;
                repeat (6) tick();
                @(negedge clk);
                checking_stall = 0;
                s_axis_cc_tready = 1;
            end
            n = 0;
            while (completed == 0 && n < 30) begin tick(); n = n + 1; end
            if (completed == 0 || accepted != 1) begin
                $display("FAIL: completed=%0d accepted=%0d",completed,accepted);
                errors = errors + 1;
                $display("TEST ABORT: assertion failure");
                $finish;
            end
            if (errors == 0)
                $display("PASS: stall=%0d expected_dw=%08h", stall, value);
            repeat (3) tick();
        end
    endtask

    initial begin
        $display("BAR4 first-DWORD completion test");
        run_case(0,32'h89abcdef);
        run_case(1,32'h12345678);
        if (errors == 0)
            $display("PASS: both BAR4 first-DWORD cases");
        else
            $display("FAIL: %0d errors", errors);
        $finish;
    end
    initial begin
        #10000;
        $display("FAIL: global simulation timeout");
        $fatal(1);
    end
endmodule
