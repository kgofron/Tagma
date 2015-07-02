/*******************************************************************************
devNslsDet.c

Device support for the BNL HERMES-X detector
D. P. Siddons 02/05/04
*/

#include        <stdlib.h>
#include        <stdio.h>

#include	<string.h>
#include	<math.h>

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

typedef struct nslsdetRecord {
        unsigned long   nelm;   /*Number of Elements*/
        int		cont;   /*OneShot/AutoCount*/
        float           dly;    /*Delay*/
        float           dly1;   /*Auto-mode Delay*/
        double          tp;     /*Time Preset*/
        double          tp1;    /*Auto Time Preset*/
        int		shpt;   /*Shaping time*/
        int		gain;   /*Gain*/
        short           aoen;   /*Analog out chan*/
        short           loen;   /*Leakage out chan*/
        short           eblk;   /*Enable input bias current*/
        char            pchen[MAX_CHANNELS];  /*Channel enables*/
        char		ptsen[MAX_CHANNELS];  /*Test pulse enables*/
        char		ptr1[MAX_CHANNELS];  /*trim dac 1*/
        char		ptr2[MAX_CHANNELS];  /*trim dac 2*/
        char		ptr3[MAX_CHANNELS];  /*trim dac 3*/
        char		ptr4[MAX_CHANNELS];  /*trim dac 4*/
        int		ps1[MAX_CHANNELS];	/*Scaler 1*/
        int		ps2[MAX_CHANNELS];	/*Scaler 2*/
        int		ps3[MAX_CHANNELS];	/*Scaler 3*/
} nslsdetRecord;

typedef struct cfSPI_setup {
   int mode_r;  /* mode register */
   int cmd_r; /* command register */
   }cfSPI_setup;

extern epicsMutexId SPI_lock; /*to protect access to SPI port */
/*extern cfSPI_setup SPI_setup;*/
extern int devcfSPIDebug;

void nslsdet_clearall();

volatile int devnslsdetdebug=0;

STATIC long nslsdet_report(int level);
STATIC long nslsdet_init(int after);
STATIC long nslsdet_init_record(struct nslsdetRecord *psr);
STATIC long nslsdet_get_ioint_info(int cmd, struct dbCommon *p1, IOSCANPVT *p2);
STATIC long nslsdet_reset(void);
STATIC long nslsdet_read(void *pscal);
STATIC long nslsdet_write_preset(int val);
STATIC long nslsdet_arm(int val);
STATIC long nslsdet_done(void);

/*nslsdetDSET devNslsDet = {
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
*/

STATIC Nslsdet_state nslsdet_state;
STATIC int channels[MAX_CHANNELS];
unsigned char glb;


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

	/* setup interrupt handler */
	pcallbacks = (struct callback *)psr->dpvt;
	nslsdet_state.pcallback = (struct callback *)&(pcallbacks[2]);
	status = nslsdetISRSetup();

	return(0);
}

/***************************************************
* nslsdet_arm()
* Start nslsdet count. If Gate is set to external, need to
* set callback for external gate edge detect [Not yet implemented]. If internal
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

void setdevice(int device){
unsigned int mask,temp;
  if (devcfSPIDebug >= 20)
        epicsPrintf("Setting device #%d\n\r",device);

        mask=0x000f; /* bits 0-2 are SPI device address, bit 3 is enable */
        temp=MCF5272_RD_GPIO_PCDDR(IMMP);
        MCF5272_WR_GPIO_PCDDR(IMMP,(temp | mask)); /* make outputs: should only
 do once? */

/*      temp=MCF5272_RD_GPIO_PCDAT(IMMP)&0xfff7;*//* bit 3 is enable bit */
/*      MCF5272_WR_GPIO_PCDAT(IMMP,temp); *//* clear enable bit */

        temp=MCF5272_RD_GPIO_PCDAT(IMMP)&0xfff0;
        MCF5272_WR_GPIO_PCDAT(IMMP,(temp | device)); /* ouput address */

        temp=MCF5272_RD_GPIO_PCDAT(IMMP) | 0x0008; /* enable bit */
        MCF5272_WR_GPIO_PCDAT(IMMP,temp); /* set enable bit */
}

void cleardevice(int device){
unsigned int temp;
        temp=MCF5272_RD_GPIO_PCDAT(IMMP) & 0xfff7;/* bit 3 is enable bit */
        MCF5272_WR_GPIO_PCDAT(IMMP,temp); /* clear enable bit */
}

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


