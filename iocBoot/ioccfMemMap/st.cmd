## NSLS HERMES detector RTEMS startup script


< envPaths
epicsEnvSet(EPICS_CA_MAX_ARRAY_BYTES,65536)


## Register all support components
dbLoadDatabase("dbd/cfMemMap.dbd")
dbLoadDatabase("dbd/nslsdetRecord.dbd")

cfMemMap_registerRecordDeviceDriver(pdbbase)
 
#initialize cfMemMap
#devcfMemMapConfig(0,0x10000096,1)
devcfSPIConfig(0,1,8)
devcfSPIConfig(1,1,8)
devcfSPIConfig(6,1,8)
devNslsDetConfig(1,1,64)

## Load record instances
dbLoadRecords("db/cfSpiDacs.db")
dbLoadRecords("db/cfSpiDacs2.db")
dbLoadRecords("db/cfSpiAdcs.db")
dbLoadRecords("db/det1.db")

#var cfMemMapDebug 12
#var devnslsdetdebug 0

iocInit()
