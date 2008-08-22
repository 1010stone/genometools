Name "gt gff3 -help"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -help"
  grep $last_stdout, "Report bugs to"
end

Name "gt gff3 -noop"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 -noop", :retval => 1)
  grep $last_stderr, "unknown option"
end

Name "gt gff3 short test (stdin)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 < #{$testdata}gff3_file_1_short.txt"
  run "env LC_ALL=C sort #{$last_stdout}"
  run "diff #{$last_stdout} #{$testdata}gff3_file_1_short_sorted.txt"
end

Name "gt gff3 short test (file)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gff3_file_1_short.txt"
  run "env LC_ALL=C sort #{$last_stdout}"
  run "diff #{$last_stdout} #{$testdata}gff3_file_1_short_sorted.txt"
end

Name "gt gff3 short test (compressed output)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -o test -gzip #{$testdata}gff3_file_1_short.txt"
  grep $last_stderr, "appending it"
end

Name "gt gff3 prob 1"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_prob_1.gff3", :retval => 1)
end

Name "gt gff3 prob 2"
Keywords "gt_gff3"
Test do
  run "env LC_ALL=C #{$bin}gt gff3 -sort #{$testdata}gt_gff3_prob_2.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_2.out"
end

Name "gt gff3 prob 3"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_3.gff3"
end

Name "gt gff3 prob 5"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -sort #{$testdata}gt_gff3_prob_5.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_5.out"
end

Name "gt gff3 prob 6"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 -sort #{$testdata}gt_gff3_prob_6.in", :retval => 1)
  grep($last_stderr, /does not contain/);
end

Name "gt gff3 prob 7 (unsorted)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_7.in | #{$bin}gt gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_7.unsorted"
end

Name "gt gff3 prob 7 (sorted)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -sort #{$testdata}gt_gff3_prob_7.in | #{$bin}gt gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_7.sorted"
end

Name "gt gff3 prob 8"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_8.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_8.out"
end

Name "gt gff3 prob 9"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_9.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_9.out"
end

Name "gt gff3 prob 10"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_10.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_10.out"
end

Name "gt gff3 prob 11"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_prob_11.in"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_prob_11.out"
end

Name "gt gff3 prob 12"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_prob_12.gff3", :retval => 1)
  grep $last_stderr, "has not been previously defined"
end

Name "gt gff3 prob 12 (-checkids)"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 -checkids #{$testdata}gt_gff3_prob_12.gff3", :retval => 1)
  grep $last_stderr, "is separated from its counterpart on line 5 by terminator"
end

Name "gt gff3 test 1.1"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -o /dev/null -force -v #{$testdata}gt_gff3_test_1.in"
end

Name "gt gff3 test 1.2"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 - - < #{$testdata}gt_gff3_test_1.in", :retval => 1)
end

Name "gt gff3 test 1.3"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_1.in | #{$bin}gt gff3"
end

Name "gt gff3 test 2"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_test_2.gff3", :retval => 1)
end

Name "gt gff3 test 3"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_3.gff3"
end

# make sure the -typecheck-built-in option works (for tests below!)
Name "gt gff3 -typecheck-built-in"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 -typecheck-built-in " +
           "#{$testdata}standard_gene_as_tree.gff3")
end

4.upto(14) do |i|
  Name "gt gff3 test #{i}"
  Keywords "gt_gff3"
  Test do
    run_test("#{$bin}gt gff3 -typecheck-built-in " +
             "#{$testdata}/gt_gff3_test_#{i}.gff3", :retval => 1)
  end
end

Name "gt gff3 test 15"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_15.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_15.out"
end

Name "gt gff3 test 16"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_16.gff3"
end

Name "gt gff3 test 17"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_17.gff3"
end

Name "gt gff3 test 18"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_18.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_18.gff3"
end

Name "gt gff3 test 19"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_19.gff3"
end

Name "gt gff3 test 20"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_test_20.gff3", :retval => 1);
  grep($last_stderr, /could not parse/);
end

Name "gt gff3 test 21"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_test_21.gff3", :retval => 1);
  grep($last_stderr, /does not equal required version/);
end

Name "gt gff3 test 22"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_22.gff3 | #{$bin}gt gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_22.gff3"
end

