/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpeglayer3.cc
// It's for MPEG Layer 3
// I've made array of superior functions for speed.
// Extend TO_FOUR_THIRDS to negative.
// Bug fix : maplay 1.2+ have wrong TO_FOUR_THIRDS ranges.
// Force to mono!!
// MPEG-2 is implemented
// Speed up in fixstereo (maybe buggy) 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>

#include "MPEGaudio.h"
#if defined(_WIN32) && defined(_MSC_VER)
// disable warnings about double to float conversions
#pragma warning(disable: 4244 4305)
#endif

inline void Mpegbitwindow::wrap(void)
{
  int p=bitindex>>3;
  point&=(WINDOWSIZE-1);

  if(p>=point)
  {
    for(register int i=4;i<point;i++)
      buffer[WINDOWSIZE+i]=buffer[i];
  }
  *((int *)(buffer+WINDOWSIZE))=*((int *)buffer);
}

#define MUL3(a) (((a)<<1)+(a))

#define REAL0 0

#define ARRAYSIZE (SBLIMIT*SSLIMIT)
#define REALSIZE (sizeof(REAL))

#ifdef PI
#undef PI
#endif
#define PI     3.141593
#define PI_12  (PI/12.0)
#define PI_18  (PI/18.0)
#define PI_24  (PI/24.0)
#define PI_36  (PI/36.0)
#define PI_72  (PI/72.0)

#ifdef NATIVE_ASSEMBLY
inline void long_memset(void * s,unsigned int c,int count)
{
__asm__ __volatile__(
  "cld\n\t"
  "rep ; stosl\n\t"
  : /* no output */
  :"a" (c), "c" (count/4), "D" ((long) s)
  :"cx","di","memory");
}
#endif

#define FOURTHIRDSTABLENUMBER (1<<13)

static REAL two_to_negative_half_pow[40];
static REAL TO_FOUR_THIRDSTABLE[FOURTHIRDSTABLENUMBER*2];
static REAL POW2[256];
static REAL POW2_1[8][2][16];
static REAL ca[8],cs[8];



static REAL cos1_6=cos(PI/6.0*1.0);
static REAL cos2_6=cos(PI/6.0*2.0);
static REAL win[4][36];
static REAL cos_18[9];
static REAL hsec_36[9],hsec_12[3];

typedef struct
{
  REAL l,r;
}RATIOS;

static RATIOS rat_1[16],rat_2[2][64];

void MPEGaudio::layer3initialize(void)
{
  static bool initializedlayer3=false;
  register int i;
  int j,k,l;
#if 0
	double td = 0.0;
printf( "%f\n", td );
    //pow( 0.0, 1.333333 );
printf( "A\n" );
    pow( td, 1.333333 );
printf( "B\n" );
#endif

  layer3framestart=0;
  currentprevblock=0;

  {
    for(l=0;l<2;l++)
      for(i=0;i<2;i++)
	for(j=0;j<SBLIMIT;j++)
	  for(k=0;k<SSLIMIT;k++)
	    prevblck[l][i][j][k]=0.0f;
  }

  bitwindow.initialize();

  if(initializedlayer3)return;

  // Calculate win
  {
    for(i=0;i<18;i++)
      win[0][i]=win[1][i]=0.5*sin(PI_72*(double)(2*i+1))/cos(PI_72*(double)(2*i+19));
    for(;i<36;i++)
      win[0][i]=win[3][i]=0.5*sin(PI_72*(double)(2*i+1))/cos(PI_72*(double)(2*i+19));

    for(i=0;i<6;i++)
    {
      win[1][i+18]=0.5/cos(PI_72*(double)(2*(i+18)+19));
      win[3][i+12]=0.5/cos(PI_72*(double)(2*(i+12)+19));
      win[1][i+24]=0.5*sin(PI_24*(double)(2*i+13))/cos(PI_72*(double)(2*(i+24)+19));
      win[1][i+30]=win[3][i]=0.0;
      win[3][i+6 ]=0.5*sin(PI_24*(double)(2*i+1))/cos(PI_72*(double)(2*(i+6)+19));
    }
    for(i=0;i<12;i++)
      win[2][i]=0.5*sin(PI_24*(double)(2*i+1))/cos(PI_24*(double)(2*i+7));
  }

  for(i=0;i<9;i++)
    cos_18[i]=cos(PI_18*double(i));
  for(i=0;i<9;i++)
    hsec_36[i]=0.5/cos(PI_36*double(i*2+1));
  for(i=0;i<3;i++)
    hsec_12[i]=0.5/cos(PI_12*double(i*2+1));

  for(i=0;i<40;i++)
    two_to_negative_half_pow[i]=(REAL)pow(2.0,-0.5*(double)i);

  {
    REAL *TO_FOUR_THIRDS=TO_FOUR_THIRDSTABLE+FOURTHIRDSTABLENUMBER;

//KR
    for( i = 0; i < FOURTHIRDSTABLENUMBER; i++ )
	{
      TO_FOUR_THIRDS[-i]= -(TO_FOUR_THIRDS[i] = (REAL)pow((double)i,4.0/3.0));
	}
  }

  for(i=0;i<256;i++)POW2[i]=(REAL)pow(2.0,(0.25*(i-210.0)));
  for(i=0;i<8;i++)
    for(j=0;j<2;j++)
      for(k=0;k<16;k++)POW2_1[i][j][k]=pow(2.0,(-2.0*i)-(0.5*(1.0+j)*k));

  {
    static REAL TAN12[16]=
    { 0.0,        0.26794919, 0.57735027  , 1.0,
      1.73205081, 3.73205081, 9.9999999e10,-3.73205081,
      -1.73205081,-1.01,      -0.57735027,  -0.26794919,
      0.0,        0.26794919, 0.57735027,   1.0};

    for(i=0;i<16;i++)
    {
      rat_1[i].l=TAN12[i]/(1.0+TAN12[i]);
      rat_1[i].r=1.0/(1.0+TAN12[i]);
    }
  }

#define IO0 ((double)0.840896415256)
#define IO1 ((double)0.707106781188)
  rat_2[0][0].l=rat_2[0][0].r=
  rat_2[1][0].l=rat_2[1][0].r=1.;

  for(i=1;i<64;i++)
    if((i%2)==1)
    {
      rat_2[0][i].l=pow(IO0,(i+1)/2);
      rat_2[1][i].l=pow(IO1,(i+1)/2);
      rat_2[0][i].r=
      rat_2[1][i].r=1.;
    }
    else
    {
      rat_2[0][i].l=
      rat_2[1][i].l=1.;
      rat_2[0][i].r=pow(IO0,i/2);
      rat_2[1][i].r=pow(IO1,i/2);
    }

  {
    static REAL Ci[8]=
    {-0.6f,-0.535f,-0.33f,-0.185f,-0.095f,-0.041f,-0.0142f,-0.0037f};
    REAL sq;

    for(int i=0;i<8;i++)
    {
      sq=sqrt(1.0f+Ci[i]*Ci[i]);
      cs[i]=1.0f/sq;
      ca[i]=Ci[i]*cs[i];
    }
  }

  initializedlayer3=true;
}

bool MPEGaudio::layer3getsideinfo(void)
{
  sideinfo.main_data_begin=getbits(9);

  if(!inputstereo)sideinfo.private_bits=getbits(5);
  else sideinfo.private_bits=getbits(3);
  
    sideinfo.ch[LS].scfsi[0]=getbit();
    sideinfo.ch[LS].scfsi[1]=getbit();
    sideinfo.ch[LS].scfsi[2]=getbit();
    sideinfo.ch[LS].scfsi[3]=getbit();
  if(inputstereo)
  {
    sideinfo.ch[RS].scfsi[0]=getbit();
    sideinfo.ch[RS].scfsi[1]=getbit();
    sideinfo.ch[RS].scfsi[2]=getbit();
    sideinfo.ch[RS].scfsi[3]=getbit();
  }

  for(int gr=0,ch;gr<2;gr++)
    for(ch=0;;ch++)
    {
      layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);

      gi->part2_3_length       =getbits(12);
      gi->big_values           =getbits(9);
      gi->global_gain          =getbits(8);
      gi->scalefac_compress    =getbits(4);
      gi->window_switching_flag=getbit();
      if(gi->window_switching_flag)
      {
	gi->block_type      =getbits(2);
	gi->mixed_block_flag=getbit();
	
	gi->table_select[0] =getbits(5);
	gi->table_select[1] =getbits(5);
	
	gi->subblock_gain[0]=getbits(3);
	gi->subblock_gain[1]=getbits(3);
	gi->subblock_gain[2]=getbits(3);
	
	/* Set region_count parameters since they are implicit in this case. */
	if(gi->block_type==0)
	{
	  /* printf("Side info bad: block_type == 0 in split block.\n");
	     exit(0); */
	  return false;
	}
	else if (gi->block_type==2 && gi->mixed_block_flag==0)
	     gi->region0_count=8; /* MI 9; */
	else gi->region0_count=7; /* MI 8; */
	gi->region1_count=20-(gi->region0_count);
      }
      else
      {
	gi->table_select[0] =getbits(5);
	gi->table_select[1] =getbits(5);
	gi->table_select[2] =getbits(5);
	gi->region0_count   =getbits(4);
	gi->region1_count   =getbits(3);
	gi->block_type      =0;
      }
      gi->preflag           =getbit();
      gi->scalefac_scale    =getbit();
      gi->count1table_select=getbit();

      gi->generalflag=gi->window_switching_flag && (gi->block_type==2);

      if(!inputstereo || ch)break;
    }

  return true;
}

