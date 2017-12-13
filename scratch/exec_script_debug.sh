#!/bin/bash

cd /home/patolin/Documentos/proyecto-ups/ns-allinone-3.25/ns-3.25

./waf --run "simulador-ups" --command-template="gdb %s" 
