# Rebuild only the SNN reconfigurable module after an HLS change (Kuba, 21.11).
# Keeps the CNN parent (impl_1: static + CNN) untouched; child_0_impl_1 reuses
# its locked static routing, so the full CNN bitstream and CNN partial stay valid.
# Usage: vivado -mode batch -source scripts/rebuild_snn_only.tcl
set proj [file normalize [file join [file dirname [info script]] .. assignment2.xpr]]
open_project $proj

update_ip_catalog -rebuild
puts "=== IP status before upgrade"
report_ip_status

# The SNN IP lives in the SNN.bd source of the DFX container.
open_bd_design [get_files SNN.bd]
# report_ip_status flags SNN_snn_mlp_0_0 with "IP revision change"; the scoped
# copy SNN_inst_0_snn_mlp_0_0 follows when main.bd regenerates below.
set snn_ips [get_ips SNN_snn_mlp_0_0]
puts "SNN IP instances: $snn_ips"
upgrade_ip $snn_ips -log ip_upgrade_snn_fix.log
save_bd_design
close_bd_design [current_bd_design]

open_bd_design [get_files main.bd]
validate_bd_design -force -assign_dfx_addressing
save_bd_design
generate_target all [get_files main.bd]
close_bd_design [current_bd_design]

puts "=== Run status after IP update"
foreach r {impl_1 child_0_impl_1 CNN_inst_0_synth_1 SNN_inst_0_synth_1} {
    puts "$r: [get_property STATUS [get_runs $r]] needs_refresh=[get_property NEEDS_REFRESH [get_runs $r]]"
}

reset_runs SNN_inst_0_synth_1
launch_runs SNN_inst_0_synth_1 -jobs 2
wait_on_run SNN_inst_0_synth_1
if {![string match {*Complete!*} [get_property STATUS [get_runs SNN_inst_0_synth_1]]]} {error "SNN synthesis failed."}

reset_runs child_0_impl_1
launch_runs child_0_impl_1 -to_step write_bitstream -jobs 2
wait_on_run child_0_impl_1
if {![string match {write_bitstream Complete!*} [get_property STATUS [get_runs child_0_impl_1]]]} {error "SNN child implementation failed."}

open_run child_0_impl_1
report_timing_summary -file [file join [file dirname $proj] docs snn_fix_timing_summary.rpt]
report_utilization -file [file join [file dirname $proj] docs snn_fix_utilization.rpt]
puts "SNN_FIX_BITSTREAM_COMPLETE"