bool MPEGaudio::layer3getsideinfo_2(void)
{
  sideinfo.main_data_begin=getbits(8);

  if(!inputstereo)sideinfo.private_bits=getbit();
  else sideinfo.private_bits=getbits(2);
  
  for(int ch=0;;ch++)
  {
    layer3grinfo *gi=&(sideinfo.ch[ch].gr[0]);

    gi->part2_3_length       =getbits(12);
    gi->big_values           =getbits(9);
    gi->global_gain          =getbits(8);
    gi->scalefac_compress    =getbits(9);
    gi->window_switching_flag=getbit();
    if(gi->window_switching_flag)
    {
      gi->block_type      =getbits(2);
      gi->mixed_block_flag=getbit();

      gi->table_select[0] =getbits(5);
      gi->table_select[1] =getbits(5);
	
      gi->subblock_gain[0]=getbits(3);
      gi->subblock_gain[1]=getbits(3);
      gi->subblock_gain[2]=getbits(3);
	
      /* Set region_count parameters since they are implicit in this case. */
      if(gi->block_type==0)
      {
	/* printf("Side info bad: block_type == 0 in split block.\n");
	   exit(0); */
	return false;
      }
      else if (gi->block_type==2 && gi->mixed_block_flag==0)
	gi->region0_count=8; /* MI 9; */
      else gi->region0_count=7; /* MI 8; */
      gi->region1_count=20-(gi->region0_count);
    }
    else
    {
      gi->table_select[0] =getbits(5);
      gi->table_select[1] =getbits(5);
      gi->table_select[2] =getbits(5);
      gi->region0_count   =getbits(4);
      gi->region1_count   =getbits(3);
      gi->block_type      =0;
    }
    gi->scalefac_scale    =getbit();
    gi->count1table_select=getbit();
    
    gi->generalflag=gi->window_switching_flag && (gi->block_type==2);
    
    if(!inputstereo || ch)break;
  }

  return true;
}

void MPEGaudio::layer3getscalefactors(int ch,int gr)
{
  static int slen[2][16]={{0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
			  {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}};

  layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);
  register layer3scalefactor *sf=(&scalefactors[ch]);
  int l0,l1;

  {
    int scale_comp=gi->scalefac_compress;

    l0=slen[0][scale_comp];
    l1=slen[1][scale_comp];
  }  
  if(gi->generalflag)
  {
    if(gi->mixed_block_flag)
    {                                 /* MIXED */ /* NEW-ag 11/25 */
      sf->l[0]=wgetbits9(l0);sf->l[1]=wgetbits9(l0);
      sf->l[2]=wgetbits9(l0);sf->l[3]=wgetbits9(l0);
      sf->l[4]=wgetbits9(l0);sf->l[5]=wgetbits9(l0);
      sf->l[6]=wgetbits9(l0);sf->l[7]=wgetbits9(l0);

      sf->s[0][ 3]=wgetbits9(l0);sf->s[1][ 3]=wgetbits9(l0);
      sf->s[2][ 3]=wgetbits9(l0);
      sf->s[0][ 4]=wgetbits9(l0);sf->s[1][ 4]=wgetbits9(l0);
      sf->s[2][ 4]=wgetbits9(l0);
      sf->s[0][ 5]=wgetbits9(l0);sf->s[1][ 5]=wgetbits9(l0);
      sf->s[2][ 5]=wgetbits9(l0);

      sf->s[0][ 6]=wgetbits9(l1);sf->s[1][ 6]=wgetbits9(l1);
      sf->s[2][ 6]=wgetbits9(l1);
      sf->s[0][ 7]=wgetbits9(l1);sf->s[1][ 7]=wgetbits9(l1);
      sf->s[2][ 7]=wgetbits9(l1);
      sf->s[0][ 8]=wgetbits9(l1);sf->s[1][ 8]=wgetbits9(l1);
      sf->s[2][ 8]=wgetbits9(l1);
      sf->s[0][ 9]=wgetbits9(l1);sf->s[1][ 9]=wgetbits9(l1);
      sf->s[2][ 9]=wgetbits9(l1);
      sf->s[0][10]=wgetbits9(l1);sf->s[1][10]=wgetbits9(l1);
      sf->s[2][10]=wgetbits9(l1);
      sf->s[0][11]=wgetbits9(l1);sf->s[1][11]=wgetbits9(l1);
      sf->s[2][11]=wgetbits9(l1);

      sf->s[0][12]=sf->s[1][12]=sf->s[2][12]=0;
    }
    else 
    {  /* SHORT*/
      sf->s[0][ 0]=wgetbits9(l0);sf->s[1][ 0]=wgetbits9(l0);
      sf->s[2][ 0]=wgetbits9(l0);
      sf->s[0][ 1]=wgetbits9(l0);sf->s[1][ 1]=wgetbits9(l0);
      sf->s[2][ 1]=wgetbits9(l0);
      sf->s[0][ 2]=wgetbits9(l0);sf->s[1][ 2]=wgetbits9(l0);
      sf->s[2][ 2]=wgetbits9(l0);
      sf->s[0][ 3]=wgetbits9(l0);sf->s[1][ 3]=wgetbits9(l0);
      sf->s[2][ 3]=wgetbits9(l0);
      sf->s[0][ 4]=wgetbits9(l0);sf->s[1][ 4]=wgetbits9(l0);
      sf->s[2][ 4]=wgetbits9(l0);
      sf->s[0][ 5]=wgetbits9(l0);sf->s[1][ 5]=wgetbits9(l0);
      sf->s[2][ 5]=wgetbits9(l0);

      sf->s[0][ 6]=wgetbits9(l1);sf->s[1][ 6]=wgetbits9(l1);
      sf->s[2][ 6]=wgetbits9(l1);
      sf->s[0][ 7]=wgetbits9(l1);sf->s[1][ 7]=wgetbits9(l1);
      sf->s[2][ 7]=wgetbits9(l1);
      sf->s[0][ 8]=wgetbits9(l1);sf->s[1][ 8]=wgetbits9(l1);
      sf->s[2][ 8]=wgetbits9(l1);
      sf->s[0][ 9]=wgetbits9(l1);sf->s[1][ 9]=wgetbits9(l1);
      sf->s[2][ 9]=wgetbits9(l1);
      sf->s[0][10]=wgetbits9(l1);sf->s[1][10]=wgetbits9(l1);
      sf->s[2][10]=wgetbits9(l1);
      sf->s[0][11]=wgetbits9(l1);sf->s[1][11]=wgetbits9(l1);
      sf->s[2][11]=wgetbits9(l1);

      sf->s[0][12]=sf->s[1][12]=sf->s[2][12]=0;
    }
  }
  else
  {   /* LONG types 0,1,3 */
    if(gr==0)
    {
      sf->l[ 0]=wgetbits9(l0);sf->l[ 1]=wgetbits9(l0);
      sf->l[ 2]=wgetbits9(l0);sf->l[ 3]=wgetbits9(l0);
      sf->l[ 4]=wgetbits9(l0);sf->l[ 5]=wgetbits9(l0);
      sf->l[ 6]=wgetbits9(l0);sf->l[ 7]=wgetbits9(l0);
      sf->l[ 8]=wgetbits9(l0);sf->l[ 9]=wgetbits9(l0);
      sf->l[10]=wgetbits9(l0);
      sf->l[11]=wgetbits9(l1);sf->l[12]=wgetbits9(l1);
      sf->l[13]=wgetbits9(l1);sf->l[14]=wgetbits9(l1);
      sf->l[15]=wgetbits9(l1);
      sf->l[16]=wgetbits9(l1);sf->l[17]=wgetbits9(l1);
      sf->l[18]=wgetbits9(l1);sf->l[19]=wgetbits9(l1);
      sf->l[20]=wgetbits9(l1);
    }
    else
    {
      if(sideinfo.ch[ch].scfsi[0]==0)
      {
	sf->l[ 0]=wgetbits9(l0);sf->l[ 1]=wgetbits9(l0);
	sf->l[ 2]=wgetbits9(l0);sf->l[ 3]=wgetbits9(l0);
	sf->l[ 4]=wgetbits9(l0);sf->l[ 5]=wgetbits9(l0);
      }
      if(sideinfo.ch[ch].scfsi[1]==0)
      {
	sf->l[ 6]=wgetbits9(l0);sf->l[ 7]=wgetbits9(l0);
	sf->l[ 8]=wgetbits9(l0);sf->l[ 9]=wgetbits9(l0);
	sf->l[10]=wgetbits9(l0);
      }
      if(sideinfo.ch[ch].scfsi[2]==0)
      {
	sf->l[11]=wgetbits9(l1);sf->l[12]=wgetbits9(l1);
	sf->l[13]=wgetbits9(l1);sf->l[14]=wgetbits9(l1);
	sf->l[15]=wgetbits9(l1);
      }
      if(sideinfo.ch[ch].scfsi[3]==0)
      {
	sf->l[16]=wgetbits9(l1);sf->l[17]=wgetbits9(l1);
	sf->l[18]=wgetbits9(l1);sf->l[19]=wgetbits9(l1);
	sf->l[20]=wgetbits9(l1);
      }
    }
    sf->l[21]=sf->l[22]=0;
  }
}

