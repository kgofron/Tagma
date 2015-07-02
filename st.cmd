## Example RTEMS startup script

## You may have to change cfMemMap to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("dbd/cfMemMap.dbd")
dbLoadDatabase("dbd/nslsdetRecord.dbd")

cfMemMap_registerRecordDeviceDriver(pdbbase) 

#initialize DACS, ADCs and detector ASICs
devcfSPIConfig(0,1,8)
devcfSPIConfig(1,1,8)
devcfSPIConfig(6,1,8)
devNslsDetConfig(1,1,640)

## Load record instances
dbLoadRecords("db/cfSpiDacs.db","B=XF:10ID-ES,D=det1{Det-Tagma:1}")
dbLoadRecords("db/cfSpiDacs2.db","B=XF:10ID-ES,D=det1{Det-Tagma:1}")
dbLoadRecords("db/cfSpiAdcs.db","B=XF:10ID-ES,D=det1{Det-Tagma:1}")
dbLoadRecords("db/det1.db","B=XF:10ID-ES,D=det1{Det-Tagma:1}")

## Set this to see messages from mySub
#var mySubDebug 1
#var cfMemMapDebug 12
#var devnslsdetdebug 0

iocInit()
