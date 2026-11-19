`timescale 1ns / 1ps
// Integrated BAR4 sixteen-DWORD read diagnostic.
// Uses real portal controller, XPM UltraRAM, and CC transmitter.
// CQ decoding is bypassed by forcing decoded request nets.
module tb_portal_bar4_sixteen_dw;
  reg clk=0;
  always #2 clk=~clk;
  reg reset_n=0;
  reg cc_ready=1;
  wire [127:0] cc_data;
  wire [3:0] cc_keep;
  wire cc_last, cc_valid;
  wire [32:0] cc_user;
  wire compl_done;
  integer errors=0, accepted=0, done_count=0;
  integer test_beats=0;
  reg checking=0;
  reg [127:0] previous_data;
  reg [3:0] previous_keep;
  reg previous_last;
  reg [32:0] previous_user;
  reg stalled=0;

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
       if (stalled && (!cc_valid ||
           {cc_data,cc_keep,cc_last,cc_user} !==
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
        $display("CC accepted t=%0t beat=%0d data=%032h keep=%h last=%b",
                 $time,test_beats,cc_data,cc_keep,cc_last);
        case (test_beats)
          1: if (cc_data[127:96] !== 32'h55667788 ||
                 cc_keep !== 4'hf || cc_last !== 1'b0) begin
               $display("FAIL: beat 1 expected DW0=55667788 keep=f last=0");
               errors=errors+1;
             end
 
          2: if (cc_data !== 128'h99aabbcc_01234567_89abcdef_11223344 ||
                 cc_keep !== 4'hf || cc_last !== 1'b0) begin
               $display("FAIL: beat 2 expected DW4..DW1=99aabbcc_01234567_89abcdef_11223344");
               errors=errors+1;
             end
 
          3: if (cc_data !==
                 128'h22334455_10203040_ddeeff00_55667799 ||
                 cc_keep !== 4'hf || cc_last !== 1'b0) begin
               $display("FAIL: beat 3 expected DW8..DW5");
               errors=errors+1;
             end

          4: if (cc_data !==
                 128'h13579bdf_8899aabb_66778899_44556677 ||
                 cc_keep !== 4'hf || cc_last !== 1'b0) begin
               $display("FAIL: beat 4 expected DW12..DW9");
               errors=errors+1;
             end

          5: if (cc_data[95:0] !==
                 96'hfedcba98_76543210_abcdef01 ||
                 cc_keep !== 4'h7 || cc_last !== 1'b1) begin
               $display("FAIL: beat 5 expected DW15..DW13");
               errors=errors+1;
             end     
             
          default: begin
            $display("FAIL: unexpected extra CC beat");
            errors=errors+1;
          end
        endcase
      end
      if (compl_done) done_count=done_count+1;
    end
  end

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

  task read_sixteen;
    input [22:0] address;
    integer start_accepted, start_done, local_timeout;
    begin
      start_accepted=accepted;
      start_done=done_count;
      test_beats=0;
      cc_ready=1; // First diagnostic: no backpressure.
      force dut.req_addr = address;
      force dut.payload_len = 11'd16;
      force dut.req_len = 11'd16;
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
      while (done_count==start_done && local_timeout<100) begin
        @(negedge clk);
        local_timeout=local_timeout+1;
      end
      if (done_count==start_done) begin
        $display("FAIL: sixteen-DWORD compl_done timeout at %h",address);
        errors=errors+1;
      end
      repeat(6) @(negedge clk);
      if (accepted != start_accepted+5) begin
        $display("FAIL: expected five accepted CC beats; got %0d",accepted-start_accepted);
        errors=errors+1;
      end
      if (dut.bar4_pio_active_q !== 1'b0) begin
        $display("FAIL: BAR4 controller did not return to idle after sixteen-DWORD request");
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
always @(posedge clk) begin
    if (dut.bar4_pio_active_q) begin
        $display(
          "BAR4 t=%0t state=%0d addr=%h words=%0d issue=%b consumed=%b valid=%b mem=%032h",
          $time,
          dut.bar4_pio_state_q,
          dut.bar4_pio_addr_q,
          dut.bar4_pio_words_q,
          dut.bar4_pio_mem_issue,
          dut.bar4_pio_word_consumed,
          dut.bar4_pio_data_valid_q,
          dut.bar4_mem_rd_data
        );
        $display(
  "SYNC t=%0t rem=%0d ready=%b wait=%b low_seen=%b ccvalid=%b ccready=%b len=%0d",
  $time,
  dut.tx_i.pio_tx_sm_128.dw_rem,
  dut.tx_i.pio_tx_sm_128.rd_data_ready,
  dut.tx_i.pio_tx_sm_128.bar4_wait_next_word_q,
  dut.tx_i.pio_tx_sm_128.bar4_valid_low_seen_q,
  cc_valid,
  cc_ready,
  dut.tx_i.len_i
);
    end
end
always @(posedge clk) begin
    if (dut.tx_i.rq_state ==
        dut.tx_i.PIO_TX_COMPL_WD_N_DW) begin

        $display(
            "CCSYNC t=%0t len=%0d rem=%0d ready=%b valid=%b s0low=%08h s0mid=%08h s1=%024h",
            $time,
            dut.tx_i.len_i,
            dut.tx_i.pio_tx_sm_128.dw_rem,
            dut.tx_i.pio_tx_sm_128.rd_data_ready,
            cc_valid,
            dut.tx_i.rd_data_s0[31:0],
            dut.tx_i.rd_data_s0[159:128],
            dut.tx_i.rd_data_s1
        );
    end
end
  initial begin
    $display("BAR4 sixteen-DWORD integrated diagnostic (CC always ready)");
    repeat(8) @(negedge clk);
    reset_n=1;
    repeat(8) @(negedge clk);
    // Same first UltraRAM word as the passing four-DWORD test.
    preload(16'h0010,128'h01234567_89abcdef_11223344_55667788);
    
    // Adjacent 128-bit word: DW4=99aabbcc, DW5=55667799,
    // DW6=ddeeff00, DW7=10203040.
    preload(16'h0011,128'h10203040_ddeeff00_55667799_99aabbcc);
    
    // Third UltraRAM word: DW8..DW11
    preload(16'h0012,128'h8899aabb_66778899_44556677_22334455);

    // Fourth UltraRAM word: DW12..DW15
    preload(16'h0013,128'hfedcba98_76543210_abcdef01_13579bdf);
    checking=1;
    read_sixteen({2'b10,21'h00100});
    checking=0;
    if (errors==0) $display("PASS: integrated BAR4 sixteen-DWORD read");
    else $display("FAIL: integrated BAR4 sixteen-DWORD errors=%0d",errors);
    $finish;
  end

  initial begin
    #10000;
    $display("FAIL: global simulation timeout");
    $finish;
  end
  
// Temporary BAR4 assembler diagnostic
always @(posedge clk) begin
    if (cc_valid && cc_ready) begin
        $display("DEBUG t=%0t rd_data_s0=%096h rd_data_s1=%024h",
                 $time,
                 dut.tx_i.rd_data_s0,
                 dut.tx_i.rd_data_s1);
    end
end
always @(posedge clk) begin
    if ($time >= 120.000 && $time <= 180.000) begin
        $display(
          "ASM t=%0t state=%0d rem=%0d ready=%b wait=%b valid=%b data=%032h s0low=%032h",
          $time,
          dut.tx_i.pio_tx_sm_128.rd_data_state,
          dut.tx_i.pio_tx_sm_128.dw_rem,
          dut.tx_i.pio_tx_sm_128.rd_data_ready,
          dut.tx_i.pio_tx_sm_128.bar4_wait_next_word_q,
          dut.bar4_pio_data_valid_q,
          dut.tx_i.rd_data[127:0],
          dut.tx_i.rd_data_s0[127:0]
        );
    end
end
endmodule