void MPEGaudio::layer3getscalefactors_2(int ch)
{
  static int sfbblockindex[6][3][4]=
  {
    {{ 6, 5, 5, 5},{ 9, 9, 9, 9},{ 6, 9, 9, 9}},
    {{ 6, 5, 7, 3},{ 9, 9,12, 6},{ 6, 9,12, 6}},
    {{11,10, 0, 0},{18,18, 0, 0},{15,18, 0, 0}},
    {{ 7, 7, 7, 0},{12,12,12, 0},{ 6,15,12, 0}},
    {{ 6, 6, 6, 3},{12, 9, 9, 6},{ 6,12, 9, 6}},
    {{ 8, 8, 5, 0},{15,12, 9, 0},{ 6,18, 9, 0}}
  };

  int sb[54];
  layer3grinfo *gi=&(sideinfo.ch[ch].gr[0]);
  register layer3scalefactor *sf=(&scalefactors[ch]);

  {
    int blocktypenumber,sc;
    int blocknumber;
    int slen[4];

    if(gi->block_type==2)blocktypenumber=1+gi->mixed_block_flag;
    else blocktypenumber=0;

    sc=gi->scalefac_compress;
    if(!((extendedmode==1 || extendedmode==3) && (ch==1)))
    {
      if(sc<400)
      {
	slen[0]=(sc>>4)/5;
	slen[1]=(sc>>4)%5;
	slen[2]=(sc%16)>>2;
	slen[3]=(sc%4);
	gi->preflag=0;
	blocknumber=0;
      }
      else if(sc<500)
      {
	sc-=400;
	slen[0]=(sc>>2)/5;
	slen[1]=(sc>>2)%5;
	slen[2]=sc%4;
	slen[3]=0;
	gi->preflag=0;
	blocknumber=1;
      }
      else // if(sc<512)
      {
	sc-=500;
	slen[0]=sc/3;
	slen[1]=sc%3;
	slen[2]=0;
	slen[3]=0;
	gi->preflag=1;
	blocknumber=2;
      }
    }
    else
    {
      sc>>=1;
      if(sc<180)
      {
	slen[0]=sc/36;
	slen[1]=(sc%36)/6;
	slen[2]=(sc%36)%6;
	slen[3]=0;
	gi->preflag=0;
	blocknumber=3;
      }
      else if(sc<244)
      {
	sc-=180;
	slen[0]=(sc%64)>>4;
	slen[1]=(sc%16)>>2;
	slen[2]=sc%4;
	slen[3]=0;
	gi->preflag=0;
	blocknumber=4;
      }
      else // if(sc<255)
      {
	sc-=244;
	slen[0]=sc/3;
	slen[1]=sc%3;
	slen[2]=
	slen[3]=0;
	gi->preflag=0;
	blocknumber=5;
      }
    }

    {
      int i,j,k,*si;

      si=sfbblockindex[blocknumber][blocktypenumber];
      for(i=0;i<45;i++)sb[i]=0;

      for(k=i=0;i<4;i++)
	for(j=0;j<si[i];j++,k++)
	  if(slen[i]==0)sb[k]=0;
	  else sb[k]=wgetbits(slen[i]);
    }
  }


  {
    int sfb,window;
    int k=0;

    if(gi->window_switching_flag && (gi->block_type==2))
    {
      if(gi->mixed_block_flag)
      {
	for(sfb=0;sfb<8;sfb++)sf->l[sfb]=sb[k++];
	sfb=3;
      }
      else sfb=0;

      for(;sfb<12;sfb++)
	for(window=0;window<3;window++)
	  sf->s[window][sfb]=sb[k++];

      sf->s[0][12]=sf->s[1][12]=sf->s[2][12]=0;
    }
    else
    {
      for(sfb=0;sfb<21;sfb++)
	sf->l[sfb]=sb[k++];
      sf->l[21]=sf->l[22]=0;
    }
  }
}


typedef unsigned int HUFFBITS;
#define MXOFF   250

/* do the huffman-decoding 						*/
/* note! for counta,countb -the 4 bit value is returned in y, discard x */
// Huffman decoder for tablename<32
void MPEGaudio::huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y)
{  
  HUFFBITS level=(1<<(sizeof(HUFFBITS)*8-1));
  int point=0;

  /* Lookup in Huffman table. */
  for(;;)
  {
    if(h->val[point][0]==0)
    {   /*end of tree*/
      int xx,yy;

      xx=h->val[point][1]>>4;
      yy=h->val[point][1]&0xf;

      if(h->linbits)
      {
	if((h->xlen)==(unsigned)xx)xx+=wgetbits(h->linbits);
	if(xx)if(wgetbit())xx=-xx;
	if((h->ylen)==(unsigned)yy)yy+=wgetbits(h->linbits);
	if(yy)if(wgetbit())yy=-yy;
      }
      else
      {
	if(xx)if(wgetbit())xx=-xx;
	if(yy)if(wgetbit())yy=-yy;
      }
      *x=xx;*y=yy;
      break;
    } 

    point+=h->val[point][wgetbit()];
    
    level>>=1;
    if(!(level || ((unsigned)point<ht->treelen)))
    {
      register int xx,yy;

      xx=(h->xlen<<1);// set x and y to a medium value as a simple concealment
      yy=(h->ylen<<1);

      // h->xlen and h->ylen can't be 1 under tablename 32
      //      if(xx)
	if(wgetbit())xx=-xx;
      //      if(yy)
	if(wgetbit())yy=-yy;

      *x=xx;*y=yy;
      break;
    }
  }
}

// Huffman decoder tablenumber>=32
void MPEGaudio::huffmandecoder_2(const HUFFMANCODETABLE *h,
				       int *x,int *y,int *v,int *w)
{  
  HUFFBITS level=(1<<(sizeof(HUFFBITS)*8-1));
  int point=0;

  /* Lookup in Huffman table. */
  for(;;)
  {
    if(h->val[point][0]==0)
    {   /*end of tree*/
      register int t=h->val[point][1];

      if(t&8)*v=1-(wgetbit()<<1); else *v=0;
      if(t&4)*w=1-(wgetbit()<<1); else *w=0;
      if(t&2)*x=1-(wgetbit()<<1); else *x=0;
      if(t&1)*y=1-(wgetbit()<<1); else *y=0;
      break;
    } 
    point+=h->val[point][wgetbit()];
    level>>=1;
    if(!(level || ((unsigned)point<ht->treelen)))
    {
      *v=1-(wgetbit()<<1);
      *w=1-(wgetbit()<<1);
      *x=1-(wgetbit()<<1);
      *y=1-(wgetbit()<<1);
      break;
    }
  }
}

typedef struct
{
  int l[23];
  int s[14];
}SFBANDINDEX;

