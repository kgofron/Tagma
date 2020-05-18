#include "ellLib.h"
#include "epicsMutex.h"
#include "link.h"
#include "epicsTime.h"
#include "epicsTypes.h"

#ifndef INCshapeTH
#define INCshapeTH
typedef enum {
	shapeT_1,
	shapeT_2,
	shapeT_3,
	shapeT_4
}shapeT;
#endif /*INCshapeTH*/

#ifndef INCnslsdetGateH
#define INCnslsdetGateH
typedef enum {
	nslsdetGate_N,
	nslsdetGate_Y
}nslsdetGate;
#endif /*INCnslsdetGateH*/

#ifndef INCnslsdetCONTH
#define INCnslsdetCONTH
typedef enum {
	nslsdetCONT_OneShot,
	nslsdetCONT_AutoCount
}nslsdetCONT;
#endif /*INCnslsdetCONTH*/

#ifndef INCnslsdetCNTH
#define INCnslsdetCNTH
typedef enum {
	nslsdetCNT_Done,
	nslsdetCNT_Count
}nslsdetCNT;
#endif /*INCnslsdetCNTH*/

#ifndef INCGainH
#define INCGainH
typedef enum {
	gain_H,
	gain_L
}Gain;
#endif /*INCGainH*/
#ifndef INCnslsdetH
#define INCnslsdetH
typedef struct nslsdetRecord {
	char		name[61]; /*Record Name*/
	char		desc[29]; /*Descriptor*/
	char		asg[29]; /*Access Security Group*/
	epicsEnum16	scan;	/*Scan Mechanism*/
	epicsEnum16	pini;	/*Process at iocInit*/
	short		phas;	/*Scan Phase*/
	short		evnt;	/*Event Number*/
	short		tse;	/*Time Stamp Event*/
	DBLINK		tsel;	/*Time Stamp Link*/
	epicsEnum16	dtyp;	/*Device Type*/
	short		disv;	/*Disable Value*/
	short		disa;	/*Disable*/
	DBLINK		sdis;	/*Scanning Disable*/
	epicsMutexId	mlok;	/*Monitor lock*/
	ELLLIST		mlis;	/*Monitor List*/
	unsigned char	disp;	/*Disable putField*/
	unsigned char	proc;	/*Force Processing*/
	epicsEnum16	stat;	/*Alarm Status*/
	epicsEnum16	sevr;	/*Alarm Severity*/
	epicsEnum16	nsta;	/*New Alarm Status*/
	epicsEnum16	nsev;	/*New Alarm Severity*/
	epicsEnum16	acks;	/*Alarm Ack Severity*/
	epicsEnum16	ackt;	/*Alarm Ack Transient*/
	epicsEnum16	diss;	/*Disable Alarm Sevrty*/
	unsigned char	lcnt;	/*Lock Count*/
	unsigned char	pact;	/*Record active*/
	unsigned char	putf;	/*dbPutField process*/
	unsigned char	rpro;	/*Reprocess */
	void		*asp;	/*Access Security Pvt*/
	struct putNotify *ppn;	/*addr of PUTNOTIFY*/
	struct putNotifyRecord *ppnr;	/*pputNotifyRecord*/
	struct scan_element *spvt;	/*Scan Private*/
	struct rset	*rset;	/*Address of RSET*/
	struct dset	*dset;	/*DSET address*/
	void		*dpvt;	/*Device Private*/
	struct dbRecordType *rdes;	/*Address of dbRecordType*/
	struct lockRecord *lset;	/*Lock Set*/
	epicsEnum16	prio;	/*Scheduling Priority*/
	unsigned char	tpro;	/*Trace Processing*/
	char bkpt;	/*Break Point*/
	unsigned char	udf;	/*Undefined*/
	epicsTimeStamp	time;	/*Time*/
	DBLINK		flnk;	/*Forward Process Link*/
	float		vers;	/*Code Version*/
	void *           val;	/*Value*/
	void *           bptr;	/*Buffer Pointer*/
	unsigned long	nval;	/*Number of values*/
	unsigned long	nelm;	/*Number of Elements*/
	unsigned long	nord;	/*Number elements read*/
	double		freq;	/*Time base freq*/
	epicsEnum16	cnt;	/*Count*/
	epicsEnum16	pcnt;	/*Prev Count*/
	short		ss;	/*nslsdet state*/
	short		us;	/*User state*/
	epicsEnum16	cont;	/*OneShot/AutoCount*/
	DBLINK		inp;	/*Input Specification*/
	short		card;	/*Card Number*/
	float		dly;	/*Delay*/
	float		dly1;	/*Auto-mode Delay*/
	short		nch;	/*Number of Channels*/
	epicsEnum16	g1;	/*Gate Control*/
	double		tp;	/*Time Preset*/
	double		tp1;	/*Auto Time Preset*/
	epicsInt32		pr1;	/*Preset 1*/
	double		t;	/*Timer*/
	epicsEnum16	shpt;	/*Shaping time*/
	epicsEnum16	gain;	/*Gain*/
	short		aoen;	/*Analog out chan*/
	short		loen;	/*Leakage out chan*/
	short		eblk;	/*Enable input bias current*/
	void *           chen;	/*Value*/
	void *           pchen;	/*Buffer Pointer*/
	void *           tsen;	/*Value*/
	void *           ptsen;	/*Buffer Pointer*/
	void *           tr1;	/*Value*/
	void *           ptr1;	/*Buffer Pointer*/
	void *           tr2;	/*Value*/
	void *           ptr2;	/*Buffer Pointer*/
	void *           tr3;	/*Value*/
	void *           ptr3;	/*Buffer Pointer*/
	void *           tr4;	/*Value*/
	void *           ptr4;	/*Buffer Pointer*/
	void *           s1;	/*Value*/
	void *           ps1;	/*Buffer Pointer*/
	void *           s2;	/*Value*/
	void *           ps2;	/*Buffer Pointer*/
	void *           s3;	/*Value*/
	void *           ps3;	/*Buffer Pointer*/
	char		nm1[16]; /*Scaler A name*/
	char		nm2[16]; /*Scaler B name*/
	char		nm3[16]; /*Scaler C name*/
	char		egu[16]; /*Units Name*/
	short		prec;	/*Display Precision*/
	DBLINK		cout;	/*CNT Output*/
} nslsdetRecord;
#define nslsdetRecordNAME	0
#define nslsdetRecordDESC	1
#define nslsdetRecordASG	2
#define nslsdetRecordSCAN	3
#define nslsdetRecordPINI	4
#define nslsdetRecordPHAS	5
#define nslsdetRecordEVNT	6
#define nslsdetRecordTSE	7
#define nslsdetRecordTSEL	8
#define nslsdetRecordDTYP	9
#define nslsdetRecordDISV	10
#define nslsdetRecordDISA	11
#define nslsdetRecordSDIS	12
#define nslsdetRecordMLOK	13
#define nslsdetRecordMLIS	14
#define nslsdetRecordDISP	15
#define nslsdetRecordPROC	16
#define nslsdetRecordSTAT	17
#define nslsdetRecordSEVR	18
#define nslsdetRecordNSTA	19
#define nslsdetRecordNSEV	20
#define nslsdetRecordACKS	21
#define nslsdetRecordACKT	22
#define nslsdetRecordDISS	23
#define nslsdetRecordLCNT	24
#define nslsdetRecordPACT	25
#define nslsdetRecordPUTF	26
#define nslsdetRecordRPRO	27
#define nslsdetRecordASP	28
#define nslsdetRecordPPN	29
#define nslsdetRecordPPNR	30
#define nslsdetRecordSPVT	31
#define nslsdetRecordRSET	32
#define nslsdetRecordDSET	33
#define nslsdetRecordDPVT	34
#define nslsdetRecordRDES	35
#define nslsdetRecordLSET	36
#define nslsdetRecordPRIO	37
#define nslsdetRecordTPRO	38
#define nslsdetRecordBKPT	39
#define nslsdetRecordUDF	40
#define nslsdetRecordTIME	41
#define nslsdetRecordFLNK	42
#define nslsdetRecordVERS	43
#define nslsdetRecordVAL	44
#define nslsdetRecordBPTR	45
#define nslsdetRecordNVAL	46
#define nslsdetRecordNELM	47
#define nslsdetRecordNORD	48
#define nslsdetRecordFREQ	49
#define nslsdetRecordCNT	50
#define nslsdetRecordPCNT	51
#define nslsdetRecordSS	52
#define nslsdetRecordUS	53
#define nslsdetRecordCONT	54
#define nslsdetRecordINP	55
#define nslsdetRecordCARD	56
#define nslsdetRecordDLY	57
#define nslsdetRecordDLY1	58
#define nslsdetRecordNCH	59
#define nslsdetRecordG1	60
#define nslsdetRecordTP	61
#define nslsdetRecordTP1	62
#define nslsdetRecordPR1	63
#define nslsdetRecordT	64
#define nslsdetRecordSHPT	65
#define nslsdetRecordGAIN	66
#define nslsdetRecordAOEN	67
#define nslsdetRecordLOEN	68
#define nslsdetRecordEBLK	69
#define nslsdetRecordCHEN	70
#define nslsdetRecordPCHEN	71
#define nslsdetRecordTSEN	72
#define nslsdetRecordPTSEN	73
#define nslsdetRecordTR1	74
#define nslsdetRecordPTR1	75
#define nslsdetRecordTR2	76
#define nslsdetRecordPTR2	77
#define nslsdetRecordTR3	78
#define nslsdetRecordPTR3	79
#define nslsdetRecordTR4	80
#define nslsdetRecordPTR4	81
#define nslsdetRecordS1	82
#define nslsdetRecordPS1	83
#define nslsdetRecordS2	84
#define nslsdetRecordPS2	85
#define nslsdetRecordS3	86
#define nslsdetRecordPS3	87
#define nslsdetRecordNM1	88
#define nslsdetRecordNM2	89
#define nslsdetRecordNM3	90
#define nslsdetRecordEGU	91
#define nslsdetRecordPREC	92
#define nslsdetRecordCOUT	93
#endif /*INCnslsdetH*/
#ifdef GEN_SIZE_OFFSET
#ifdef __cplusplus
extern "C" {
#endif
#include <epicsExport.h>
static int nslsdetRecordSizeOffset(dbRecordType *pdbRecordType)
{
    nslsdetRecord *prec = 0;
  pdbRecordType->papFldDes[0]->size=sizeof(prec->name);
  pdbRecordType->papFldDes[0]->offset=(short)((char *)&prec->name - (char *)prec);
  pdbRecordType->papFldDes[1]->size=sizeof(prec->desc);
  pdbRecordType->papFldDes[1]->offset=(short)((char *)&prec->desc - (char *)prec);
  pdbRecordType->papFldDes[2]->size=sizeof(prec->asg);
  pdbRecordType->papFldDes[2]->offset=(short)((char *)&prec->asg - (char *)prec);
  pdbRecordType->papFldDes[3]->size=sizeof(prec->scan);
  pdbRecordType->papFldDes[3]->offset=(short)((char *)&prec->scan - (char *)prec);
  pdbRecordType->papFldDes[4]->size=sizeof(prec->pini);
  pdbRecordType->papFldDes[4]->offset=(short)((char *)&prec->pini - (char *)prec);
  pdbRecordType->papFldDes[5]->size=sizeof(prec->phas);
  pdbRecordType->papFldDes[5]->offset=(short)((char *)&prec->phas - (char *)prec);
  pdbRecordType->papFldDes[6]->size=sizeof(prec->evnt);
  pdbRecordType->papFldDes[6]->offset=(short)((char *)&prec->evnt - (char *)prec);
  pdbRecordType->papFldDes[7]->size=sizeof(prec->tse);
  pdbRecordType->papFldDes[7]->offset=(short)((char *)&prec->tse - (char *)prec);
  pdbRecordType->papFldDes[8]->size=sizeof(prec->tsel);
  pdbRecordType->papFldDes[8]->offset=(short)((char *)&prec->tsel - (char *)prec);
  pdbRecordType->papFldDes[9]->size=sizeof(prec->dtyp);
  pdbRecordType->papFldDes[9]->offset=(short)((char *)&prec->dtyp - (char *)prec);
  pdbRecordType->papFldDes[10]->size=sizeof(prec->disv);
  pdbRecordType->papFldDes[10]->offset=(short)((char *)&prec->disv - (char *)prec);
  pdbRecordType->papFldDes[11]->size=sizeof(prec->disa);
  pdbRecordType->papFldDes[11]->offset=(short)((char *)&prec->disa - (char *)prec);
  pdbRecordType->papFldDes[12]->size=sizeof(prec->sdis);
  pdbRecordType->papFldDes[12]->offset=(short)((char *)&prec->sdis - (char *)prec);
  pdbRecordType->papFldDes[13]->size=sizeof(prec->mlok);
  pdbRecordType->papFldDes[13]->offset=(short)((char *)&prec->mlok - (char *)prec);
  pdbRecordType->papFldDes[14]->size=sizeof(prec->mlis);
  pdbRecordType->papFldDes[14]->offset=(short)((char *)&prec->mlis - (char *)prec);
  pdbRecordType->papFldDes[15]->size=sizeof(prec->disp);
  pdbRecordType->papFldDes[15]->offset=(short)((char *)&prec->disp - (char *)prec);
  pdbRecordType->papFldDes[16]->size=sizeof(prec->proc);
  pdbRecordType->papFldDes[16]->offset=(short)((char *)&prec->proc - (char *)prec);
  pdbRecordType->papFldDes[17]->size=sizeof(prec->stat);
  pdbRecordType->papFldDes[17]->offset=(short)((char *)&prec->stat - (char *)prec);
  pdbRecordType->papFldDes[18]->size=sizeof(prec->sevr);
  pdbRecordType->papFldDes[18]->offset=(short)((char *)&prec->sevr - (char *)prec);
  pdbRecordType->papFldDes[19]->size=sizeof(prec->nsta);
  pdbRecordType->papFldDes[19]->offset=(short)((char *)&prec->nsta - (char *)prec);
  pdbRecordType->papFldDes[20]->size=sizeof(prec->nsev);
  pdbRecordType->papFldDes[20]->offset=(short)((char *)&prec->nsev - (char *)prec);
  pdbRecordType->papFldDes[21]->size=sizeof(prec->acks);
  pdbRecordType->papFldDes[21]->offset=(short)((char *)&prec->acks - (char *)prec);
  pdbRecordType->papFldDes[22]->size=sizeof(prec->ackt);
  pdbRecordType->papFldDes[22]->offset=(short)((char *)&prec->ackt - (char *)prec);
  pdbRecordType->papFldDes[23]->size=sizeof(prec->diss);
  pdbRecordType->papFldDes[23]->offset=(short)((char *)&prec->diss - (char *)prec);
  pdbRecordType->papFldDes[24]->size=sizeof(prec->lcnt);
  pdbRecordType->papFldDes[24]->offset=(short)((char *)&prec->lcnt - (char *)prec);
  pdbRecordType->papFldDes[25]->size=sizeof(prec->pact);
  pdbRecordType->papFldDes[25]->offset=(short)((char *)&prec->pact - (char *)prec);
  pdbRecordType->papFldDes[26]->size=sizeof(prec->putf);
  pdbRecordType->papFldDes[26]->offset=(short)((char *)&prec->putf - (char *)prec);
  pdbRecordType->papFldDes[27]->size=sizeof(prec->rpro);
  pdbRecordType->papFldDes[27]->offset=(short)((char *)&prec->rpro - (char *)prec);
  pdbRecordType->papFldDes[28]->size=sizeof(prec->asp);
  pdbRecordType->papFldDes[28]->offset=(short)((char *)&prec->asp - (char *)prec);
  pdbRecordType->papFldDes[29]->size=sizeof(prec->ppn);
  pdbRecordType->papFldDes[29]->offset=(short)((char *)&prec->ppn - (char *)prec);
  pdbRecordType->papFldDes[30]->size=sizeof(prec->ppnr);
  pdbRecordType->papFldDes[30]->offset=(short)((char *)&prec->ppnr - (char *)prec);
  pdbRecordType->papFldDes[31]->size=sizeof(prec->spvt);
  pdbRecordType->papFldDes[31]->offset=(short)((char *)&prec->spvt - (char *)prec);
  pdbRecordType->papFldDes[32]->size=sizeof(prec->rset);
  pdbRecordType->papFldDes[32]->offset=(short)((char *)&prec->rset - (char *)prec);
  pdbRecordType->papFldDes[33]->size=sizeof(prec->dset);
  pdbRecordType->papFldDes[33]->offset=(short)((char *)&prec->dset - (char *)prec);
  pdbRecordType->papFldDes[34]->size=sizeof(prec->dpvt);
  pdbRecordType->papFldDes[34]->offset=(short)((char *)&prec->dpvt - (char *)prec);
  pdbRecordType->papFldDes[35]->size=sizeof(prec->rdes);
  pdbRecordType->papFldDes[35]->offset=(short)((char *)&prec->rdes - (char *)prec);
  pdbRecordType->papFldDes[36]->size=sizeof(prec->lset);
  pdbRecordType->papFldDes[36]->offset=(short)((char *)&prec->lset - (char *)prec);
  pdbRecordType->papFldDes[37]->size=sizeof(prec->prio);
  pdbRecordType->papFldDes[37]->offset=(short)((char *)&prec->prio - (char *)prec);
  pdbRecordType->papFldDes[38]->size=sizeof(prec->tpro);
  pdbRecordType->papFldDes[38]->offset=(short)((char *)&prec->tpro - (char *)prec);
  pdbRecordType->papFldDes[39]->size=sizeof(prec->bkpt);
  pdbRecordType->papFldDes[39]->offset=(short)((char *)&prec->bkpt - (char *)prec);
  pdbRecordType->papFldDes[40]->size=sizeof(prec->udf);
  pdbRecordType->papFldDes[40]->offset=(short)((char *)&prec->udf - (char *)prec);
  pdbRecordType->papFldDes[41]->size=sizeof(prec->time);
  pdbRecordType->papFldDes[41]->offset=(short)((char *)&prec->time - (char *)prec);
  pdbRecordType->papFldDes[42]->size=sizeof(prec->flnk);
  pdbRecordType->papFldDes[42]->offset=(short)((char *)&prec->flnk - (char *)prec);
  pdbRecordType->papFldDes[43]->size=sizeof(prec->vers);
  pdbRecordType->papFldDes[43]->offset=(short)((char *)&prec->vers - (char *)prec);
  pdbRecordType->papFldDes[44]->size=sizeof(prec->val);
  pdbRecordType->papFldDes[44]->offset=(short)((char *)&prec->val - (char *)prec);
  pdbRecordType->papFldDes[45]->size=sizeof(prec->bptr);
  pdbRecordType->papFldDes[45]->offset=(short)((char *)&prec->bptr - (char *)prec);
  pdbRecordType->papFldDes[46]->size=sizeof(prec->nval);
  pdbRecordType->papFldDes[46]->offset=(short)((char *)&prec->nval - (char *)prec);
  pdbRecordType->papFldDes[47]->size=sizeof(prec->nelm);
  pdbRecordType->papFldDes[47]->offset=(short)((char *)&prec->nelm - (char *)prec);
  pdbRecordType->papFldDes[48]->size=sizeof(prec->nord);
  pdbRecordType->papFldDes[48]->offset=(short)((char *)&prec->nord - (char *)prec);
  pdbRecordType->papFldDes[49]->size=sizeof(prec->freq);
  pdbRecordType->papFldDes[49]->offset=(short)((char *)&prec->freq - (char *)prec);
  pdbRecordType->papFldDes[50]->size=sizeof(prec->cnt);
  pdbRecordType->papFldDes[50]->offset=(short)((char *)&prec->cnt - (char *)prec);
  pdbRecordType->papFldDes[51]->size=sizeof(prec->pcnt);
  pdbRecordType->papFldDes[51]->offset=(short)((char *)&prec->pcnt - (char *)prec);
  pdbRecordType->papFldDes[52]->size=sizeof(prec->ss);
  pdbRecordType->papFldDes[52]->offset=(short)((char *)&prec->ss - (char *)prec);
  pdbRecordType->papFldDes[53]->size=sizeof(prec->us);
  pdbRecordType->papFldDes[53]->offset=(short)((char *)&prec->us - (char *)prec);
  pdbRecordType->papFldDes[54]->size=sizeof(prec->cont);
  pdbRecordType->papFldDes[54]->offset=(short)((char *)&prec->cont - (char *)prec);
  pdbRecordType->papFldDes[55]->size=sizeof(prec->inp);
  pdbRecordType->papFldDes[55]->offset=(short)((char *)&prec->inp - (char *)prec);
  pdbRecordType->papFldDes[56]->size=sizeof(prec->card);
  pdbRecordType->papFldDes[56]->offset=(short)((char *)&prec->card - (char *)prec);
  pdbRecordType->papFldDes[57]->size=sizeof(prec->dly);
  pdbRecordType->papFldDes[57]->offset=(short)((char *)&prec->dly - (char *)prec);
  pdbRecordType->papFldDes[58]->size=sizeof(prec->dly1);
  pdbRecordType->papFldDes[58]->offset=(short)((char *)&prec->dly1 - (char *)prec);
  pdbRecordType->papFldDes[59]->size=sizeof(prec->nch);
  pdbRecordType->papFldDes[59]->offset=(short)((char *)&prec->nch - (char *)prec);
  pdbRecordType->papFldDes[60]->size=sizeof(prec->g1);
  pdbRecordType->papFldDes[60]->offset=(short)((char *)&prec->g1 - (char *)prec);
  pdbRecordType->papFldDes[61]->size=sizeof(prec->tp);
  pdbRecordType->papFldDes[61]->offset=(short)((char *)&prec->tp - (char *)prec);
  pdbRecordType->papFldDes[62]->size=sizeof(prec->tp1);
  pdbRecordType->papFldDes[62]->offset=(short)((char *)&prec->tp1 - (char *)prec);
  pdbRecordType->papFldDes[63]->size=sizeof(prec->pr1);
  pdbRecordType->papFldDes[63]->offset=(short)((char *)&prec->pr1 - (char *)prec);
  pdbRecordType->papFldDes[64]->size=sizeof(prec->t);
  pdbRecordType->papFldDes[64]->offset=(short)((char *)&prec->t - (char *)prec);
  pdbRecordType->papFldDes[65]->size=sizeof(prec->shpt);
  pdbRecordType->papFldDes[65]->offset=(short)((char *)&prec->shpt - (char *)prec);
  pdbRecordType->papFldDes[66]->size=sizeof(prec->gain);
  pdbRecordType->papFldDes[66]->offset=(short)((char *)&prec->gain - (char *)prec);
  pdbRecordType->papFldDes[67]->size=sizeof(prec->aoen);
  pdbRecordType->papFldDes[67]->offset=(short)((char *)&prec->aoen - (char *)prec);
  pdbRecordType->papFldDes[68]->size=sizeof(prec->loen);
  pdbRecordType->papFldDes[68]->offset=(short)((char *)&prec->loen - (char *)prec);
  pdbRecordType->papFldDes[69]->size=sizeof(prec->eblk);
  pdbRecordType->papFldDes[69]->offset=(short)((char *)&prec->eblk - (char *)prec);
  pdbRecordType->papFldDes[70]->size=sizeof(prec->chen);
  pdbRecordType->papFldDes[70]->offset=(short)((char *)&prec->chen - (char *)prec);
  pdbRecordType->papFldDes[71]->size=sizeof(prec->pchen);
  pdbRecordType->papFldDes[71]->offset=(short)((char *)&prec->pchen - (char *)prec);
  pdbRecordType->papFldDes[72]->size=sizeof(prec->tsen);
  pdbRecordType->papFldDes[72]->offset=(short)((char *)&prec->tsen - (char *)prec);
  pdbRecordType->papFldDes[73]->size=sizeof(prec->ptsen);
  pdbRecordType->papFldDes[73]->offset=(short)((char *)&prec->ptsen - (char *)prec);
  pdbRecordType->papFldDes[74]->size=sizeof(prec->tr1);
  pdbRecordType->papFldDes[74]->offset=(short)((char *)&prec->tr1 - (char *)prec);
  pdbRecordType->papFldDes[75]->size=sizeof(prec->ptr1);
  pdbRecordType->papFldDes[75]->offset=(short)((char *)&prec->ptr1 - (char *)prec);
  pdbRecordType->papFldDes[76]->size=sizeof(prec->tr2);
  pdbRecordType->papFldDes[76]->offset=(short)((char *)&prec->tr2 - (char *)prec);
  pdbRecordType->papFldDes[77]->size=sizeof(prec->ptr2);
  pdbRecordType->papFldDes[77]->offset=(short)((char *)&prec->ptr2 - (char *)prec);
  pdbRecordType->papFldDes[78]->size=sizeof(prec->tr3);
  pdbRecordType->papFldDes[78]->offset=(short)((char *)&prec->tr3 - (char *)prec);
  pdbRecordType->papFldDes[79]->size=sizeof(prec->ptr3);
  pdbRecordType->papFldDes[79]->offset=(short)((char *)&prec->ptr3 - (char *)prec);
  pdbRecordType->papFldDes[80]->size=sizeof(prec->tr4);
  pdbRecordType->papFldDes[80]->offset=(short)((char *)&prec->tr4 - (char *)prec);
  pdbRecordType->papFldDes[81]->size=sizeof(prec->ptr4);
  pdbRecordType->papFldDes[81]->offset=(short)((char *)&prec->ptr4 - (char *)prec);
  pdbRecordType->papFldDes[82]->size=sizeof(prec->s1);
  pdbRecordType->papFldDes[82]->offset=(short)((char *)&prec->s1 - (char *)prec);
  pdbRecordType->papFldDes[83]->size=sizeof(prec->ps1);
  pdbRecordType->papFldDes[83]->offset=(short)((char *)&prec->ps1 - (char *)prec);
  pdbRecordType->papFldDes[84]->size=sizeof(prec->s2);
  pdbRecordType->papFldDes[84]->offset=(short)((char *)&prec->s2 - (char *)prec);
  pdbRecordType->papFldDes[85]->size=sizeof(prec->ps2);
  pdbRecordType->papFldDes[85]->offset=(short)((char *)&prec->ps2 - (char *)prec);
  pdbRecordType->papFldDes[86]->size=sizeof(prec->s3);
  pdbRecordType->papFldDes[86]->offset=(short)((char *)&prec->s3 - (char *)prec);
  pdbRecordType->papFldDes[87]->size=sizeof(prec->ps3);
  pdbRecordType->papFldDes[87]->offset=(short)((char *)&prec->ps3 - (char *)prec);
  pdbRecordType->papFldDes[88]->size=sizeof(prec->nm1);
  pdbRecordType->papFldDes[88]->offset=(short)((char *)&prec->nm1 - (char *)prec);
  pdbRecordType->papFldDes[89]->size=sizeof(prec->nm2);
  pdbRecordType->papFldDes[89]->offset=(short)((char *)&prec->nm2 - (char *)prec);
  pdbRecordType->papFldDes[90]->size=sizeof(prec->nm3);
  pdbRecordType->papFldDes[90]->offset=(short)((char *)&prec->nm3 - (char *)prec);
  pdbRecordType->papFldDes[91]->size=sizeof(prec->egu);
  pdbRecordType->papFldDes[91]->offset=(short)((char *)&prec->egu - (char *)prec);
  pdbRecordType->papFldDes[92]->size=sizeof(prec->prec);
  pdbRecordType->papFldDes[92]->offset=(short)((char *)&prec->prec - (char *)prec);
  pdbRecordType->papFldDes[93]->size=sizeof(prec->cout);
  pdbRecordType->papFldDes[93]->offset=(short)((char *)&prec->cout - (char *)prec);
    pdbRecordType->rec_size = sizeof(*prec);
    return(0);
}
epicsExportRegistrar(nslsdetRecordSizeOffset);
#ifdef __cplusplus
}
#endif
#endif /*GEN_SIZE_OFFSET*/
