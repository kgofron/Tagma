/*******************************************************************************
nslsdetRecord.c
Record support for NSLS HERMES-X (up to 640 element) silicon detector

Modified by D. Peter Siddons 11/03 from

scalerRecord.c
Record-support routines for <= 32-channel, 32-bit scaler

Original Author: Tim Mooney
Date: 1/16/95

Experimental Physics and Industrial Control System (EPICS)

Copyright 1995, the University of Chicago Board of Governors.

This software was produced under U.S. Government contract
W-31-109-ENG-38 at Argonne National Laboratory.

Initial development by:
	The X-ray Optics Group
	Experimental Facilities Division
	Advanced Photon Source
	Argonne National Laboratory

OSI by S. Kate Feng, NSLS, BNL 3/03

Modification Log:
-----------------
.01  6/26/93	tmm     Lecroy-scaler record
.02  1/16/95    tmm     Joerger-scaler
.03  8/28/95    tmm     Added .vers (code version) and .card (VME-card number)
                        fields 
.04  2/8/96     tmm     v1.7:  Fixed bug: was posting CNT field several times
                        when done.
.05  2/21/96    tmm     v1.71:  precision of vers field is 2
.06  6/5/96     tmm     v1.8: precision defaults to PREC field
.07  8/16/96    tmm     v2.0: conversion to EPICS 3.13
.07  8/16/96    tmm     v2.1: fixed off-by-one problem (in 3.13 version)
.09  2/27/97    tmm     v2.11: fix TP/PR1 problem
.10  3/03/97    tmm     v3.0: allow auto-count overridden by user count
.11  7/09/97    tmm     v3.1: init_record wasn't posting TP, PR1, FREQ, or CARD
.12 11/14/97    tmm     v3.2: fixed bug: if .cnt went true then false during the
                        .dly period, scaler would lock up.
.13  4/24/98    tmm     v3.3 call recGblFwdLink only when user count completes.
.14  10/2/98    tmm     v3.4 if dbPutNotify then delay longer before resuming
                        autocount.
.15  3/24/99    tmm     v3.5 call recGblFwdLink whenever scaler is idle and
						CNT = 0.
.16  4/21/99    tmm     v3.6 call recGblFwdLink whenever USER_STATE_IDLE and
						SCALER_STATE_IDLE and CNT makes a transition to 0.
.17  7/14/99    tmm     v3.7 minor fixes
.18 11/04/99    tmm     v3.8 added link to start and stop external process that
                        should coincide with scaler-integration period.
.19  7/14/99    tmm     v3.9 hold time before autocount wipes scalers increased
                        to 5 sec for all count requests.
.20  ?          ?       v3.10 changed max number of signals from 16 to 32
.21  11/12/01   tmm     v3.11 hold time before autocount wipes scalers is
                        volatile int
.22  01/08/02   tmm     v3.12 Set VAL field to T and post after completed count
.23  11/03/03   dps	mods for NSLS silicon multi-detector, put version to 0.10
*******************************************************************************/
#define VERSION 0.90

#include        <epicsVersion.h>

#if EPICS_VERSION < 3 || (EPICS_VERSION==3 && EPICS_REVISION < 14)
#define NOT_YET_OSI
#endif

#if defined(NOT_YET_OSI) || defined(VXWORKSTARGET)
#include	<vxWorks.h>
#ifndef __vxworks
#define __vxworks 1
#endif
#include	<types.h>
#include	<stdio.h>
#include	<stdioLib.h>
#include	<lstLib.h>
#include	<string.h>
#include	<wdLib.h>
#else
#include        <epicsTimer.h>

extern epicsTimerQueueId	nslsdetWdTimerQ;