static SFBANDINDEX sfBandIndextable[2][3]=
{
  // MPEG 1
  {{{0,4,8,12,16,20,24,30,36,44,52,62,74,90,110,134,162,196,238,288,342,418,576},
    {0,4,8,12,16,22,30,40,52,66,84,106,136,192}},
   {{0,4,8,12,16,20,24,30,36,42,50,60,72,88,106,128,156,190,230,276,330,384,576},
    {0,4,8,12,16,22,28,38,50,64,80,100,126,192}},
   {{0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576},
    {0,4,8,12,16,22,30,42,58,78,104,138,180,192}}},

  // MPEG 2
  {{{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
    {0,4,8,12,18,24,32,42,56,74,100,132,174,192}},
   {{0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
    {0,4,8,12,18,26,36,48,62,80,104,136,180,192}},
   {{0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
    {0,4,8,12,18,26,36,48,62,80,104,134,174,192}}}
};


void MPEGaudio::layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT])
{
  layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);
  int part2_3_end=layer3part2start+(gi->part2_3_length);
  int region1Start,region2Start;
  int i,e=gi->big_values<<1;

  /* Find region boundary for short block case. */
  if(gi->generalflag)
  {
    /* Region2. */
    region1Start=36; /* sfb[9/3]*3=36 */
    region2Start=576;/* No Region2 for short block case. */
  }
  else
  {          /* Find region boundary for long block case. */
    region1Start=sfBandIndextable[version][frequency].l[gi->region0_count+1];
    region2Start=sfBandIndextable[version][frequency].l[gi->region0_count+
						        gi->region1_count+2];
  }

  /* Read bigvalues area. */
  for(i=0;i<e;)
  {
    const HUFFMANCODETABLE *h;
    register int end;
      
    if     (i<region1Start)
    {
      h=&ht[gi->table_select[0]];
      if(region1Start>e)end=e; else end=region1Start;
    }
    else if(i<region2Start)
    {
      h=&ht[gi->table_select[1]];
      if(region2Start>e)end=e; else end=region2Start;
    }
    else
    {
      h=&ht[gi->table_select[2]];
      end=e;
    }

    if(h->treelen)
      while(i<end)
      {
	huffmandecoder_1(h,&out[0][i],&out[0][i+1]);
	i+=2;
      }
    else
      for(;i<end;i+=2)
	out[0][i]  =
	out[0][i+1]=0;
  }

  /* Read count1 area. */
  const HUFFMANCODETABLE *h=&ht[gi->count1table_select+32];
  while(bitwindow.gettotalbit()<part2_3_end)
  {
    huffmandecoder_2(h,&out[0][i+2],&out[0][i+3],
		     &out[0][i  ],&out[0][i+1]);
    i+=4;

    if(i>=ARRAYSIZE)
    {
      bitwindow.rewind(bitwindow.gettotalbit()-part2_3_end);
      return;
    }
  }
  
  for(;i<ARRAYSIZE;i++)out[0][i]=0;
  bitwindow.rewind(bitwindow.gettotalbit()-part2_3_end);
}


static int pretab[22]={0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};

REAL MPEGaudio::layer3twopow2(int scale,int preflag,
				     int pretab_offset,int l)
{
  int index=l;
  
  if(preflag)index+=pretab_offset;
  return(two_to_negative_half_pow[index<<scale]);
}

REAL MPEGaudio::layer3twopow2_1(int a,int b,int c)
{
  return POW2_1[a][b][c];
}

void MPEGaudio::layer3dequantizesample(int ch,int gr,
				       int   in[SBLIMIT][SSLIMIT],
				       REAL out[SBLIMIT][SSLIMIT])
{
  layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);
  SFBANDINDEX *sfBandIndex=&(sfBandIndextable[version][frequency]);
  REAL globalgain=POW2[gi->global_gain];
  REAL *TO_FOUR_THIRDS=TO_FOUR_THIRDSTABLE+FOURTHIRDSTABLENUMBER;

  /* choose correct scalefactor band per block type, initalize boundary */
  /* and apply formula per block type */
  
  if(!gi->generalflag)
  {                                          /* LONG blocks: 0,1,3 */
    int next_cb_boundary;
    int cb=-1,index=0;
    REAL factor;

    do
    {
      next_cb_boundary=sfBandIndex->l[(++cb)+1];
      factor=globalgain*
	     layer3twopow2(gi->scalefac_scale,gi->preflag,
			   pretab[cb],scalefactors[ch].l[cb]);
      for(;index<next_cb_boundary;)
      {
	out[0][index]=factor*TO_FOUR_THIRDS[in[0][index]];index++;
	out[0][index]=factor*TO_FOUR_THIRDS[in[0][index]];index++;
      }
    }while(index<ARRAYSIZE);
  }
  else if(!gi->mixed_block_flag)
  {
    int cb=0,index=0;
    int cb_width;

    do
    {
      cb_width=(sfBandIndex->s[cb+1]-sfBandIndex->s[cb])>>1;

      for(register int k=0;k<3;k++)
      {
	register REAL factor;
	register int count=cb_width;

	factor=globalgain*
	       layer3twopow2_1(gi->subblock_gain[k],gi->scalefac_scale,
			       scalefactors[ch].s[k][cb]);
	do{
	  out[0][index]=factor*TO_FOUR_THIRDS[in[0][index]];index++;
	  out[0][index]=factor*TO_FOUR_THIRDS[in[0][index]];index++;
	}while(--count);
      }
      cb++;
    }while(index<ARRAYSIZE);
  }
  else
  {
    int cb_begin=0,cb_width=0;
    int cb=0;
    int next_cb_boundary=sfBandIndex->l[1]; /* LONG blocks: 0,1,3 */
    int index;

    /* Compute overall (global) scaling. */
    {
      for(int sb=0;sb<SBLIMIT;sb++)
      {
	int *i=in[sb];
	REAL *o=out[sb];

	o[ 0]=globalgain*TO_FOUR_THIRDS[i[ 0]];o[ 1]=globalgain*TO_FOUR_THIRDS[i[ 1]];
	o[ 2]=globalgain*TO_FOUR_THIRDS[i[ 2]];o[ 3]=globalgain*TO_FOUR_THIRDS[i[ 3]];
	o[ 4]=globalgain*TO_FOUR_THIRDS[i[ 4]];o[ 5]=globalgain*TO_FOUR_THIRDS[i[ 5]];
	o[ 6]=globalgain*TO_FOUR_THIRDS[i[ 6]];o[ 7]=globalgain*TO_FOUR_THIRDS[i[ 7]];
	o[ 8]=globalgain*TO_FOUR_THIRDS[i[ 8]];o[ 9]=globalgain*TO_FOUR_THIRDS[i[ 9]];
	o[10]=globalgain*TO_FOUR_THIRDS[i[10]];o[11]=globalgain*TO_FOUR_THIRDS[i[11]];
	o[12]=globalgain*TO_FOUR_THIRDS[i[12]];o[13]=globalgain*TO_FOUR_THIRDS[i[13]];
	o[14]=globalgain*TO_FOUR_THIRDS[i[14]];o[15]=globalgain*TO_FOUR_THIRDS[i[15]];
	o[16]=globalgain*TO_FOUR_THIRDS[i[16]];o[17]=globalgain*TO_FOUR_THIRDS[i[17]];
      }
    }

    for(index=0;index<SSLIMIT*2;index++)
    {
      if(index==next_cb_boundary)
      {
	if(index==sfBandIndex->l[8])
	{
	  next_cb_boundary=sfBandIndex->s[4];
	  next_cb_boundary=MUL3(next_cb_boundary);
	  cb=3;
	  cb_width=sfBandIndex->s[4]-sfBandIndex->s[3];
	  cb_begin=sfBandIndex->s[3];
	  cb_begin=MUL3(cb_begin);
	}
	else if(index<sfBandIndex->l[8])
	  next_cb_boundary=sfBandIndex->l[(++cb)+1];
	else 
	{
	  next_cb_boundary=sfBandIndex->s[(++cb)+1];
	  next_cb_boundary=MUL3(next_cb_boundary);
	  cb_begin=sfBandIndex->s[cb];
	  cb_width=sfBandIndex->s[cb+1]-cb_begin;
	  cb_begin=MUL3(cb_begin);
	}
      }
      /* LONG block types 0,1,3 & 1st 2 subbands of switched blocks */
      out[0][index]*=layer3twopow2(gi->scalefac_scale,gi->preflag,
				   pretab[cb],scalefactors[ch].l[cb]);
    }

    for(;index<ARRAYSIZE;index++)
    { 
      if(index==next_cb_boundary)
      {
	if(index==sfBandIndex->l[8])
	{
	  next_cb_boundary=sfBandIndex->s[4];
	  next_cb_boundary=MUL3(next_cb_boundary);
	  cb=3;
	  cb_width=sfBandIndex->s[4]-sfBandIndex->s[3];
	  cb_begin=sfBandIndex->s[3];
	  cb_begin=(cb_begin<<2)-cb_begin;
	}
	else if(index<sfBandIndex->l[8])
	  next_cb_boundary=sfBandIndex->l[(++cb)+1];
	else 
	{
	  next_cb_boundary=sfBandIndex->s[(++cb)+1];
	  next_cb_boundary=MUL3(next_cb_boundary);
	  cb_begin=sfBandIndex->s[cb];
	  cb_width=sfBandIndex->s[cb+1]-cb_begin;
	  cb_begin=MUL3(cb_begin);
	}
      }
      {
	int t_index=(index-cb_begin)/cb_width;
	out[0][index]*=layer3twopow2_1(gi->subblock_gain[t_index],
				       gi->scalefac_scale,
				       scalefactors[ch].s[t_index][cb]);
      }
    }
  }
}

