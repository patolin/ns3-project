#!/usr/bin/tclsh
set infile [open "ns2mobility.tcl" r]
set outfile [open "ns2mobility_process.tcl" w]

set busqueda "set Z_ "

proc GetRand { m M } { return [expr $m+rand()*($M-$m)] }
    
while { [gets $infile linea] >= 0} {
	
    set inicio [string first $busqueda $linea]
    
    if {$inicio != -1} {
	set inicio [expr $inicio+6]
	set nueva_linea [string range $linea 0 $inicio]

	set altura [GetRand 1.0 2.0]
	set nueva_linea $nueva_linea$altura
	puts $outfile $nueva_linea
    } else {
	puts $outfile $linea
    }
}

close $infile
close $outfile
