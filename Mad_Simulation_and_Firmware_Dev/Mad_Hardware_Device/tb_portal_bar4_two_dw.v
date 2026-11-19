`timescale 1ns / 1ps
// Integrated BAR4 read test: real portal controller + real XPM UltraRAM + real CC TX.
// CQ decoding is intentionally bypassed by forcing the decoded request nets.
module tb_portal_bar4_two_dw;
  reg clk=0;
  always #2 clk=~clk; // 250 MHz
  reg reset_n=0;
  reg cc_ready=1;
  wire [127:0] cc_data;
  wire [3:0] cc_keep;
  wire cc_last, cc_valid;
  wire [32:0] cc_user;
  wire compl_done;
  integer errors=0, accepted=0, done_count=0;
  integer timeout;
  reg [127:0] previous_data;
  reg [3:0] previous_keep;
  reg previous_last;
  reg [32:0] previous_user;
  reg stalled=0;
  reg [31:0] expected_dw=0;
  reg checking=0;
  reg two_dw_mode=0;
  reg [31:0] expected_dw2=0;
  integer test_beats=0;

  pcie_device_portal dut (
    .user_clk(clk), .reset_n(reset_n),
    .m_axis_cq_tdata(128'd0), .m_axis_cq_tlast(1'b0),
    .m_axis_cq_tvalid(1'b0), .m_axis_cq_tuser(88'd0),
    .m_axis_cq_tkeep(4'd0), .pcie_cq_np_req_count(6'd0),
    .m_axis_rc_tdata(128'd0), .m_axis_rc_tlast(1'b0),
    .m_axis_rc_tvalid(1'b0), .m_axis_rc_tkeep(4'd0),
    .m_axis_rc_tuser(75'd0),
    .s_axis_cc_tdata(cc_data), .s_axis_cc_tkeep(cc_keep),
    .s_axis_cc_tlast(cc_last), .s_axis_cc_tready(cc_ready),
    .s_axis_cc_tvalid(cc_valid), .s_axis_cc_tuser(cc_user),
    .cfg_msg_received(1'b0), .cfg_msg_received_type(5'd0),
    .cfg_msg_data(8'd0), .compl_done(compl_done),
    .s_axis_rq_tready(1'b1), .wr_busy(1'b0)
  );

  always @(posedge clk) begin
    if (reset_n && checking) begin
      if (stalled && ({cc_data,cc_keep,cc_last,cc_user} !==
                      {previous_data,previous_keep,previous_last,previous_user})) begin
        $display("FAIL: CC signals changed under backpressure at %0t",$time);
        errors=errors+1;
      end
      stalled = cc_valid && !cc_ready;
      previous_data=cc_data;
      previous_keep=cc_keep;
      previous_last=cc_last;
      previous_user=cc_user;
      if (cc_valid && cc_ready) begin
        accepted=accepted+1;
        test_beats=test_beats+1;
        $display("CC accepted t=%0t beat=%0d data=%032h keep=%h last=%b",$time,test_beats,cc_data,cc_keep,cc_last);
        if (!two_dw_mode) begin
          if (cc_data[127:96] !== expected_dw || cc_keep !== 4'hf || cc_last !== 1'b1) begin
            $display("FAIL: expected single DWORD %08h in CC data[127:96]",expected_dw);
            errors=errors+1;
          end
        end else begin
          // For a 128-bit CC interface, the 96-bit completion descriptor
          // and first payload DWORD occupy the first beat. Remaining
          // payload DWORDs begin at bits [31:0] of the following beat.
          case (test_beats)
            1: if (cc_data[127:96] !== expected_dw || cc_keep !== 4'hf || cc_last !== 1'b0) begin
                 $display("FAIL: two-DWORD first beat: expected %08h, keep=f, last=0",expected_dw);
                 errors=errors+1;
               end
            2: if (cc_data[31:0] !== expected_dw2 || cc_keep[0] !== 1'b1 || cc_last !== 1'b1) begin
                 $display("FAIL: two-DWORD second beat: expected DWORD %08h in data[31:0], keep[0]=1, last=1",expected_dw2);
                 errors=errors+1;
               end
            default: begin
              $display("FAIL: unexpected extra CC beat");
              errors=errors+1;
            end
          endcase
        end
      end
      if (compl_done) done_count=done_count+1;
    end
  end

  // Write a real 128-bit UltraRAM word through its actual registered port.
  // Initialization forces the portal's write-command signals only; the XPM
  // memory instance and its clock/write behavior are not bypassed.
  task preload;
    input [15:0] address;
    input [127:0] value;
    begin
      @(negedge clk);
      force dut.bar4_wr_pending = 1'b1;
      force dut.bar4_wr_addr_q = address;
      force dut.bar4_wr_data_q = value;
      @(negedge clk);
      release dut.bar4_wr_pending;
      release dut.bar4_wr_addr_q;
      release dut.bar4_wr_data_q;
      repeat(4) @(negedge clk);
    end
  endtask

  task read_one;
    input [22:0] address;
    input [31:0] wanted;
    input integer stall_cycles;
    integer start_accepted, start_done;
    begin
      start_accepted=accepted;
      start_done=done_count;
      expected_dw=wanted;
      two_dw_mode=0;
      test_beats=0;
      cc_ready=0;
      // Force only the decoded request information. The BAR4 controller,
      // UltraRAM, data assembler, and CC transmitter remain unforced.
      force dut.req_addr = address;
      force dut.payload_len = 11'd1;
      force dut.req_len = 11'd1;
      force dut.req_be = 8'hff;
      force dut.req_tc = 3'd0;
      force dut.req_attr = 3'd0;
      force dut.req_rid = 16'h1234;
      force dut.req_tag = 8'h56;
      force dut.req_at = 2'd0;
      force dut.req_mem = 1'b1;
      force dut.req_mem_lock = 1'b0;
      force dut.req_compl = 1'b0;
      force dut.req_compl_ur = 1'b0;
      @(negedge clk);
      force dut.req_compl_wd = 1'b1;
      @(negedge clk);
      force dut.req_compl_wd = 1'b0;
      timeout=0;
      while (!cc_valid && timeout<100) begin
        @(negedge clk);
        timeout=timeout+1;
      end
      if (!cc_valid) begin
        $display("FAIL: CC valid timeout for address %h",address);
        errors=errors+1;
      end
      repeat(stall_cycles) @(negedge clk);
      cc_ready=1;
      timeout=0;
      while (done_count==start_done && timeout<100) begin
        @(negedge clk);
        timeout=timeout+1;
      end
      if (done_count==start_done) begin
        $display("FAIL: completion-done timeout for address %h",address);
        errors=errors+1;
      end
      repeat(6) @(negedge clk);
      if (accepted != start_accepted+1) begin
        $display("FAIL: expected one accepted CC beat, saw %0d",accepted-start_accepted);
        errors=errors+1;
      end
      if (dut.bar4_pio_active_q !== 1'b0) begin
        $display("FAIL: BAR4 controller did not return to idle");
        errors=errors+1;
      end
      release dut.req_compl_wd;
      release dut.req_compl;
      release dut.req_compl_ur;
      release dut.req_addr;
      release dut.payload_len;
      release dut.req_len;
      release dut.req_be;
      release dut.req_tc;
      release dut.req_attr;
      release dut.req_rid;
      release dut.req_tag;
      release dut.req_at;
      release dut.req_mem;
      release dut.req_mem_lock;
      repeat(5) @(negedge clk);
    end
  endtask

  // Diagnostic two-DWORD request entirely within one 128-bit UltraRAM word.
  // Keeps all portal RTL untouched; the monitor prints each accepted CC beat.
  task read_two;
    input [22:0] address;
    input [31:0] wanted0;
    input [31:0] wanted1;
    input integer stall_cycles;
    integer start_accepted, start_done, local_timeout;
    begin
      start_accepted=accepted;
      start_done=done_count;
      expected_dw=wanted0;
      expected_dw2=wanted1;
      two_dw_mode=1;
      test_beats=0;
      cc_ready=0;
      force dut.req_addr = address;
      force dut.payload_len = 11'd2;
      force dut.req_len = 11'd2;
      force dut.req_be = 8'hff;
      force dut.req_tc = 3'd0;
      force dut.req_attr = 3'd0;
      force dut.req_rid = 16'h1234;
      force dut.req_tag = 8'h56;
      force dut.req_at = 2'd0;
      force dut.req_mem = 1'b1;
      force dut.req_mem_lock = 1'b0;
      force dut.req_compl = 1'b0;
      force dut.req_compl_ur = 1'b0;
      @(negedge clk);
      force dut.req_compl_wd = 1'b1;
      @(negedge clk);
      force dut.req_compl_wd = 1'b0;
      local_timeout=0;
      while (!cc_valid && local_timeout<100) begin
        @(negedge clk);
        local_timeout=local_timeout+1;
      end
      if (!cc_valid) begin
        $display("FAIL: two-DWORD CC valid timeout at %h",address);
        errors=errors+1;
      end
      repeat(stall_cycles) @(negedge clk);
      cc_ready=1;
      local_timeout=0;
      while (done_count==start_done && local_timeout<100) begin
        @(negedge clk);
        local_timeout=local_timeout+1;
      end
      if (done_count==start_done) begin
        $display("FAIL: two-DWORD compl_done timeout");
        errors=errors+1;
      end
      repeat(6) @(negedge clk);
      if (accepted != start_accepted+2) begin
        $display("FAIL: expected two accepted CC beats; got %0d",accepted-start_accepted);
        errors=errors+1;
      end
      if (dut.bar4_pio_active_q !== 1'b0) begin
        $display("FAIL: BAR4 controller did not return to idle after two-DWORD request");
        errors=errors+1;
      end
      release dut.req_compl_wd;
      release dut.req_compl;
      release dut.req_compl_ur;
      release dut.req_addr;
      release dut.payload_len;
      release dut.req_len;
      release dut.req_be;
      release dut.req_tc;
      release dut.req_attr;
      release dut.req_rid;
      release dut.req_tag;
      release dut.req_at;
      release dut.req_mem;
      release dut.req_mem_lock;
      two_dw_mode=0;
      repeat(5) @(negedge clk);
    end
  endtask

  initial begin
    $display("BAR4 integrated controller + UltraRAM + CC test");
    repeat(8) @(negedge clk);
    reset_n=1;
    repeat(8) @(negedge clk);
    preload(16'h0010,128'h01234567_89abcdef_11223344_55667788);
    preload(16'h0020,128'hfedcba98_12345678_aabbccdd_eeff0011);
    checking=1;
    // Address includes BAR4 region 2'b10 at bits [22:21].
    // First DWORD of a 128-bit word is its low 32 bits.
    read_one({2'b10,21'h00100},32'h55667788,0);
    read_one({2'b10,21'h00200},32'heeff0011,5);
    $display("Starting two-DWORD same-UltraRAM-word diagnostic");
    // At byte offset 0x100: DWORD0=55667788, DWORD1=11223344.
    read_two({2'b10,21'h00100},32'h55667788,32'h11223344,5);
    checking=0;
    if(errors==0) $display("PASS: integrated BAR4 single- and two-DWORD reads");
    else $display("FAIL: integrated BAR4 test errors=%0d",errors);
    $finish;
  end
  initial begin
    #10000;
    $display("FAIL: global simulation timeout");
    $finish;
  end
endmodule