void MPEGaudio::layer3fixtostereo(int gr,REAL in[2][SBLIMIT][SSLIMIT])
{
  layer3grinfo *gi=&(sideinfo.ch[0].gr[gr]);
  SFBANDINDEX *sfBandIndex=&(sfBandIndextable[version][frequency]);
  
  int ms_stereo=(mode==joint) && (extendedmode & 0x2);
  int i_stereo =(mode==joint) && (extendedmode & 0x1);

  if(!inputstereo)
  { /* mono , bypass xr[0][][] to lr[0][][]*/
    // memcpy(out[0][0],in[0][0],ARRAYSIZE*REALSIZE);
    return;
  } 
    
  if(i_stereo)
  {
    int i;
    int    is_pos[ARRAYSIZE];
    RATIOS is_ratio[ARRAYSIZE];
    RATIOS *ratios;

    if(version)ratios=rat_2[gi->scalefac_compress%2];
    else ratios=rat_1;

    /* initialization */
    for(i=0;i<ARRAYSIZE;i+=2)is_pos[i]=is_pos[i+1]=7;

    if(gi->generalflag)
    {
      if(gi->mixed_block_flag)           // Part I
      {
	int max_sfb=0;

	for(int j=0;j<3;j++)
	{
	  int sfb,sfbcnt=2;

	  for(sfb=12;sfb>=3;sfb--)
	  {
	    int lines;

	    i=sfBandIndex->s[sfb];
	    lines=sfBandIndex->s[sfb+1]-i;
	    i=MUL3(i)+(j+1)*lines-1;
	    for(;lines>0;lines--,i--)
	      if(in[1][0][i]!=0.0f)
	      {
		sfbcnt=sfb;
		sfb=0;break;        // quit loop
	      }
	  }
	  sfb=sfbcnt+1;
	    
	  if(sfb>max_sfb)max_sfb=sfb;
	    
	  for(;sfb<12;sfb++)
	  {
	    int k,t;

	    t=sfBandIndex->s[sfb];
	    k=sfBandIndex->s[sfb+1]-t;
	    i=MUL3(t)+j*k;

	    t=scalefactors[1].s[j][sfb];
	    if(t!=7)
	    {
	      RATIOS r=ratios[t];

	      for(;k>0;k--,i++){
		is_pos[i]=t;is_ratio[i]=r;}
	    }
	    else
	      for(;k>0;k--,i++)is_pos[i]=t;
	  }
	  sfb=sfBandIndex->s[10];
	  sfb=MUL3(sfb)+j*(sfBandIndex->s[11]-sfb);

	  {
	    int k,t;

	    t=sfBandIndex->s[11];
	    k=sfBandIndex->s[12]-t;
	    i=MUL3(t)+j*k;

	    t=is_pos[sfb];
	    if(t!=7)
	    {
	      RATIOS r=is_ratio[sfb];

	      for(;k>0;k--,i++){
		is_pos[i]=t;is_ratio[i]=r;}
	    }
	    else
	      for(;k>0;k--,i++)is_pos[i]=t;
	  }
	}

	if(max_sfb<=3)
	{
	  {
	    REAL temp;
	    int k;

	    temp=in[1][0][0];in[1][0][0]=1.0;
	    for(k=3*SSLIMIT-1;in[1][0][k]==0.0;k--);
	    in[1][0][0]=temp;
	    for(i=0;sfBandIndex->l[i]<=k;i++);
	  }
	  {
	    int sfb=i;

	    i=sfBandIndex->l[i];
	    for(;sfb<8;sfb++)
	    {
	      int t=scalefactors[1].l[sfb];
	      int k=sfBandIndex->l[sfb+1]-sfBandIndex->l[sfb];

	      if(t!=7)
	      {
		RATIOS r=ratios[t];

		for(;k>0;k--,i++){
		  is_pos[i]=t;is_ratio[i]=r;}
	      }
	      else for(;k>0;k--,i++)is_pos[i]=t;
	    }
	  }
	}
      }
      else                                // Part II
      {
	for(int j=0;j<3;j++)
	{
	  int sfb;
      int sfbcnt=-1;

	  for(sfb=12;sfb>=0;sfb--)
	  {
	    int lines;

	    {
	      int t;

	      t=sfBandIndex->s[sfb];
	      lines=sfBandIndex->s[sfb+1]-t;
	      i=MUL3(t)+(j+1)*lines-1;
	    }
		  
	    for(;lines>0;lines--,i--)
	      if(in[1][0][i]!=0.0f)
	      {
		sfbcnt=sfb;
		sfb=0;break;       // quit loop
	      }
	  }

	  for(sfb=sfbcnt+1;sfb<12;sfb++)
	  {
	    int k,t;

	    t=sfBandIndex->s[sfb];
	    k=sfBandIndex->s[sfb+1]-t;
	    i=MUL3(t)+j*k;

	    t=scalefactors[1].s[j][sfb];
	    if(t!=7)
	    {
	      RATIOS r=ratios[t];

	      for(;k>0;k--,i++){
		is_pos[i]=t;is_ratio[i]=r;}
	    }
	    else for(;k>0;k--,i++)is_pos[i]=t;
	  }

	  {
	    int t1=sfBandIndex->s[10],
	        t2=sfBandIndex->s[11];
	    int k,tt;

	    tt=MUL3(t1)+j*(t2-t1);
	    k =sfBandIndex->s[12]-t2;
	    if(is_pos[tt]!=7)
	    {
	      RATIOS r=is_ratio[tt];
	      int  t=is_pos[tt];

	      i   =MUL3(t1)+j*k;
	      for(;k>0;k--,i++){
		is_pos[i]=t;is_ratio[i]=r;}
	    }
	    else
	      for(;k>0;k--,i++)is_pos[i]=7;
	  }
	}
      }
    }
    else // ms-stereo (Part III)
    {
      {
	REAL temp;
	int k;

	temp=in[1][0][0];in[1][0][0]=1.0;
	for(k=ARRAYSIZE-1;in[1][0][k]==0.0;k--);
	in[1][0][0]=temp;
	for(i=0;sfBandIndex->l[i]<=k;i++);
      }

      {
	int sfb;

	sfb=i;
	i=sfBandIndex->l[i];
	for(;sfb<21;sfb++)
        {
	  int k,t;

	  k=sfBandIndex->l[sfb+1]-sfBandIndex->l[sfb];
	  t=scalefactors[1].l[sfb];
	  if(t!=7)
	  {
	    RATIOS r=ratios[t];

	    for(;k>0;k--,i++){
	      is_pos[i]=t;is_ratio[i]=r;}
	  }
	  else
	    for(;k>0;k--,i++)is_pos[i]=t;
	}
      }

      if (i <= sfBandIndex->l[21]) {
	int k,t,tt;

	tt=sfBandIndex->l[20];
	k=576-sfBandIndex->l[21];
	t=is_pos[tt];
	if(t!=7)
	{
	  RATIOS r=is_ratio[tt];

	  for(;k>0;k--,i++){
	    is_pos[i]=t;is_ratio[i]=r;}
	}
	else
	  for(;k>0;k--,i++)is_pos[i]=t;
      }
    }

    if(ms_stereo)
    {
      i=ARRAYSIZE-1;
      do{
	if(is_pos[i]==7)
	{
	  register REAL t=in[LS][0][i];
	  in[LS][0][i]=(t+in[RS][0][i])*0.7071068f;
	  in[RS][0][i]=(t-in[RS][0][i])*0.7071068f;
	}
	else
	{
	  in[RS][0][i]=in[LS][0][i]*is_ratio[i].r;
	  in[LS][0][i]*=is_ratio[i].l;
	}
      }while(i--);
    }
    else
    {
      i=ARRAYSIZE-1;
      do{
	if(is_pos[i]!=7)
	{
	  in[RS][0][i]=in[LS][0][i]*is_ratio[i].r;
	  in[LS][0][i]*=is_ratio[i].l;
	}
      }while(i--);
    }
  }
  else
  {
    if(ms_stereo)
    {
      int i=ARRAYSIZE-1;
      do{
	register REAL t=in[LS][0][i];

	in[LS][0][i]=(t+in[RS][0][i])*0.7071068f;
	in[RS][0][i]=(t-in[RS][0][i])*0.7071068f;
      }while(i--);
    }
  }

  // channels==2
}

