TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += cfMemMapSupport

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += nslsdetRecord
# install devXxxSoft.dbd into <top>/dbd
#DBD += cfMemMapSupport.dbd

# The following are compiled and added to the Support library
cfMemMapSupport_SRCS += devCfMemMap_bi_bo.c
cfMemMapSupport_SRCS += devCfSPI.c
cfMemMapSupport_SRCS += nslsdetRecord.c
cfMemMapSupport_SRCS += devNslsdet.c
cfMemMapSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# build an ioc application

PROD_IOC = cfMemMap
# <name>.dbd will be created from <name>Include.dbd
DBD += cfMemMap.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
cfMemMap_SRCS += cfMemMap_registerRecordDeviceDriver.cpp
cfMemMap_SRCS_DEFAULT += cfMemMapMain.cpp
cfMemMap_SRCS_vxWorks += -nil-

# Add locally compiled object code
#cfMemMap_SRCS += dbSubExample.c

cfMemMap_LIBS += cfMemMapSupport

# NOTES:
#      1)It is not possible to build sncExample both as a component of cfMemMap
#        and standalone. You must choose only one.
#      2)To build sncExample SNCSEQ must be defined in <top>/configure/RELEASE

# The following builds sncExample as a component of cfMemMap
#cfMemMap_SRCS += sncExample.stt
#cfMemMap_LIBS += seq pv

cfMemMap_LIBS += $(EPICS_BASE_IOC_LIBS)

# The following builds sncExample as a standalone application
#PROD_HOST += sncExample
#sncExample_SNCFLAGS += +m
#sncExample_SRCS += sncExample.stt
#sncExample_LIBS += seq pv
#sncExample_LIBS += $(EPICS_BASE_HOST_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
#define _RamSize=0xff0000

