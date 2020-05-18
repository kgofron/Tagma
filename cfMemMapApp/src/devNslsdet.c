/*******************************************************************************
devNslsDet.c

Device support for the BNL HERMES-X detector
D. P. Siddons 02/05/04

derived from:

********************************************************************************
devScaler.c
Device-support routines for Lecroy 1151 16-channel, 32-bit scaler

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

OSI  by S. Kate Feng 3/03

Modification Log:
-----------------
.01  6/26/93	tmm     Lecroy scaler
.02  1/16/95    tmm     v1.0 Joerger scaler
.03  6/11/96    tmm     v1.1 fixed test distinguishing VSC16 and VSC8
.04  6/28/96    tmm     v1.2 use vsc_num_cards instead of MAX_SCALER_CARDS
.05  8/20/96    tmm     v1.3 scalerstate, localaddr no longer marked 'volatile'
.06 10/03/96    tmm     v1.4 fix off-by-one problem
.07  3/03/96    tmm     v1.5 fix test distinguishing VSC16 and VSC8 for ECL/NIM
.08  3/03/97    tmm     v1.6 add reboot hook (disable interrupts & reset)
.09  4/24/98    tmm     v1.7 use callbackRequest instead of scanIoReq
.10  3/20/03    skf     OSI
.11  1/31/04	dps	mods for NSLS multi-silicon detector
*******************************************************************************/
#include        <epicsVersion.h>

#if EPICS_VERSION < 3 || (EPICS_VERSION==3 && EPICS_REVISION < 14)
#define NOT_YET_OSI
#endif

#ifdef HAS_IOOPS_H
#include        <basicIoOps.h>
#endif

/*typedef unsigned int uint32;
typedef unsigned short uint16;
*/
#include        <stdlib.h>
#include        <stdio.h>
#include        <epicsTimer.h>
#include        <epicsThread.h>
#include        <epicsExport.h>

epicsTimerQueueId       nslsdetWdTimerQ=0;


#include	<string.h>
#include	<math.h>

#include	<alarm.h>
/* #include	<dbRecType.h>  tmm: EPICS 3.13 */
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<dbCommon.h>
#include	<dbScan.h>
#include        <recGbl.h>
#include	<recSup.h>
#include	<devSup.h>
/*#include	<drvSup.h> */
#include	<dbScan.h>
#include	<special.h>
#include        <callback.h>
#include	<epicsInterrupt.h>
#include 	<iocsh.h>
#include	<epicsExport.h>
#include        "mcf5272.h" /* for definitions of coldfire port access code */
#include	<bsp.h>
#include	"nslsdetRecord.h"
#include	"devNslsdet.h"

#define IMMP         0x10000000 /* aka MBAR, BSP-specific */

#define FAST_LOCK epicsMutexId
#define FASTLOCKINIT(PFAST_LOCK) (*(PFAST_LOCK) = epicsMutexCreate())
#define FASTLOCK(PFAST_LOCK) epicsMutexLock(*(PFAST_LOCK));
#define TRYLOCK(PFAST_LOCK) epicsMutexTryLock(*(PFAST_LOCK));
#define FASTUNLOCK(PFAST_LOCK) epicsMutexUnlock(*(PFAST_LOCK));
#define ERROR -1
#define OK 0

/*** Debug support ***/
#define PRIVATE_FUNCTIONS 0	/* normal:1, debug:0 */
#if PRIVATE_FUNCTIONS
#define STATIC static
#else
#define STATIC
#endif
#ifdef NODEBUG
#define Debug0(l,f) ;
#define Debug(l,f,v) ;
#else
#define Debug0(l,FMT) {  if(l <= devnslsdetdebug) \
			{ printf("%s(%d):",__FILE__,__LINE__); \
			  printf(FMT); } }
#define Debug(l,FMT,V) {  if(l <= devnslsdetdebug) \
			{ printf("%s(%d):",__FILE__,__LINE__); \
			  printf(FMT,V); } }
#endif

#define BIT_SET(reg,bit) reg = reg | (1 << bit)
#define BIT_CLR(reg,bit) reg = reg & ~(1 << bit)
#define BIT_TST(reg,bit) reg & (1 << bit)

typedef struct cfSPI_setup {
   int mode_r;  /* mode register */
   int cmd_r; /* command register */
   }cfSPI_setup;

extern epicsMutexId SPI_lock;
extern cfSPI_setup SPI_setup;
extern int devcfSPIDebug;

void nslsdet_clearall();

volatile int devnslsdetdebug=0;
epicsExportAddress(int, devnslsdetdebug);

