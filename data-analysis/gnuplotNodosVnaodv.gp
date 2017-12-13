set terminal png size 800,500 enhanced font "Helvetica,20"
set style line 2 lc rgb 'black' lt 1 lw 1
set style data linespoints 
set style histogram cluster gap 1
set style fill pattern border -1
set boxwidth 0.9
set grid ytics
set xlabel "Collaborators %"
set output 'APPBytes.png'
set title "APP Bytes Transferred"
set ylabel "Bytes"
plot "app-bytesRx.dat" with linespoints title 'Rx', "app-bytesTx.dat" with linespoints title 'Tx'
set output 'APPPackets.png'
set title "APP Packets Transferred"
set ylabel "Packets"
plot "app-PacketsRx.dat" with linespoints title 'Rx', "app-PacketsTx.dat" with linespoints title 'Tx'
set output 'VNAODVBytes.png'
set title "VNAODV Bytes Transferred"
set ylabel "Bytes"
plot "vnaodv-bytesRx.dat" with linespoints title 'Rx', "vnaodv-bytesTx.dat" with linespoints title 'Tx'
set output 'VNAODVPackets.png'
set title "VNAODV Packets Tx"
set ylabel "Packets"
plot "vnaodv-PacketsTx.dat" with linespoints title 'Tx'
set output 'DownloadTime.png'
set title "Average Download Time"
set ylabel "Seconds"
plot "t1_descarga.dat" with linespoints title 'Time'
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
set title "Augmentation Requests per download"
set ylabel "Requests"
plot "augmentation.dat" with linespoints title 'Requests'
set output 'AppOverhead.png'
set title "APP Overhead"
set ylabel "Bytes"
plot "app-overhead.dat" with linespoints title 'Bytes'
set output 'VnaodvOverhead.png'
set title "VNAodv Overhead"
set ylabel "Bytes"
plot "vnaodv-overhead.dat" with linespoints title 'Bytes'

