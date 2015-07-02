/* devCfSPI.c
*****************************************************************
   Author: D. Peter Siddons, 
           Brookhaven National Laboratory,
   Date: 1/26/04
*****************************************************************

derived from:

 	devA32Vme.c	*/

/*****************************************************************
 *
 *      Original Author :                     Ned D. Arnold
 *      Original Date:                        11/21/97
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


/*	This version has all spi devices selected from external decoder */

/*To Use this device support, Include the following before iocInit */
/* devCFSPIConfig(card,a32base,nreg)  */
/*    card    = card number                           */
/*    a32base = base address of card                  */
/*    nreg    = number of A32 registers on this card  */
/* For Example					      */
/* devCFSPIConfig(0, 0x10000000, 1)        */
/* card is interpreted as ID of SPI device */
/* a32base is ignored for SPI devices, but must be non-zero. */
/* nreg is number of signals in device */

 /**********************************************************************/
 /** Brief Description of device support                              **/
 /**						    	              **/
 /** THis support provide access to devices attached to the coldfire  **/
 /** SPI interface, in particular DAC's and ADC's. Only ao and ai     **/
 /** records are supported.	  				      **/
 /** 	  **/
 /** 	  **/
 /** 	  **/
 /**						    	              **/
 /** Record type     Signal #           Parm Field                    **/
 /**                                                                  **/
 /**    ai          reg                width, type; 		      **/
 /**    ao          reg                width, type;                   **/
 /**                                                                  **/
 /** reg specifies the register number (0 - 7) for chips with multiple**/
 /** channels.                                                        **/
 /** Parm field must be provided, no defaults are assumed ...         **/
 /** In ai and ao type is either 0 - unipolar, 1 -bipolar             **/
 /**                                                                  **/
 /**                                                                  **/
 /**********************************************************************/


#define FAST_LOCK epicsMutexId
#define FASTLOCKINIT(PFAST_LOCK) (*(PFAST_LOCK) = epicsMutexCreate())
#define FASTLOCK(PFAST_LOCK) epicsMutexLock(*(PFAST_LOCK));
#define TRYLOCK(PFAST_LOCK) epicsMutexTryLock(*(PFAST_LOCK));
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

static long init_ai(), read_ai();
static long init_ao(), write_ao();
static long checkCard(), write_card(), read_card();

static long devcfSPIReport();
void devcfSPI_init(int after);
short spi_read(int card, int reg);
void spi_write(int card, int reg, unsigned long value);

volatile int  devcfSPIDebug = 0;
epicsExportAddress(int, devcfSPIDebug);

/***** devcfSPIDebug *****/

/** devcfSPIDebug == 0 --- no debugging messages **/
/** devcfSPIDebug >= 5 --- hardware initialization information **/
/** devcfSPIDebug >= 10 -- record initialization information **/
/** devcfSPIDebug >= 15 -- write commands **/
/** devcfSPIDebug >= 20 -- read commands **/


#define MAX_NUM_CARDS    8
#define MAX_ACTIVE_REGS  8   /* max registers allowed */
#define MAX_ACTIVE_BITS  16   /* largest bnumber of bits expected */
#define MAX_A32_ADDRESS  7	/* 3 bits to decode for chip selects */
#define MIN_A32_ADDRESS  0
#define cfSPI_INPUT	0
#define cfSPI_OUTPUT	1
#define IMMP		0x10000000 /* aka MBAR, BSP-specific */
/*#define NO_DEVICE	0	*//* need a non-existent SPI device # for parking */

FAST_LOCK	SPI_lock;	/* SPI is shared resource; lock accesses */

/* Register layout */
typedef struct a32Reg {
  unsigned long reg[MAX_ACTIVE_REGS];
}a32Reg;

typedef struct a16Reg {
  unsigned short reg[MAX_ACTIVE_REGS];
}a16Reg;

typedef struct ioCard {  /* Unique for each card */
  unsigned short    base;    /* index of this card's chip */
  int		    card_type; /* 0 or 1, input or output: SPI is ro or wo, not rw */
  int               nReg;    /* Number of registers in this chip */
  FAST_LOCK         lock;    /* semaphore */
  IOSCANPVT         ioscanpvt; /* records to process upon interrupt */
}ioCard;

static struct ioCard cards[MAX_NUM_CARDS]; /* array of card info */

typedef struct cfSPIDpvt { /* unique for each record */
  int  reg;   /* index of sub-device (determined by signal #*/
  int  nbit;  /* no of significant bits */
  int  type;  /* Type either 0 or 1 for uni, bi polar */
  unsigned long  value; /* to provide readback for ao records */
}cfSPIDpvt;