/*static long vsc_num_cards = 1;*/
STATIC long nslsdet_report(int level);
STATIC long nslsdet_init(int after);
STATIC long nslsdet_init_record(struct nslsdetRecord *psr);
STATIC long nslsdet_get_ioint_info(int cmd, struct dbCommon *p1, IOSCANPVT *p2);
STATIC long nslsdet_reset(void);
STATIC long nslsdet_read(void *pscal);
STATIC long nslsdet_write_preset(int val);
STATIC long nslsdet_arm(int val);
STATIC long nslsdet_done(void);
/*STATIC long nslsdet_read-s1(int *s1);
STATIC long nslsdet_read-s1(int *s1);
STATIC long nslsdet_read-s1(int *s1);
*/
nslsdetDSET devNslsDet = {
	9, 
	nslsdet_report,
	nslsdet_init,
	nslsdet_init_record,
	nslsdet_get_ioint_info,
	nslsdet_reset,
	nslsdet_read,
	nslsdet_write_preset,
	nslsdet_arm,
	nslsdet_done
};
epicsExportAddress(dset, devNslsDet);


STATIC Nslsdet_state nslsdet_state;
STATIC int channels[MAX_CHANNELS];
unsigned char glb;


/**************************************************
* nslsdet_report()
***************************************************/
STATIC long nslsdet_report(int level)
{
	if (nslsdet_state.card_exists) {
		printf("  NSLS Multi-element Silicon Detector #%d\n: No. elements= %d\n",0,nslsdet_state.num_channels);
		printf("imm->timer.trr1 = %d\n",imm->timer.trr1);
		printf("imm->timer.tmr1 = %d\n",imm->timer.tmr1);
	}
	return (0);
}

/**************************************************
* nslsdet_shutdown()
***************************************************/
STATIC int nslsdet_shutdown()
{
		if (nslsdet_reset() <0) return(ERROR);
        return(0);
}

/**************************************************
* nslsdetISR() handles counter gate. Only facility offered
  by internal counter is toggle-on-timeout, so gating 
  is done by setting a small timeout to bring gate active, 
  then setting a real timeout so output toggles at end.
  This can only handle times up to ~5 seconds. Longer times
  are done by repeating this sequence until sufficient total
  time has been accumulated.
  System state is held by nslsdet_state.done. Valid values are:
  CT_START	0
  CT_COUNTING	1
  CT_MORE	2
  CT_ALLDONE	3
***************************************************/
rtems_isr nslsdetISR(rtems_vector_number vector)
{
#define IRQ_MSG_BUFF_SIZE 5
#define IRQ_MSG_BUFF_CNT 50

static char irqMsgBuff[IRQ_MSG_BUFF_SIZE][IRQ_MSG_BUFF_CNT];
short temp;
      
imm->timer.ter1 = MCF5272_TIMER_TER_REF | MCF5272_TIMER_TER_CAP; /* clear pending */
if (devnslsdetdebug>=5){
	snprintf(irqMsgBuff[4],IRQ_MSG_BUFF_CNT,"Entry: done= %d  counter = %d  Ref. = %d\n\r",nslsdet_state.done,imm->timer.tcn1,imm->timer.trr1);
	epicsInterruptContextMessage(irqMsgBuff[4]);
	}
	
switch(nslsdet_state.done){
	case CT_MORE:{/* means more than 5 sec count was requested, and
				     some is still left to count.*/
		imm->timer.trr1 = nslsdet_state.delta; /* set to make interrupt after short time, and toggle TOUT1 high  */
		nslsdet_state.done = CT_START; /* tell us to count again */
		if (devnslsdetdebug>=5){ 
			snprintf(irqMsgBuff[2],IRQ_MSG_BUFF_CNT,"More:nslsdet_state.clkreg = %d\n\r",nslsdet_state.clkreg);
			epicsInterruptContextMessage(irqMsgBuff[2]);
			}

		break;
		}
		
	case CT_COUNTING:{/* means one-shot is done. Disable. */	
	        imm->timer.tmr1 = nslsdet_state.mode & nslsdet_INTDISABLE;
        	imm->timer.tmr1 = MCF5272_TIMER_TMR_CLK_STOP | MCF5272_TIMER_TMR_RST;
        	/* tell record support the hardware is done counting */
		nslsdet_state.done=CT_ALLDONE;
		if (devnslsdetdebug>=5){ /*epicsInterruptContextMessage("stop\n");*/
			snprintf(irqMsgBuff[2],IRQ_MSG_BUFF_CNT,"Stop:Ref = %d\n\r",imm->timer.trr1);
			snprintf(irqMsgBuff[3],IRQ_MSG_BUFF_CNT,"Stop:Events = %d  Counter = %d\n\r",imm->timer.ter1,imm->timer.tcn1);
			epicsInterruptContextMessage(irqMsgBuff[2]);
			epicsInterruptContextMessage(irqMsgBuff[3]);
			}
		/* get the record processed */
		callbackRequest(nslsdet_state.pcallback);
		break;
		}

	case CT_START:{/* means this is first entry to routine after arm. 
						set real compare reg. */
		
		/* requested >5sec count time. Set up for multiple intervals. */ 
		if(nslsdet_state.clkreg > 65535){
			imm->timer.trr1 = 65535;
			nslsdet_state.clkreg -= 65535;
			nslsdet_state.done = CT_MORE;
			}
		else{
			imm->timer.trr1 = nslsdet_state.clkreg;
			nslsdet_state.done=CT_COUNTING;
			}
	        if (devnslsdetdebug>=5){ /*epicsInterruptContextMessage("start\n");*/
			snprintf(irqMsgBuff[0],IRQ_MSG_BUFF_CNT,"Start: Done = %d  Clkreg = %d\n\r",nslsdet_state.done,nslsdet_state.clkreg);
			snprintf(irqMsgBuff[1],IRQ_MSG_BUFF_CNT,"Start: Ref = %d\n\r",imm->timer.trr1);
			epicsInterruptContextMessage(irqMsgBuff[0]);
			epicsInterruptContextMessage(irqMsgBuff[1]);
			}
		break;
		}
	}
	
}



