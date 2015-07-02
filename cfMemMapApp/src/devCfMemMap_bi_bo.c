/* devCfMemMap.c
   Author: D. Peter Siddons, 
           Brookhaven National Laboratory,
   Date: 12/20/03

derived from:

 	devA32Vme.c	*/

/*****************************************************************
 *
 *      Author :                     Ned D. Arnold
 *      Date:                        11/21/97
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *****************************************************************
 *                         COPYRIGHT NOTIFICATION
 *****************************************************************

 * THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
 * AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
 * AND IN ALL SOURCE LISTINGS OF THE CODE.
 
 * (C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO
 
 * Argonne National Laboratory (ANL), with facilities in the States of 
 * Illinois and Idaho, is owned by the United States Government, and
 * operated by the University of Chicago under provision of a contract
 * with the Department of Energy.

 * Portions of this material resulted from work developed under a U.S.
 * Government contract and are subject to the following license:  For
 * a period of five years from March 30, 1993, the Government is
 * granted for itself and others acting on its behalf a paid-up,
 * nonexclusive, irrevocable worldwide license in this computer
 * software to reproduce, prepare derivative works, and perform
 * publicly and display publicly.  With the approval of DOE, this
 * period may be renewed for two additional five year periods. 
 * Following the expiration of this period or periods, the Government
 * is granted for itself and others acting on its behalf, a paid-up,
 * nonexclusive, irrevocable worldwide license in this computer
 * software to reproduce, prepare derivative works, distribute copies
 * to the public, perform publicly and display publicly, and to permit
 * others to do so.

 *****************************************************************
 *                               DISCLAIMER
 *****************************************************************

 * NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
 * THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
 * MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
 * LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
 * USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
 * DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
 * OWNED RIGHTS.  

 *****************************************************************
 * LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
 * DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
 *****************************************************************

 * Modification Log:
 * -----------------
 * 01-23-98   nda       initially functional
 * 02-25-98   mr        modified ai and ao to support 2's complement.
 * 07-22-98   mr        Fixed Param field to accomadate both i`uni and bi polar
 * 			Inputs and outputs (AI, AO records)..
 * 10-06-98   nda       fixed a bug with li,lo,ai,ao where sum of bit+
 *                      numbits > MAX_ACTIVE_BITS
 *
 * 12-18-03   dps	Modified for Coldfire bit I/O
*              
 */


/*To Use this device support, Include the following before iocInit */
/* devCFMemMapConfig(card,a32base,nreg,iVector,iLevel)  */
/*    card    = card number                           */
/*    a32base = base address of card                  */
/*    nreg    = number of A32 registers on this card  */
/*    iVector = interrupt vector (MRD100 ONLY !!) Not used for Coldfire     */
/*    iLevel  = interrupt level  (MRD100 ONLY !!) Not used for Coldfire    */
/* For Example					      */
/* devCFMemMapConfig(0, 0x10000000, 1, 0, 0)        */


 /**********************************************************************/
 /** Brief Description of device support                              **/
 /**						    	              **/
 /** This device support allows access to any memory register         **/
 /** module found in the Coldfire on-chip I/O module space.           **/
 /** The bit field of interest                                        **/
 /** is described in the PARM field of the INP or OUT link.           **/
 /** This allows a generic driver to be used without hard-coding      **/
 /** register numbers within the software.                            **/
 /**						    	              **/
 /** Record type     Signal #           Parm Field                    **/
 /**                                                                  **/
 /**    ai          reg_offset     lsb, width, type; not for Coldfire **/
 /**    ao          reg_offset     lsb, width, type; not for Coldfire **/
 /**    bi          reg_offset     bit #                              **/
 /**    bo          reg_offset     bit #                              **/
 /**    longin      reg_offset     lsb, width; not for Coldfire       **/
 /**    longout     reg_offset     lsb, width; not for Coldfire       **/
 /**    mbbi        reg_offset     lsb, width; not for Coldfire       **/
 /**    mbbo        reg_offset     lsb, width; not for Coldfire       **/
 /**                                                                  **/
 /** reg_offset is specified by the register number (0,1,2,3, etc)    **/
 /** Parm field must be provided, no defaults are assumed ...         **/
 /** In ai and ao type is either 0 - unipolar, 1 -bipolar             **/
 /**                                                                  **/
 /**                                                                  **/
 /**********************************************************************/


#define FAST_LOCK epicsMutexId
#define FASTLOCKINIT(PFAST_LOCK) (*(PFAST_LOCK) = epicsMutexCreate())
#define FASTLOCK(PFAST_LOCK) epicsMutexLock(*(PFAST_LOCK));
#define FASTUNLOCK(PFAST_LOCK) epicsMutexUnlock(*(PFAST_LOCK));
#define ERROR -1
#define OK 0

