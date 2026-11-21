# Same steps as Vitis launch config hello_world_app_hw_1 (reset, program full CNN bit, ps7_init, download ELF, run)
# Run from anywhere: paths are relative to this project (scripts/..).
set ws [file dirname [file dirname [file normalize [info script]]]]
connect
targets -set -nocase -filter {name =~ "APU*"}
rst -system
after 1000
targets -set -nocase -filter {name =~ "xc7z020*"}
fpga -file $ws/hello_world/_ide/bitstream/main_wrapper.bit
targets -set -nocase -filter {name =~ "APU*"}
source $ws/hello_world/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
rst -processor
dow $ws/hello_world/build/hello_world.elf
con
puts "BOARD_PROGRAMMED_AND_RUNNING"
disconnect