inline void layer3reorder_1(int version,int frequency,
			    REAL  in[SBLIMIT][SSLIMIT],
			    REAL out[SBLIMIT][SSLIMIT])
{
  SFBANDINDEX *sfBandIndex=&(sfBandIndextable[version][frequency]);
  int sfb,sfb_start,sfb_lines;
  
  /* NO REORDER FOR LOW 2 SUBBANDS */
  out[0][ 0]=in[0][ 0];out[0][ 1]=in[0][ 1];out[0][ 2]=in[0][ 2];
  out[0][ 3]=in[0][ 3];out[0][ 4]=in[0][ 4];out[0][ 5]=in[0][ 5];
  out[0][ 6]=in[0][ 6];out[0][ 7]=in[0][ 7];out[0][ 8]=in[0][ 8];
  out[0][ 9]=in[0][ 9];out[0][10]=in[0][10];out[0][11]=in[0][11];
  out[0][12]=in[0][12];out[0][13]=in[0][13];out[0][14]=in[0][14];
  out[0][15]=in[0][15];out[0][16]=in[0][16];out[0][17]=in[0][17];

  out[1][ 0]=in[1][ 0];out[1][ 1]=in[1][ 1];out[1][ 2]=in[1][ 2];
  out[1][ 3]=in[1][ 3];out[1][ 4]=in[1][ 4];out[1][ 5]=in[1][ 5];
  out[1][ 6]=in[1][ 6];out[1][ 7]=in[1][ 7];out[1][ 8]=in[1][ 8];
  out[1][ 9]=in[1][ 9];out[1][10]=in[1][10];out[1][11]=in[1][11];
  out[1][12]=in[1][12];out[1][13]=in[1][13];out[1][14]=in[1][14];
  out[1][15]=in[1][15];out[1][16]=in[1][16];out[1][17]=in[1][17];


  /* REORDERING FOR REST SWITCHED SHORT */
  for(sfb=3,sfb_start=sfBandIndex->s[3],
	sfb_lines=sfBandIndex->s[4]-sfb_start;
      sfb<13;
      sfb++,sfb_start=sfBandIndex->s[sfb],
	(sfb_lines=sfBandIndex->s[sfb+1]-sfb_start))
  {
    for(int freq=0;freq<sfb_lines;freq++)
    {
      int src_line=sfb_start+(sfb_start<<1)+freq;
      int des_line=src_line+(freq<<1);
      out[0][des_line  ]=in[0][src_line               ];
      out[0][des_line+1]=in[0][src_line+sfb_lines     ];
      out[0][des_line+2]=in[0][src_line+(sfb_lines<<1)];
    }
  }
}

#ifdef _MSC_VER	// VC++ 6 generates bad optimized code here, I'm not sure why...
#pragma optimize( "g", off )
#endif
void layer3reorder_2(int version,int frequency,REAL  in[SBLIMIT][SSLIMIT],
			    REAL out[SBLIMIT][SSLIMIT])
{
  SFBANDINDEX *sfBandIndex=&(sfBandIndextable[version][frequency]);
  int sfb,sfb_start,sfb_lines;
  
  for(sfb=0,sfb_start=0,sfb_lines=sfBandIndex->s[1];
      sfb<13;
      sfb++,sfb_start=sfBandIndex->s[sfb],
	(sfb_lines=sfBandIndex->s[sfb+1]-sfb_start))
  {
    for(int freq=0;freq<sfb_lines;freq++)
    {
      int src_line=sfb_start+(sfb_start<<1)+freq;
      int des_line=src_line+(freq<<1);

      out[0][des_line  ]=in[0][src_line               ];
      out[0][des_line+1]=in[0][src_line+sfb_lines     ];
      out[0][des_line+2]=in[0][src_line+(sfb_lines<<1)];
    }
  }
}
#ifdef _MSC_VER
#pragma optimize( "g", on )
#endif


inline void layer3antialias_1(REAL  in[SBLIMIT][SSLIMIT])
{
  for(int ss=0;ss<8;ss++)
  {
    REAL bu,bd; /* upper and lower butterfly inputs */
    
    bu=in[0][17-ss];bd=in[1][ss];
    in[0][17-ss]=(bu*cs[ss])-(bd*ca[ss]);
    in[1][ss]   =(bd*cs[ss])+(bu*ca[ss]);
  }
}

inline
void layer3antialias_2(REAL  in[SBLIMIT][SSLIMIT],
		       REAL out[SBLIMIT][SSLIMIT])
{
  out[0][0]=in[0][0];out[0][1]=in[0][1];
  out[0][2]=in[0][2];out[0][3]=in[0][3];
  out[0][4]=in[0][4];out[0][5]=in[0][5];
  out[0][6]=in[0][6];out[0][7]=in[0][7];

  for(int index=SSLIMIT;index<=(SBLIMIT-1)*SSLIMIT;index+=SSLIMIT)
  {
    for(int n=0;n<8;n++)
    {
      REAL bu,bd;

      bu=in[0][index-n-1];bd=in[0][index+n];
      out[0][index-n-1]=(bu*cs[n])-(bd*ca[n]);
      out[0][index+n  ]=(bd*cs[n])+(bu*ca[n]);
    }
    out[0][index-SSLIMIT+8]=in[0][index-SSLIMIT+8];
    out[0][index-SSLIMIT+9]=in[0][index-SSLIMIT+9];
  }

  out[31][ 8]=in[31][ 8];out[31][ 9]=in[31][ 9];
  out[31][10]=in[31][10];out[31][11]=in[31][11];
  out[31][12]=in[31][12];out[31][13]=in[31][13];
  out[31][14]=in[31][14];out[31][15]=in[31][15];
  out[31][16]=in[31][16];out[31][17]=in[31][17];
}

void MPEGaudio::layer3reorderandantialias(int ch,int gr,
					  REAL  in[SBLIMIT][SSLIMIT],
					  REAL out[SBLIMIT][SSLIMIT])
{
  register layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);

  if(gi->generalflag)
  {
    if(gi->mixed_block_flag)
    {
      //fprintf(stderr,"Notchecked!");
      layer3reorder_1  (version,frequency,in,out);    // Not checked...
      layer3antialias_1(out);
    }
    else
      layer3reorder_2(version,frequency,in,out);
  }
  else
    layer3antialias_2(in,out);
}

static void dct36(REAL *inbuf,REAL *prevblk1,REAL *prevblk2,REAL *wi,REAL *out)
{
#define MACRO0(v) {                                 \
    REAL tmp;                                       \
    out2[9+(v)]=(tmp=sum0+sum1)*wi[27+(v)];         \
    out2[8-(v)]=tmp * wi[26-(v)];  }                \
    sum0-=sum1;                                     \
    ts[SBLIMIT*(8-(v))]=out1[8-(v)]+sum0*wi[8-(v)]; \
    ts[SBLIMIT*(9+(v))]=out1[9+(v)]+sum0*wi[9+(v)]; 
#define MACRO1(v) { \
    REAL sum0,sum1; \
    sum0=tmp1a+tmp2a; \
    sum1=(tmp1b+tmp2b)*hsec_36[(v)]; \
    MACRO0(v); }