/*#include	<vme.h>*/
#include	"mcf5272.h" /* for definitions of coldfire port access code */ 
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<math.h>
#include	<epicsTimer.h>
#include	<epicsThread.h>
#include	<epicsRingBytes.h>
#include	<epicsMutex.h>
#include	<epicsEvent.h>

#include	<alarm.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<dbCommon.h>
#include	<dbScan.h>
#include	<recGbl.h>
#include        <recSup.h>
#include	<devSup.h>

/*#include	<module_types.h> */

#include	<special.h>

#include        <aoRecord.h>
#include        <aiRecord.h>
#include        <boRecord.h>
#include        <biRecord.h>
#include        <longinRecord.h>
#include        <longoutRecord.h>
#include        <mbboRecord.h>
#include        <mbbiRecord.h>

#include	<link.h>
#include	<iocsh.h>
#include	<epicsExport.h>

static long init_bi(), read_bi();
static long init_bo(), write_bo();
static long checkCard(), write_card(), read_card();
static long get_bi_int_info();
static void devA32_isr();
static void devA32RebootFunc();

static long devcfMemMapReport();

static int  devcfMemMapDebug = 12;
epicsExportAddress(int, devcfMemMapDebug);

/***** devcfMemMapDebug *****/

/** devcfMemMapDebug == 0 --- no debugging messages **/
/** devcfMemMapDebug >= 5 --- hardware initialization information **/
/** devcfMemMapDebug >= 10 -- record initialization information **/
/** devcfMemMapDebug >= 15 -- write commands **/
/** devcfMemMapDebug >= 20 -- read commands **/


#define MAX_NUM_CARDS    1
#define MIN_A32_ADDRESS  0x10000000
#define MAX_A32_ADDRESS  0x10001800
#define MAX_ACTIVE_REGS  64   /* largest register number allowed */
#define MAX_ACTIVE_BITS  16   /* largest bit # expected */

#define IVEC_REG           29   /* Interrupt Vector Register (MRD100 ONLY) */
#define IVEC_ENA_REG       28   /* Interrupt Enable Register (MRD100 ONLY) */
#define IVEC_MASK          0xff /* Interrupt Vector Mask     (MRD100 ONLY) */
#define IVEC_ENA_MASK      0x10 /* Interrupt Enable Mask     (MRD100 ONLY) */
#define IVEC_REENABLE_MASK 0xf  /* Interrupt Re-enable Mask  (MRD100 ONLY) */
#define IVEC_REENABLE_REG  27   /* Interrupt Re-enable Reg   (MRD100 ONLY) */

/* Register layout */
typedef struct a32Reg {
  unsigned long reg[MAX_ACTIVE_REGS];
}a32Reg;

typedef struct a16Reg {
  unsigned short reg[MAX_ACTIVE_REGS];
}a16Reg;

typedef struct ioCard {  /* Unique for each card */
  volatile a16Reg  *base;    /* address of this card's registers */
  int               nReg;    /* Number of registers on this card */
  FAST_LOCK         lock;    /* semaphore */
  IOSCANPVT         ioscanpvt; /* records to process upon interrupt */
}ioCard;

static struct ioCard cards[MAX_NUM_CARDS]; /* array of card info */

typedef struct cfMemMapDpvt { /* unique for each record */
  unsigned short  reg;   /* index of register to use (determined by signal #*/
  unsigned short  lbit;  /* least significant bit of interest */
  unsigned short  nbit;  /* no of significant bits */
  unsigned short  type;  /* Type either 0 or 1 for uni, bi polar */
  unsigned long   mask;  /* mask to use ...  */
}cfMemMapDpvt;

/* Define the dset for cfMemMap */
typedef struct {
	long		number;
	DEVSUPFUN	report;		/* used by dbior */
	DEVSUPFUN	init;	
	DEVSUPFUN	init_record;	/* called 1 time for each record */
	DEVSUPFUN	get_ioint_info;	
	DEVSUPFUN	read_write;
        DEVSUPFUN       special_linconv;
} cfMemMapdset;

cfMemMapdset devBicfMemMap =   {6, devcfMemMapReport, NULL, init_bi, NULL, read_bi,  NULL};
epicsExportAddress(dset,devBicfMemMap);
cfMemMapdset devBocfMemMap =   {6, devcfMemMapReport, NULL, init_bo, NULL, write_bo, NULL};
epicsExportAddress(dset,devBocfMemMap);

