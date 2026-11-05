`timescale 1ns / 1ps
// Combined BAR4 read regression: 1, 2, 4, 8, 16 DWORDs.
// Real portal, CC transmitter and XPM UltraRAM; decoded CQ request nets forced.
// First run: CC always ready. Each case resets and reloads the memory.
module tb_portal_bar4_cross_128;
  reg clk=0;
  always #2 clk=~clk;
  reg reset_n=0, cc_ready=1;
  wire [127:0] cc_data;
  wire [3:0] cc_keep;
  wire cc_last, cc_valid;
  wire [32:0] cc_user;
  wire compl_done;
  integer errors=0, accepted=0, done_count=0, test_beats=0;
  integer length_dw=0, case_errors=0;
  integer curr_offset_dw = 0;
  reg checking=0, stalled=0;
  reg [127:0] previous_data;
  reg [3:0] previous_keep;
  reg previous_last;
  reg [32:0] previous_user;
  reg [31:0] words [0:15];
  reg [127:0] expected_data;
  reg [3:0] expected_keep;
  reg expected_last;
  integer j, k, payload_index, payload_in_beat;
  integer stall_cycles = 0;
  reg inject_stall = 0;

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


  // The completion descriptor occupies 96 bits of the first CC beat.
  // Only the payload DWORD and descriptor-independent control bits are
  // checked in beat 1; all later valid payload DWORDs are checked exactly.
  always @(posedge clk) begin
    if (reset_n && checking) begin
      if (stalled && (!cc_valid ||
         {cc_data,cc_keep,cc_last,cc_user} !==
         {previous_data,previous_keep,previous_last,previous_user})) begin    
        $display("FAIL: length=%0d CC changed while stalled at %0t",length_dw,$time);
        errors=errors+1;
      end
      stalled = cc_valid && !cc_ready;
      previous_data=cc_data;
      previous_keep=cc_keep;
      previous_last=cc_last;
      previous_user=cc_user;
      if (cc_valid && cc_ready) begin
        test_beats=test_beats+1;
        accepted=accepted+1;
        expected_data=128'd0;
        expected_keep=4'hf;
        expected_last=(test_beats == ((length_dw+2)/4+1));
        if (test_beats == 1) begin
          expected_data[127:96]=words[curr_offset_dw]; //words[0];
          if (cc_data[127:96] !== words[curr_offset_dw] ||
              cc_keep !== 4'hf || cc_last !== (length_dw==1)) begin
            $display("FAIL: length=%0d beat=1 got=%032h keep=%h last=%b",
                     length_dw,cc_data,cc_keep,cc_last);
            errors=errors+1;
          end
        end else begin
          // Beat 2 carries DW1-DW4; later beats carry four DWORDs each.
          // Subsequent beats carry four payload DWORDs each.
          payload_index=1+(test_beats-2)*4;
          payload_in_beat=length_dw-payload_index;
          if (payload_in_beat>4) payload_in_beat=4;
          expected_keep=(4'b0001 << payload_in_beat)-1;
          for (j=0;j<payload_in_beat;j=j+1)
            expected_data[j*32 +: 32]=words[curr_offset_dw+payload_index+j];
          // Unused DWORD lanes on the last beat are don't-care.
          for (j=0;j<payload_in_beat;j=j+1)
            if (cc_data[j*32 +: 32] !== expected_data[j*32 +: 32]) begin
              $display("FAIL: length=%0d beat=%0d DW%0d got=%08h expected=%08h",
                       length_dw,test_beats,curr_offset_dw+payload_index+j,
                       cc_data[j*32 +: 32],words[curr_offset_dw+payload_index+j]);
              errors=errors+1;
            end
          if (cc_keep !== expected_keep ||
              cc_last !== (payload_index+payload_in_beat==length_dw)) begin
            $display("FAIL: length=%0d beat=%0d keep=%h expected=%h last=%b",
                     length_dw,test_beats,cc_keep,expected_keep,cc_last);
            errors=errors+1;
          end
        end
        $display("CC accepted length=%0d beat=%0d data=%032h keep=%h last=%b",
                 length_dw,test_beats,cc_data,cc_keep,cc_last);
        if (test_beats > ((length_dw+2)/4+1)) begin
          $display("FAIL: extra CC beat for length=%0d",length_dw);
          errors=errors+1;
        end
      end
      if (compl_done) done_count=done_count+1;
    end
  end
  
// Diagnostic: stall the final CC beat of a 16-DWORD completion.
always @(negedge clk) begin
    if (!reset_n || !checking || length_dw != 16 || !inject_stall) begin
        cc_ready = 1'b1;
        stall_cycles = 0;
    end
    else if (test_beats == 4 && stall_cycles < 4) begin
        cc_ready = 1'b0;
        stall_cycles = stall_cycles + 1;
        $display("BACKPRESSURE: t=%0t cycle=%0d valid=%b ready=%b beats=%0d",
                 $time, stall_cycles, cc_valid, cc_ready, test_beats);
    end
    else begin
        cc_ready = 1'b1;
    end
end

  task run_case;
    input integer n;
    input integer do_reset;
    input integer offset_dw;
    integer start_done, start_accepted, timeout_count, expected_beats;
    
    begin
      // Reset DUT between cases. XPM contents are reloaded after reset.
      //checking=0;
      //reset_n=0;
      //cc_ready=1;
      //repeat(8) @(negedge clk);
      //reset_n=1;
      //repeat(8) @(negedge clk);
      checking=0;
      cc_ready=1;

      if (do_reset) begin
          reset_n=0;
          repeat(8) @(negedge clk);
          reset_n=1;
          repeat(8) @(negedge clk);
      end
      
      if (do_reset) begin
      preload(16'h0010,128'h01234567_89abcdef_11223344_55667788);
      preload(16'h0011,128'h10203040_ddeeff00_55667799_99aabbcc);
      preload(16'h0012,128'h8899aabb_66778899_44556677_22334455);
      preload(16'h0013,128'hfedcba98_76543210_abcdef01_13579bdf);
      end
      
      length_dw=n;
      curr_offset_dw=offset_dw;
      test_beats=0;
      stalled=0;
      start_done=done_count;
      start_accepted=accepted;
      case_errors=errors;
      expected_beats=(n==1)?1:1+((n+2)/4);
      checking=1;
      //force dut.req_addr = {2'b10, 21'h00100 + (curr_offset_dw * 4)};
      //force dut.req_addr = {2'b10, (21'h00100 + (curr_offset_dw << 2))};
      force dut.req_addr = 23'h400100 + (curr_offset_dw * 23'd4);
      force dut.payload_len = n;
      force dut.req_len = n;
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
      $display("BAR4 DIAG: req_addr=%06h region=%b rd_addr=%04h lane=%b",
               dut.req_addr, dut.req_addr[22:21], dut.bar4_rd_addr, dut.req_addr[3:2]);
               
      $display("BAR4 STATE: state=%0d active=%b valid=%b words=%0d addr=%04h data=%032h",
               dut.bar4_pio_state_q, dut.bar4_pio_active_q, dut.bar4_pio_data_valid_q,
               dut.bar4_pio_words_q, dut.bar4_pio_addr_q, dut.bar4_pio_data_q);
               
      timeout_count=0;
      while (done_count==start_done && timeout_count<150) begin
        @(negedge clk);
        timeout_count=timeout_count+1;
      end
      if (done_count==start_done) begin
        $display("FAIL: length=%0d compl_done timeout",n);
        errors=errors+1;
      end
      repeat(6) @(negedge clk);
      if (accepted-start_accepted != expected_beats) begin
        $display("FAIL: length=%0d expected %0d beats, got %0d",
                 n,expected_beats,accepted-start_accepted);
        errors=errors+1;
      end
      if (dut.bar4_pio_active_q !== 1'b0) begin
        $display("FAIL: length=%0d BAR4 controller not idle",n);
        errors=errors+1;
      end
      checking=0;
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
      if (errors==case_errors)
        $display("PASS: BAR4 %0d-DWORD read",n);
      else
        $display("FAIL: BAR4 %0d-DWORD read (%0d new errors)",n,errors-case_errors);
      repeat(5) @(negedge clk);
    end
  endtask

  initial begin
    words[0]=32'h55667788; words[1]=32'h11223344;
    words[2]=32'h89abcdef; words[3]=32'h01234567;
    words[4]=32'h99aabbcc; words[5]=32'h55667799;
    words[6]=32'hddeeff00; words[7]=32'h10203040;
    words[8]=32'h22334455; words[9]=32'h44556677;
    words[10]=32'h66778899; words[11]=32'h8899aabb;
    words[12]=32'h13579bdf; words[13]=32'habcdef01;
    words[14]=32'h76543210; words[15]=32'hfedcba98;
    run_case(1,1, 0);
    run_case(2,1, 0);
    run_case(4,1, 0);
    run_case(8,1, 0);
    
    // Two 16-DWORD reads, second without reset or reload.
    $display("BEGIN: consecutive 16-DWORD BAR4 reads");
    inject_stall = 1'b0;
    run_case(16,1, 0);
    
    run_case(16, 0, 0);  // Second read, WITHOUT reset
    $display("END: consecutive 16-DWORD BAR4 reads");
      
    // First address-offset experiment.
    $display("BEGIN: BAR4 offset +1 DWORD test");
    inject_stall = 1'b0;
    run_case(2, 1, 1);
    $display("END: BAR4 offset +1 DWORD test");
    
    $display("BEGIN: BAR4 cross 128 dword test");
    run_case(2, 1, 3);
    
    if (errors==0) $display("PASS: all BAR4 length regression tests");
    else $display("FAIL: BAR4 length regression errors=%0d",errors);
    $finish;
  end

  initial begin
    #20000;
    $display("FAIL: global regression timeout");
    $finish;
  end
endmodule
