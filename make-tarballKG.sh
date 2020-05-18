
#!/bin/sh

set -ex

#make -w

#
# Create NFS-accessable area
#
#rm -rf /tftpboot/epics/det1/*
#cp -r db dbd /tftpboot/epics/det1
#mkdir -p /tftpboot/epics/det1/bin/RTEMS-uC5272
#cp bin/RTEMS-uC5272/cfMemMap.boot /tftpboot/epics/det1/bin/ucdimm.boot
#mkdir -p /tftpboot/epics/det1/iocBoot/ioccfMemMap
#cat >/tftpboot/epics/det1/iocBoot/ioccfMemMap/st.cmd <<\!!!
#epicsEnvSet(ARCH,"RTEMS-uC5272")
#epicsEnvSet(IOC,"ioccfMemMap")
#epicsEnvSet(TOP,"/epics/det1")
#epicsEnvSet(USER,"ColdFire")
#cd "${TOP}"
#!!!
#cat st.cmd >>/tftpboot/epics/det1/iocBoot/ioccfMemMap/st.cmd


#
# Create and download a standalone (NVRAM-based) IOC image
#
chmod -R a+w tarball/*
rm -rf tarball
mkdir tarball
cp -r db dbd tarball
cat >tarball/st.cmd <<\!!!
epicsEnvSet(ARCH,"RTEMS-uC5272")
epicsEnvSet(IOC,"ioccfMemMap")
epicsEnvSet(TOP,"/")
epicsEnvSet(USER,"ColdFire")
epicsEnvSet(EPICS_CA_MAX_ARRAY_BYTES,65536)
cd "${TOP}"
!!!
cat st.cmd >>tarball/st.cmd
chmod -R a-w tarball/*
(
cat bin/RTEMS-uC5272/cfMemMap.boot
(cd tarball ; tar cf - *)
) >bin/RTEMS-uC5272/cfMemMap.image
# ucSend bin/RTEMS-uC5282/vlinac.image
