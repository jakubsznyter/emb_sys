# Export a matching DFX handoff only after both configurations have rebuilt.
set dfx_root [get_property DIRECTORY [current_project]]
set dfx_report_dir [file join $dfx_root docs dfx_upgrade]
set dfx_out [file join $dfx_root artifacts dfx]
file mkdir $dfx_out
pr_verify -full_check \
    -initial [file join [get_property DIRECTORY [get_runs impl_1]] main_wrapper_routed.dcp] \
    -additional [list [file join [get_property DIRECTORY [get_runs child_0_impl_1]] main_wrapper_routed.dcp]] \
    -file [file join $dfx_report_dir pr_verify.rpt]
puts DFX_PR_VERIFY_PASS
foreach {run model} {impl_1 cnn child_0_impl_1 snn} {
    if {![string match {write_bitstream Complete!*} [get_property STATUS [get_runs $run]]]} {
        error "Bitstream run is incomplete: $run"
    }
    open_run $run
    report_timing_summary -file [file join $dfx_report_dir ${model}_timing_summary.rpt]
    report_route_status -file [file join $dfx_report_dir ${model}_route_status.rpt]
    report_drc -file [file join $dfx_report_dir ${model}_implemented_drc.rpt]
    set bad_drc [get_drc_violations -quiet -filter {SEVERITY == Error || SEVERITY == "Critical Warning"}]
    if {[llength $bad_drc]} {error "Implemented design has DRC errors/critical warnings: $bad_drc"}
    foreach delay_type {max min} {
        set worst [get_timing_paths -quiet -delay_type $delay_type -max_paths 1]
        if {![llength $worst] || [get_property SLACK $worst] < 0} {
            error "Timing check failed ($model / $delay_type). See the saved report."
        }
        puts "DFX_TIMING_PASS $model $delay_type [get_property SLACK $worst]"
    }
    if {$model eq "cnn"} {
        write_hw_platform -fixed -include_bit -force -file [file join $dfx_out main_wrapper.xsa]
        file copy -force [file join $dfx_out main_wrapper.xsa] [file join $dfx_root main_wrapper.xsa]
    }
    close_design
}
source [file join $dfx_root scripts partial_to_bin.tcl]
# Keep the exported simulation/IP copy in sync with regenerated SmartConnect.
export_ip_user_files -of_objects [get_files */main.bd] -no_script -force
puts DFX_HANDOFF_EXPORTED