#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<alarm.h>
#include	<callback.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include        <dbScan.h>
#include        <dbEvent.h>
#include	<dbFldTypes.h>
#include	<errMdef.h>
#include	<recSup.h>
#include        <recGbl.h>
#include	<devSup.h>
#include	<special.h>
#include 	<iocsh.h>
#include	<epicsExport.h>
#define GEN_SIZE_OFFSET
#include	"nslsdetRecord.h"
#undef GEN_SIZE_OFFSET
#include	"devNslsdet.h"

#define nslsdet_STATE_IDLE 0
#define nslsdet_STATE_WAITING 1
#define nslsdet_STATE_COUNTING 2

#define USER_STATE_IDLE 0
#define USER_STATE_WAITING 1
#define USER_STATE_REQSTART 2
#define USER_STATE_COUNTING 3

#ifdef NODEBUG
#define Debug(l,FMT,V) ;
#else
#define Debug(l,FMT,V) {  if(l <= nslsdetRecordDebug) \
			{ printf("%s(%d):",__FILE__,__LINE__); \
			  printf(FMT,V); } }
#endif
volatile int nslsdetRecordDebug = 0;
epicsExportAddress(int,nslsdetRecordDebug);
volatile int nslsdet_wait_time = 3;
epicsExportAddress(int,nslsdet_wait_time);

#define MIN(a,b) (a)<(b)?(a):(b)
#define MAX(a,b) (a)>(b)?(a):(b)


extern Nslsdet_state nslsdet_state;

/* Create RSET - Record Support Entry Table*/
#define report NULL
#define initialize NULL
static long init_record();
static long process();
static long special();
#define get_value NULL
static long cvt_dbaddr();
static long get_array_info();
static long put_array_info();
#define get_units NULL
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
#define get_graphic_double NULL
#define get_control_double NULL
#define get_alarm_double NULL

rset nslsdetRSET = {
	RSETNUMBER,
	report,
	initialize,
	init_record,
	process,
	special,
	get_value,
	cvt_dbaddr,
	get_array_info,
	put_array_info,
	get_units,
	get_precision,
	get_enum_str,
	get_enum_strs,
	put_enum_str,
	get_graphic_double,
	get_control_double,
	get_alarm_double
};
epicsExportAddress(rset,nslsdetRSET);

static void do_alarm();
static void monitor();

static void deviceCallbackFunc(struct callback *pcb)
{
    struct dbCommon *precord=pcb->precord;

    dbScanLock(precord);
    process(precord);
    dbScanUnlock(precord);
}

static void userCallbackFunc(struct callback *pcb)
{
    nslsdetRecord *pscal = (nslsdetRecord *)pcb->precord;

	/*
	 * User asked us to start counting after a delay that has now expired.
	 * If user has not rescinded that request in the meantime, tell
	 * process() to start counting.
	 */
	if (pscal->us == USER_STATE_WAITING && pscal->cnt) {
		pscal->us = USER_STATE_REQSTART;
		(void)scanOnce((void *)pscal);
	}
}

static void autoCallbackFunc(struct callback *pcb)
{
    nslsdetRecord *pscal = (nslsdetRecord *)pcb->precord;

	(void)scanOnce((void *)pscal);
}

static long cvt_dbaddr(paddr)
struct dbAddr *paddr;
{
   int index;
   index=dbGetFieldIndex(paddr);
   