Name "gt gff3 test 22 (-sort)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -sort #{$testdata}gt_gff3_test_22.gff3 | #{$bin}gt gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_22.gff3"
end

Name "gt gff3 test 23"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_23.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_23.gff3"
end

Name "gt gff3 test 24"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_24.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_23.gff3"
end

Name "gt gff3 test 25"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}gt_gff3_test_25.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_test_25.out"
end

Name "gt gff3 test 26"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_test_26.gff3", :retval => 1)
end

Name "gt gff3 test 27"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_test_27.gff3", :retval => 1)
  grep($last_stderr, /before the corresponding/);
end

Name "gt gff3 test additional attribute"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}additional_attribute.gff3"
  run "diff #{$last_stdout} #{$testdata}additional_attribute.gff3"
end

Name "gt gff3 fail 1"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_fail_1.gff3", :retval => 1)
end

Name "gt gff3 test option -addintrons"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -addintrons #{$testdata}addintrons.gff3"
  run "diff #{$last_stdout} #{$testdata}addintrons.out"
end

Name "gt gff3 test option -offset 1000"
Keywords "gt_gff3 offset"
Test do
  run_test "#{$bin}gt gff3 -offset 1000 #{$testdata}gt_gff3_offset_test.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_offset_test.out1000"
end

Name "gt gff3 test option -offset -1"
Keywords "gt_gff3 offset"
Test do
  run_test "#{$bin}gt gff3 -offset -1 #{$testdata}gt_gff3_offset_test.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_offset_test.out-1"
end

Name "gt gff3 test option -offset -999"
Keywords "gt_gff3 offset"
Test do
  run_test "#{$bin}gt gff3 -offset -999 #{$testdata}gt_gff3_offset_test.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_offset_test.out-999"
end

Name "gt gff3 test option -offset -1001 (overflow)"
Keywords "gt_gff3 offset"
Test do
  run_test("#{$bin}gt gff3 -offset -1001 #{$testdata}gt_gff3_offset_test.gff3",
           :retval => 1)
end

Name "gt gff3 test option -offsetfile"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -offsetfile #{$testdata}gt_gff3_offsetfile_test.offsetfile #{$testdata}gt_gff3_offsetfile_test.gff3"
  run "diff #{$last_stdout} #{$testdata}gt_gff3_offsetfile_test.out"
end

Name "gt gff3 test option -mergefeat"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -sort -mergefeat #{$testdata}mergefeat.gff3"
  run "diff #{$last_stdout} #{$testdata}mergefeat.out"
end

Name "gt gff3 fail option -offsetfile"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 -offsetfile #{$testdata}empty_file #{$testdata}gt_gff3_offsetfile_test.gff3", :retval => 1)
end

Name "gt gff3 fail attribute after dot"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}attribute_after_dot.gff3", :retval => 1)
  grep $last_stderr, "more than one attribute token defined"
end

Name "gt gff3 fail attribute with multiple equal signs"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}attribute_w_multiple_equals.gff3", :retval => 1)
  grep $last_stderr, "does not contain exactly one"
end

Name "gt gff3 fail inconsistent sequence ids"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}inconsistent_sequence_ids.gff3", :retval => 1)
  grep $last_stderr, "has different sequence id"
end

Name "gt gff3 fail range check"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_range_check.gff3", :retval => 1)
  grep $last_stderr, "is not contained in range"
end

Name "gt gff3 fail illegal region start"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_illegal_region_start.gff3", :retval => 1)
  grep $last_stderr, "illegal region start"
end

Name "gt gff3 fail illegal feature start"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}gt_gff3_illegal_feature_start.gff3", :retval => 1)
  grep $last_stderr, "illegal feature start"
end

Name "gt gff3 corrupt gff3 header"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}corrupt_gff3_header.txt", :retval => 1)
  grep $last_stderr, "could not parse integer"
end

Name "gt gff3 corrupt target attribute"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}corrupt_target_attribute.gff3",
           :retval => 1)
  grep $last_stderr, "must have 3 or 4 blank separated entries"
end

Name "gt gff3 target attribute with swapped range"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}target_attribute_swapped_range.gff3",
           :retval => 1)
  grep $last_stderr, "start '2' is larger then end '1' on line"
end