typedef struct cfSPI_setup {
   int mode_r;	/* mode register */
   int cmd_r; /* command register */
   }cfSPI_setup;

static struct cfSPI_setup SPI_setup[MAX_NUM_CARDS]; /* array of SPI setups */

/* Define the dset for cfSPI */
typedef struct {
	long		number;
	DEVSUPFUN	report;		/* used by dbior */
	DEVSUPFUN	init;	
	DEVSUPFUN	init_record;	/* called 1 time for each record */
	DEVSUPFUN       get_ioint_info;
	DEVSUPFUN	read_write;
        DEVSUPFUN       special_linconv;
} cfSPIdset;

cfSPIdset devAicfSPI =   {6, devcfSPIReport, NULL, init_ai, NULL, read_ai,  NULL};
epicsExportAddress(dset,devAicfSPI);
cfSPIdset devAocfSPI =   {6, devcfSPIReport, NULL, init_ao, NULL, write_ao, NULL};
epicsExportAddress(dset,devAocfSPI);


/**************************************************************************
 **************************************************************************/

static long devcfSPIReport()
{
int		cardNum = 0;

  for(cardNum=0; cardNum < MAX_NUM_CARDS; cardNum++) {
    if(cards[cardNum].base != NULL) {
      epicsPrintf("  Card #%d at %d\n", cardNum, cards[cardNum].base);
      epicsPrintf("  Registers: %d\n",cards[cardNum].nReg );
    }
  }
return(0);
}


/**************************************************************************
*
* Initialization of A32/D32 Card
*
***************************************************************************/
int devcfSPIConfig(card,a32base,nregs)
int	      card;
unsigned short a32base;
int	      nregs;
/*int	      iVector;
int	      iLevel;*/
{

int temp;

  if((card >= MAX_NUM_CARDS) || (card < 0)) {
      epicsPrintf("devcfSPIConfig: Invalid Card # %d \n",card);
      return(ERROR);
  }
  else{
  	epicsPrintf("devcfSPIConfig: Valid Card # %d \n",card);
	}

  if((a32base >= MAX_A32_ADDRESS)||(a32base <= MIN_A32_ADDRESS)) {
      epicsPrintf("devcfSPIConfig: Invalid Card Address %d \n",a32base);
      return(ERROR);
  }
  else {
  	cards[card].base=a32base;
	epicsPrintf("devcfSPIConfig: Valid Card Address %d \n",a32base);
	}


  if(nregs > MAX_ACTIVE_REGS) {
      epicsPrintf("devcfSPIConfig: # of registers (%d) exceeds max\n",nregs);
      return(ERROR);
  }
  else {
      cards[card].nReg = nregs;
      epicsPrintf("devcfSPIConfig: # of registers = %d\n",nregs);
      FASTLOCKINIT(&(SPI_lock));
      FASTUNLOCK(&(SPI_lock));  /* Init the board lock */
      
      FASTLOCK(&(SPI_lock));
      MCF5272_WR_QSPI_QWR(IMMP,0x8000); /* set HALT bit */
      FASTUNLOCK(&(SPI_lock));
  }

  /* initialize SPI_setup structures. Device specific, customize! */
  switch(card){
        case 0: {       /* ADC chip */
                        SPI_setup[card].mode_r = 0xa110;/* mode register */
                        SPI_setup[card].cmd_r = 0x0000; /* command register */
                        break;
                }
        case 1: {       /* DAC chip #1 */
                        SPI_setup[card].mode_r = 0x8108; /*0x8018; mode register: master, 16bits pol=0, phas=1 */
                        SPI_setup[card].cmd_r = 0x4000; /* command register */
                        break;
                }
        case 2: {       SPI_setup[card].mode_r = 0;     /* mode register */
                        SPI_setup[card].cmd_r = 0; /* command register */
                        break;
                }
        case 3: {       SPI_setup[card].mode_r = 0;     /* mode register */
                        SPI_setup[card].cmd_r = 0; /* command register */
                        break;
                }
        case 4: {       SPI_setup[card].mode_r = 0;     /* mode register */
                        SPI_setup[card].cmd_r = 0; /* command register */
                        break;
                }
        case 5: {       SPI_setup[card].mode_r = 0;     /* mode register */
                        SPI_setup[card].cmd_r = 0; /* command register */
                        break;
                }
        case 6: {       /* DAC chip #2 */
                        SPI_setup[card].mode_r = 0x8108; /* 0x8018; mode register */
                        SPI_setup[card].cmd_r = 0x4000; /* command register */
                        break;
                }
        case 7: {       SPI_setup[card].mode_r = 0;     /* mode register */
                        SPI_setup[card].cmd_r = 0; /* command register */
                        break;
                }
        default: return(ERROR);
        }

  return(OK);
}