#define MACRO2(v) {                    \
    REAL sum0,sum1;                    \
    sum0=tmp2a-tmp1a;                  \
    sum1=(tmp2b-tmp1b) * hsec_36[(v)]; \
    MACRO0(v); }

  {
    register REAL *in = inbuf;
   
    in[17]+=in[16];in[16]+=in[15];in[15]+=in[14];in[14]+=in[13]; 
    in[13]+=in[12];in[12]+=in[11];in[11]+=in[10];in[10]+=in[ 9];
    in[ 9]+=in[ 8];in[ 8]+=in[ 7];in[ 7]+=in[ 6];in[ 6]+=in[ 5];
    in[ 5]+=in[ 4];in[ 4]+=in[ 3];in[ 3]+=in[ 2];in[ 2]+=in[ 1];
    in[ 1]+=in[ 0];

    in[17]+=in[15];in[15]+=in[13];in[13]+=in[11];in[11]+=in[ 9];
    in[ 9]+=in[ 7];in[7] +=in[ 5];in[ 5]+=in[ 3];in[ 3]+=in[ 1];

    {
      register REAL *c = cos_18;
      register REAL *out2 = prevblk2;
      register REAL *out1 = prevblk1;
      register REAL *ts = out;
      
      REAL ta33,ta66,tb33,tb66;

      ta33=in[2*3+0]*c[3];
      ta66=in[2*6+0]*c[6];
      tb33=in[2*3+1]*c[3];
      tb66=in[2*6+1]*c[6];

      { 
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[1]+ta33          +in[2*5+0]*c[5]+in[2*7+0]*c[7];
	tmp1b=          in[2*1+1]*c[1]+tb33          +in[2*5+1]*c[5]+in[2*7+1]*c[7];
	tmp2a=in[2*0+0]+in[2*2+0]*c[2]+in[2*4+0]*c[4]+ta66          +in[2*8+0]*c[8];
	tmp2b=in[2*0+1]+in[2*2+1]*c[2]+in[2*4+1]*c[4]+tb66          +in[2*8+1]*c[8];
	MACRO1(0);
	MACRO2(8);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=(in[2*1+0]-in[2*5+0]-in[2*7+0])*c[3];
	tmp1b=(in[2*1+1]-in[2*5+1]-in[2*7+1])*c[3];
	tmp2a=(in[2*2+0]-in[2*4+0]-in[2*8+0])*c[6]-in[2*6+0]+in[2*0+0];
	tmp2b=(in[2*2+1]-in[2*4+1]-in[2*8+1])*c[6]-in[2*6+1]+in[2*0+1];
	MACRO1(1);
	MACRO2(7);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[5]-ta33          -in[2*5+0]*c[7]+in[2*7+0]*c[1];
	tmp1b=          in[2*1+1]*c[5]-tb33          -in[2*5+1]*c[7]+in[2*7+1]*c[1];
	tmp2a=in[2*0+0]-in[2*2+0]*c[8]-in[2*4+0]*c[2]+ta66          +in[2*8+0]*c[4];
	tmp2b=in[2*0+1]-in[2*2+1]*c[8]-in[2*4+1]*c[2]+tb66          +in[2*8+1]*c[4];
	MACRO1(2);
	MACRO2(6);
      }

      {
	REAL tmp1a,tmp2a,tmp1b,tmp2b;
	tmp1a=          in[2*1+0]*c[7]-ta33          +in[2*5+0]*c[1]-in[2*7+0]*c[5];
	tmp1b=          in[2*1+1]*c[7]-tb33          +in[2*5+1]*c[1]-in[2*7+1]*c[5];
	tmp2a=in[2*0+0]-in[2*2+0]*c[4]+in[2*4+0]*c[8]+ta66          -in[2*8+0]*c[2];
	tmp2b=in[2*0+1]-in[2*2+1]*c[4]+in[2*4+1]*c[8]+tb66          -in[2*8+1]*c[2];
	MACRO1(3);
	MACRO2(5);
      }

      {
        REAL sum0,sum1;
    	sum0= in[2*0+0]-in[2*2+0]+in[2*4+0]-in[2*6+0]+in[2*8+0];
    	sum1=(in[2*0+1]-in[2*2+1]+in[2*4+1]-in[2*6+1]+in[2*8+1])*hsec_36[4];
	MACRO0(4);
      }
    }
  }
}


static void dct12(REAL *in,REAL *prevblk1,REAL *prevblk2,register REAL *wi,register REAL *out)
{
#define DCT12_PART1   \
        in5=in[5*3];  \
  in5+=(in4=in[4*3]); \
  in4+=(in3=in[3*3]); \
  in3+=(in2=in[2*3]); \
  in2+=(in1=in[1*3]); \
  in1+=(in0=in[0*3]); \
                      \
  in5+=in3;in3+=in1;  \
                      \
  in2*=cos1_6;        \
  in3*=cos1_6;

#define DCT12_PART2         \
  in0+=in4*cos2_6;          \
                            \
  in4=in0+in2;              \
  in0-=in2;                 \
                            \
  in1+=in5*cos2_6;          \
                            \
  in5=(in1+in3)*hsec_12[0]; \
  in1=(in1-in3)*hsec_12[2]; \
                            \
  in3=in4+in5;              \
  in4-=in5;                 \
                            \
  in2=in0+in1;              \
  in0-=in1;

  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb1=prevblk1;
    out[SBLIMIT*0]=pb1[0];out[SBLIMIT*1]=pb1[1];out[SBLIMIT*2]=pb1[2];
    out[SBLIMIT*3]=pb1[3];out[SBLIMIT*4]=pb1[4];out[SBLIMIT*5]=pb1[5];
 
    DCT12_PART1;
    
    {
      REAL tmp0,tmp1=(in0-in4);
      {
	register REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      out[(17-1)*SBLIMIT]=pb1[17-1]+tmp0*wi[11-1];
      out[(12+1)*SBLIMIT]=pb1[12+1]+tmp0*wi[ 6+1];
      out[(6 +1)*SBLIMIT]=pb1[6 +1]+tmp1*wi[ 1  ];
      out[(11-1)*SBLIMIT]=pb1[11-1]+tmp1*wi[ 5-1];
    }

    DCT12_PART2;
    out[(17-0)*SBLIMIT]=pb1[17-0]+in2*wi[11-0];
    out[(12+0)*SBLIMIT]=pb1[12+0]+in2*wi[ 6+0];
    out[(12+2)*SBLIMIT]=pb1[12+2]+in3*wi[ 6+2];
    out[(17-2)*SBLIMIT]=pb1[17-2]+in3*wi[11-2];

    out[( 6+0)*SBLIMIT]=pb1[ 6+0]+in0*wi[0];
    out[(11-0)*SBLIMIT]=pb1[11-0]+in0*wi[5-0];
    out[( 6+2)*SBLIMIT]=pb1[ 6+2]+in4*wi[2];
    out[(11-2)*SBLIMIT]=pb1[11-2]+in4*wi[5-2];
  }

  in++;
  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb2 = prevblk2;
 
    DCT12_PART1;

    {
      REAL tmp0,tmp1=(in0-in4);
      {
	REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      pb2[5-1]=tmp0*wi[11-1];
      pb2[0+1]=tmp0*wi[6+1];
      out[(12+1)*SBLIMIT]+=tmp1*wi[1];
      out[(17-1)*SBLIMIT]+=tmp1*wi[5-1];
    }

    DCT12_PART2;

    pb2[5-0]=in2*wi[11-0];
    pb2[0+0]=in2*wi[6+0];
    pb2[0+2]=in3*wi[6+2];
    pb2[5-2]=in3*wi[11-2];

    out[(12+0)*SBLIMIT]+=in0*wi[0];
    out[(17-0)*SBLIMIT]+=in0*wi[5-0];
    out[(12+2)*SBLIMIT]+=in4*wi[2];
    out[(17-2)*SBLIMIT]+=in4*wi[5-2];
  }

  in++; 
  {
    REAL in0,in1,in2,in3,in4,in5;
    register REAL *pb2 = prevblk2;
    pb2[12]=pb2[13]=pb2[14]=pb2[15]=pb2[16]=pb2[17]=0.0;

    DCT12_PART1;

    {
      REAL tmp0,tmp1=(in0-in4);
      {
	REAL tmp2=(in1-in5)*hsec_12[1];
	tmp0=tmp1+tmp2;
	tmp1-=tmp2;
      }
      pb2[11-1]=tmp0*wi[11-1];
      pb2[ 6+1]=tmp0*wi[6+1];
      pb2[ 0+1]+=tmp1*wi[1];
      pb2[ 5-1]+=tmp1*wi[5-1];
    }

    DCT12_PART2;
    pb2[11-0]=in2*wi[11-0];
    pb2[ 6+0]=in2*wi[ 6+0];
    pb2[ 6+2]=in3*wi[ 6+2];
    pb2[11-2]=in3*wi[11-2];

    pb2[ 0+0]+=in0*wi[0  ];
    pb2[ 5-0]+=in0*wi[5-0];
    pb2[ 0+2]+=in4*wi[2  ];
    pb2[ 5-2]+=in4*wi[5-2];
  }
}

void MPEGaudio::layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
			                   REAL out[SSLIMIT][SBLIMIT])
{
  layer3grinfo *gi=&(sideinfo.ch[ch].gr[gr]);
  int bt1,bt2;
  REAL *prev1,*prev2;

  prev1=prevblck[ch][currentprevblock][0];
  prev2=prevblck[ch][currentprevblock^1][0];

  bt1 = gi->mixed_block_flag ? 0 : gi->block_type;
  bt2 = gi->block_type;

  {
    REAL *ci=(REAL *)in,
         *co=(REAL *)out;
    int  i;

    if(downfrequency)i=(SBLIMIT/2)-2;
    else i=SBLIMIT-2;

    if(bt2==2)
    {
      if(!bt1)
      {
 	dct36(ci,prev1,prev2,win[0],co);
	ci+=SSLIMIT;prev1+=SSLIMIT;prev2+=SSLIMIT;co++;
 	dct36(ci,prev1,prev2,win[0],co);
      }
      else
      {
	dct12(ci,prev1,prev2,win[2],co);
	ci+=SSLIMIT;prev1+=SSLIMIT;prev2+=SSLIMIT;co++;
	dct12(ci,prev1,prev2,win[2],co);
      }

      do{
	ci+=SSLIMIT;prev1+=SSLIMIT;prev2+=SSLIMIT;co++;
	dct12(ci,prev1,prev2,win[2],co);
      }while(--i);
    }
    else
    {
      dct36(ci,prev1,prev2,win[bt1],co);
      ci+=SSLIMIT;prev1+=SSLIMIT;prev2+=SSLIMIT;co++;
      dct36(ci,prev1,prev2,win[bt1],co);

      do
      {
	ci+=SSLIMIT;prev1+=SSLIMIT;prev2+=SSLIMIT;co++;
	dct36(ci,prev1,prev2,win[bt2],co);
      }while(--i);
    }
  }
}

