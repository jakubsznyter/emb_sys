# Run as: vivado -source scripts/open_project.tcl
set assignment_root [file normalize [file join [file dirname [info script]] ..]]
set_param board.repoPaths [list [file join $assignment_root board_files]]
open_project [file join $assignment_root assignment2.xpr]