/**************************************************************************
 **************************************************************************/
static long devcfMemMapReport()
{
int             i;
int		cardNum = 0;
unsigned short   regData;

  for(cardNum=0; cardNum < MAX_NUM_CARDS; cardNum++) {
    if(cards[cardNum].base != NULL) {
      printf("  Card #%d at %p\n", cardNum, cards[cardNum].base);
      for(i=0; i < cards[cardNum].nReg; i++) {
          regData = cards[cardNum].base->reg[i];
          printf("    Register %d -> %x (%lu)\n", i, regData, regData);
      }
    }
  }
return(0);
}


/**************************************************************************
*
* Initialization of A32/D32 Card
*
***************************************************************************/
int devcfMemMapConfig(card,a32base,nregs)
int	      card;
unsigned long a32base;
int	      nregs;
/*int	      iVector;
int	      iLevel;*/
{


  if((card >= MAX_NUM_CARDS) || (card < 0)) {
      epicsPrintf("devcfMemMapConfig: Invalid Card # %d \n",card);
      return(ERROR);
  }
  else{
  	epicsPrintf("devcfMemMapConfig: Valid Card # %d \n",card);
	}

  if((a32base >= MAX_A32_ADDRESS)||(a32base <= MIN_A32_ADDRESS)) {
      epicsPrintf("devcfMemMapConfig: Invalid Card Address %ul \n",a32base);
      return(ERROR);
  }
  else {
  	cards[card].base=a32base;
	epicsPrintf("devcfMemMapConfig: Valid Card Address %ul \n",a32base);
	}


  if(nregs > MAX_ACTIVE_REGS) {
      epicsPrintf("devcfMemMapConfig: # of registers (%d) exceeds max\n",nregs);
      return(ERROR);
  }
  else {
      cards[card].nReg = nregs;
      epicsPrintf("devcfMemMapConfig: # of registers = %d\n",nregs);
      FASTLOCKINIT(&(cards[card].lock));
      FASTUNLOCK(&(cards[card].lock));  /* Init the board lock */
  }
 
  return(OK);
}

/*int devcfMemMapConfigCreate(const char *name,int port,const char *inet_addr,int mem_len);*/

static const iocshArg devcfMemMapConfigArg0 = {"Card #", iocshArgInt};
static const iocshArg devcfMemMapConfigArg1 = {"Base address", iocshArgInt};
static const iocshArg devcfMemMapConfigArg2 = {"No. regs", iocshArgInt};
/*static const iocshArg devcfMemMapConfigArg3 = {"Memory buffer size", iocshArgInt};*/
static const iocshArg * const devcfMemMapConfigArgs[3] = {&devcfMemMapConfigArg0,
                                                       &devcfMemMapConfigArg1,
                                                       &devcfMemMapConfigArg2,
                                                       /*&devcfMemMapConfigArg3*/};
						       
static const iocshFuncDef devcfMemMapConfigFuncDef={"devcfMemMapConfig",3,devcfMemMapConfigArgs};
static void devcfMemMapConfigCallFunc(const iocshArgBuf *args)
{
 devcfMemMapConfig((int) args[0].ival, (unsigned long) args[1].ival, (int) args[2].ival);
}

void registercfMemMapConfig(void){
	iocshRegister(&devcfMemMapConfigFuncDef,&devcfMemMapConfigCallFunc);
	}
	
epicsExportRegistrar(registercfMemMapConfig);

/**************************************************************************
 *
 * BI record interrupt routine
 *
 **************************************************************************/
static long get_bi_int_info(cmd, pbi, ppvt)
int                     cmd;
struct biRecord         *pbi;
IOSCANPVT               *ppvt;
{

   struct vmeio           *pvmeio = (struct vmeio *)(&pbi->inp.value);

   if(cards[pvmeio->card].ioscanpvt != NULL) {
       *ppvt = cards[pvmeio->card].ioscanpvt;
       return(OK);
   }
   else {
       return(ERROR);
   }
}


/**************************************************************************
 *
 * Interrupt service routine
 *
 **************************************************************************/
static void devA32_isr(int card)
{
   scanIoRequest(cards[card].ioscanpvt);
   write_card(card, IVEC_REENABLE_REG, IVEC_REENABLE_MASK, IVEC_REENABLE_MASK);
}


/******************************************************************************
 *
 * A function to disable interrupts in case we get a ^X style reboot.
 *
 ******************************************************************************/
static void devA32RebootFunc(void)
{
  int   card = 0;

  while (card < MAX_NUM_CARDS)
  {
    if (cards[card].ioscanpvt != NULL) {
        write_card(card, IVEC_ENA_REG, IVEC_ENA_MASK, 0);
    }
    card++;
  }
  return;
}


