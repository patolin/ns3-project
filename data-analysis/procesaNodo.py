import sys
import os
import pprint

def getInfoNode(archivo):
    evtTaskFinished=[]
    salida=[]
    nodosApp=[]
    eventos={}
    datosNodo={}
    totalesNodo={}

    logPackets=0
    totalDescargas=0
    SCMAStartedTx=0
    SCMAStartedTx1=0
    SCMAStartedRx=0
    IBR={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    SCMA={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0 }
    TOTALES={"bytesTx":0, "bytesRx":0, "PacketsTx":0, "PacketsRx":0, "descargas": 0}
    
    m_sendTask={}
    m_cloudResourceRx={}
    m_cloudResourceTx={}

    for i in range(0,1000): 
        m_sendTask[str(i)]=False
        m_cloudResourceTx[str(i)]=False
        m_cloudResourceRx[str(i)]=False

    f=open(archivo, "r")
    for linea in f.xreadlines():
        if "Time:" in linea:
            linea=linea[0:-1]
            #print linea
            arrLinea=linea.split("\t")
            tiempo=arrLinea[0].split(":")[1]
            evento=arrLinea[1].split(":")[1]
            nodo=arrLinea[2].split(":")[1]

            
            capa=""
            tipo=""
            timeStart=0.0
            timeEnd=0.0

            if (nodo not in datosNodo):
                datosNodo[nodo]={"nubes": 0, "descargas": 0, "t_descarga":0.0, "scma-bytesTx":0, "scma-bytesRx":0, "scma-PacketsTx":0, "scma-PacketsRx":0,  "ibr-bytesTx":0, "ibr-bytesRx":0, "ibr-PacketsTx":0, "ibr-PacketsRx":0,  "total-bytesTx":0, "total-bytesRx":0, "total-PacketsTx":0, "total-PacketsRx":0, "augmentation":0}
            if (evento=="IbrRP-PktTx"):
                capa="ibr"
                tipo="Tx"
            if (evento=="IbrRP-PktRx"):
                capa="ibr"
                tipo="Rx"
            if (evento=="ScmaStateMachine-PktTx"):
                capa="scma"
                tipo="Tx"
            if (evento=="ScmaStateMachine-PktRx"):
                capa="scma"
                tipo="Rx"

            if (capa!=""):
                captPkt=True
                if (capa=="scma" and tipo=="Tx"):
                    eventoInt=arrLinea[5].split(":")[1]
                    if (eventoInt==" SCMA - M_SendTaskToCN"):
                        #print ":"+evento+":"+str(nodo)
                        if (m_sendTask[nodo]==True):
                            captPkt=False
                            # contabilizzmos el paquete en ibr
                            #icapa="ibr"
                        m_sendTask[nodo]=True
                    if (eventoInt==" SCMA - M_CloudResourceInfo"):
                        if (m_cloudResourceTx[nodo]==True):
                            captPkt=False
                            # contabilizamos el paquete a IBR
                            #capa="ibr"
                        m_cloudResourceTx[nodo]=True
                if (capa=="scma" and tipo=="Rx"):
                    evento=arrLinea[6].split(":")[1]
                    if (eventoInt==" SCMA - M_CloudResourceInfo"):
                        if (m_cloudResourceRx[nodo]==True):
                            captPkt=False
                            # contabilizamos el paquete en ibr
                            #capa="ibr"
                        m_cloudResourceRx[nodo]=True


                #captPkt=True
                if captPkt==True:
                    #print "* "+capa+"-bytes"+tipo+"\t"+str(nodo)+"\t"+str(arrLinea[4].split(":")[1])+"\t"+str(datosNodo[nodo][capa+"-bytes"+tipo])
                    numBytes=int(arrLinea[4].split(":")[1])
                    datosNodo[nodo][capa+"-bytes"+tipo]+=numBytes
                    datosNodo[nodo][capa+"-Packets"+tipo]+=1
                    datosNodo[nodo]["total-bytes"+tipo]+=numBytes
                    datosNodo[nodo]["total-Packets"+tipo]+=1

                    if (capa=="ibr"):
                        IBR["bytes"+tipo]+=numBytes
                        IBR["Packets"+tipo]+=1
                #else:
                #    print "Pkt no capturado:\t"+str(nodo)+"\t"+eventoInt

            #contabilizamos el requerimiento de nube
            if (evento=="ScmaStateMachine-AugmentationRequest"):
                datosNodo[nodo]["augmentation"]+=1

            #contabilizamos una nueva nube
            if (evento=="ScmaStateMachine-TaskDistribution"):
                m_sendTask[nodo]=False
                m_cloudResourceTx[nodo]=False
                m_cloudResourceRx[nodo]=False

                datosNodo[nodo]["nubes"]+=1
                datosNodo[nodo]["t_start"]=float(tiempo)
                if (nodo not in nodosApp):
                    nodosApp.append(nodo)
            
            #contabilizamos la descarga terminada
            if (evento=="ScmaStateMachine-TaskFinished"):
                datosNodo[nodo]["descargas"]+=1
                datosNodo[nodo]["t_end"]=float(tiempo)
                datosNodo[nodo]["t_descarga"]+=datosNodo[nodo]["t_end"]-datosNodo[nodo]["t_start"]
                totalDescargas+=1
                m_sendTask[nodo]=False
                m_cloudResourceTx[nodo]=False
                m_cloudResourceRx[nodo]=False


    f.close()
    #obtenemos la info de los nodos que han tenido descargas
    for nApp in datosNodo:
        if (datosNodo[nApp]["descargas"]>0):
            print "Nodo: "+str(nApp)+" -> "+str(datosNodo[nApp]["descargas"])
            totalesNodo[nApp]=datosNodo[nApp]

    # para pruebas, dejamos el nodo con el mayor numero de descargas
    #totalesNodo={}
    #temp=0
    #for nApp in nodosApp:
    #    if (datosNodo[nApp]["descargas"]>0):
    #        if (temp==0):
    #            totalesNodo[0]=datosNodo[nApp]
    #        if (datosNodo[nApp]["descargas"]>totalesNodo[0]["descargas"]):
    #            totalesNodo[0]=datosNodo[nApp]
    #        temp+=1
    # **************************************************************

    #obtenemos los promedios por nodo/descargas
    numNodos=0
    downBase={}
    listTmp=[]


    for nodo in totalesNodo:
        listTmp.append(totalesNodo[nodo])
    
    #pp.pprint(totalesNodo)
    ##for nodo in totalesNodo:
        ##listTmp.append(totalesNodo[nodo])
        #print nodo
        #print numNodos
        ##if (numNodos==0):
        ##    downBase=totalesNodo[nodo]
        ##else:
        ##    a=downBase
        ##    b=totalesNodo[nodo]
        ##    #print "a:"
        ##    #print a
        ##    #print "b:"
        ##    #print b
        ##    #downBase=dict(a.items() + b.items() + [(k, a[k] + b[k]) for k in set(b) & set(a)])
        ##    downBase=sumaDicts(a,b)
        ##numNodos+=1
        ###print "res:"+str(nodo)
        ###print downBase
   
    #print "Totales Nodos"
    #pp.pprint(totalesNodo)
    print "Lista Tmp"
    pp.pprint(listTmp) 
    downBase=promediaDicts(listTmp)
    #print "Descargas: "+str(downBase["descargas"])

    downBase["numNodos"]=len(totalesNodo)
    downBase["totalDescargas"]=totalDescargas
    downBase["ratioRxTx"]=float(downBase["total-PacketsRx"])/float(downBase["total-PacketsTx"])
    downBase["scma-overhead"]=downBase["scma-bytesRx"]+downBase["scma-bytesTx"]
    downBase["ibr-bytesRx"]=IBR["bytesRx"]/int(downBase["totalDescargas"])
    downBase["ibr-bytesTx"]=IBR["bytesTx"]/int(downBase["totalDescargas"])
    downBase["ibr-PacketsRx"]=IBR["PacketsRx"]/int(downBase["totalDescargas"])
    downBase["ibr-PacketsTx"]=IBR["PacketsTx"]/int(downBase["totalDescargas"])
    downBase["ibr-overhead"]=downBase["ibr-bytesRx"]+downBase["ibr-bytesTx"]

    print "Final"
    pp.pprint(downBase)
    return downBase 


def sumaDicts(dA, dB):
    #print "dA"
    #print dictA
    #print "dB"
    #print dictB
    for key in dA:
        #print key+":"+str(type(dictA[key]))
        #if (isinstance(dA[key], (int, float))):
        #    #print str(dictA[key])+"+"+str(dictB[key])
        #    #dA[key]=float(dA[key])
        #    dA[key]+=float(dB[key])
        #    #print str(dictA[key])+"="
        if (isinstance(dA[key], dict)):
            for subKey in dA[key]:
                #dA[key][subKey]=float(dA[key][subKey])
                dA[key][subKey]+=float(dB[key][subKey])
        else:
            dA[key]+=float(dB[key])
    return dA

def promediaDicts(dictA):
    #print dictA
    dictSalida={}
    sal1={}
    numDatos=len(dictA)
    contador=0
    #totalizamos
    for i in range(0,numDatos):
    #for d1 in dictA:
        d1=dictA[i]
        print "*** a sumar: id "+str(i)
        #pp.pprint(d1)
        if (i==0):
            dictSalida=dictA[i]
        else:
            dA=dictSalida
            dB=dictA[i]
            
            for key in dA:
                print key
                dictSalida[key]=dA[key]+dB[key]
            
            

            #dictSalida=dA
            #print "*** parcial"
            #pp.pprint(dictSalida)
        contador+=1
    #promediamos
    #print "**** Numdatos: "+str(numDatos)
    #print "**** Totales"
    #pp.pprint(dictSalida)
    for key in dictSalida:
        #print key+":"+str(type(dictA[key]))
        if (isinstance(dictSalida[key], (int, float))):
            dictSalida[key]=dictSalida[key]/float(numDatos)
            #print str(dictA[key])+"="
        if (isinstance(dictSalida[key], dict)):
            for subKey in dictSalida[key]:
                dictSalida[key][subKey]=dictSalida[key][subKey]/float(numDatos)
    return dictSalida

if __name__ == "__main__":
    #programa principal
    pp=pprint.PrettyPrinter(indent=4)
    carpetas=['25', '37', '50', '62', '75']
    dir=sys.argv[1]+"/"
    print "Procesando directorio: "+dir
    
    #print procesaArchivo(dir+listaArch[0])
    dictFinal={}
    
    for carpeta in carpetas:
        carpetaDatos=dir+carpeta
        listaArch=os.listdir(carpetaDatos)
        for archivo in listaArch:
            archivoDatos=carpetaDatos+'/'+archivo
            print archivoDatos
            datosTemp=getInfoNode(archivoDatos)
            if (carpeta not in dictFinal):
                dictFinal[carpeta]=[]
            dictFinal[carpeta].append(datosTemp)
            #print str(carpeta)+": tam: "+str(len(dictFinal[carpeta]))
    #obtenemos los promedios
    for porcColabs in dictFinal:
        #print "*** "+str(porcColabs)
        #pp.pprint(dictFinal[porcColabs])
        promedio=promediaDicts(dictFinal[porcColabs])
        dictFinal[porcColabs]=promedio
        #pp.pprint(promedio)
    print ""
    print "********** FINAL **********"
    pp.pprint(dictFinal)
    print "* Generando archivos de datos individuales"
    listKeys=["totalDescargas", "t_descarga", "nubes", "scma-PacketsRx", "scma-PacketsTx", "scma-bytesRx", "scma-bytesTx", "ibr-PacketsRx", "ibr-PacketsTx", "ibr-bytesRx", "ibr-bytesTx", "ratioRxTx", "nubes", "numNodos", "augmentation","scma-overhead", "ibr-overhead"]
    for keyArch in listKeys:
        arch=open(keyArch+'.dat', 'w')
        print "Procesando: "+keyArch
        for vals in carpetas:
            valor=0
            valor=dictFinal[str(vals)][keyArch]
            #if ("-" in keyArch):                
            #    arrKeys=keyArch.split("-")
            #    valor=dictFinal[str(vals)][arrKeys[0]][arrKeys[1]]/float(dictFinal[str(vals)]["totalDescargas"])
            #else:
            #    valor=dictFinal[str(vals)][keyArch]
            linea=str(vals)+"\t"+str(float(valor))
            
            arch.write(linea+os.linesep)
        arch.close()
    os.system('gnuplot gnuplotNodos.gp')