/**************************************************
* nslsdetISRSetup()
***************************************************/
STATIC int nslsdetISRSetup(void)
{
  short temp;
  	
	Debug(5, "nslsdetISRSetup: Entry %i.\n\r",0);
	  	  
	  /* Configure timer1 interrupts */
          set_vector( nslsdetISR, TIMER1_IRQ_VECTOR, nslsdet_RTEMS);
          imm->sim.icr1 = MCF5272_SIM_ICR_TMR1_IL( TIMER1_IRQ_LEVEL );

          /* configure output port */
          temp=imm->gpio.pdcnt & (~(3 << 12));
          temp = temp | MCF5272_GPIO_PDCNT_TOUT1;
          imm->gpio.pdcnt = temp;

          /* set up Timer 1 mode register */
          nslsdet_state.mode = ((250 << 8) | (1 << 5) | (1 <<3) | ( 5 )); /* /256, toggle out, restart, clk/16, enable timer */
          nslsdet_state.clkreg = mcf5272_get_CPU_clock_speed()/16/256/1000*nslsdet_state.tp;
          nslsdet_state.delta = nslsdet_DELTA*(mcf5272_get_CPU_clock_speed()/16/250/1000);
          printf("Mode = %x  Clock reg = %i   Delta = %i\n\r",nslsdet_state.mode,nslsdet_state.clkreg,nslsdet_state.delta);
 	  Debug(5, "nslsdetISRSetup: Exit %i\n\r", 0);
	return (OK);
}


/***************************************************
* initialize all software and hardware
* nslsdet_init()
****************************************************/
STATIC long nslsdet_init(int after)
{
 int temp;
	Debug(2,"nslsdet_init(): entry, after = %d\n\r", after);
	if (after) return(0);

	nslsdet_state.card_exists = 1;

	Debug(3,"nslsdet_init: Total cards = %d\n\n\r",1);

	if (nslsdet_shutdown() < 0)
		epicsPrintf ("nslsdet_init: nslsdet_shutdown() failed\n\r"); 

        nslsdetWdTimerQ=epicsTimerQueueAllocate(
				1, /* ok to share queue */
				epicsThreadPriorityLow);
	
 	Debug0(3,"nslsdetWdTimerQ created\n\r");
	Debug(3,"nslsdet_init: nslsdet %i initialized\n\r",0);

	/* PA0,PA1,PA3 pins to output 
	   Pk_det rst, gate dir, ADC CS */			
	temp=MCF5272_RD_GPIO_PADDR(IMMP) | 0x000b; 
	MCF5272_WR_GPIO_PADDR(IMMP,temp); 
	
	
	/* (PC0-PC7) pins to output 
	   SPI address & strobe, 
	   Pk_det arm & Pk_found rst, token clk,
	   global rst. */			
	temp=MCF5272_RD_GPIO_PCDDR(IMMP) | 0x00ff; 
	MCF5272_WR_GPIO_PCDDR(IMMP,temp);
	/* set SPI address to 0, Pk_det_arm,Pk_found_rst 
	   token_clk and global_reset to zero */
	temp= MCF5272_RD_GPIO_PCDAT(IMMP) & 0xff00;
	MCF5272_WR_GPIO_PCDAT(IMMP,temp);
	 
	/* Read/write (PB7) and gate (PB5) pins to gpio */			
	temp=MCF5272_RD_GPIO_PBCNT(IMMP) & ~(0x0000c000); 
	MCF5272_WR_GPIO_PBCNT(IMMP,temp);
	
	/* Read/write (PB7) pin to output, gate (PB5) to input.
	   Set PB7 high (read). */			
	temp=(MCF5272_RD_GPIO_PBDDR(IMMP) | 0x00000080); 
	MCF5272_WR_GPIO_PBDDR(IMMP,temp);
	temp=(MCF5272_RD_GPIO_PBDAT(IMMP) | 0x0080);
	MCF5272_WR_GPIO_PBDAT(IMMP,temp); 

	/* release global chip reset */
	temp= MCF5272_RD_GPIO_PCDAT(IMMP) | 0x0080;
	MCF5272_WR_GPIO_PCDAT(IMMP,temp); 

        /* configure timer output port */
        temp=imm->gpio.pdcnt & (~(3 << 12));
        temp = temp | MCF5272_GPIO_PDCNT_TOUT1;
        imm->gpio.pdcnt = temp;

	return(0);
}

