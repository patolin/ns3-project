#/usr/bin/env python

from easygui import *

import os
import subprocess

def num(s):
    try:
        return int(s)
    except ValueError:
        return float(s)

sumoDir = diropenbox(title="Directorio actual de SUMO", default="~/Documentos/proyecto-ups/ns-allinone-3.25/sumo-0.22.0")

print(sumoDir);

msg = "Ingrese los parametros de los vehiculos"
title = "Manhatann grid network generator"
fieldNames = ["Intersecciones X", "Intersecciones Y", "Regiones X", "Regiones Y", "Numero de vias", "Ancho de via", "Ancho de division via", "Ancho de vereda", "Vel. Max. [Km/h]", "Num. vehiculos", "Num. tramas", "Tamano del paquete [B]", "Num. Paquetes por trama", "Intervalo entre tramas [s]", "Num. APs", "Tiempo de Simulacion [s]", "Potencia Tx"]
fieldValues = ["8", "8", "3", "3", "2", "10", "0.1", "10", "10", "300", "10", "1000", "1000000", "0.1", "6", "300", "A"]  
fieldValues = multenterbox(msg,title, fieldNames, fieldValues)


while 1:
    if fieldValues == None: exit();
    errmsg = ""
    for i in range(len(fieldNames)):
      if fieldValues[i].strip() == "":
        errmsg = "Se deben llenar todos los campos\n\n"
    if errmsg == "": break # no problems found
    fieldValues = multenterbox(errmsg, title, fieldNames, fieldValues)

# generation of the vehicular network configuration file
netcfgfile = open("confile.netccfg",'w')

netcfgfile.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n")
netcfgfile.write("<configuration xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://sumo.dlr.de/xsd/netgenerateConfiguration.xsd\">\n")
netcfgfile.write("   <grid_network>\n")
netcfgfile.write("       <grid value=\"1\"/>\n")
netcfgfile.write("       <grid.x-number value=\"" + fieldValues[0] + "\"/>\n")
netcfgfile.write("       <grid.y-number value=\"" + fieldValues[1] + "\"/>\n")

#roadWidth = 2.0 * float(fieldValues[4]) * 3.2 + (2.0 * float(fieldValues[4]) - 1) * 0.1 + 2.0 * float(fieldValues[6]);
#regionSize = 4.0 * float(fieldValues[5]) + roadWidth;

roadWidth = float(fieldValues[4])*float(fieldValues[5])+float(fieldValues[6])*(float(fieldValues[4])-1.0)
regionSize = roadWidth + (float(fieldValues[7])*2.0)



gridLengthX = regionSize*(float(fieldValues[2]) + 1)
netcfgfile.write("       <grid.x-length value=\"" + str(gridLengthX) + "\"/>\n")
gridLengthY = regionSize*(float(fieldValues[3]) + 1)
netcfgfile.write("       <grid.y-length value=\"" + str(gridLengthY) + "\"/>\n")
netcfgfile.write("   </grid_network>\n")
netcfgfile.write("   <output>\n")
netcfgfile.write("       <output-file value=\"netfile.net.xml\"/>\n")
netcfgfile.write("   </output>\n")
netcfgfile.write("   <processing>\n")
netcfgfile.write("       <no-turnarounds value=\"true\"/>\n")
netcfgfile.write("       <offset.x value=\"" + str(regionSize/2.0)+ "\"/>\n")
netcfgfile.write("       <offset.y value=\"" + str(regionSize/2.0)+ "\"/>\n")
netcfgfile.write("       <offset.disable-normalization value=\"true\"/>\n")
netcfgfile.write("   </processing>\n")
netcfgfile.write("   <building_defaults>\n")
netcfgfile.write("     <default.speed value=\"" + str(float(fieldValues[8])*(10.0/36)) + "\"/>\n")
netcfgfile.write("     <default.lanenumber value=\"1\"/>\n")
netcfgfile.write("     <default.sidewalk-width value=\"" + str(float(fieldValues[5]) + float(fieldValues[6])) + "\"/>\n")
netcfgfile.write("   </building_defaults>\n")
netcfgfile.write("</configuration>\n")

netcfgfile.close()

subprocess.Popen(sumoDir + "/bin/netgenerate -c ./confile.netccfg", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("Network file generated", title = "NetGenerate: Vehicular network file generation")

subprocess.Popen(sumoDir + "/tools/trip/randomTrips.py -n ./netfile.net.xml -r ./routefile.rou.xml -e " + fieldValues[15] + " -i " + str(int(fieldValues[0])*int(fieldValues[1])) + " --min-distance=" + str(regionSize*(int(fieldValues[0])*float(fieldValues[1]))) + " --maxtries=100", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("Routes file generated", title = "RandomTrips: Vehicular routes file generation")


sumocfgfile = open("sumoconfile.sumocfg",'w')

sumocfgfile.write("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n")
sumocfgfile.write("<configuration xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://sumo.dlr.de/xsd/sumoConfiguration.xsd\">\n\n")
sumocfgfile.write("    <input>\n")
sumocfgfile.write("        <net-file value=\"netfile.net.xml\"/>\n")
sumocfgfile.write("        <route-files value=\"routefile_process.rou.xml\"/>\n")
sumocfgfile.write("    </input>\n\n")
sumocfgfile.write("    <time>\n")
sumocfgfile.write("        <begin value=\"0\"/>\n")
sumocfgfile.write("        <end value=\"" + str(50 + float(fieldValues[15])) + "\"/>\n")
sumocfgfile.write("    </time>\n")
sumocfgfile.write("</configuration>\n")

sumocfgfile.close()

subprocess.Popen("./script_rou.tcl", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("Route file readjusted", title = "ScriptRou: Route adjustment")

subprocess.Popen(sumoDir + "/bin/sumo -c ./sumoconfile.sumocfg --fcd-output ./sumoTrace.xml", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("SUMO traces file generated", title = "SUMO: Traces generation")

subprocess.Popen(sumoDir + "/tools/traceExporter.py --fcd-input ./sumoTrace.xml --ns2mobility-output ./ns2mobility.tcl", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("NS-2 traces file generated", title = "TraceExporter: Traces exportation")

subprocess.Popen("./script.tcl", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.readlines()
#msgbox ("Traces file readjusted", title = "Script: Traces adjustment")

ns3devDir = diropenbox(title="Select your ns-3-dev directory", default="~/Documentos/proyecto-ups/ns-allinone-3.25/ns-3.25/")

traceFile = fileopenbox(title="Select the NS-2 traces file (ns2mobility_process)", default="./ns2mobility_process.tcl")

execScript = open("exec_script.sh",'w')

execScript.write("#!/bin/bash\n\n")
execScript.write("cd " + ns3devDir + "\n\n")
execScript.write("./waf --run \"simulador-ups --duration=" + fieldValues[15] + " --nodeNum=" + fieldValues[9] + " --powerClass="+fieldValues[16]+" --traceFile=" + traceFile + " --streamNum=" + fieldValues[10] + " --verbose=true --hotspotNum=" + fieldValues[14] + " --packetSize=" + fieldValues[11] + " --numPackets=" + fieldValues[12] + " --interval=" + fieldValues[13] + " --scnGridX=" + fieldValues[0] + " --scnGridY=" + fieldValues[1] + " --regionsX=" + fieldValues[2] + " --regionsY=" + fieldValues[3] + " --lanesNum=" + fieldValues[4] + " --pavementWidth=" + fieldValues[5] + " --shoulderWidth=" + fieldValues[7] + "\" --vis\n")


execScript.close()

os.chmod("exec_script.sh", 0744)

#subprocess.Popen("./exec_script.sh", shell=True)
