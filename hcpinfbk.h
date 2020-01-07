/* Copyright IBM Corp. 2013, 2014 */

#pragma pack(packed)
#pragma anonstruct(on)
struct inf0par {
  unsigned char  /*  XL1 */ infpflg1;      /* Partition Flag Byte 1 reserved for */
                                           /* IBM use. */
  unsigned char  /*  XL1 */ infpflg2;      /* Partition Flag Byte 2 reserved for */
                                           /* IBM use. */
  unsigned char  /*  XL1 */ infpval1;      /* Partition Validity Byte 1 */
  #define INFPPROC 128                     /* Processor Count Validity */
                                           /* This bit being on indicates that */
                                           /* INFPSCPS, INFPDCPS, INFPSIFL, */
                                           /* and INFPDIFL contain valid counts. */
  #define INFPWBCC 64                      /* Partition weight-based capped */
                                           /* capacity validity. */
                                           /* This bit being on indicates that */
                                           /* INFPWBCP and INFPWBIF are valid. */
  #define INFPACC 32                       /* Partition absolute capped capacity */
                                           /* validity. */
                                           /* This bit being on indicates that */
                                           /* INFPABCP and INFPABIF are valid. */
  #define INFPPID 16                       /* Partition ID Validity. */
                                           /* This bit being on indicates that */
                                           /* a SYSIB 2.2.2 was obtained from */
                                           /* STSI and information reported in */
                                           /* the following fields is valid: */
                                           /* INFPPNUM, INFPPNAM */
  unsigned char  /*  XL1 */ infpval2;      /* Partition Validity Byte 2 reserved */
                                           /* for IBM use. */
  short int      /*  HL2 */ infppnum;      /* Logical Partition Number. This is */
                                           /* the Logical-Partition Number */
                                           /* reported by STSI 2.2.2. */
  short int      /*  HL2 */ infpscps;      /* Count of shared logical CPs */
                                           /* configured for this partition. */
  short int      /*  HL2 */ infpdcps;      /* Count of dedicated logical CPs */
                                           /* configured for this partition. */
  short int      /*  HL2 */ infpsifl;      /* Count of shared logical IFLs */
                                           /* configured for this partition. */
  short int      /*  HL2 */ infpdifl;      /* Count of dedicated logical IFLs */
                                           /* configured for this partition. */
  char           __filler0[2];
  unsigned char  /*  CL8 */ infppnam[8];   /* Logical Partition Name, in EBCDIC */
                                           /* format. This is the Logical- */
                                           /* Partition Name reported by STSI */
                                           /* 2.2.2. */
  int            /*  FL4 */ infpwbcp;      /* Partition weight-based capped */
                                           /* capacity for CPs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
                                           /* Cap is applicable only to shared */
  /* processors.                       @U7206P4 */
  int            /*  FL4 */ infpabcp;      /* Partition absolute capped */
                                           /* capacity for CPs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
                                           /* Cap is applicable only to shared */
  /* processors.                       @U7206P4 */
  int            /*  FL4 */ infpwbif;      /* Partition weight-based capped */
                                           /* capacity for IFLs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
                                           /* Cap is applicable only to shared */
  /* processors.                       @U7206P4 */
  int            /*  FL4 */ infpabif;      /* Partition absolute capped */
                                           /* capacity for IFLs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
                                           /* Cap is applicable only to shared */
  /* processors.                       @U7206P4 */
  #define INF0PSIZ 40                      /* Size of Partition Section in bytes */
  #define INF0PSZD 5                       /* Size of Partition Section in DWs */
};
struct inf0mac {
  unsigned char  /*  XL1 */ infmflg1;      /* Machine Flag Byte 1 reserved */
                                           /* for IBM use */
  unsigned char  /*  XL1 */ infmflg2;      /* Machine Flag Byte 2 reserved */
                                           /* for IBM use */
  unsigned char  /*  XL1 */ infmval1;      /* Machine Validity Byte 1 */
  #define INFMPROC 128                     /* Processor Count Validity */
                                           /* When this bit is on, it indicates that */
                                           /* INFMSCPS, INFMDCPS, INFMSIFL, */
                                           /* and INFMDIFL contain valid counts. */
                                           /* The validity bit may be off when: */
                                           /* - STHYI support is not available on */
                                           /* a lower level hypervisor, or */
                                           /* - Global Performance Data is not */
                                           /* enabled. */
  #define INFMMID 64                       /* Machine ID Validity */
                                           /* This bit being on indicates that */
                                           /* a SYSIB 1.1.1 was obtained from */
                                           /* STSI and information reported */
                                           /* in the following fields is valid: */
                                           /* INFMTYPE, INFMMANU, INFMSEQ, */
                                           /* and INFMPMAN. */
  #define INFMMNAM 32                      /* Machine Name Validity */
                                           /* This bit being on indicates that */
                                           /* the INFMNAME field is valid. */
  unsigned char  /*  XL1 */ infmval2;      /* Machine Validity Byte 2 */
                                           /* Reserved for IBM use */
  short int      /*  HL2 */ infmscps;      /* Count of shared CPs configured */
                                           /* in the machine or in the physical */
                                           /* partition if the system is */
                                           /* physically partitioned. */
  short int      /*  HL2 */ infmdcps;      /* Count of dedicated CPs configured */
                                           /* in the machine or in the physical */
                                           /* partition if the system is */
                                           /* physically partitioned. */
  short int      /*  HL2 */ infmsifl;      /* Count of shared IFLs configured */
                                           /* in the machine or in the physical */
                                           /* partition if the system is */
                                           /* physically partitioned. */
  short int      /*  HL2 */ infmdifl;      /* Count of dedicated IFLs configured */
                                           /* in the machine or in the physical */
                                           /* partition if the system is */
                                           /* physically partitioned. */
  unsigned char  /*  CL8 */ infmname[8];   /* Machine Name, in EBCDIC format. */
                                           /* This is the CPC name associated */
                                           /* with the processor. */
  unsigned char  /*  CL4 */ infmtype[4];   /* Machine Type, in EBCDIC format. */
                                           /* This is the machine type */
                                           /* reported by STSI 1.1.1 (Basic */
                                           /* Machine Configuration). */
  unsigned char  /*  CL16 */ infmmanu[16]; /* Machine Manufacturer, in EBCDIC */
                                           /* format. This is the name of the */
                                           /* manufacturer of the configuration */
                                           /* reported by STSI 1.1.1.. */
  unsigned char  /*  CL16 */ infmseq[16];  /* Sequence Code, in EBCDIC format. */
                                           /* This is the sequence code of */
                                           /* the configuration reported by */
                                           /* STSI 1.1.1. */
  unsigned char  /*  CL4 */ infmpman[4];   /* Plant of Manufacture, in EBCDIC */
                                           /* format. This is the 4-byte code */
                                           /* reported by STSI 1.1.1. */
  #define INF0MSIZ 64                      /* Size of Machine Section in bytes */
  #define INF0MSZD 8                       /* Size of Machine Section in DWs */
  char           __filler0[4];
};
struct inf0hyp {
  unsigned char  /*  XL1 */ infyflg1;      /* Hypervisor Flag Byte 1            @U7105P4 */
  #define INFYLMCN 128                     /* Guest CPU usage hard limiting is  @U7105P4 */
  /* using the consumption method.     @U7105P4 */
  unsigned char  /*  XL1 */ infyflg2;      /* Hypervisor Flag Byte 2 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infyval1;      /* Hypervisor Validity Byte 1 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infyval2;      /* Hypervisor Validity Byte 2 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infytype;      /* Hypervisor type */
  #define INFYTVM 1                        /* z/VM is the hypervisor */
  char           __filler0[3];
  unsigned char  /*  CL8 */ infysyid[8];   /* System Identifier, in EBCDIC */
                                           /* format, left justified and padded */
                                           /* with blanks. This is the value */
                                           /* generally specified on the */
                                           /* SYSTEM_IDentifier statement in */
                                           /* the system configuration file. */
                                           /* Blank if non-existent. */
  unsigned char  /*  CL8 */ infyclnm[8];   /* Cluster Name, in EBCDIC format, */
                                           /* left justified and padded with */
                                           /* blanks. This is the name on the */
                                           /* SSI statement in the system */
                                           /* configuration file. Blank if */
                                           /* non-existent. */
  short int      /*  HL2 */ infyscps;      /* Number of CPs shared among guests */
                                           /* of this hypervisor. */
  short int      /*  HL2 */ infydcps;      /* Number of CPs dedicated to guests */
                                           /* of this hypervisor. */
  short int      /*  HL2 */ infysifl;      /* Number of IFLs shared among guests */
                                           /* of this hypervisor. */
  short int      /*  HL2 */ infydifl;      /* Number of IFLs dedicated to guests */
                                           /* of this hypervisor. */
  #define INF0YSIZ 32                      /* Size of Hypervisor Section in bytes */
  #define INF0YSZD 4                       /* Size of Hypervisor Section in DWs */
};
struct inf0hdyg {
  short int      /*  HL2 */ infyoff;       /* Offset to Hypervisor Section */
                                           /* mapped by INF0HYP */
  short int      /*  HL2 */ infylen;       /* Length of Hypervisor Section in */
                                           /* bytes */
  short int      /*  HL2 */ infgoff;       /* Offset to Guest Section mapped */
                                           /* by INF0GST */
  short int      /*  HL2 */ infglen;       /* Length of Guest Section in bytes */
  #define INF0HYSZ 8                       /* Size of section description in bytes */
  #define INF0HYSD 1                       /* Size of section description in DWs */
};
struct inf0hdr {
  unsigned char  /*  XL1 */ infhflg1;      /* Header Flag Byte 1 */
                                           /* These flag settings indicate */
                                           /* the environment that the instruction */
                                           /* was executed in and may influence */
                                           /* the value of validity bits. */
                                           /* The validity bits, not these flags, */
                                           /* should be used to determine if a */
                                           /* field is valid. */
  #define INFGPDU 128                      /* Global Performance Data unavailable */
  #define INFSTHYI 64                      /* One or more hypervisor levels */
                                           /* below this level does */
                                           /* not support the STHYI instruction. */
                                           /* When this flag is set the value */
                                           /* of INFGPDU is not meaningful */
                                           /* because the state of the Global */
                                           /* Performance Data setting cannot */
  /* be determined.                    @U7221DN */
  #define INFVSI 32                        /* Virtualization stack is incomplete. */
                                           /* This bit indicates one of 2 cases: */
                                           /* 1. One or more hypervisor levels */
                                           /* does not support STHYI. For this case */
                                           /* INFSTHYI will also be set. */
                                           /* 2. There were more than 3 levels */
                                           /* of guest/hypervisor information */
                                           /* to report. */
  #define INFBASIC 16                      /* Execution environment is not */
                                           /* within a logical partition. */
  unsigned char  /*  XL1 */ infhflg2;      /* Header Flag Byte 2 reserved for IBM use */
  unsigned char  /*  XL1 */ infhval1;      /* Header Validity Byte 1 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infhval2;      /* Header Validity Byte 2 */
                                           /* Reserved for IBM use */
  char           __filler0[3];
  unsigned char  /*  XL1 */ infhygct;      /* Count of reported hypervisors/ */
                                           /* guests.  This indicates how many */
                                           /* hypervisor/guest sections are in */
                                           /* the response buffer up to a */
                                           /* maximum of 3. */
  short int      /*  HL2 */ infhtotl;      /* Total length of the response */
                                           /* buffer in bytes, including the */
                                           /* Header, Machine, Partition, and */
                                           /* Hypervisor and Guest sections. */
  short int      /*  HL2 */ infhdln;       /* Length of Header Section in bytes */
  short int      /*  HL2 */ infmoff;       /* Offset to Machine Section mapped */
                                           /* by INF0MAC */
  short int      /*  HL2 */ infmlen;       /* Length of Machine Section in bytes */
  short int      /*  HL2 */ infpoff;       /* Offset to Partition Section mapped */
                                           /* by INF0PAR */
  short int      /*  HL2 */ infplen;       /* Length of Partition Section in bytes */
  #define INF0YGMX 3                       /* Maximum Hypervisor/Guest sections */
  union {
    unsigned char  /*  XL8 */ infhygs1[8];   /* Hypervisor/Guest Header 1 */
    struct {
      short int      /*  HL2 */ infhoff1;      /* Offset to Hypervisor Section 1, */
                                               /* mapped by INF0HYP */
      short int      /*  HL2 */ infhlen1;      /* Length of Hypervisor Section 1 in */
                                               /* bytes */
      short int      /*  HL2 */ infgoff1;      /* Offset to Guest Section 1 mapped */
                                               /* by INF0GST */
      short int      /*  HL2 */ infglen1;      /* Length of Guest Section 1 in bytes */
    };
  };
  union {
    unsigned char  /*  XL8 */ infhygs2[8];   /* Hypervisor/Guest Header 2 */
    struct {
      short int      /*  HL2 */ infhoff2;      /* Offset to Hypervisor Section 2 */
                                               /* mapped by INF0HYP */
      short int      /*  HL2 */ infhlen2;      /* Length of Hypervisor Section 2 in */
                                               /* bytes */
      short int      /*  HL2 */ infgoff2;      /* Offset to Guest Section 2 mapped */
                                               /* by INF0GST */
      short int      /*  HL2 */ infglen2;      /* Length of Guest Section 2 in bytes */
    };
  };
  union {
    unsigned char  /*  XL8 */ infhygs3[8];   /* Hypervisor/Guest Header 3 */
    struct {
      short int      /*  HL2 */ infhoff3;      /* Offset to Hypervisor Section 3 */
                                               /* mapped by INF0HYP */
      short int      /*  HL2 */ infhlen3;      /* Length of Hypervisor Section 3 in */
                                               /* bytes */
      short int      /*  HL2 */ infgoff3;      /* Offset to Guest Section 3 mapped */
                                               /* by INF0GST */
      short int      /*  HL2 */ infglen3;      /* Length of Guest Section 3 in bytes */
      #define INF0HDSZ 48                      /* Size of header in bytes */
      #define INF0HDSD 6                       /* Size of Header in DWs */
    };
  };
  char           __filler1[4];
};
struct inf0gst {
  unsigned char  /*  XL1 */ infgflg1;      /* Guest Flag Byte 1 */
  #define INFGMOB 128                      /* Guest is mobility eligible */
  #define INFGMCPT 64                      /* Guest has multiple CPU types */
  #define INFGCPLH 32                      /* Guest CP dispatch type has */
                                           /* LIMITHARD cap. */
  #define INFGIFLH 16                      /* Guest IFL dispatch type has */
                                           /* LIMITHARD cap. */
  unsigned char  /*  XL1 */ infgflg2;      /* Guest Flag Byte 2 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infgval1;      /* Guest Validity Byte 1 */
                                           /* Reserved for IBM use */
  unsigned char  /*  XL1 */ infgval2;      /* Guest Validity Byte 2 */
                                           /* Reserved for IBM use */
  unsigned char  /*  CL8 */ infgusid[8];   /* Guest's userid, in EBCDIC format */
  short int      /*  HL2 */ infgscps;      /* Number of guest shared CPs */
  short int      /*  HL2 */ infgdcps;      /* Number of guest dedicated CPs */
  unsigned char  /*  XL1 */ infgcpdt;      /* Dispatch type for guest CPs */
                                           /* 00 General Purpose (CP) */
  char           __filler0[3];
  int            /*  FL4 */ infgcpcc;      /* Guest current capped capacity for */
                                           /* shared virtual CPs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
  short int      /*  HL2 */ infgsifl;      /* Number of guest shared IFLs */
  short int      /*  HL2 */ infgdifl;      /* Number of guest dedicated IFLs */
  unsigned char  /*  XL1 */ infgifdt;      /* Dispatch type for guest IFLs. This */
                                           /* field is valid if INFGSIFL or */
                                           /* INFGDIFL is greater than zero. */
                                           /* 00 General Purpose (CP) */
                                           /* 03 Integrated Facility for LINUX (IFL) */
  char           __filler1[3];
  int            /*  FL4 */ infgifcc;      /* Guest current capped capacity for */
                                           /* shared virtual IFLs, a scaled */
                                           /* number where X'00010000' */
                                           /* represents one CPU. */
                                           /* Zero if not capped. */
  unsigned char  /*  XL1 */ infgpflg;      /* CPU Pool Capping Flags */
  #define INFGPCLH 128                     /* CPU Pool's CP virtual type has */
  /* LIMITHARD cap.                    @U7068P4 */
  #define INFGPCPC 64                      /* CPU Pool's CP virtual type has */
  /* CAPACITY cap.                     @U7068P4 */
  #define INFGPILH 32                      /* CPU Pool's IFL virtual type has */
  /* LIMITHARD cap.                    @U7068P4 */
  #define INFGPIFC 16                      /* CPU Pool's IFL virtual type has */
  /* CAPACITY cap.                     @U7068P4 */
  char           __filler2[3];
  unsigned char  /*  CL8 */ infgpnam[8];   /* CPU pool name. Blanks if not in a */
                                           /* CPU Pool. */
  int            /*  FL4 */ infgpccc;      /* CPU pool capped capacity for shared */
                                           /* virtual CPs, a scaled number where */
                                           /* X'00010000' represents one CPU. */
                                           /* Zero if not capped. */
  int            /*  FL4 */ infgpicc;      /* CPU pool capped capacity for shared */
                                           /* virtual IFLs, a scaled number where */
                                           /* X'00010000' represents one CPU. */
                                           /* Zero if not capped. */
  #define INF0GSIZ 56                      /* Size of Guest Section in bytes */
  #define INF0GSZD 7                       /* Size of Guest Section in DWs */
};
struct infbk {
  long long      /*  DL8 */ infresp;       /* Minimum response buffer size      @U7059P4 */
  /* for all function codes.           @U7059P4 */
  #define INFRCOK 0                        /* Success                           @U7128DN */
  #define INFRFC 4                         /* Unsupported function code         @U7128DN */
  #define INFCAPAC 0                       /* FC=0 - Obtain CPU Capacity Info   @U7128DN */
};
#pragma anonstruct(pop)
#pragma pack(reset)

