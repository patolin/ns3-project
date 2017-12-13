import sys
import os

#evtTaskFinished=[]
def getTasksFinished(archivo):
    evtTaskFinished=[]
    salida=[]
    nodosApp=[]
    eventos={}
    datosNodo={}
    totalesNodo={}
    logPackets=0
    SCMAStartedTx=0
    SCMAStartedTx1=0
    SCMAStartedRx=0
    IBR={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    SCMA={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    TOTALES={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    f=open(archivo, "r")
    for linea in f.xreadlines():
        if "Time:" in linea:
            linea=linea[0:-1]
            #print linea
            arrLinea=linea.split("\t")
            tiempo=arrLinea[0].split(":")[1]
            evento=arrLinea[1].split(":")[1]
            nodo=arrLinea[2].split(":")[1]
            # sumamos los totales por nodo
            if (nodo not in datosNodo):
                datosNodo[nodo]=TOTALES
            if (evento=="IbrRP-PktTx" or evento=="ScmaStateMachine-PktTx"):
                datosNodo[nodo]["bytesTx"]+=int(arrLinea[4].split(":")[1])
                datosNodo[nodo]["PacketsTx"]+=1
            if (evento=="IbrRP-PktRx" or evento=="ScmaStateMachine-PktRx"):
                datosNodo[nodo]["bytesRx"]+=int(arrLinea[4].split(":")[1])
                datosNodo[nodo]["PacketsRx"]+=1

            if (logPackets==1):
                try:
                    if (evento=="IbrRP-PktTx"):
                        IBRNumPacketsTx=IBRNumPacketsTx+1
                        IBRNumBytesTx=IBRNumBytesTx+int(arrLinea[4].split(":")[1])

                        #eventos[nodo]["IBRPacketsTx"]=eventos[nodo]["IBRPacketsTx"]+1
                        #eventos[nodo]["IBRBytesTx"]=eventos[nodo]["IBRBytesTx"]+int(arrLinea[4].split(":")[1])

                    if (evento=="IbrRP-PktRx"):
                        IBRNumPacketsRx=IBRNumPacketsRx+1
                        IBRNumBytesRx=IBRNumBytesRx+int(arrLinea[4].split(":")[1])
                        #eventos[nodo]["IBRPacketsRx"]=eventos[nodo]["IBRPacketsRx"]+1
                        #eventos[nodo]["IBRBytesRx"]=eventos[nodo]["IBRBytesRx"]+int(arrLinea[4].split(":")[1])
                    
                    if (evento=="ScmaStateMachine-PktTx"):
                            #SCMANumPacketsTx=SCMANumPacketsTx+1
                            #SCMANumBytesTx=SCMANumBytesTx+int(arrLinea[4].split(":")[1]) 
                            #print arrLinea[5].split(":")[1]
                            
                            if (arrLinea[5].split(":")[1]==" SCMA - M_SendTaskToCN"):
                                if (SCMAStartedTx==0):
                                    SCMANumPacketsTx=SCMANumPacketsTx+1
                                    SCMANumBytesTx=SCMANumBytesTx+int(arrLinea[4].split(":")[1])
                                    SCMAStartedTx=1
                            elif (arrLinea[5].split(":")[1]==" SCMA - M_CloudResourceInfo"):
                                if (SCMAStartedTx1==0):
                                    SCMANumPacketsTx=SCMANumPacketsTx+1
                                    SCMANumBytesTx=SCMANumBytesTx+int(arrLinea[4].split(":")[1])
                                    SCMAStartedTx1=1
                            else:
                                SCMANumPacketsTx=SCMANumPacketsTx+1
                                SCMANumBytesTx=SCMANumBytesTx+int(arrLinea[4].split(":")[1]) 


                            #eventos[nodo]["SCMAPacketsTx"]=eventos[nodo]["SCMAPacketsTx"]+1
                            #eventos[nodo]["SCMABytesTx"]=eventos[nodo]["SCMABytesTx"]+int(arrLinea[4].split(":")[1])
                    if (evento=="ScmaStateMachine-PktRx"):
                        if (arrLinea[6].split(":")[1]!=" SCMA - M_CloudResourceInfo"):
                            SCMANumPacketsRx=SCMANumPacketsRx+1
                            SCMANumBytesRx=SCMANumBytesRx+int(arrLinea[4].split(":")[1])  
                        else:
                            if (SCMAStartedRx==0): 
                                SCMANumPacketsRx=SCMANumPacketsRx+1
                                SCMANumBytesRx=SCMANumBytesRx+int(arrLinea[4].split(":")[1])  
                                SCMAStartedRx=1

                #    #eventos[nodo]["SCMAPacketsRx"]=eventos[nodo]["SCMAPacketsRx"]+1
                    #    #eventos[nodo]["SCMABytesRx"]=eventos[nodo]["SCMABytesRx"]+int(arrLinea[4].split(":")[1])
                except:
                    nodo=nodo    

            if (evento=="ScmaStateMachine-TaskDistribution"):
                if (nodo not in nodosApp):
                    nodosApp.append(nodo)
                if (logPackets==0):
                    logPackets=1
                    SCMAStartedTx=0
                    SCMAStartedRx=0
                    SCMANumBytesRx=0
                    SCMANumPacketsRx=0
                    IBRNumBytesRx=0
                    IBRNumPacketsRx=0
                    SCMANumBytesTx=0
                    SCMANumPacketsTx=0
                    IBRNumBytesTx=0
                    IBRNumPacketsTx=0
                    #IBR={"BytesTx":0, "BytesRx":0, "PacketsTx":0, "PacketsRx":0 }
                    #SCMA={"BytesTx":0, "BytesRx":0, "PacketsTx":0, "PacketsRx":0 }
                eventos[nodo]={"Evt_0":evento, "Time_0":float(tiempo),
                        "IBRPacketsTx":IBRNumPacketsTx,
                        "IBRPacketsRx":IBRNumPacketsRx,
                        "IBRBytesTx":IBRNumBytesTx, 
                        "IBRBytesRx":IBRNumBytesRx,
                        "SCMAPacketsTx": SCMANumPacketsTx,
                        "SCMAPacketsRx": SCMANumPacketsRx,
                        "SCMABytesTx": SCMANumBytesTx,
                        "SCMABytesRx": SCMANumBytesRx }

               


            if (evento=="ScmaStateMachine-TaskFinished"):
                eventos[nodo]["Evt_1"]=evento
                eventos[nodo]["Time_1"]=float(tiempo)
                eventos[nodo]["Time_Download"]=eventos[nodo]["Time_1"]-eventos[nodo]["Time_0"]
                evtTaskFinished.append({"node":nodo, "info":eventos[nodo]})
                logPackets=0
                SCMAStarted=0
                #print eventos[nodo]

                         

    f.close()
    for nApp in nodosApp:
        totalesNodo[nApp]=datosNodo[nApp]
    #print "Nodos App: "+str(len(nodosApp))
    #print nodosApp
    #print totalesNodo
    return (evtTaskFinished,totalesNodo)

carpetas=['25', '37', '50', '62', '75']
dir=sys.argv[1]+"/"
print "Procesando directorio: "+dir

#print procesaArchivo(dir+listaArch[0])
dictFinal={}

for carpeta in carpetas:
    carpetaDatos=dir+carpeta
    listaArch=os.listdir(carpetaDatos)
    IBR={"BytesTx":0, "BytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    SCMA={"BytesTx":0, "BytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    SCMANumBytesRx=0
    SCMANumPacketsRx=0
    IBRNumBytesRx=0
    IBRNumPacketsRx=0
    SCMANumBytesTx=0
    SCMANumPacketsTx=0
    IBRNumBytesTx=0
    IBRNumPacketsTx=0


    downBase={}
    for archivo in listaArch:
        archivoDatos=carpetaDatos+'/'+archivo
        print archivoDatos
        datosTemp=getTasksFinished(archivoDatos)
        datosLeidos=datosTemp[0]
        datosTotales=datosTemp[1]
        #datosLeidos=getTasksFinished(archivoDatos)

        #get the totals
        countDownloads=0
        for download in datosLeidos:
            if (countDownloads==0):
                downBase=download["info"]
            else:
                a=downBase
                b=download["info"]
                #print a
                #print b
                downBase=dict(a.items() + b.items() + [(k, a[k] + b[k]) for k in set(b) & set(a)])
                
            countDownloads=countDownloads+1
        # get the average for the app nodes
        numAppNodes=0
        appNodeBase={}
        for nodeTotal in datosTotales:
            if numAppNodes==0:
                appNodeBase=datosTotales[nodeTotal]
            else:
                a=appNodeBase
                b=datosTotales[nodeTotal]
                appNodeBase=dict(a.items() + b.items() + [(k, a[k] + b[k]) for k in set(b) & set(a)]) 
            numAppNodes+=1
        for key in appNodeBase.keys():
           appNodeBase[key]=appNodeBase[key]/numAppNodes

        print appNodeBase 

    for key in downBase.keys():
        if (not isinstance(downBase[key],basestring)):
            downBase[key]=downBase[key]/float(countDownloads)
        else:
            del downBase[key]
    downBase["totalDownloads"]=countDownloads
    downBase["TxRxRatio"]=float(appNodeBase["bytesTx"])/float(appNodeBase["bytesRx"])
    #print evtTaskFinished
    #print downBase
    #print countDownloads
    for key in downBase:
        if (key not in dictFinal):
            dictFinal[key]={}
        dictFinal[key][int(carpeta)]=downBase[key]

print dictFinal

#print "***** datos por nodo *****"
#print nodosApp
#print datosNodo
#print "**************************"

print "Generando archivo de datos para gnuplot"

#para generar histogramas
#arch=open('datos.dat', 'w')
#for key in dictFinal:
#    linea=str(key)
#    for variable in dictFinal[key]:
#        valor=dictFinal[key][variable]
#        linea+="\t"+str(valor)
#    arch.write(linea + os.linesep)
#arch.close()
#os.system('gnuplot gnuplot.gp')

#para graficas de lineas
for key in dictFinal:
    arch=open(key+'.dat', 'w')
    #for variable in dictFinal[key]:
    for variable in carpetas:
        linea=str(variable)+'\t'+str(dictFinal[key][int(variable)])
        arch.write(linea+os.linesep)
    arch.close()
os.system('gnuplot gnuplot.gp')
