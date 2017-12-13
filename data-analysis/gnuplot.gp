set terminal png size 800,500 enhanced font "Helvetica,20"
set style line 2 lc rgb 'black' lt 1 lw 1
set style data linespoints 
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 0.9
set grid ytics
set xlabel "Collaborators %"
set output 'SCMABytes.png'
set title "SCMA Bytes Transferred"
set ylabel "Bytes"
plot "SCMABytesRx.dat" with linespoints title 'Rx', "SCMABytesTx.dat" with linespoints title 'Tx'
set output 'SCMAPackets.png'
set title "SCMA Packets Transferred"
set ylabel "Packets"
plot "SCMAPacketsRx.dat" with linespoints title 'Rx', "SCMAPacketsTx.dat" with linespoints title 'Tx'
set output 'IBRBytes.png'
set title "IBR Bytes Transferred"
set ylabel "Bytes"
plot "IBRBytesRx.dat" with linespoints title 'Rx', "IBRBytesTx.dat" with linespoints title 'Tx'
set output 'IBRPackets.png'
set title "IBR Packets Transferred"
set ylabel "Packets"
plot "IBRPacketsRx.dat" with linespoints title 'Rx', "IBRPacketsTx.dat" with linespoints title 'Tx'
set output 'DownloadTime.png'
set title "Average Download Time"
set ylabel "Seconds"
plot "Time_Download.dat" with linespoints title 'Time'
set output 'Downloads.png'
set title "Total Downloads"
set ylabel "Downloads"
plot "totalDownloads.dat" with linespoints title 'Downloads'