/**************************************************************************
 *
 * BO Initialization (Called one time for each BO Coldfire record)
 *
 **************************************************************************/
static long init_bo(pbo)
struct boRecord *pbo;
{
    long                status = 0;
    int                 card, args, bit;
    unsigned short 	rawVal;
    cfMemMapDpvt         *pPvt;
    volatile unsigned short *IMMP;
    volatile unsigned short	cf_temp;


    epicsPrintf("init_bo\n");

    /* bo.out must be an VME_IO */
    switch (pbo->out.type) {
    case (VME_IO) :
 
      if(pbo->out.value.vmeio.card > MAX_NUM_CARDS) {
	pbo->pact = 1;		/* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bo: Card #%d exceeds max: ->%s<- \n",pbo->out.value.vmeio.card, pbo->name);
        return(ERROR);
      }
      else{
      	epicsPrintf("devcfMemMap_bo: Card # = %d: ->%s<- \n",pbo->out.value.vmeio.card, pbo->name);
	}

      card = pbo->out.value.vmeio.card;

      if(cards[card].base == NULL) {
	pbo->pact = 1;		/* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bo: Card #%d not initialized: ->%s<-\n",pbo->out.value.vmeio.card,pbo->name);
        return(ERROR);
      }
      else{
      	epicsPrintf("devcfMemMap_bo: Card #%d base address = %p: ->%s<-\n",pbo->out.value.vmeio.card,cards[card].base,pbo->name);
	}

      if (pbo->out.value.vmeio.signal >= cards[card].nReg) {
        pbo->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bo: Signal # %d exceeds registers: ->%s<-\n",
                     cards[card].nReg,pbo->name);
        return(ERROR);
      }
      else{
      	epicsPrintf("devcfMemMap_bo: Signal # = %d: ->%s<-\n",cards[card].nReg,pbo->name);
	}

      printf("Parm = %s\n",pbo->out.value.vmeio.parm);
      args = sscanf(pbo->out.value.vmeio.parm, "%d", &bit);
 
      if((args != 1) || (bit >= MAX_ACTIVE_BITS)) {
        pbo->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bo: Invalid Bit # %d, args=%d: ->%s<-\n",
                     bit,args,pbo->name);
        return(ERROR);
      }
      else{
      	epicsPrintf("devcfMemMap_bo: Bit # in parm field = %d: ->%s<-\n",
                    bit, pbo->name);
	}

      pbo->dpvt = (void *)calloc(1, sizeof(struct cfMemMapDpvt));
      pPvt = (cfMemMapDpvt *)pbo->dpvt;

      pPvt->reg =  pbo->out.value.vmeio.signal;
      pPvt->lbit = bit;
      pPvt->mask = 1 << pPvt->lbit;
      pbo->mask = pPvt->mask;
      
      /* set port direction to out */
      IMMP=((long)cards[card].base & 0xfffff000);
      cf_temp=MCF5272_RD_GPIO_PCDDR(IMMP); /* only port C accessible yet */
      MCF5272_WR_GPIO_PCDDR(IMMP,cf_temp | pPvt->mask);

      if (read_card(card, pPvt->reg, pPvt->mask, &rawVal) == OK)
         {
         pbo->rbv = pbo->rval = rawVal;
         }
      else 
         {
         status = 2;
         }
      break;
         
    default :
	pbo->pact = 1;		/* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bo: Illegal OUT field ->%s<- \n", pbo->name);
        status = ERROR;
    }
    return(status);
}

/**************************************************************************
 *
 * BI Initialization (Called one time for each BI record)
 *
 **************************************************************************/
static long init_bi(pbi)
struct biRecord *pbi;
{
    long                status = 0;
    int                 card, args, bit;
    unsigned short       rawVal;
    cfMemMapDpvt         *pPvt;
    volatile unsigned short *IMMP;
    volatile unsigned short	cf_temp;
   
    epicsPrintf("init_bi\n");
    /* bi.inp must be an VME_IO */
    switch (pbi->inp.type) {
    case (VME_IO) :

      if(pbi->inp.value.vmeio.card > MAX_NUM_CARDS) {
        pbi->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap: Card #%d exceeds max: ->%s<- \n",pbi->inp.value.vmeio.card, pbi->name);
        return(ERROR);
      }

      card = pbi->inp.value.vmeio.card;

      if(cards[card].base == NULL) {
        pbi->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bi: Card #%d not initialized: ->%s<-\n",pbi->inp.value.vmeio.card,pbi->name);
        return(ERROR);
      }

      if (pbi->inp.value.vmeio.signal >= cards[card].nReg) {
        pbi->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bi: Signal # exceeds registers: ->%s<-\n",
                     pbi->name);
        return(ERROR);
      }

      args = sscanf(pbi->inp.value.vmeio.parm, "%d", &bit);

      if((args != 1) || (bit >= MAX_ACTIVE_BITS)) {
        pbi->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bi: Invalid Bit # in parm field: ->%s<-\n",
                     pbi->name);
        return(ERROR);
      }
      pbi->dpvt = (void *)calloc(1, sizeof(struct cfMemMapDpvt));
      pPvt = (cfMemMapDpvt *)pbi->dpvt;

      pPvt->reg =  pbi->inp.value.vmeio.signal;
      pPvt->lbit = bit;
      pPvt->mask = 1 << pPvt->lbit;
      pbi->mask = pPvt->mask;
      
            /* set port direction to in */

      IMMP=((long)cards[card].base & 0xfffff000);
      cf_temp=MCF5272_RD_GPIO_PCDDR(IMMP); /* only port C accessible yet */
      MCF5272_WR_GPIO_PCDDR(IMMP,cf_temp & ~(pPvt->mask));

      if (read_card(card, pPvt->reg, pPvt->mask, &rawVal) == OK)
         {
         pbi->rval = rawVal;
         status = 0;
         }
      else
         {
         status = 2;
         }
      break;

    default :
        pbi->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfMemMap_bi: Illegal INP field ->%s<- \n", pbi->name);
        status = ERROR;
    }
    return(status);
}


/**************************************************************************
 *
 * Perform a write operation from a BO record
 *
 **************************************************************************/
static long write_bo(pbo)
struct boRecord *pbo;
{