Name "gt gff3 empty attribute"
Keywords "gt_gff3 gff3_attribute"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}empty_attribute.gff3", :retval => 1)
  grep $last_stderr, "has no tag"
end

Name "gt gff3 empty attribute name"
Keywords "gt_gff3 gff3_attribute"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}empty_attribute_name.gff3", :retval => 1)
  grep $last_stderr, "has no tag"
end

Name "gt gff3 empty id attribute"
Keywords "gt_gff3 gff3_attribute"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}empty_id_attribute.gff3", :retval => 1)
  grep $last_stderr, "has no value"
end

Name "gt gff3 empty other attribute"
Keywords "gt_gff3 gff3_attribute"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}empty_other_attribute.gff3",
           :retval => 1)
  grep $last_stderr, "has no value"
end

Name "gt gff3 empty parent attribute"
Keywords "gt_gff3 gff3_attribute"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}empty_parent_attribute.gff3",
           :retval => 1)
  grep $last_stderr, "has no value"
end

# test OBO file parsing
obo_gff3_file="#{$testdata}standard_gene_as_tree.gff3"

Name "gt gff3 -typecheck empty file"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck #{$testdata}empty_file #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "unexpected end-of-file"
end

Name "gt gff3 -typecheck blank line"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck #{$testdata}obo_files/blank_line.obo " +
           "#{obo_gff3_file}", :retval => 1)
  grep $last_stderr, "unexpected end-of-file"
end

Name "gt gff3 -typecheck comment line"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck #{$testdata}obo_files/comment_line.obo " +
           "#{obo_gff3_file}", :retval => 1)
  grep $last_stderr, "unexpected end-of-file"
end

Name "gt gff3 -typecheck blank-comment line"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/blank_comment_line.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "unexpected end-of-file"
end

Name "gt gff3 -typecheck tag only"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/tag_only.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "expected character ':'"
end

Name "gt gff3 -typecheck missing value"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/missing_value.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "unexpected newline"
end

Name "gt gff3 -typecheck minimal header"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/minimal_header.obo #{$testdata}empty_file"
end

Name "gt gff3 -typecheck corrupt header"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/corrupt_header.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "does not contain \"format-version\" tag"
end

Name "gt gff3 -typecheck minimal stanza"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/minimal_stanza.obo #{$testdata}empty_file"
end

Name "gt gff3 -typecheck corrupt term stanza"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/corrupt_term_stanza.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "lacks required \"name\" tag"
end

Name "gt gff3 -typecheck corrupt typedef stanza"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/corrupt_typedef_stanza.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "lacks required \"name\" tag"
end

Name "gt gff3 -typecheck corrupt instance stanza"
Keywords "gt_gff3 typecheck"
Test do
  run_test("#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/corrupt_instance_stanza.obo #{obo_gff3_file}",
           :retval => 1)
  grep $last_stderr, "lacks required \"instance_of\" tag"
end

Name "gt gff3 -typecheck windows newline"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/windows_newline.obo #{$testdata}empty_file"
end

Name "gt gff3 -typecheck comment in stanza"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck " +
           "#{$testdata}obo_files/comment_in_stanza.obo #{$testdata}empty_file"
end

Name "gt gff3 -typecheck sofa.obo"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck #{$obodir}sofa.obo #{obo_gff3_file}"
end

Name "gt gff3 -typecheck so.obo"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck #{$obodir}so.obo #{obo_gff3_file}"
end

Name "gt gff3 -typecheck so-xp.obo"
Keywords "gt_gff3 typecheck"
Test do
  run_test "#{$bin}gt gff3 -typecheck #{$obodir}so-xp.obo #{obo_gff3_file}"
end

Name "gt gff3 blank attributes"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}blank_attributes.gff3"
end

Name "gt gff3 minimal fasta file"
Keywords "gt_gff3 fasta"
Test do
  run_test "#{$bin}gt gff3 -width 50 #{$testdata}minimal_fasta.gff3"
  run "diff #{$last_stdout} #{$testdata}minimal_fasta.gff3"
end

Name "gt gff3 minimal fasta file (without directive)"
Keywords "gt_gff3 fasta"
Test do
  run_test "#{$bin}gt gff3 -width 50 " +
           "#{$testdata}minimal_fasta_without_directive.gff3"
  run "diff #{$last_stdout} #{$testdata}minimal_fasta.gff3"