/*int devcfSPIConfigCreate(const char *name,int port,const char *inet_addr,int mem_len);*/

static const iocshArg devcfSPIConfigArg0 = {"Card #", iocshArgInt};
static const iocshArg devcfSPIConfigArg1 = {"Base address", iocshArgInt};
static const iocshArg devcfSPIConfigArg2 = {"No. regs", iocshArgInt};

static const iocshArg * const devcfSPIConfigArgs[3] = {&devcfSPIConfigArg0,
                                                       &devcfSPIConfigArg1,
                                                       &devcfSPIConfigArg2,
                                                       };
						       
static const iocshFuncDef devcfSPIConfigFuncDef={"devcfSPIConfig",3,devcfSPIConfigArgs};
static void devcfSPIConfigCallFunc(const iocshArgBuf *args)
{
 devcfSPIConfig((int) args[0].ival, (unsigned short) args[1].ival, (int) args[2].ival);
}

void registercfSPIConfig(void){
	iocshRegister(&devcfSPIConfigFuncDef,&devcfSPIConfigCallFunc);
	}
	
epicsExportRegistrar(registercfSPIConfig);



/**************************************************************************
 *
 * SPI Initialization (Called twice for each declared device())
 * This is repetitive, but what can you do?
 * ao writes a 16-bit word, 4 address bits and 12 data bits.
 * ai writes 32 bits, then reads 32 and extracts 12 to make value.
 * This routine sets up data structures to support these two modes.
 *
 **************************************************************************/
/*static long devcfSPI_init(int after){*/
/* Initialize common bits of SPI port. Per-device details done by 
   init_ai/ao
*/

/*}*/



/**************************************************************************
 *
 * AI Initialization (Called one time for each AI record)
 *
 **************************************************************************/
static long init_ai(pai)
struct aiRecord   *pai;
{
    long                status = 0;
    int                 card, args, numBits, twotype;
    cfSPIDpvt         *pPvt;

    /* ai.inp must be an VME_IO */
    switch (pai->inp.type) {
    case (VME_IO) :
      if(pai->inp.value.vmeio.card > MAX_NUM_CARDS) {
        pai->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Card #%d exceeds max: ->%s<- \n",pai->inp.value.vmeio.card, pai->name);
        return(ERROR);
      }

      card = pai->inp.value.vmeio.card;

      if(cards[card].base == NULL) {
        pai->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Card #%d not initialized: ->%s<-\n",pai->inp.value.vmeio.card,
                     pai->name);
        return(ERROR);
      }

      if (pai->inp.value.vmeio.signal >= cards[card].nReg) {
        pai->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Signal # exceeds registers: ->%s<-\n",
                     pai->name);
        return(ERROR);
      }

      args = sscanf(pai->inp.value.vmeio.parm, "%d,%d", &numBits, &twotype);

      if( (args != 2) || (numBits <= 0) || (numBits > MAX_ACTIVE_BITS) ||
         	(twotype > 1) || (twotype < 0) ) {
        pai->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf
        ("devcfSPI: Invalid Width/Type in parm field: ->%s<-\n",
                     pai->name);
        return(ERROR);
      }
      
      cards[card].card_type = cfSPI_INPUT;
      
      pai->dpvt = (void *)calloc(1, sizeof(struct cfSPIDpvt));
      pPvt = (cfSPIDpvt *)pai->dpvt;

      pPvt->reg =  pai->inp.value.vmeio.signal;
      pPvt->nbit = numBits;
      pPvt->type = twotype;
      pai->eslo = (pai->eguf - pai->egul)/(pow(2,numBits)-1);
      
/*  Shift Raw value if Bi-polar */
      if (pPvt->type ==1) 
         pai->roff = pow(2,(numBits-1));

      status = OK;

      break;
    default :
        pai->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Illegal INP field ->%s<- \n", pai->name);
        status = ERROR;
    }
    return(status);
}



/**************************************************************************
 *
 * AO Initialization (Called one time for each AO record)
 *
 **************************************************************************/
static long init_ao(pao)
struct aoRecord   *pao;
{
    long                status = 0L;
    unsigned long       rawVal=0L;
    int                 card,
    			reg, 
    			args, 
			numBits, 
			twotype;
    cfSPIDpvt         *pPvt;