/***************************************************
* nslsdet_init_record()
****************************************************/
STATIC long nslsdet_init_record(struct nslsdetRecord *psr)
{
	int status;
	int args, channels;
	
	struct callback *pcallbacks;

	Debug(5,"nslsdet_init_record: card %d\n\r", 0);

	/* inp must be an VME_IO */
	switch (psr->inp.type)
	{
	case (VME_IO) : break;
	default:
		recGblRecordError(S_dev_badBus,(void *)psr,
			"devnslsdet (init_record) Illegal OUT Bus Type");
		return(S_dev_badBus);
	}

	Debug(5,"VME nslsdet: card %d\n", 0);
	if (!nslsdet_state.card_exists)
	{
		recGblRecordError(S_dev_badCard,(void *)psr,
		    "devnslsdet (init_record) card does not exist!");
		return(S_dev_badCard);
	}

	if (nslsdet_state.card_in_use)
	{
		recGblRecordError(S_dev_badSignal,(void *)psr,
		    "devnslsdet (init_record) card already in use!");
		return(S_dev_badSignal);
	}

	nslsdet_state.card_in_use = 1;
	args = sscanf(psr->inp.value.vmeio.parm, "%d", &channels);

	if(channels>MAX_CHANNELS)
	{
		recGblRecordError(S_dev_badSignal,(void *)psr,
		    "devnslsdet: Too many channels!");
		return(S_dev_badSignal);
	}
	psr->nch = channels;
	nslsdet_state.num_channels = channels;
	nslsdet_state.tp = psr->tp;
	nslsdet_state.clkreg=mcf5272_get_CPU_clock_speed()/16/256/1000*nslsdet_state.tp;
	nslsdet_state.gate = 0;
/*	eeprom_read_all(); */ /* get saved values from eeprom */
/*	unwrap(psr); */ /* populate record arrays */

	/* setup interrupt handler */
	pcallbacks = (struct callback *)psr->dpvt;
	nslsdet_state.pcallback = (struct callback *)&(pcallbacks[2]);
	status = nslsdetISRSetup();

	return(0);
}


/***************************************************
* nslsdet_get_ioint_info()
****************************************************/
STATIC long nslsdet_get_ioint_info(int cmd, struct dbCommon *prec, IOSCANPVT *ppvt)
{

	Debug(5,"nslsdet_get_ioint_info: cmd = %d\n\r", cmd);
	*ppvt = nslsdet_state.ioscanpvt;
	return(0);
}

/***************************************************
* nslsdet_reset()
****************************************************/
STATIC long nslsdet_reset(void)
{

	Debug(5,"nslsdet_reset: card %d\n\r", 0);
	nslsdet_arm(0);
	return(0);
}

/***************************************************
* nslsdet_read()
* Read hardware if necessary, and populate record 
* arrays with individual scaler values. Set VAL to 
* sums of scalers.
****************************************************/

STATIC long nslsdet_read(void *pscal)
{
nslsdetRecord *pdet = (nslsdetRecord *)pscal;
int status,i;
long *val;
int *s1,*s2,*s3;
val=pdet->bptr;
s1=pdet->ps1;
s2=pdet->ps2;
s3=pdet->ps3;
status=0;
	
	/* need to check if detector needs to be read for fresh data
	   if not, simply return old data */
	if(nslsdet_state.newdata){
	  Debug0(3,"Reading hardware\n\r");
	  status=getcounts(pdet);
	  nslsdet_state.newdata=0;
	  val[0]=0;
	  val[1]=0;
	  val[2]=0;
	  for(i=0;i<MAX_CHANNELS;i++){
		val[0]+=s1[i];
		val[1]+=s2[i];
		val[2]+=s3[i];
		}
	  }
	else{
	  Debug0(3,"Using old data\n\r");
	  }
	  
	Debug(8,"nslsdet_read: status %i\n\r", status);
	return((long)status);
}

/***************************************************
* nslsdet_write_preset(). Only sets up variable for count
****************************************************/
STATIC long nslsdet_write_preset(int val)
{
	Debug(5,"nslsdet_write_preset: val = %d\n\r",(int)val);
	nslsdet_state.tp = val;
	return(0);
}

