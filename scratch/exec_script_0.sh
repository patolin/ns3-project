#!/bin/bash

cd /home/patolin/Documentos/proyecto-ups/ns-allinone-3.25/ns-3.25

./waf --run "simulador-ups --duration=300 --nodeNum=10 --powerClass=A --traceFile=/home/patolin/Documentos/proyecto-ups/ns-allinone-3.25/ns-3.25/scratch/ns2mobility_process_0.tcl --streamNum=10 --verbose=true --hotspotNum=6 --packetSize=1000 --numPackets=1000000 --interval=0.1 --scnGridX=8 --scnGridY=8 --regionsX=3 --regionsY=3 --lanesNum=2 --pavementWidth=10 --shoulderWidth=10"
