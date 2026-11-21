# Rebuild both configurations after the static DFX IO upgrade.
# Run with assignment2.xpr open. Does not alter the legacy Vitis SNN platform.
current_bd_design main
validate_bd_design -force -assign_dfx_addressing
save_bd_design
generate_target all [get_files */main.bd]
create_ip_run [get_files */main.bd]
reset_runs synth_1
launch_runs synth_1 -jobs 2
wait_on_run synth_1
if {![string match {*Complete!*} [get_property STATUS [get_runs synth_1]]]} {error "Top-level synthesis failed."}
open_run synth_1 -pr_config config_1
report_drc -ruledecks {default dfx_floorplan_checks} -file [file join [get_property DIRECTORY [current_project]] docs dfx_upgrade floorplan_drc.rpt]
set failures [get_drc_violations -quiet -filter {SEVERITY == Error || SEVERITY == "Critical Warning"}]
if {[llength $failures]} {error "Fix DRC errors/critical warnings before implementation: $failures"}
close_design
launch_runs impl_1 -to_step write_bitstream -jobs 2
wait_on_run impl_1
if {![string match {write_bitstream Complete!*} [get_property STATUS [get_runs impl_1]]]} {error "CNN parent implementation failed."}
if {![string match {write_bitstream Complete!*} [get_property STATUS [get_runs child_0_impl_1]]]} {
    launch_runs child_0_impl_1 -to_step write_bitstream -jobs 2
    wait_on_run child_0_impl_1
}
if {![string match {write_bitstream Complete!*} [get_property STATUS [get_runs child_0_impl_1]]]} {error "SNN child implementation failed."}
puts DFX_BOTH_BITSTREAMS_COMPLETE