/***************************************************
* nslsdet_arm()
* Start nslsdet count. If Gate is set to external, need to
* set callback for external gate edge detect. If internal
* then Timer 1 does the job.
*
* val == 0 means stop
* val != 0 means start
****************************************************/
STATIC long nslsdet_arm(int val)
{
int temp;

Debug(5,"nslsdet_arm: val= %d\n\r", val);
if(val){
/*  if(nslsdet_state.gate){*/ /* means external timing control */
/*  	} */
/*  else{ */ /* use Timer 1 for gate signal */
  	  
	  /* Reset Timer... */
	  
  	imm->timer.tmr1 = imm->timer.tmr1 & (~MCF5272_TIMER_TMR_RST); 
  	imm->timer.tmr1 = MCF5272_TIMER_TMR_CLK_STOP | MCF5272_TIMER_TMR_OM;
	imm->timer.tcn1 = 0;  /* zero count */
	imm->timer.ter1 = MCF5272_TIMER_TER_REF | MCF5272_TIMER_TER_CAP; /* clear pending */

	  /* Setup Timer... */
	
	nslsdet_state.clkreg = mcf5272_get_CPU_clock_speed()/16/250/1000*nslsdet_state.tp;
        nslsdet_state.delta = nslsdet_DELTA*(mcf5272_get_CPU_clock_speed()/16/250/1000);
	nslsdet_clearall(); /* clear detector counters in case we were previously counting */
	imm->timer.trr1 = nslsdet_state.delta; /* set to make interrupt after short time, and toggle TOUT1 high  */
	imm->timer.tmr1 = nslsdet_state.mode | nslsdet_INTENABLE; /* enable compare interrupt and start clock */

/*	} */

  }
else{
   /* Check if currently counting. If so, stop timer and clear counters. 
      We will need to do something to ensure that gate signal returns to 
      low (i.e. inactive) state before returning from this routine      */

	imm->timer.tmr1 = nslsdet_state.mode & nslsdet_INTDISABLE; /* disable compare interrupt,
								   and set up prescaler,output */
       	imm->timer.tmr1 = imm->timer.tmr1 & (~MCF5272_TIMER_TMR_RST);
	imm->timer.tcn1 = 0;  /* zero count */
	imm->timer.ter1 = MCF5272_TIMER_TER_REF | MCF5272_TIMER_TER_CAP; /* clear pending */
	}
nslsdet_state.done = CT_START; /* clear hardware-done flag */
return(0);
}


/***************************************************
* nslsdet_done()
****************************************************/
STATIC long nslsdet_done(void)
{
	if (nslsdet_state.done==CT_ALLDONE) {
		/* clear hardware-done flag */
		nslsdet_state.done = CT_START;
		return(1);
	} else {
		return(0);
	}
}


/*****************************************************
* VSCSetup()
* User (startup file) calls this function to configure
* us for the hardware.
*****************************************************/
void devNslsDetConfig(int num_cards,	/* maximum number of cards in crate */
	   int ndets,		/* Max. no. detectors */
	   unsigned channels)	/* Max. no. channels */
{
/* For NSLS detector this is null, since this is an embedded system
   and only one configuration is possible. */
   if(num_cards != 1){
   	printf("Configure: Illegal number of cards\n");
   }
   if(ndets != 1){
   	printf("Configure: Illegal number of detectors\n");
   }
   if((channels >MAX_CHANNELS)||(channels<32)){
   	printf("Configure: Illegal number of channels\n");
   }
   MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set QSPI HALT bit */
}

/******************************************************************
* Routines to assemble SPI strings from arrays, and to talk to 
* ASIC via SPI port.
*******************************************************************/

int wrap(void *pscal){
   nslsdetRecord *pdet = (nslsdetRecord *)pscal;
   int i,chip,index;
   int bits;
   char *tr1, *tr2, *tr3, *tr4, *chen, *tsen;
   
   tr1 = pdet->ptr1;
   tr2 = pdet->ptr2;
   tr3 = pdet->ptr3;
   tr4 = pdet->ptr4;
   chen = pdet->pchen;
   tsen = pdet->ptsen;
   
   glb=0;
   if(pdet->eblk) BIT_SET(glb,4);
   switch(pdet->shpt){
   case 0:{ 
   		break;
		}
   case 1:{
   		BIT_SET(glb,5);
		break;
		}
   case 2:{
		BIT_SET(glb,6);
		break;
		}
   case 3:{
		BIT_SET(glb,5);
		BIT_SET(glb,6);
		break;
		}
	}
   if(pdet->gain) BIT_SET(glb,7);

   for(chip=0;chip<NCHIPS;chip++){
    for(i=0;i<=31;i++){
      index=i+chip*32;
/*      Debug(10,"index=%i\n",index); */
      bits=0;
      if(chen[index]) BIT_SET(bits,0);
      if(tsen[index]) BIT_SET(bits,1);
      if(pdet->loen==index) BIT_SET(bits,2);
      if(pdet->aoen==index) BIT_SET(bits,3);
      channels[index] = 0|\
			 ((tr4[index]&0x3f)<<26)|\
                         ((tr3[index]&0x3f)<<20)|\
			 ((tr2[index]&0x3f)<<14)|\
			 ((tr1[index]&0x3f)<<8)|
                         (bits<<4);
/*	Debug(10,"channel=%x\n",channels[index]);*/
      }
   }
return(0);
}