end

Name "gt gff3 standard fasta example"
Keywords "gt_gff3 fasta"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}standard_fasta_example.gff3"
end

Name "gt gff3 two fasta sequences"
Keywords "gt_gff3 fasta"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}two_fasta_seqs.gff3"
  run "diff #{$last_stdout} #{$testdata}two_fasta_seqs.gff3"
end

Name "gt gff3 two fasta sequences without sequence region"
Keywords "gt_gff3 fasta"
Test do
  run_test "#{$bin}gt gff3 -sort #{$testdata}two_fasta_seqs_without_sequence_regions.gff3"
  run "diff #{$last_stdout} #{$testdata}two_fasta_seqs.gff3"
end

Name "gt gff3 simple multi-feature (round-trip)"
Keywords "gt_gff3 multi-feature"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}multi_feature_simple.gff3"
  run "diff #{$last_stdout} #{$testdata}multi_feature_simple.gff3"
end

Name "gt gff3 simple multi-feature (reverted)"
Keywords "gt_gff3 multi-feature"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}multi_feature_simple_reverted.gff3"
  run "diff #{$last_stdout} #{$testdata}multi_feature_simple.gff3"
end

Name "gt gff3 simple multi-feature (undefined parent)"
Keywords "gt_gff3 multi-feature"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}multi_feature_undefined_parent.gff3",
           :retval => 1)
  grep $last_stderr, "has not been previously defined"
end

Name "gt gff3 multi-feature (with pseudo-feature)"
Keywords "gt_gff3 multi-feature pseudo-feature"
Test do
  run_test "#{$bin}gt gff3 -width 50 " +
           "#{$testdata}standard_fasta_example_with_id.gff3"
  run "diff #{$last_stdout} #{$testdata}standard_fasta_example_with_id.out"
end

Name "gt gff3 pseudo-feature minimal"
Keywords "gt_gff3 pseudo-feature"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}pseudo_feature_minimal.gff3"
  run "diff #{$last_stdout} #{$testdata}pseudo_feature_minimal.gff3"
end

Name "gt gff3 negative sequence region start"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}sequence_region_negative_start.gff3",
           :retval => 1)
  grep $last_stderr, "start '-1' is negative"
end

Name "gt gff3 negative sequence region end"
Keywords "gt_gff3"
Test do
  run_test("#{$bin}gt gff3 #{$testdata}sequence_region_negative_end.gff3",
           :retval => 1)
  grep $last_stderr, "end '-1497228' is negative"
end

Name "gt gff3 multiple top-level parents"
Keywords "gt_gff3 pseudo-feature"
Test do
  run_test "#{$bin}gt gff3 #{$testdata}multiple_top_level_parents.gff3"
  run "diff #{$last_stdout} #{$testdata}multiple_top_level_parents.gff3"
end

Name "gt gff3 undefined parent (one of two)"
Keywords "gt_gff3"
Test do
  run_test "#{$bin}gt gff3 -tidy #{$testdata}undefined_parent.gff3"
end

def large_gff3_test(name, file)
  Name "gt gff3 #{name}"
  Keywords "gt_gff3"
  Test do
    run_test "#{$bin}gt gff3 #{$gttestdata}gff3/#{file}"
  end

  Name "gt gff3 #{name} (-sort)"
  Keywords "gt_gff3"
  Test do
    run_test "#{$bin}gt gff3 -sort -width 80 " + "#{$gttestdata}gff3/#{file}"
    run      "diff #{$last_stdout} #{$gttestdata}gff3/#{file}.sorted"
  end

  Name "gt gff3 #{name} (sorted)"
  Keywords "gt_gff3"
  Test do
    run_test "#{$bin}gt gff3 -width 80 #{$gttestdata}gff3/#{file}.sorted"
    run      "diff #{$last_stdout} #{$gttestdata}gff3/#{file}.sorted"
  end
end

if $gttestdata then
  large_gff3_test("Saccharomyces cerevisiae", "saccharomyces_cerevisiae.gff")
  large_gff3_test("Aaegypti", "aaegypti.BASEFEATURES-AaegL1.1.gff3")
  large_gff3_test("Drosophila melanogaster",
                  "Drosophila_melanogaster.BDGP5.4.50.gff3")
end
