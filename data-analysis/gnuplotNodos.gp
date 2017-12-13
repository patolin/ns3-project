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
plot "scma-bytesRx.dat" with linespoints title 'Rx', "scma-bytesTx.dat" with linespoints title 'Tx'
set output 'SCMAPackets.png'
set title "SCMA Packets Transferred"
set ylabel "Packets"
plot "scma-PacketsRx.dat" with linespoints title 'Rx', "scma-PacketsTx.dat" with linespoints title 'Tx'
set output 'IBRBytes.png'
set title "IBR Bytes Transferred"
set ylabel "Bytes"
plot "ibr-bytesRx.dat" with linespoints title 'Rx', "ibr-bytesTx.dat" with linespoints title 'Tx'
set output 'IBRPackets.png'
set title "IBR Packets Tx"
set ylabel "Packets"
plot "ibr-PacketsTx.dat" with linespoints title 'Tx'
set output 'DownloadTime.png'
set title "Average Download Time"
set ylabel "Seconds"
plot "t_descarga.dat" with linespoints title 'Time'
set output 'Downloads.png'
set title "Total Downloads"
set ylabel "Downloads"
plot "totalDescargas.dat" with linespoints title 'Downloads'
set output 'RatioTxRx.png'
set title "Rx Tx Packets Ratio"
set ylabel "Ratio"
plot "ratioRxTx.dat" with linespoints title 'Rx Tx Ratio'
set output 'Clouds.png'
set title "Average Clouds"
set ylabel "Clouds"
plot "nubes.dat" with linespoints title 'Clouds Completed'
set output 'DownloadNodes.png'
set title "Download Nodes"
set ylabel "Nodes"
plot "numNodos.dat" with linespoints title 'Download Nodes'
set output 'Augmentation.png'
set title "Augmentation Requests"
set ylabel "Requests"
plot "augmentation.dat" with linespoints title 'Requests'
set output 'ScmaOverhead.png'
set title "SCMA Overhead"
set ylabel "Bytes"
plot "scma-overhead.dat" with linespoints title 'Bytes'
set output 'IbrOverhead.png'
set title "Ibr Overhead"
set ylabel "Bytes"
plot "ibr-overhead.dat" with linespoints title 'Bytes'

