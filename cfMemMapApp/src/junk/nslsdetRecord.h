#include "ellLib.h"
#include "epicsMutex.h"
#include "link.h"
#include "epicsTime.h"
#include "epicsTypes.h"
#define MAX_CHANNELS 384

#ifndef INCnslsdetG1H
#define INCnslsdetG1H
typedef enum {
	nslsdetG1_N,
	nslsdetG1_Y
}nslsdetG1;
#endif /*INCnslsdetG1H*/

#ifndef INCnslsdetD1H
#define INCnslsdetD1H
typedef enum {
	nslsdetD1_Up,
	nslsdetD1_Dn
}nslsdetD1;
#endif /*INCnslsdetD1H*/

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
	double		val[3];	/*Result (sum of scalers)*/
	double		freq;	/*Time base freq*/
	epicsEnum16	cnt;	/*Count*/
	epicsEnum16	pcnt;	/*Prev Count*/
	short		ss;	/*Scaler state*/
	short		us;	/*User state*/
	epicsEnum16	cont;	/*OneShot/AutoCount*/
	DBLINK		out;	/*Output Specification*/
	short		card;	/*Card Number*/
	float		rate;	/*Display Rate (Hz.)*/
	float		rat1;	/*Auto Display Rate (Hz.)*/
	float		dly;	/*Delay*/
	float		dly1;	/*Auto-mode Delay*/
	short		nch;	/*Number of Channels*/
	epicsEnum16	g1;	/*Gate Control 1*/
	double		tp;	/*Time Preset*/
	double		tp1;	/*Time Preset*/
	epicsInt32	pr1;	/*Preset 1*/
	double		t;	/*Timer*/
	int		shpt;	/* shaping time */
	int		gain;	/* gain */
	int		aoen;	/* analog out enable ch# */
	int		loen;	/* leakage mon enable ch# */
	char		chen[MAX_CHANNELS]; /* channel disable */
	char		tsen[MAX_CHANNELS]; /* channel test pulse enable */
	char		tr1[MAX_CHANNELS]; /* trim dacs lo1 */
	char		tr2[MAX_CHANNELS]; /* trim dacs hi1 */
	char		tr3[MAX_CHANNELS]; /* trim dacs lo2 */
	char		tr4[MAX_CHANNELS]; /* trim dacs hi2 */
	epicsInt32	s1[MAX_CHANNELS];	/*Scaler 1*/
	epicsInt32	s2[MAX_CHANNELS];	/*Scaler 2*/
	epicsInt32	s3[MAX_CHANNELS];	/*Scaler 3*/
	char		nm1[16]; /*Scaler 1 name*/
	char		nm2[16]; /*Scaler 2 name*/
	char		nm3[16]; /*Scaler 3 name*/
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
#define nslsdetRecordFREQ	45
#define nslsdetRecordCNT	46
#define nslsdetRecordPCNT	47
#define nslsdetRecordSS	48
#define nslsdetRecordUS	49
#define nslsdetRecordCONT	50
#define nslsdetRecordOUT	51
#define nslsdetRecordCARD	52
#define nslsdetRecordDLY	55
#define nslsdetRecordDLY1	56
#define nslsdetRecordNCH	57
#define nslsdetRecordG1	58
#define nslsdetRecordTP	59
#define nslsdetRecordTP1	60
#define nslsdetRecordPR1	61
#define nslsdetRecordT	62
#define nslsdetRecordSHPT 63
#define nslsdetRecordGAIN 64
#define nslsdetRecordAOEN 65
#define nslsdetRecordLOEN 66
#define nslsdetRecordCHEN 67
#define nslsdetRecordTSEN 68
#define nslsdetRecordTR1	69
#define nslsdetRecordTR2	70
#define nslsdetRecordTR3	71
#define nslsdetRecordTR4	72
#define nslsdetRecordS1	73
#define nslsdetRecordS2	74
#define nslsdetRecordS3	75
#define nslsdetRecordNM1	76
#define nslsdetRecordNM2	77
#define nslsdetRecordNM3	78
#define nslsdetRecordEGU	79
#define nslsdetRecordPREC	80
#define nslsdetRecordCOUT	81
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
  pdbRecordType->papFldDes[45]->size=sizeof(prec->freq);
  pdbRecordType->papFldDes[45]->offset=(short)((char *)&prec->freq - (char *)prec);
  pdbRecordType->papFldDes[46]->size=sizeof(prec->cnt);
  pdbRecordType->papFldDes[46]->offset=(short)((char *)&prec->cnt - (char *)prec);
  pdbRecordType->papFldDes[47]->size=sizeof(prec->pcnt);
  pdbRecordType->papFldDes[47]->offset=(short)((char *)&prec->pcnt - (char *)prec);
  pdbRecordType->papFldDes[48]->size=sizeof(prec->ss);
  pdbRecordType->papFldDes[48]->offset=(short)((char *)&prec->ss - (char *)prec);
  pdbRecordType->papFldDes[49]->size=sizeof(prec->us);
  pdbRecordType->papFldDes[49]->offset=(short)((char *)&prec->us - (char *)prec);
  pdbRecordType->papFldDes[50]->size=sizeof(prec->cont);
  pdbRecordType->papFldDes[50]->offset=(short)((char *)&prec->cont - (char *)prec);
  pdbRecordType->papFldDes[51]->size=sizeof(prec->out);
  pdbRecordType->papFldDes[51]->offset=(short)((char *)&prec->out - (char *)prec);
  pdbRecordType->papFldDes[52]->size=sizeof(prec->card);
  pdbRecordType->papFldDes[52]->offset=(short)((char *)&prec->card - (char *)prec);
  pdbRecordType->papFldDes[53]->size=sizeof(prec->rate);
  pdbRecordType->papFldDes[53]->offset=(short)((char *)&prec->rate - (char *)prec);
  pdbRecordType->papFldDes[54]->size=sizeof(prec->rat1);
  pdbRecordType->papFldDes[54]->offset=(short)((char *)&prec->rat1 - (char *)prec);
  pdbRecordType->papFldDes[55]->size=sizeof(prec->dly);
  pdbRecordType->papFldDes[55]->offset=(short)((char *)&prec->dly - (char *)prec);
  pdbRecordType->papFldDes[56]->size=sizeof(prec->dly1);
  pdbRecordType->papFldDes[56]->offset=(short)((char *)&prec->dly1 - (char *)prec);
  pdbRecordType->papFldDes[57]->size=sizeof(prec->nch);
  pdbRecordType->papFldDes[57]->offset=(short)((char *)&prec->nch - (char *)prec);
  pdbRecordType->papFldDes[58]->size=sizeof(prec->g1);
  pdbRecordType->papFldDes[58]->offset=(short)((char *)&prec->g1 - (char *)prec);
  pdbRecordType->papFldDes[59]->size=sizeof(prec->tp);
  pdbRecordType->papFldDes[59]->offset=(short)((char *)&prec->tp - (char *)prec);
  pdbRecordType->papFldDes[60]->size=sizeof(prec->tp1);
  pdbRecordType->papFldDes[60]->offset=(short)((char *)&prec->tp1 - (char *)prec);
  pdbRecordType->papFldDes[61]->size=sizeof(prec->pr1);
  pdbRecordType->papFldDes[61]->offset=(short)((char *)&prec->pr1 - (char *)prec);
  pdbRecordType->papFldDes[62]->size=sizeof(prec->t);
  pdbRecordType->papFldDes[62]->offset=(short)((char *)&prec->t - (char *)prec);
  
  pdbRecordType->papFldDes[63]->size=sizeof(prec->shpt);
  pdbRecordType->papFldDes[63]->offset=(short)((char *)&prec->shpt - (char *)prec);
  pdbRecordType->papFldDes[64]->size=sizeof(prec->gain);
  pdbRecordType->papFldDes[64]->offset=(short)((char *)&prec->gain - (char *)prec);
  pdbRecordType->papFldDes[65]->size=sizeof(prec->aoen);
  pdbRecordType->papFldDes[65]->offset=(short)((char *)&prec->aoen - (char *)prec);
  pdbRecordType->papFldDes[66]->size=sizeof(prec->loen);
  pdbRecordType->papFldDes[66]->offset=(short)((char *)&prec->loen - (char *)prec);
  pdbRecordType->papFldDes[67]->size=sizeof(prec->chen);
  pdbRecordType->papFldDes[67]->offset=(short)((char *)&prec->chen - (char *)prec);
  pdbRecordType->papFldDes[68]->size=sizeof(prec->tsen);
  pdbRecordType->papFldDes[68]->offset=(short)((char *)&prec->tsen - (char *)prec);


  pdbRecordType->papFldDes[69]->size=sizeof(prec->tr1);
  pdbRecordType->papFldDes[69]->offset=(short)((char *)&prec->tr1 - (char *)prec);
  pdbRecordType->papFldDes[70]->size=sizeof(prec->tr2);
  pdbRecordType->papFldDes[70]->offset=(short)((char *)&prec->tr2 - (char *)prec);
  pdbRecordType->papFldDes[71]->size=sizeof(prec->tr3);
  pdbRecordType->papFldDes[71]->offset=(short)((char *)&prec->tr3 - (char *)prec);
  pdbRecordType->papFldDes[72]->size=sizeof(prec->tr4);
  pdbRecordType->papFldDes[72]->offset=(short)((char *)&prec->tr4 - (char *)prec);
  
  pdbRecordType->papFldDes[73]->size=sizeof(prec->s1);
  pdbRecordType->papFldDes[73]->offset=(short)((char *)&prec->s1 - (char *)prec);
  pdbRecordType->papFldDes[74]->size=sizeof(prec->s2);
  pdbRecordType->papFldDes[74]->offset=(short)((char *)&prec->s2 - (char *)prec);
  pdbRecordType->papFldDes[75]->size=sizeof(prec->s3);
  pdbRecordType->papFldDes[75]->offset=(short)((char *)&prec->s3 - (char *)prec);
  pdbRecordType->papFldDes[76]->size=sizeof(prec->nm1);
  pdbRecordType->papFldDes[76]->offset=(short)((char *)&prec->nm1 - (char *)prec);
  pdbRecordType->papFldDes[77]->size=sizeof(prec->nm2);
  pdbRecordType->papFldDes[77]->offset=(short)((char *)&prec->nm2 - (char *)prec);
  pdbRecordType->papFldDes[78]->size=sizeof(prec->nm3);
  pdbRecordType->papFldDes[78]->offset=(short)((char *)&prec->nm3 - (char *)prec);
  pdbRecordType->papFldDes[79]->size=sizeof(prec->egu);
  pdbRecordType->papFldDes[79]->offset=(short)((char *)&prec->egu - (char *)prec);
  pdbRecordType->papFldDes[80]->size=sizeof(prec->prec);
  pdbRecordType->papFldDes[80]->offset=(short)((char *)&prec->prec - (char *)prec);
  pdbRecordType->papFldDes[81]->size=sizeof(prec->cout);
  pdbRecordType->papFldDes[81]->offset=(short)((char *)&prec->cout - (char *)prec);
    pdbRecordType->rec_size = sizeof(*prec);
    return(0);
}
epicsExportRegistrar(nslsdetRecordSizeOffset);
#ifdef __cplusplus
}
#endif
#endif /*GEN_SIZE_OFFSET*/