/* take channel struct and unpack it into arrays */
int unwrap(void *pscal){
 nslsdetRecord *pdet = (nslsdetRecord *)pscal;
 int i,chip, bits,index; 
 char *tr1, *tr2, *tr3, *tr4, *chen, *tsen;
   
   tr1 = pdet->ptr1;
   tr2 = pdet->ptr2;
   tr3 = pdet->ptr3;
   tr4 = pdet->ptr4;
   chen = pdet->pchen;
   tsen = pdet->ptsen;

   index=0;
   for(chip=0;chip<NCHIPS;chip++){
      bits=0;
      for(i=0;i<=31;i++){
         chen[index]=0;
         tsen[index]=0;
         bits=(channels[index] & 0x0f);
         if (bits & 0x10) chen[index]=1;
         if (bits & 0x20) tsen[index]=1;
         if (bits & 0x40) pdet->loen=index;
         if (bits & 0x80) pdet->aoen=index;
         tr4[index]=(char)((channels[index]>>26)&0x003f);
         tr3[index]=(char)((channels[index]>>20)&0x003f);
         tr2[index]=(char)((channels[index]>>14)&0x003f);
         tr1[index]=(char)((channels[index]>>8)&0x003f);
	 index++;
      }
   }
return(0);
}

extern void setdevice(int device);

extern void cleardevice(int device);

void token_step(void){
short temp,j;
	temp=MCF5272_RD_GPIO_PCDAT(IMMP) | 0x0040; /* token clk high */
        MCF5272_WR_GPIO_PCDAT(IMMP,temp);
	j+=1; /* delay */ 
	temp=MCF5272_RD_GPIO_PCDAT(IMMP) & ~(0x0040); /* token clk low */
        MCF5272_WR_GPIO_PCDAT(IMMP,temp); 
}

void setread(void){
short temp;
	temp=MCF5272_RD_GPIO_PBDAT(IMMP) | 0x0080; /* R/W high */
        MCF5272_WR_GPIO_PBDAT(IMMP,temp);
}

void setwrite(void){
short temp;
	temp=MCF5272_RD_GPIO_PBDAT(IMMP) & ~(0x0080); /* R/W bit low */
        MCF5272_WR_GPIO_PBDAT(IMMP,temp);
}

#define ERDSR 5
#define EWRSR 1
#define EREAD 3
#define EWRITE 2
#define EWREN 6

/*****************************************************************
* Get nbytes of data from the QSPI interface into buff. All 
* chip-select setup must be done previously, and the SPI device 
* must be locked before entry and unlocked on exit. 
* Only multiples of 8 bits (bytes) are allowed.
******************************************************************/
void spimget(int nbytes, unsigned char *buff){
short temp,i,count,t;

memset(buff,0,nbytes); /* clear buffer */
  if (devcfSPIDebug >= 20)
        epicsPrintf("spimget entry nbytes %d\n\r",nbytes);

  count=0;
  while(nbytes){
    if(nbytes>16){
       nbytes-=16;
       temp=16;
       }
    else{ 
       temp=nbytes;
       nbytes=0;
       }

        MCF5272_WR_QSPI_QMR(IMMP,0xa308); /* 8 bits per transfer */
        MCF5272_WR_QSPI_QAR(IMMP,0x20);   /* first command loc. */
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store 'temp' commands */
	}
        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,0);          /* store 'temp' dummy data words */
	}
        MCF5272_WR_QSPI_QWR(IMMP,(temp-1)<<8);       /* 'temp' words to transfer */

        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer */

        if (devcfSPIDebug >= 20)
           epicsPrintf("spimget: Waiting for SPI transfer.....\n\r");

        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

        t = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

        MCF5272_WR_QSPI_QAR(IMMP,0x10); /* point to read data */
      for(i=0;i<temp;i++){
	buff[count] = (char)(MCF5272_RD_QSPI_QDR(IMMP) & 0xff); /* read data words */
	count++;
	}
     }
  if (devcfSPIDebug >= 20){
        epicsPrintf("spimget exit nbytes #%d\n\r",count);
	epicsPrintf("spimget: %x %x %x\n\r",buff[0],buff[1],buff[2]);
	}
}

/*****************************************************************
* Put nbytes of data to the QSPI interface from buff. All 
* chip-select setup must be done previously, and the SPI device 
* must be locked before entry and unlocked on exit. 
* Only multiples of 8 bits (bytes) are allowed.
******************************************************************/
void spimput(int nbytes,unsigned char *buff){
short temp,i,count,dummy;

  if (devcfSPIDebug >= 20)
        epicsPrintf("spimput entry nbytes %d\n\r",nbytes);

  count=0;
  while(nbytes){
    if(nbytes>16){
       nbytes-=16;
       temp=16;
       }
    else{ 
       temp=nbytes;
       nbytes=0;
       }

        MCF5272_WR_QSPI_QMR(IMMP,0xa108); /* 8 bits per transfer */
        MCF5272_WR_QSPI_QAR(IMMP,0x20);
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store 'temp' commands */
	}
        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,buff[count]);          /* store 'temp' dummy data words */
	count++;			/* increment buff pointer */
	}
        MCF5272_WR_QSPI_QWR(IMMP,(temp-1)<<8);       /* 'temp' words to transfer */

        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer */

        if (devcfSPIDebug >= 20)
           epicsPrintf("spimput: Waiting for SPI transfer.....\n\r");

        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

        temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

        MCF5272_WR_QSPI_QAR(IMMP,0x10); /* point to read data */
      for(i=0;i<temp;i++){
        dummy = MCF5272_RD_QSPI_QDR(IMMP); /* read meaningless data words */
	}
     }
  if (devcfSPIDebug >= 20)
        epicsPrintf("spimput exit nbytes #%d\n\r",nbytes);
}