int getcounts(void *pscal){ /* read all counters from chip j */
/* get NCHIPS x 32 x 3 24-bit values via SPI */

nslsdetRecord *pdet = (nslsdetRecord *)pscal; 
int i,chip,index;
int *s1, *s2, *s3;

Debug0(5,"getcounts\n\r");
s1=pdet->ps1;
s2=pdet->ps2;
s3=pdet->ps3;

FASTLOCK(&(SPI_lock));
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

/* This routine is specifically for reading the MAX1202/1203 8-channel 12-bit
   ADC chip. */
short spi_read(int card, int reg){
struct cfSPI_setup setup;
short result = 0;
volatile short data0 = 0;
volatile short data1 = 0;
volatile short data2 = 0;
short cmd;
short temp;


  if (devcfSPIDebug >= 10)
        epicsPrintf("SPI_read card %d reg %d\n\r",card, reg);
  setup = SPI_setup[card];
  cmd = (0x008f | ((reg & 7) << 4)); /* Start bit, channel #, single-ended, uni
polar, external clock */
  FASTLOCK(&(SPI_lock)); /* grab SPI port lock */

        MCF5272_WR_QSPI_QWR(IMMP,0x0000); /* clear HALT bit */

        setdevice(card); /* configure chip-selects */

        MCF5272_WR_QSPI_QMR(IMMP,setup.mode_r);
        MCF5272_WR_QSPI_QAR(IMMP,0x20);
        MCF5272_WR_QSPI_QDR(IMMP,setup.cmd_r);  /* store 1st command */
        MCF5272_WR_QSPI_QDR(IMMP,setup.cmd_r);  /* store 2nd command */
        MCF5272_WR_QSPI_QDR(IMMP,setup.cmd_r);  /* store 3rd command */
        MCF5272_WR_QSPI_QWR(IMMP,0x0200);       /* three words to transfer */
        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
        MCF5272_WR_QSPI_QDR(IMMP,cmd);          /* store data word #0 */
        MCF5272_WR_QSPI_QDR(IMMP,0);            /* store data word #1 */
        MCF5272_WR_QSPI_QDR(IMMP,0);            /* store data word #2 */
        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /*
start transfer */

        if (devcfSPIDebug >= 10)
           epicsPrintf("Waiting for SPI transfer.....\n\r");

        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0)
                ; /* wait for transfer to finish */

        temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

        MCF5272_WR_QSPI_QAR(IMMP,0x10);
        data0 = MCF5272_RD_QSPI_QDR(IMMP); /* read data word #0 */
        data1 = MCF5272_RD_QSPI_QDR(IMMP); /* read data word #1 */
        data2 = MCF5272_RD_QSPI_QDR(IMMP); /* read data word #2 */

        cleardevice(card);

        MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit */

  FASTUNLOCK(&(SPI_lock));
        result = ((data1 & 0x00ff) << 6) | ((data2 & 0x00ff) >> 2);
        if (devcfSPIDebug >= 10){
                epicsPrintf("SPI read %hx %hx %hx = %hx from card #%d, reg #%d\
n\r",data0,data1,data2,result,card,reg);
                }

  return(result);
}

/* This routine is specifically for writing to the TLV5630I chip,
   an 8-channel 12-bit DAC. */
void spi_write(int card, int reg, unsigned long value){
struct cfSPI_setup setup;
int temp, data;

  if (devcfSPIDebug >= 20)
        epicsPrintf("SPI_write card %d reg %d val=%ld\n\r",card, reg, value);

  setup = SPI_setup[card];
  data = ((int)value & 0x0fff)| reg << 12;

  FASTLOCK(&(SPI_lock));
        MCF5272_WR_QSPI_QWR(IMMP,0x0000); /* clear HALT bit */
        setdevice(card); /* configure chip selects */

        MCF5272_WR_QSPI_QMR(IMMP,setup.mode_r); /* set mode */
        MCF5272_WR_QSPI_QAR(IMMP,0x20);         /* point to first cmd loc. */
        MCF5272_WR_QSPI_QDR(IMMP,setup.cmd_r);  /* store command */
        MCF5272_WR_QSPI_QWR(IMMP,0x00);         /* only one word to transfer */
        MCF5272_WR_QSPI_QAR(IMMP,0x00);         /* point to first data loc */
        MCF5272_WR_QSPI_QDR(IMMP,data);         /* store data */
        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /*
start transfer */

        if (devcfSPIDebug >= 20)
           epicsPrintf("Waiting for SPI transfer.....\n\r");

        while((MCF5272_RD_QSPI_QIR(IMMP) & MCF5272_QSPI_QIR_SPIF)==0)
                ; /* wait for transfer to finish */

        temp = MCF5272_RD_QSPI_QIR(IMMP); /* clear done flag */
        MCF5272_WR_QSPI_QIR(IMMP,(temp | MCF5272_QSPI_QIR_SPIF));

        MCF5272_WR_QSPI_QAR(IMMP,0x10);
        temp = MCF5272_RD_QSPI_QDR(IMMP); /* read data */

        cleardevice(card);
        MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit */
  FASTUNLOCK(&(SPI_lock));

        if (devcfSPIDebug >= 20)
           epicsPrintf("SPI_write %ld to card #%d, reg #%d\n\r",value,card,reg)
;
}
