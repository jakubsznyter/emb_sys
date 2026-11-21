# Source in the Tcl console with assignment2.xpr open. Existing .bit files are reused.
if {[current_project -quiet] eq ""} {error "Open assignment2.xpr first."}
foreach {run variant bitname} {
    impl_1 cnn main_i_CNN_0_CNN_inst_0_partial.bit
    child_0_impl_1 snn main_i_CNN_0_SNN_inst_0_partial.bit
} {
    set run_dir [get_property DIRECTORY [get_runs $run]]
    set bitfile [file join $run_dir $bitname]
    if {![file exists $bitfile]} {error "Missing existing partial bitstream: $bitfile"}
    write_cfgmem -force -format BIN -interface SMAPx32 -disablebitswap \
        -loadbit [list up 0x0 $bitfile] -file [file join $run_dir ${variant}.bin]
}
