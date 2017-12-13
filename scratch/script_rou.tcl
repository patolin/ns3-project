#!/usr/bin/tclsh
set infile [open "routefile.rou.xml" r]
set outfile [open "routefile_process.rou.xml" w]

set busqueda "depart=\""
set depart "0.00\">"
    
while { [gets $infile linea] >= 0} {
	
    set inicio [string first $busqueda $linea]
    
    if {$inicio != -1} {
	set inicio [expr $inicio+7]
	set nueva_linea [string range $linea 0 $inicio]

	set nueva_linea $nueva_linea$depart
	puts $outfile $nueva_linea
    } else {
	puts $outfile $linea
    }
}

close $infile
close $outfile