int getcounter(void){
short temp,i,t;
int count;

  if (devcfSPIDebug >= 20)
        epicsPrintf("getcounter entry\n\r");

  temp = 3; /* 24-bit counters */
       MCF5272_WR_QSPI_QMR(IMMP,0xa108); /* 8 bits per transfer */
        MCF5272_WR_QSPI_QAR(IMMP,0x20);   /* first command loc. */
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store 'temp' commands */
	}
        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
      for(i=0;i<temp;i++){
        MCF5272_WR_QSPI_QDR(IMMP,0);          /* store 'temp' dummy data words */
	}
        MCF5272_WR_QSPI_QWR(IMMP,(temp-1)<<8);       /* 'temp' words to transfer */

        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer */

        if (devcfSPIDebug >= 20)
           epicsPrintf("Waiting for SPI transfer.....\n\r");

        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

        t = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
        MCF5272_WR_QSPI_QIR(IMMP,(t | MCF5272_QSPI_QIR_SPIF));

        MCF5272_WR_QSPI_QAR(IMMP,0x10); /* point to read data */
        
	count=0;
	count = (MCF5272_RD_QSPI_QDR(IMMP) & 0xff); /* read 3 data words */
	count = (count << 8) | (MCF5272_RD_QSPI_QDR(IMMP) & 0xff);
	count = (count << 8) | (MCF5272_RD_QSPI_QDR(IMMP) & 0xff);
     
  if (devcfSPIDebug >= 20){
        epicsPrintf("getcounter exit: count = %d\n\r",count);
	}
	return(count);
}

int enable_ASIC_spi(void){
int temp;
	temp=(MCF5272_RD_GPIO_PCDAT(IMMP) | 0x0010); /* set PC4 */
	MCF5272_WR_GPIO_PCDAT(IMMP,temp);
	}

int disable_ASIC_spi(void){
int temp;
	temp=(MCF5272_RD_GPIO_PCDAT(IMMP) & 0xffef); /* clear PC4 */
	MCF5272_WR_GPIO_PCDAT(IMMP,temp);
	}

int getcounts(void *pscal){ /* read all counters from chip j */
/* get NCHIPS x 32 x 3 24-bit values via SPI 2 */

nslsdetRecord *pdet = (nslsdetRecord *)pscal; 
int i,chip,index;
int *s1, *s2, *s3;

Debug0(5,"getcounts\n\r");
s1=pdet->ps1;
s2=pdet->ps2;
s3=pdet->ps3;

FASTLOCK(&(SPI_lock));
  enable_ASIC_spi();
  MCF5272_WR_QSPI_QWR(IMMP,0x0000); /* clear HALT bit */
  setread();
  setdevice(CHIPS); /* point to first chip */ 
  index=0;
  for (chip=0;chip<NCHIPS;chip++){
   for(i=31;i>=0;i--){
   	
	s1[index] = getcounter()>> 1; /* hack to take care of missing ASIC bit */
   	s2[index] = getcounter()>> 1; /* hack to take care of missing ASIC bit */
   	s3[index] = getcounter()>> 1; /* hack to take care of missing ASIC bit */	
	index++;
	}
   /* move to next chip */
   token_step();
   if(chip==0){ 
     cleardevice(CHIPS); /*remove CS from first chip after moving on */
     } 
   }
   token_step(); /* move token out of chips */
   MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit */
   disable_ASIC_spi();
FASTUNLOCK(&(SPI_lock));
return(0);
}

