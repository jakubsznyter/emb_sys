# Program the PYNQ-Z2 over JTAG and start the ARM application (no Vivado project needed).
# Same steps as the Vitis launch configuration: reset, full bitstream (static + CNN),
# PS7 init, download ELF (contains both CNN and SNN partial bitstreams), run.
# Usage:  xsdb program_board.tcl
set kit [file dirname [file normalize [info script]]]
connect
targets -set -nocase -filter {name =~ "APU*"}
rst -system
after 1000
targets -set -nocase -filter {name =~ "xc7z020*"}
fpga -file [file join $kit hardware main_wrapper.bit]
targets -set -nocase -filter {name =~ "APU*"}
source [file join $kit hardware ps7_init.tcl]
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
rst -processor
dow [file join $kit hardware hello_world.elf]
con
puts "BOARD_PROGRAMMED_AND_RUNNING"
disconnect