  unsigned short 	rawVal;
  cfMemMapDpvt           *pPvt = (cfMemMapDpvt *)pbo->dpvt;

  if (write_card(pbo->out.value.vmeio.card, pPvt->reg, pbo->mask, pbo->rval) 
        == OK)
  {
    if(read_card(pbo->out.value.vmeio.card, pPvt->reg, pbo->mask, &rawVal) 
        == OK)
    {
      pbo->rbv = rawVal;
      return(0);
    }
  }

  /* Set an alarm for the record */
  recGblSetSevr(pbo, WRITE_ALARM, INVALID_ALARM);
  return(2);
}

/**************************************************************************
 *
 * Perform a read operation from a BI record
 *
 **************************************************************************/
static long read_bi(pbi)
struct biRecord *pbi;
{

  unsigned short       rawVal;
  cfMemMapDpvt           *pPvt = (cfMemMapDpvt *)pbi->dpvt;

  if (read_card(pbi->inp.value.vmeio.card, pPvt->reg, pbi->mask,&rawVal) == OK)
  {
     pbi->rval = rawVal;
     return(0);
  }

  /* Set an alarm for the record */
  recGblSetSevr(pbi, READ_ALARM, INVALID_ALARM);
  return(2);
}


/**************************************************************************
 *
 * Raw read a bitfield from the card
 *
 **************************************************************************/
static long read_card(card, reg, mask, value)
short           card;  
unsigned short  reg;
unsigned long   mask;   /* created in init_bo() */
unsigned short  *value; /* the value to return from the card */
{
  if (checkCard(card) == ERROR)
    return(ERROR);

  *value = cards[card].base->reg[reg] & mask;

  if (devcfMemMapDebug >= 20)
    epicsPrintf("devcfMemMap: read 0x%8.8X from card %d\n", *value, card);

  return(OK);
}



/**************************************************************************
 *
 * Write a bitfield to the card retaining the states of the other bits
 *
 **************************************************************************/
static long write_card(card, reg, mask, value)
short           card;
unsigned short  reg;
unsigned long   mask;
unsigned short   value;
{
  if (checkCard(card) == ERROR)
    return(ERROR);

  FASTLOCK(&(cards[card].lock));
  cards[card].base->reg[reg] = ((cards[card].base->reg[reg] & ~mask) | 
                              (value & mask));
  FASTUNLOCK(&(cards[card].lock));

  if (devcfMemMapDebug >= 15)
    epicsPrintf("devcfMemMap: wrote 0x%8.8X to card %d\n",
            cards[card].base->reg[reg], card);

  return(0);
}


/**************************************************************************
 *
 * Make sure card number is valid
 *
 **************************************************************************/
static long checkCard(card)
short   card;
{
  if ((card >= MAX_NUM_CARDS) || (cards[card].base == NULL))
    return(ERROR);
  else
    return(OK);
}