int stuffit(void *pscal){ /* stuff structs into chp */
nslsdetRecord *pdet = (nslsdetRecord *)pscal;
int i,index,chip;
unsigned char tmp8;
unsigned short tmp16;
int temp;

  wrap(pdet); /* set up all bits and regs in channels[] array */
  FASTLOCK(&(SPI_lock));
  	MCF5272_WR_QSPI_QWR(IMMP,0x0000); /* clear HALT bit, to force any SPI setup changes to take effect. */
	setwrite();
	enable_ASIC_spi();
	setdevice(CHIPS); /* set chip-select for first chip */
	index=0;
	for (chip=0;chip<NCHIPS;chip++){
	    for(i=31;i>=0;i--){
				
	        MCF5272_WR_QSPI_QMR(IMMP,0x8108); /* set mode, 16 bits */
	        MCF5272_WR_QSPI_QAR(IMMP,0x20);         /* point to first cmd loc. */
	        MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store command */
	        MCF5272_WR_QSPI_QWR(IMMP,0x00);         /* only one word to transfer */
	        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
	        tmp16=channels[index] >> 16;
		MCF5272_WR_QSPI_QDR(IMMP,tmp16);         /* store data 1*/
	        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer*/

        	if (devcfSPIDebug >= 20)
        	   epicsPrintf("Waiting for SPI transfer #1.....\n\r");

        	while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

	        temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
	        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

	        MCF5272_WR_QSPI_QAR(IMMP,0x10);
	        temp = MCF5272_RD_QSPI_QDR(IMMP); /* read data */
		
		MCF5272_WR_QSPI_QMR(IMMP,0xb108); /* set mode, 12 bits */
	        MCF5272_WR_QSPI_QAR(IMMP,0x20);         /* point to first cmd loc. */
	        MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store command */
	        MCF5272_WR_QSPI_QWR(IMMP,0x00);         /* only one word to transfer */
	        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
		tmp16=(channels[index] & 0xffff)>>4;
		MCF5272_WR_QSPI_QDR(IMMP,tmp16);         /* store data */
	        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer*/

        	if (devcfSPIDebug >= 20)
	           epicsPrintf("Waiting for SPI transfer #2.....\n\r");

	        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

	        temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
	        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

	        MCF5272_WR_QSPI_QAR(IMMP,0x10);
	        temp = MCF5272_RD_QSPI_QDR(IMMP); /* read data */
		index++;
		}

   	   tmp8 = glb;
   	   MCF5272_WR_QSPI_QMR(IMMP,0xa108); /* set mode, 8 bits */
   	   MCF5272_WR_QSPI_QAR(IMMP,0x20);	   /* point to first cmd loc. */
   	   MCF5272_WR_QSPI_QDR(IMMP,0x4000);  /* store command */
   	   MCF5272_WR_QSPI_QWR(IMMP,0x00);	   /* only one word to transfer */
   	   MCF5272_WR_QSPI_QAR(IMMP,0x00);	   /* point to first data loc */
   	   MCF5272_WR_QSPI_QDR(IMMP,tmp8);	    /* store data 2*/
   	   MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer*/

	   if (devcfSPIDebug >= 20)
   	      epicsPrintf("Waiting for SPI transfer #3.....\n\r");

   	   while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0); /* wait for transfer to finish */

   	   temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
   	   MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

   	   MCF5272_WR_QSPI_QAR(IMMP,0x10);
   	   temp = MCF5272_RD_QSPI_QDR(IMMP); /* read data */
	   token_step();
	   if(chip==0){
		cleardevice(CHIPS);
		}
	}
	token_step(); /* move token out of chips */
	MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit so SPI setup is re-initialized at next entry. */
  	disable_ASIC_spi();
  FASTUNLOCK(&(SPI_lock));
return(0);
}

/***************************************************
* nslsdet_clearall(). Toggle all detector chipselects 
* to clear counters
****************************************************/
void nslsdet_clearall(void)
{
int i,j;

FASTLOCK(&(SPI_lock));
	MCF5272_WR_QSPI_QWR(IMMP,0x0000); /* clear HALT bit */
	setdevice(CHIPS);
	token_step();
	cleardevice(CHIPS);
	for(i=1;i<NCHIPS;i++){
		token_step();
		j+=1;
		}
	token_step();
	MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit */
FASTUNLOCK(&(SPI_lock));
}



static const iocshArg devNslsDetConfigArg0 = {"Card #", iocshArgInt};
static const iocshArg devNslsDetConfigArg1 = {"No. detectors", iocshArgInt};
static const iocshArg devNslsDetConfigArg2 = {"No. channels", iocshArgInt};

static const iocshArg * const devNslsDetConfigArgs[3] = {&devNslsDetConfigArg0,
                                                       &devNslsDetConfigArg1,
                                                       &devNslsDetConfigArg2,
                                                       };

static const iocshFuncDef devNslsDetConfigFuncDef={"devNslsDetConfig",3,devNslsDetConfigArgs};
static void devNslsDetConfigCallFunc(const iocshArgBuf *args)
{
 devNslsDetConfig((int) args[0].ival, (unsigned short) args[1].ival, (int) args[2]
.ival);
}

void registerNslsDetConfig(void){
        iocshRegister(&devNslsDetConfigFuncDef,&devNslsDetConfigCallFunc);
        }

epicsExportRegistrar(registerNslsDetConfig);

void pstate(void){
	printf("In device support:\n");
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
	
	}

static const iocshFuncDef pstateFuncDef={"pstate",0,NULL};
static void pstateCallFunc(void)
{
 pstate();
}

void registerpstate(void){
        iocshRegister(&pstateFuncDef,&pstateCallFunc);
        }

epicsExportRegistrar(registerpstate);