   nslsdetRecord *pnslsdet=(nslsdetRecord *)paddr->precord;
   switch(index){
   case nslsdetRecordVAL:{
      paddr->pfield = (void *)(pnslsdet->bptr);
      paddr->no_elements = 3;
      paddr->field_type = DBF_LONG;
      paddr->field_size = sizeof(long);
      paddr->dbr_field_type = DBF_LONG;
      break;
      }
    case nslsdetRecordS1:{
      paddr->pfield = (void *)(pnslsdet->ps1);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_LONG;
      paddr->field_size = sizeof(long);
      paddr->dbr_field_type = DBF_LONG;
      break;
      }
    case nslsdetRecordS2:{
      paddr->pfield = (void *)(pnslsdet->ps2);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_LONG;
      paddr->field_size = sizeof(long);
      paddr->dbr_field_type = DBF_LONG;
      break;
      }
    case nslsdetRecordS3:{
      paddr->pfield = (void *)(pnslsdet->ps3);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_LONG;
      paddr->field_size = sizeof(long);
      paddr->dbr_field_type = DBF_LONG;
      break;
      }
    case nslsdetRecordTR1:{
      paddr->pfield = (void *)(pnslsdet->ptr1);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    case nslsdetRecordTR2:{
      paddr->pfield = (void *)(pnslsdet->ptr2);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    case nslsdetRecordTR3:{
      paddr->pfield = (void *)(pnslsdet->ptr3);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    case nslsdetRecordTR4:{
      paddr->pfield = (void *)(pnslsdet->ptr4);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    case nslsdetRecordCHEN:{
      paddr->pfield = (void *)(pnslsdet->pchen);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    case nslsdetRecordTSEN:{
      paddr->pfield = (void *)(pnslsdet->ptsen);
      paddr->no_elements = pnslsdet->nelm;
      paddr->field_type = DBF_CHAR;
      paddr->field_size = sizeof(char);
      paddr->dbr_field_type = DBF_CHAR;
      paddr->special = SPC_MOD;
      break;
      }
    }
   return(0);
}

static long get_array_info(paddr,no_elements,offset)
struct dbAddr *paddr;
long *no_elements;
long *offset;
{
   nslsdetRecord *pnslsdet=(nslsdetRecord *)paddr->precord;

   *no_elements =  pnslsdet->nord;
   *offset = 0;
   return(0);
}

static long put_array_info(paddr,nNew)
struct dbAddr *paddr;
long nNew;
{
nslsdetRecord *pnslsdet=(nslsdetRecord *)paddr->precord;

   pnslsdet->nord = nNew;
   if (pnslsdet->nord > pnslsdet->nelm) pnslsdet->nord = pnslsdet->nelm;
   return(0);
}





static long init_record(pscal,pass)
nslsdetRecord *pscal;
int pass;
{

	long status;
	int i;
	long *val;
	int *s1, *s2, *s3;
	char *tr1, *tr2, *tr3, *tr4, *chen, *tsen;
	
	nslsdetDSET *pdset = (nslsdetDSET *)(pscal->dset);
	struct callback *pcallbacks, *puserCallback, *pautoCallback, *pdeviceCallback;

	printf("nslsdet init_record: pass = %d\n", pass);
	if (pass == 0) {
		if(pscal->nelm<=0) pscal->nelm=1;
        	pscal->bptr =  (char *)calloc(3,sizeof(long));
		pscal->ptr1 =  (char *)calloc(pscal->nelm,sizeof(char));
		pscal->ptr2 =  (char *)calloc(pscal->nelm,sizeof(char));
		pscal->ptr3 =  (char *)calloc(pscal->nelm,sizeof(char));
		pscal->ptr4 =  (char *)calloc(pscal->nelm,sizeof(char));
		pscal->pchen = (char *)calloc(pscal->nelm,sizeof(char));
		pscal->ptsen = (char *)calloc(pscal->nelm,sizeof(char));
		pscal->ps1 =   (char *) calloc(pscal->nelm,sizeof(int));
		pscal->ps2 =   (char *) calloc(pscal->nelm,sizeof(int));
		pscal->ps3 =   (char *) calloc(pscal->nelm,sizeof(int));
	        if(pscal->nelm==1) {
        	    pscal->nord = 1;
        	} else {
        	    pscal->nord = 0;
        	}
		pscal->vers = VERSION;
		/* Gotta have a .val field.  Make its value reproducible. */
		val = pscal->bptr;
		val[0] = 0;
		val[1] = 0;
		val[2] = 0;
		
		tr1=pscal->ptr1;
		tr2=pscal->ptr2;
		tr3=pscal->ptr3;
		tr4=pscal->ptr4;
		chen=pscal->pchen; 
		tsen=pscal->ptsen; 
		s1=pscal->ps1;
		s2=pscal->ps2;
		s3=pscal->ps3;
		
		for(i=0;i<pscal->nelm;i++){
			tr1[i] = 32;
			tr2[i] = 32;
			tr3[i] = 32;
			tr4[i] = 32;
			chen[i] = 0; 
			tsen[i] = 0; 
			s1[i] = 0;
			s2[i] = 0;
			s3[i] = 0;
			}
		stuffit(pscal);
		return (0);
	}

	
	/*** setup callback stuff (note: array of 3 callback structures) ***/
	pcallbacks = (struct callback *)(calloc(3,sizeof(struct callback)));
	pscal->dpvt = (void *)pcallbacks;

	/* first callback to implement delay */
	puserCallback = (struct callback *)&(pcallbacks[0]);
	callbackSetCallback(userCallbackFunc,&puserCallback->callback);
	callbackSetPriority(pscal->prio,&puserCallback->callback);
	puserCallback->precord = (struct dbCommon *)pscal;
 	puserCallback->wd_id = epicsTimerQueueCreateTimer( nslsdetWdTimerQ, (void(*)())callbackRequest,puserCallback );

	/* second callback to implement auto-count */
	pautoCallback = (struct callback *)&(pcallbacks[1]);
	callbackSetCallback(autoCallbackFunc,&pautoCallback->callback);
	callbackSetPriority(pscal->prio,&pautoCallback->callback);
	pautoCallback->precord = (struct dbCommon *)pscal;
 	pautoCallback->wd_id = epicsTimerQueueCreateTimer( nslsdetWdTimerQ, (void(*)())callbackRequest,pautoCallback );

	/* third callback for device support */
	pdeviceCallback = (struct callback *)&(pcallbacks[2]);
	callbackSetCallback(deviceCallbackFunc,&pdeviceCallback->callback);
	callbackSetPriority(pscal->prio,&pdeviceCallback->callback);
	callbackSetUser((struct dbCommon *)pscal,&pdeviceCallback->callback);
	pdeviceCallback->precord = (struct dbCommon *)pscal;

	/* Check that we have everything we need. */
	if (!(pdset = (nslsdetDSET *)(pscal->dset)))
	{
		recGblRecordError(S_dev_noDSET,(void *)pscal, "nslsdet: init_record");
		return(S_dev_noDSET);
	}

	Debug(2, "init_record: calling dset->init_record %d\n", 0);
	if (pdset->init_record)
	{
		status=(*pdset->init_record)(pscal);
		Debug(3, "init_record: dset->init_record returns %d\n", status);
		if (status) {
			pscal->card = -1;
			return (status);
		}
		pscal->card = pscal->inp.value.vmeio.card;
		db_post_events(pscal,&(pscal->card),DBE_VALUE);
	}

	/* default clock freq */
	if (pscal->freq == 0.0) {
		pscal->freq = 1000.0;
		db_post_events(pscal,&(pscal->freq),DBE_VALUE);
	}

	/* default count time */
	if ((pscal->tp == 0.0) && (pscal->pr1 == 0)) {
		pscal->tp = 1.0;
		db_post_events(pscal,&(pscal->pr1),DBE_VALUE);
	}

	/* convert between time and clock ticks */
	if (pscal->tp) {
		/* convert time to clock ticks */
		pscal->pr1 = (long) (pscal->tp * pscal->freq);
		db_post_events(pscal,&(pscal->pr1),DBE_VALUE);
		Debug(3, "init_record: .TP != 0, so .PR1 set to %ld\n", pscal->pr1);
	} else if (pscal->pr1 && pscal->freq) {
		/* convert clock ticks to time */
		pscal->tp = (double)(pscal->pr1 / pscal->freq);
		db_post_events(pscal,&(pscal->tp),DBE_VALUE);
		Debug(3, "init_record: .PR1/.FREQ != 0, so .TP set to %f\n", pscal->tp);
	}
	db_post_events(pscal,&(pscal->bptr),DBE_VALUE);
	db_post_events(pscal,&(pscal->ptr1),DBE_VALUE);
	db_post_events(pscal,&(pscal->ptr2),DBE_VALUE);
	db_post_events(pscal,&(pscal->ptr3),DBE_VALUE);
	db_post_events(pscal,&(pscal->ptr4),DBE_VALUE);
	db_post_events(pscal,&(pscal->pchen),DBE_VALUE);
	db_post_events(pscal,&(pscal->ptsen),DBE_VALUE);
	db_post_events(pscal,&(pscal->ps1),DBE_VALUE);
	db_post_events(pscal,&(pscal->ps2),DBE_VALUE);
	db_post_events(pscal,&(pscal->ps3),DBE_VALUE);
	return(0);
}


static long process(pscal)
nslsdetRecord *pscal;
{
	int status, prev_nslsdet_state;
	int justFinishedUserCount=0, justStartedUserCount=0, putNotifyOperation=0;
	int *ppreset = &(pscal->pr1);
	/*short *pdir = &pscal->d1;*/
	short *pgate = &pscal->g1;
	nslsdetDSET *pdset = (nslsdetDSET *)(pscal->dset);
	struct callback *pcallbacks = (struct callback *)pscal->dpvt;
	struct callback *puserCallback = (struct callback *)&(pcallbacks[0]);
	struct callback *pautoCallback = (struct callback *)&(pcallbacks[1]);

	Debug(5, "process: entry %d\n", 0);
	pscal->pact = TRUE;
	pscal->udf = FALSE;
	prev_nslsdet_state = pscal->ss;

	/* If we're being called as a result of a done-counting interrupt, */
	/* (*pdset->done)(card) will return TRUE */
	if ((*pdset->done)()) {
		pscal->ss = nslsdet_STATE_IDLE;
		/* Auto-count cycle is not allowed to reset .CNT field. */
		if (pscal->us == USER_STATE_COUNTING) {
			pscal->cnt = 0;
			pscal->us = USER_STATE_IDLE;
			justFinishedUserCount = 1;
			if (pscal->ppn) putNotifyOperation = 1;
		}
	}

	if (pscal->cnt != pscal->pcnt) {
		int handled = 0;
		if (pscal->cnt && ((pscal->us == USER_STATE_REQSTART) ||
				(pscal->us == USER_STATE_WAITING))) {
			/*** if we're already counting (auto-count), stop ***/
			if (pscal->ss == nslsdet_STATE_COUNTING) {
				(*pdset->arm)(0);
				pscal->ss = nslsdet_STATE_IDLE;
			}

			if (pscal->us == USER_STATE_REQSTART) {
				/*** start counting ***/
				
				(*pdset->reset)();				
				(*pdset->write_preset)((int)(pscal->tp*pscal->freq));
				(*pdset->arm)(1);
				
				pscal->ss = nslsdet_STATE_COUNTING;
				pscal->us = USER_STATE_COUNTING;
				handled = 1;
				justStartedUserCount = 1;
			}
		} else if (!pscal->cnt) {
			/*** stop counting ***/
			(*pdset->arm)(0);
			pscal->ss = nslsdet_STATE_IDLE;
			pscal->us = USER_STATE_IDLE;
			justFinishedUserCount = 1;
			handled = 1;
		}
		if (handled) {
			pscal->pcnt = pscal->cnt;
			Debug(2, "process: posting done flag (%d)\n", pscal->cnt);
			db_post_events(pscal,&(pscal->cnt),DBE_VALUE);
		}
	}

	if (justStartedUserCount || justFinishedUserCount) {
		/* fire .cout link to trigger anything that should coincide with scaler integration */
		status = dbPutLink(&pscal->cout, DBR_SHORT, &pscal->cnt, 1);
		if (!RTN_SUCCESS(status)) {
			Debug(5, "nslsdet:process: ERROR %d PUTTING TO COUT LINK.\n", status);
		}
	}

	/* done counting? */
	if (pscal->ss == nslsdet_STATE_IDLE) {
		recGblGetTimeStamp(pscal);
		do_alarm(pscal);
		monitor(pscal);
		
		if ((pscal->pcnt==0) && (pscal->us == USER_STATE_IDLE)) {
			if (prev_nslsdet_state == nslsdet_STATE_COUNTING) {
				
				nslsdet_state.newdata=1; /* tell device support that we acquired new data */
				(*pdset->read)(pscal); /* read data */
				db_post_events(pscal,pscal->bptr,DBE_VALUE);
				db_post_events(pscal,pscal->ps1,DBE_VALUE);
				db_post_events(pscal,pscal->ps2,DBE_VALUE);
				db_post_events(pscal,pscal->ps3,DBE_VALUE);
			}
			recGblFwdLink(pscal);
		}
	}

	/* Are we in auto-count mode and not already counting? */
	if (pscal->us == USER_STATE_IDLE && pscal->cont &&
		pscal->ss != nslsdet_STATE_COUNTING) {

	        double dly_sec=0.0;  /* seconds to delay */

		if (justFinishedUserCount) dly_sec = MAX(pscal->dly1, nslsdet_wait_time);
		if (putNotifyOperation) dly_sec = MAX(pscal->dly1, nslsdet_wait_time);
		/* if (we-have-to-wait && we-haven't-already-waited) { */
		if ( dly_sec > 0 && pscal->ss != nslsdet_STATE_WAITING) {
			Debug(5, "process: scheduling autocount restart %d\n", 0);
			/* Schedule a callback, and make a note that counting should start
			 * the next time we process (if pscal->ss is still 1 at that time).
			 */
			pscal->ss = nslsdet_STATE_WAITING;  /* tell ourselves to start next time */
			epicsTimerStartDelay(pautoCallback->wd_id, dly_sec);
		}

	       else {
			Debug(5, "process: restarting autocount %d\n", 0);
			/* Either the delay time is zero, or pscal->ss = nslsdet_STATE_WAITING
			 * (we've already waited), so start auto-count counting.
			 * Different rules apply for auto-count counting:
			 * If (.TP1 >= .001 s) we count .TP1 seconds, regardless of any
			 * presets the user may have set.
			 */
			(*pdset->reset)();
			if (pscal->tp1 >= 1.e-3) {
				(*pdset->write_preset)((int)(pscal->tp1*pscal->freq));
			}
			
			(*pdset->arm)(1);
			pscal->ss = nslsdet_STATE_COUNTING;

		}
	}

	pscal->pact = FALSE;
	return(0);
}

extern int stuffit(void *pscal);

static long special(paddr,after)
struct dbAddr *paddr;
int	after;
{
	nslsdetRecord *pscal = (nslsdetRecord *)(paddr->precord);
	nslsdetDSET *pdset = (nslsdetDSET *)(pscal->dset);
	int special_type = paddr->special;
	int i=0;
	int modified;
	/*unsigned short *pdir, *pgate; */
	/*long *ppreset; */
	struct callback *pcallbacks = (struct callback *)pscal->dpvt;
	struct callback *puserCallback = (struct callback *)&(pcallbacks[0]);
    int fieldIndex = dbGetFieldIndex(paddr);

	Debug(5, "special: entry; after=%d\n", after);
	if (!after) return (0);

	modified=0;
	
	switch (fieldIndex) {
	case nslsdetRecordCNT:
		/* Ignore redundant (pscal->cnt == 1) commands */
		if (pscal->cnt && (pscal->us != USER_STATE_IDLE)) return(0);

		/* Scan record if it's not Passive.  (If it's Passive, it'll get */
		/* scanned automatically, since .cnt is a Process-Passive field.) */
		/* Arrange to process after user-specified delay time */
		callbackSetPriority(pscal->prio,&puserCallback->callback);
                i = pscal->dly;   /* seconds to delay */

		if (i<0) i = 0;
		if (i == 0 || pscal->cnt == 0) {
			/*** handle request now ***/
			if (pscal->cnt) {
				/* start counting */
				pscal->us = USER_STATE_REQSTART;
			} else {
				/* abort any counting or request to start counting */
				switch (pscal->us) {
				case USER_STATE_WAITING:
					/* We may have a watchdog timer going.  Cancel it. */
					epicsTimerCancel(puserCallback->wd_id);
					pscal->us = USER_STATE_IDLE;
					break;
				case USER_STATE_REQSTART:
					pscal->us = USER_STATE_IDLE;
					break;
				default:
					break;
				}
			}
			if (pscal->scan) scanOnce((void *)pscal);
		}
		else {
			/* schedule callback to handle request */
			pscal->us = USER_STATE_WAITING;
			epicsTimerStartDelay(puserCallback->wd_id, pscal->dly);
		}
		break;

	case nslsdetRecordCONT:
		/* Scan record if it's not Passive.  (If it's Passive, it'll get */
		/* scanned automatically, since .cont is a Process-Passive field.) */
		if (pscal->scan) scanOnce((void *)pscal);
		break;

	case nslsdetRecordTP:
		/* convert time to clock ticks */
		pscal->pr1 = (long) (pscal->tp * pscal->freq);
		(*pdset->write_preset)((int)(pscal->tp*pscal->freq));
		db_post_events(pscal,&(pscal->pr1),DBE_VALUE);
		break;

	case nslsdetRecordPR1:
		/* convert clock ticks to time */
		pscal->tp = (double)(pscal->pr1 / pscal->freq);
		(*pdset->write_preset)((int)(pscal->pr1));
		db_post_events(pscal,&(pscal->tp),DBE_VALUE);
		break;

	case nslsdetRecordAOEN: /* set channel number which has analog output enabled */
		Debug(2, "special: AOEN %i\n", pscal->aoen);
		/* bits set by device support. Tell record to write hardware */
		modified=1;
		db_post_events(pscal,&(pscal->aoen),DBE_VALUE);
		break;
	case nslsdetRecordLOEN: /* set channel which has leakage monitor out enabled */
		Debug(2, "special: LOEN\n %i", pscal->loen);
		/* write hardware */
		modified=1; 
		db_post_events(pscal,&(pscal->loen),DBE_VALUE);
		break;
	case nslsdetRecordEBLK: /* Enable on-chip bias current generator */
		Debug(2, "special: EBLK\n %i", pscal->eblk);
		/* write hardware */
		modified=1; 
		db_post_events(pscal,&(pscal->eblk),DBE_VALUE);
		break;
	case nslsdetRecordSHPT: /* set shaping time */
		Debug(2, "special: SHPT %i\n", pscal->shpt);
		modified=1;
		db_post_events(pscal,&(pscal->shpt),DBE_VALUE);
		break;
		
	case nslsdetRecordCHEN: /* load 'channel-disabled' array */ 
		modified=1;
		db_post_events(pscal,&(pscal->pchen),DBE_VALUE);/* post change */
		Debug(2, "special: CHEN %i\n", 0);
		break;
		
	case nslsdetRecordTSEN: /* load 'test pulse input enabled' array */
		modified=1;
		Debug(2, "special: TSEN %i\n", 0);
		db_post_events(pscal,&(pscal->ptsen),DBE_VALUE);
		break;
		
	case nslsdetRecordTR1:  /* array of trim DAC values */
		modified=1;
		Debug(2, "special: TR1 %i\n", 0);
		db_post_events(pscal,&(pscal->ptr1),DBE_VALUE);
		break;
		
	case nslsdetRecordTR2:  /* array of trim DAC values */
		modified=1;
		Debug(2, "special: TR2 %i\n", 0);
		db_post_events(pscal,&(pscal->ptr2),DBE_VALUE);
		break;
		
	case nslsdetRecordTR3:  /* array of trim DAC values */
		modified=1;
		Debug(2, "special: TR3 %i\n", 0);
		db_post_events(pscal,&(pscal->ptr3),DBE_VALUE);
		break;
		
	case nslsdetRecordTR4:  /* array of trim DAC values */
		modified=1;
		Debug(2, "special: TR4 %i\n", 0);
		db_post_events(pscal,&(pscal->ptr4),DBE_VALUE);
		break;
		
	case nslsdetRecordGAIN: /* set gain Hi/Lo */
		modified=1;
		Debug(2, "special: GAIN %i\n", 0);
		db_post_events(pscal,&(pscal->gain),DBE_VALUE);
		break;

	case nslsdetRecordG1:   /* set internal or external gate drive */
		Debug(2, "special: G1 %i\n", 0);
		db_post_events(pscal,&(pscal->g1),DBE_VALUE);
		break;

	default:
		Debug(2, "special: Bad field index\n", 0);
		break;
	} /* switch (fieldIndex) */
	if(modified){
		stuffit(pscal);
		/*eeprom_write_all();*/
		}
	return(0);
}

static long get_precision(paddr, precision)
struct dbAddr *paddr;
long          *precision;
{
	nslsdetRecord *pscal = (nslsdetRecord *) paddr->precord;
    int fieldIndex = dbGetFieldIndex(paddr);

	*precision = pscal->prec;
	if (fieldIndex == nslsdetRecordVERS) {
		*precision = 2;
	} else if (fieldIndex >= nslsdetRecordVAL) {
		*precision = pscal->prec;
	} else {
		recGblGetPrec(paddr, precision);	/* Field is in dbCommon */
	}
	return (0);
}


static void do_alarm(pscal)
nslsdetRecord *pscal;
{
	if(pscal->udf == TRUE ){
		recGblSetSevr(pscal,UDF_ALARM,INVALID_ALARM);
		return;
	}
	return;
}

static void monitor(pscal)
nslsdetRecord *pscal;
{
	unsigned short monitor_mask;

	monitor_mask = recGblResetAlarms(pscal);

	monitor_mask|=(DBE_VALUE|DBE_LOG);

	/* check all value fields for changes */
	return;
}

void recpstate(void){
  int i;
	printf("In record support:\n");
	printf("\n\ncard_exists %i\n", nslsdet_state.card_exists);
	printf("num_channels %i\n", nslsdet_state.num_channels);
	printf("card_in_use %i\n", nslsdet_state.card_in_use);
	printf("count_in_progress %i\n", nslsdet_state.count_in_progress); /* count in progress? */
	printf("ident %i\n", nslsdet_state.ident); /* identification info for this card */
	printf("ioscanpvt %x\n", nslsdet_state.ioscanpvt);
	printf("done %i\n", nslsdet_state.done);
	printf("gate %i\n", nslsdet_state.gate);
	printf("tp %i\n", nslsdet_state.tp); /* time in milliseconds */
	printf("mode %x\n", nslsdet_state.mode); /* timer setup word */
	printf("clkreg %i\n", nslsdet_state.clkreg); /* compare register contents */
	printf("delta %i\n", nslsdet_state.delta); /* timer startup delay */
	printf("pcallback %x\n", nslsdet_state.pcallback);
	printf("\n");
	}

static const iocshFuncDef recpstateFuncDef={"recpstate",0,NULL};
static void recpstateCallFunc(void)
{
 recpstate();
}

void registerrecpstate(void){
        iocshRegister(&recpstateFuncDef,&recpstateCallFunc);
        }

epicsExportRegistrar(registerrecpstate);