    /* ao.out must be an VME_IO */
    switch (pao->out.type) {
    case (VME_IO) :
      if(pao->out.value.vmeio.card > MAX_NUM_CARDS) {
        pao->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Card #%d exceeds max: ->%s<- \n",pao->out.value.vmeio.card, pao->name);
        return(ERROR);
      }

      card = pao->out.value.vmeio.card;
      reg = pao->out.value.vmeio.signal;

      if(cards[card].base == NULL) {
        pao->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Card #%d not initialized: ->%s<-\n",pao->out.value.vmeio.card,
                     pao->name);
        return(ERROR);
      }

      if (pao->out.value.vmeio.signal >= cards[card].nReg) {
        pao->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Signal # exceeds registers: ->%s<-\n",
                     pao->name);
        return(ERROR);
      }

      args = sscanf(pao->out.value.vmeio.parm, "%d,%d", &numBits, &twotype);

      if( (args != 2) || (numBits <= 0) ||
         	(numBits > MAX_ACTIVE_BITS) ||
         	(twotype > 1) || (twotype < 0) ) {
        epicsPrintf
        ("devcfSPI: Invalid Width/Type in parm field: ->%s<-\n",
                     pao->name);
        return(ERROR);
      }

      cards[card].card_type=cfSPI_OUTPUT;
      
      pao->dpvt = (void *)calloc(1, sizeof(struct cfSPIDpvt));
      pPvt = (cfSPIDpvt *)pao->dpvt;

      pPvt->reg =  pao->out.value.vmeio.signal;
      pPvt->nbit = numBits;
      pPvt->type = twotype;
      pPvt->value = rawVal;
      if (devcfSPIDebug >= 20)
        epicsPrintf("init_ao 1:\n\tpPvt->reg %i\n\tpPvt->nbit %i\n\tpPvt->type %i\n\tpPvt->value %i \n\r",pPvt->reg,pPvt->nbit,pPvt->type,pPvt->value);


      pao->eslo = (pao->eguf - pao->egul)/(pow(2,numBits)-1);

/*  Shift Raw value if Bi-polar */
      if (pPvt->type == 1) 
         pao->roff = pow(2,(numBits-1));

      /* Init rval to current setting */ 
  /*    if(read_card(card,reg,&pPvt,&rawVal) == OK) { */
        pao->rbv = rawVal;
      if (devcfSPIDebug >= 20)
        epicsPrintf("init_ao 2: pao->rbv %i\n",pao->rbv);

/* here is where we do the sign extensions for Bipolar.... */        
        if (pPvt->type ==1) {
           if (pao->rbv & (2<<(pPvt->nbit-2)))
               pao->rbv |= ((2<<31) - (2<<(pPvt->nbit-2)))*2 ;
	}

        pao->rval = pao->rbv;
    /*  } */
      
      spi_write(1,8,0); /* configure DAC chips */
      spi_write(1,9,0);
      spi_write(6,8,0);
      spi_write(6,9,0);
      
      status = OK;

      break;
    default :
        pao->pact = 1;          /* make sure we don't process this thing */
        epicsPrintf("devcfSPI: Illegal OUT field ->%s<- \n", pao->name);
        status = ERROR;
    }
    return(status);
}


/**************************************************************************
 *
 * Perform a read operation from a AI record
 *
 **************************************************************************/
static long read_ai(struct aiRecord *pai)
{
  unsigned long         rawVal=0;
  int 			card,reg;
  cfSPIDpvt           *pPvt = (cfSPIDpvt *)pai->dpvt;

  card = pai->inp.value.vmeio.card;
  reg = pai->inp.value.vmeio.signal;
  
  if (read_card(card,reg,&pPvt,&rawVal) == OK)
  {
     pai->rval = rawVal;

  if (devcfSPIDebug >= 20)
          epicsPrintf("read_ai: card %i  reg %i rawVal %i\n\r", card, reg, rawVal);

/* here is where we do the sign extensions for Bipolar....  */       
        if (pPvt->type ==1) {
           if (pai->rval & (2<<(pPvt->nbit-2))) 
               pai->rval |= ((2<<31) - (2<<(pPvt->nbit-2)))*2; 

	}

     return(0);
  }

  /* Set an alarm for the record if not OK */
  recGblSetSevr(pai, READ_ALARM, INVALID_ALARM);
  return(2);

}

/**************************************************************************
 *
 * Perform a write operation from a AO record
 *
 **************************************************************************/
static long write_ao(pao)
struct aoRecord *pao;
{

  unsigned long		rawVal=0;
  int			card,reg;
  cfSPIDpvt		*pPvt = (cfSPIDpvt *)pao->dpvt;

