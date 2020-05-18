/* devScaler.h */
/* OSI by S. Kate Feng 3/03 */
/* nslsdet mods D. P. Siddons 02/05/04 */

/* ----------------Device Support Entry Table for devSCALER----------------- */


#define nslsdet_DELTA 2

#define CT_START	0
#define CT_COUNTING	1
#define CT_MORE		2
#define CT_ALLDONE	3

#define nslsdet_RTEMS 1
#define nslsdet_RAW 0
#define nslsdet_INTENABLE 0x0010
#define nslsdet_INTDISABLE 0xff7f

#define NCHIPS 20
#define EEPROM 2
#define CHIPS 3

#define MAX_CHANNELS NCHIPS * 32

typedef struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	reset;
	DEVSUPFUN	read;
	DEVSUPFUN	write_preset;
	DEVSUPFUN	arm;
	DEVSUPFUN	done;
} nslsdetDSET;

/*** nslsdet_state ***/

typedef struct {
	int card_exists;
	int num_channels;
	int card_in_use;
	int count_in_progress; /* count in progress? */
	unsigned short ident; /* identification info for this card */
	IOSCANPVT ioscanpvt;
	int done; 	/* sequence counter for ISR: 
			0=first entry, set up one-shot for time
			1=timer one-shot done
			2=more counting needed
			3=count sequence finished   */
	char gate;
	int tp; /* time in milliseconds */
	int mode; /* timer setup word */
	int clkreg; /* compare register contents */
	int delta; /* timer startup delay */
	int newdata; /* flag to tell hardware new data is available */
	CALLBACK *pcallback;
} Nslsdet_state;


/*** callback stuff ***/
struct callback {
	CALLBACK	callback;
	struct dbCommon *precord;
        epicsTimerId wd_id;
};