#define NEG(a)  (a)=-(a)

void MPEGaudio::extractlayer3(void)
{
  if(version)
  {
    extractlayer3_2();
    return;
  }

  {
    int main_data_end,flush_main;
    int bytes_to_discard;

    layer3getsideinfo();
	 
    if(issync())
    {
      for(register int i=layer3slots;i>0;i--)  // read main data.
	bitwindow.putbyte(getbyte());
    }
    else
    {
      for(register int i=layer3slots;i>0;i--)  // read main data.
	bitwindow.putbyte(getbits8());
    }

    main_data_end=bitwindow.gettotalbit()>>3;// of previous frame
    if (main_data_end < 0) // Fix from Michael Vogt
    {
      return;
    }

    if((flush_main=(bitwindow.gettotalbit() & 0x7)))
    {
      bitwindow.forward(8-flush_main);
      main_data_end++;
    }

    bytes_to_discard=layer3framestart-(main_data_end+sideinfo.main_data_begin);
    if(main_data_end>WINDOWSIZE)
    {
      layer3framestart-=WINDOWSIZE;
      bitwindow.rewind(WINDOWSIZE*8);
    }
  
    layer3framestart+=layer3slots;
  
    bitwindow.wrap();

    if(bytes_to_discard<0)return;
    bitwindow.forward(bytes_to_discard<<3);
  }

  for(int gr=0;gr<2;gr++)
  {
    union
    {
      int  is      [SBLIMIT][SSLIMIT];
      REAL hin  [2][SBLIMIT][SSLIMIT];
    }b1;
    union
    {
      REAL ro   [2][SBLIMIT][SSLIMIT];
      REAL lr   [2][SBLIMIT][SSLIMIT];
      REAL hout [2][SSLIMIT][SBLIMIT];
    }b2;


      layer3part2start=bitwindow.gettotalbit();
      layer3getscalefactors (LS,gr);
      layer3huffmandecode   (LS,gr      ,b1.is);
      layer3dequantizesample(LS,gr,b1.is,b2.ro[LS]);
    if(inputstereo)
    {
      layer3part2start=bitwindow.gettotalbit();
      layer3getscalefactors (RS,gr);
      layer3huffmandecode   (RS,gr      ,b1.is);
      layer3dequantizesample(RS,gr,b1.is,b2.ro[RS]);
    }

    layer3fixtostereo(gr,b2.ro);   // b2.ro -> b2.lr
    
    currentprevblock^=1;
      layer3reorderandantialias(LS,gr,b2.lr[LS],b1.hin[LS]);
      layer3hybrid (LS,gr,b1.hin[LS],b2.hout[LS]);
    if(outputstereo)
    {
      layer3reorderandantialias(RS,gr,b2.lr[RS],b1.hin[RS]);
      layer3hybrid (RS,gr,b1.hin[RS],b2.hout[RS]);

      register int i=2*SSLIMIT*SBLIMIT-1;
      do{
	NEG(b2.hout[0][0][i   ]);NEG(b2.hout[0][0][i- 2]);
	NEG(b2.hout[0][0][i- 4]);NEG(b2.hout[0][0][i- 6]);
	NEG(b2.hout[0][0][i- 8]);NEG(b2.hout[0][0][i-10]);
	NEG(b2.hout[0][0][i-12]);NEG(b2.hout[0][0][i-14]);
	NEG(b2.hout[0][0][i-16]);NEG(b2.hout[0][0][i-18]);
	NEG(b2.hout[0][0][i-20]);NEG(b2.hout[0][0][i-22]);
	NEG(b2.hout[0][0][i-24]);NEG(b2.hout[0][0][i-26]);
	NEG(b2.hout[0][0][i-28]);NEG(b2.hout[0][0][i-30]);
      }while((i-=2*SBLIMIT)>0);
    }
    else
    {
      register int i=SSLIMIT*SBLIMIT-1;
      do{
	NEG(b2.hout[0][0][i   ]);NEG(b2.hout[0][0][i- 2]);
	NEG(b2.hout[0][0][i- 4]);NEG(b2.hout[0][0][i- 6]);
	NEG(b2.hout[0][0][i- 8]);NEG(b2.hout[0][0][i-10]);
	NEG(b2.hout[0][0][i-12]);NEG(b2.hout[0][0][i-14]);
	NEG(b2.hout[0][0][i-16]);NEG(b2.hout[0][0][i-18]);
	NEG(b2.hout[0][0][i-20]);NEG(b2.hout[0][0][i-22]);
	NEG(b2.hout[0][0][i-24]);NEG(b2.hout[0][0][i-26]);
	NEG(b2.hout[0][0][i-28]);NEG(b2.hout[0][0][i-30]);
      }while((i-=2*SBLIMIT)>0);
    }

    for(int ss=0;ss<SSLIMIT;ss++)
      subbandsynthesis(b2.hout[LS][ss],b2.hout[RS][ss]);
  }
}

void MPEGaudio::extractlayer3_2(void)
{
  {
    int main_data_end,flush_main;
    int bytes_to_discard;

    layer3getsideinfo_2();
	 
    if(issync())
    {
      for(register int i=layer3slots;i>0;i--)  // read main data.
	bitwindow.putbyte(getbyte());
    }
    else
    {
      for(register int i=layer3slots;i>0;i--)  // read main data.
	bitwindow.putbyte(getbits8());
    }
    bitwindow.wrap();

    main_data_end=bitwindow.gettotalbit()>>3;// of previous frame

    if((flush_main=(bitwindow.gettotalbit() & 0x7)))
    {
      bitwindow.forward(8-flush_main);
      main_data_end++;
    }

    bytes_to_discard=layer3framestart-main_data_end-sideinfo.main_data_begin;
    if(main_data_end>WINDOWSIZE)
    {
      layer3framestart-=WINDOWSIZE;
      bitwindow.rewind(WINDOWSIZE*8);
    }
  
    layer3framestart+=layer3slots;
  
    if(bytes_to_discard<0) return;
    bitwindow.forward(bytes_to_discard<<3);
  }

  //  for(int gr=0;gr<2;gr++)
  {
    union
    {
      int  is      [SBLIMIT][SSLIMIT];
      REAL hin  [2][SBLIMIT][SSLIMIT];
    }b1;
    union
    {
      REAL ro   [2][SBLIMIT][SSLIMIT];
      REAL lr   [2][SBLIMIT][SSLIMIT];
      REAL hout [2][SSLIMIT][SBLIMIT];
    }b2;


      layer3part2start=bitwindow.gettotalbit();
      layer3getscalefactors_2(LS);
      layer3huffmandecode    (LS,0      ,b1.is);
      layer3dequantizesample (LS,0,b1.is,b2.ro[LS]);
    if(inputstereo)
    {
      layer3part2start=bitwindow.gettotalbit();
      layer3getscalefactors_2(RS);
      layer3huffmandecode    (RS,0      ,b1.is);
      layer3dequantizesample (RS,0,b1.is,b2.ro[RS]);
    }

    layer3fixtostereo(0,b2.ro);          // b2.ro -> b2.lr
    
    currentprevblock^=1;
      layer3reorderandantialias(LS,0,b2.lr[LS],b1.hin[LS]);
      layer3hybrid (LS,0,b1.hin[LS],b2.hout[LS]);
    if(outputstereo)
    {
      layer3reorderandantialias(RS,0,b2.lr[RS],b1.hin[RS]);
      layer3hybrid (RS,0,b1.hin[RS],b2.hout[RS]);

      register int i=2*SSLIMIT*SBLIMIT-1;
      do{
	NEG(b2.hout[0][0][i-16]);NEG(b2.hout[0][0][i-18]);
	NEG(b2.hout[0][0][i-20]);NEG(b2.hout[0][0][i-22]);
	NEG(b2.hout[0][0][i-24]);NEG(b2.hout[0][0][i-26]);
	NEG(b2.hout[0][0][i-28]);NEG(b2.hout[0][0][i-30]);
      }while((i-=2*SBLIMIT)>0);
    }
    else
    {
      register int i=SSLIMIT*SBLIMIT-1;
      do{
	NEG(b2.hout[0][0][i-16]);NEG(b2.hout[0][0][i-18]);
	NEG(b2.hout[0][0][i-20]);NEG(b2.hout[0][0][i-22]);
	NEG(b2.hout[0][0][i-24]);NEG(b2.hout[0][0][i-26]);
	NEG(b2.hout[0][0][i-28]);NEG(b2.hout[0][0][i-30]);
      }while((i-=2*SBLIMIT)>0);
    }

    for(int ss=0;ss<SSLIMIT;ss++)
      subbandsynthesis(b2.hout[LS][ss],b2.hout[RS][ss]);
  }
}