  card = pao->out.value.vmeio.card;
  reg = pao->out.value.vmeio.signal;
  
  if (write_card(card,reg,&pPvt,pao->rval) == OK)
  {
    if(read_card(card,reg,&pPvt,&rawVal)
       == OK)
    {
      pao->rbv = rawVal;
      if (devcfSPIDebug >= 20)
          epicsPrintf("write_ao: card %i  reg %i rawVal %i\n\r", card, reg, rawVal);


/* here is where we do the sign extensions for Bipolar.... */        
        if (pPvt->type ==1) {
           if (pao->rbv & (2<<(pPvt->nbit-2)))
               pao->rbv |= ((2<<31) - (2<<(pPvt->nbit-2)))*2;

	}
      
      return(0);
    }
  }

  /* Set an alarm for the record */
  recGblSetSevr(pao, WRITE_ALARM, INVALID_ALARM);
  return(2);
}


/**************************************************************************
 *
 * Raw read a value from the card
 *
 **************************************************************************/
static long read_card(int card, int reg, cfSPIDpvt *pPvt, unsigned long *value)
{
  if (checkCard(card) == ERROR)
    return(ERROR);
  if(cards[card].card_type==cfSPI_INPUT){
     *value = (unsigned long)spi_read(card,reg);
     }
  if(cards[card].card_type==cfSPI_OUTPUT) {
     *value = pPvt->value; /* i.e. last written value */
     }

  if (devcfSPIDebug >= 20)
    epicsPrintf("read_card: read %ld from card %d reg %d\n\r", *value, card, reg);

  return(OK);
}



/**************************************************************************
 *
 * Write a value to the card
 *
 **************************************************************************/
static long write_card(int card, int reg, cfSPIDpvt *pPvt, unsigned long value)

{
  if (checkCard(card) == ERROR)
    return(ERROR);
  if ((value > 4095) || (value < 0)) 
    return(ERROR);
  
  /* cards[card].base->reg[reg] = ((cards[card].base->reg[reg]) | 
                              (value)); */
  spi_write(card, reg,value);
  
  if(cards[card].card_type==cfSPI_OUTPUT){
  	pPvt->value=value;
	} 
  

  if (devcfSPIDebug >= 20)
    epicsPrintf("write_card: wrote %ld to card %d reg %d\n\r",
            value, card, reg);

  return(0);
}

/**************************************************************************
 *
 * Coldfire-specific SPI I/O routines go here.
 *
 **************************************************************************/

void setdevice(int device){
unsigned int mask,temp;
  if (devcfSPIDebug >= 20)	
	epicsPrintf("Setting device #%d\n\r",device);

	mask=0x000f; /* bits 0-2 are SPI device address, bit 3 is enable */
	temp=MCF5272_RD_GPIO_PCDDR(IMMP);
	MCF5272_WR_GPIO_PCDDR(IMMP,(temp | mask)); /* make outputs: should only do once? */

/*	temp=MCF5272_RD_GPIO_PCDAT(IMMP)&0xfff7;*//* bit 3 is enable bit */
/*	MCF5272_WR_GPIO_PCDAT(IMMP,temp); *//* clear enable bit */

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
  cmd = (0x008f | ((reg & 7) << 4)); /* Start bit, channel #, single-ended, unipolar, external clock */
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
        MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer */

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
                epicsPrintf("SPI read %hx %hx %hx = %hx from card #%d, reg #%d\n\r",data0,data1,data2,result,card,reg);
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
	
  	MCF5272_WR_QSPI_QMR(IMMP,setup.mode_r);	/* set mode */
	MCF5272_WR_QSPI_QAR(IMMP,0x20);		/* point to first cmd loc. */
	MCF5272_WR_QSPI_QDR(IMMP,setup.cmd_r);	/* store command */
	MCF5272_WR_QSPI_QWR(IMMP,0x00);		/* only one word to transfer */
	MCF5272_WR_QSPI_QAR(IMMP,0x00);		/* point to first data loc */
	MCF5272_WR_QSPI_QDR(IMMP,data);		/* store data */
	MCF5272_WR_QSPI_QDLYR(IMMP,(MCF5272_RD_QSPI_QDLYR(IMMP) | 0x8000)); /* start transfer */

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
	   epicsPrintf("SPI_write %ld to card #%d, reg #%d\n\r",value,card,reg);
}


/**************************************************************************
 *
 * Make sure card number is valid
 *
 **************************************************************************/
static long checkCard(card)
int   card;
{
  if ((card >= MAX_NUM_CARDS) || (cards[card].base == NULL))
    return(ERROR);
  else
    return(OK);
}

