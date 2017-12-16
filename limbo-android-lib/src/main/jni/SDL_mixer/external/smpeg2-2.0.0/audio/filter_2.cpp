/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Filter.cc
// Subbandsynthesis routines from maplay 1.2 for Linux
// I've modified some macros for reducing source code.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "MPEGaudio.h"

void MPEGaudio::computebuffer_2(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE])
{
  REAL p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,pa,pb,pc,pd,pe,pf;
  REAL q0,q1,q2,q3,q4,q5,q6,q7,q8,q9,qa,qb,qc,qd,qe,qf;
  REAL *out1,*out2;

  out1=buffer[currentcalcbuffer]+calcbufferoffset;
  out2=buffer[currentcalcbuffer^1]+calcbufferoffset;
#define OUT1(v,t) out1[(32-(v))*16]   =(-(out1[(v)*16]=t))
#define OUT2(v)   out2[(96-(v)-32)*16]=out2[((v)-32)*16]

  // compute new values via a fast cosine transform:
  /*  {
    register REAL *x=fraction;

    p0=x[ 0]+x[31];p1=x[ 1]+x[30];p2=x[ 2]+x[29];p3=x[ 3]+x[28];
    p4=x[ 4]+x[27];p5=x[ 5]+x[26];p6=x[ 6]+x[25];p7=x[ 7]+x[24];
    p8=x[ 8]+x[23];p9=x[ 9]+x[22];pa=x[10]+x[21];pb=x[11]+x[20];
    pc=x[12]+x[19];pd=x[13]+x[18];pe=x[14]+x[17];pf=x[15]+x[16];
  }

  q0=p0+pf;q1=p1+pe;q2=p2+pd;q3=p3+pc;
  q4=p4+pb;q5=p5+pa;q6=p6+p9;q7=p7+p8;
  q8=hcos_32[0]*(p0-pf);q9=hcos_32[1]*(p1-pe);
  qa=hcos_32[2]*(p2-pd);qb=hcos_32[3]*(p3-pc);
  qc=hcos_32[4]*(p4-pb);qd=hcos_32[5]*(p5-pa);
  qe=hcos_32[6]*(p6-p9);qf=hcos_32[7]*(p7-p8); */

  {
    register REAL *x=fraction;

    q0=x[ 0]+x[15];q1=x[ 1]+x[14];q2=x[ 2]+x[13];q3=x[ 3]+x[12];
    q4=x[ 4]+x[11];q5=x[ 5]+x[10];q6=x[ 6]+x[ 9];q7=x[ 7]+x[ 8];

    q8=hcos_32[0]*(x[ 0]-x[15]);q9=hcos_32[1]*(x[ 1]-x[14]);
    qa=hcos_32[2]*(x[ 2]-x[13]);qb=hcos_32[3]*(x[ 3]-x[12]);
    qc=hcos_32[4]*(x[ 4]-x[11]);qd=hcos_32[5]*(x[ 5]-x[10]);
    qe=hcos_32[6]*(x[ 6]-x[ 9]);qf=hcos_32[7]*(x[ 7]-x[ 8]);
  }

  p0=q0+q7;p1=q1+q6;p2=q2+q5;p3=q3+q4;
  p4=hcos_16[0]*(q0-q7);p5=hcos_16[1]*(q1-q6);
  p6=hcos_16[2]*(q2-q5);p7=hcos_16[3]*(q3-q4);
  p8=q8+qf;p9=q9+qe;pa=qa+qd;pb=qb+qc;
  pc=hcos_16[0]*(q8-qf);pd=hcos_16[1]*(q9-qe);
  pe=hcos_16[2]*(qa-qd);pf=hcos_16[3]*(qb-qc);

  q0=p0+p3;q1=p1+p2;q2=hcos_8[0]*(p0-p3);q3=hcos_8[1]*(p1-p2);
  q4=p4+p7;q5=p5+p6;q6=hcos_8[0]*(p4-p7);q7=hcos_8[1]*(p5-p6);
  q8=p8+pb;q9=p9+pa;qa=hcos_8[0]*(p8-pb);qb=hcos_8[1]*(p9-pa);
  qc=pc+pf;qd=pd+pe;qe=hcos_8[0]*(pc-pf);qf=hcos_8[1]*(pd-pe);

  p0=q0+q1;p1=hcos_4*(q0-q1);p2=q2+q3;p3=hcos_4*(q2-q3);
  p4=q4+q5;p5=hcos_4*(q4-q5);p6=q6+q7;p7=hcos_4*(q6-q7);
  p8=q8+q9;p9=hcos_4*(q8-q9);pa=qa+qb;pb=hcos_4*(qa-qb);
  pc=qc+qd;pd=hcos_4*(qc-qd);pe=qe+qf;pf=hcos_4*(qe-qf);

  {
    register REAL tmp;

    tmp=p6+p7;
    OUT2(36)=-(p5+tmp);
    OUT2(44)=-(p4+tmp);
    tmp=pb+pf;
    OUT1(10,tmp);
    OUT1(6,pd+tmp);
    tmp=pe+pf;
    OUT2(46)=-(p8+pc+tmp);
    OUT2(34)=-(p9+pd+tmp);
    tmp+=pa+pb;
    OUT2(38)=-(pd+tmp);
    OUT2(42)=-(pc+tmp);
    OUT1(2,p9+pd+pf);
    OUT1(4,p5+p7);
    OUT2(48)=-p0;
    out2[0]=-(out1[0]=p1);
    OUT1( 8,p3);
    OUT1(12,p7);
    OUT1(14,pf);
    OUT2(40)=-(p2+p3);
  }

  {
    register REAL *x=fraction;

    /*    p0=hcos_64[ 0]*(x[ 0]-x[31]);p1=hcos_64[ 1]*(x[ 1]-x[30]);
    p2=hcos_64[ 2]*(x[ 2]-x[29]);p3=hcos_64[ 3]*(x[ 3]-x[28]);
    p4=hcos_64[ 4]*(x[ 4]-x[27]);p5=hcos_64[ 5]*(x[ 5]-x[26]);
    p6=hcos_64[ 6]*(x[ 6]-x[25]);p7=hcos_64[ 7]*(x[ 7]-x[24]);
    p8=hcos_64[ 8]*(x[ 8]-x[23]);p9=hcos_64[ 9]*(x[ 9]-x[22]);
    pa=hcos_64[10]*(x[10]-x[21]);pb=hcos_64[11]*(x[11]-x[20]);
    pc=hcos_64[12]*(x[12]-x[19]);pd=hcos_64[13]*(x[13]-x[18]);
    pe=hcos_64[14]*(x[14]-x[17]);pf=hcos_64[15]*(x[15]-x[16]); */

    p0=hcos_64[ 0]*x[ 0];p1=hcos_64[ 1]*x[ 1];
    p2=hcos_64[ 2]*x[ 2];p3=hcos_64[ 3]*x[ 3];
    p4=hcos_64[ 4]*x[ 4];p5=hcos_64[ 5]*x[ 5];
    p6=hcos_64[ 6]*x[ 6];p7=hcos_64[ 7]*x[ 7];
    p8=hcos_64[ 8]*x[ 8];p9=hcos_64[ 9]*x[ 9];
    pa=hcos_64[10]*x[10];pb=hcos_64[11]*x[11];
    pc=hcos_64[12]*x[12];pd=hcos_64[13]*x[13];
    pe=hcos_64[14]*x[14];pf=hcos_64[15]*x[15];
  }

  q0=p0+pf;q1=p1+pe;q2=p2+pd;q3=p3+pc;
  q4=p4+pb;q5=p5+pa;q6=p6+p9;q7=p7+p8;
  q8=hcos_32[0]*(p0-pf);q9=hcos_32[1]*(p1-pe);
  qa=hcos_32[2]*(p2-pd);qb=hcos_32[3]*(p3-pc);
  qc=hcos_32[4]*(p4-pb);qd=hcos_32[5]*(p5-pa);
  qe=hcos_32[6]*(p6-p9);qf=hcos_32[7]*(p7-p8);

  p0=q0+q7;p1=q1+q6;p2=q2+q5;p3=q3+q4;
  p4=hcos_16[0]*(q0-q7);p5=hcos_16[1]*(q1-q6);
  p6=hcos_16[2]*(q2-q5);p7=hcos_16[3]*(q3-q4);
  p8=q8+qf;p9=q9+qe;pa=qa+qd;pb=qb+qc;
  pc=hcos_16[0]*(q8-qf);pd=hcos_16[1]*(q9-qe);
  pe=hcos_16[2]*(qa-qd);pf=hcos_16[3]*(qb-qc);

  q0=p0+p3;q1=p1+p2;q2=hcos_8[0]*(p0-p3);q3=hcos_8[1]*(p1-p2);
  q4=p4+p7;q5=p5+p6;q6=hcos_8[0]*(p4-p7);q7=hcos_8[1]*(p5-p6);
  q8=p8+pb;q9=p9+pa;qa=hcos_8[0]*(p8-pb);qb=hcos_8[1]*(p9-pa);
  qc=pc+pf;qd=pd+pe;qe=hcos_8[0]*(pc-pf);qf=hcos_8[1]*(pd-pe);

  p0=q0+q1;p1=hcos_4*(q0-q1);
  p2=q2+q3;p3=hcos_4*(q2-q3);
  p4=q4+q5;p5=hcos_4*(q4-q5);
  p6=q6+q7;p7=hcos_4*(q6-q7);
  p8=q8+q9;p9=hcos_4*(q8-q9);
  pa=qa+qb;pb=hcos_4*(qa-qb);
  pc=qc+qd;pd=hcos_4*(qc-qd);
  pe=qe+qf;pf=hcos_4*(qe-qf);

  {
    REAL tmp;

    tmp=pd+pf;
    OUT1(5,p5+p7+pb+tmp);
    tmp+=p9;
    OUT1(1,p1+tmp);
    OUT2(33)=-(p1+pe+tmp);
    tmp+=p5+p7;
    OUT1(3,tmp);
    OUT2(35)=-(p6+pe+tmp);
    tmp=pa+pb+pc+pd+pe+pf;
    OUT2(39)=-(p2+p3+tmp-pc);
    OUT2(43)=-(p4+p6+p7+tmp-pd);
    OUT2(37)=-(p5+p6+p7+tmp-pc);
    OUT2(41)=-(p2+p3+tmp-pd);
    tmp=p8+pc+pe+pf;
    OUT2(47)=-(p0+tmp);
    OUT2(45)=-(p4+p6+p7+tmp);
    tmp=pb+pf;
    OUT1(11,p7+tmp);
    tmp+=p3;
    OUT1( 9,tmp);
    OUT1( 7,pd+tmp);
    OUT1(13,p7+pf);
    OUT1(15,pf);
  }
}


#define SAVE \
        raw=(int)(r*scalefactor); \
        if(raw>MAXSCALE)raw=MAXSCALE;else if(raw<MINSCALE)raw=MINSCALE; \
	putraw(raw); \
        dp+=16;vp+=15+(15-14)
#define OS   r=*vp * *dp++
#define XX   vp+=15;r+=*vp * *dp++
#define OP   r+=*--vp * *dp++

void MPEGaudio::generatesingle_2(void)
{
  int i;
  register REAL r, *vp;
  register const REAL *dp;
  int raw;

  i=32/2;
  dp=filter;
  vp=calcbufferL[currentcalcbuffer]+calcbufferoffset; 
// actual_v+actual_write_pos;

  switch (calcbufferoffset)
  {
    case  0:for(;i;i--,vp+=15){
              OS;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  1:for(;i;i--,vp+=15){
	      OS;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  2:for(;i;i--,vp+=15){
              OS;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  3:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  4:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  5:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  6:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  7:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  8:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  9:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case 10:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;
	      SAVE;}break;
    case 11:for(;i;i--,vp+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;
	      SAVE;}break;
    case 12:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;
	      SAVE;}break;
    case 13:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;
	      SAVE;}break;
    case 14:for(;i;i--,vp+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;
	      SAVE;}break;
    case 15:for(;i;i--,vp+=31){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
  }
}

#undef OS
#undef XX
#undef OP
#undef SAVE

#define SAVE \
        raw=(int)(r1*scalefactor);  \
        if(raw>MAXSCALE)raw=MAXSCALE;else if(raw<MINSCALE)raw=MINSCALE; \
	putraw(raw);  \
        raw=(int)(r2*scalefactor);  \
        if(raw>MAXSCALE)raw=MAXSCALE;else if(raw<MINSCALE)raw=MINSCALE; \
	putraw(raw); \
        dp+=16;vp1+=15+(15-14);vp2+=15+(15-14)
#define OS r1=*vp1 * *dp; \
           r2=*vp2 * *dp++ 
#define XX vp1+=15;r1+=*vp1 * *dp; \
	   vp2+=15;r2+=*vp2 * *dp++
#define OP r1+=*--vp1 * *dp; \
	   r2+=*--vp2 * *dp++


void MPEGaudio::generate_2(void)
{
  int i;
  REAL r1,r2;
  register REAL *vp1,*vp2;
  register const REAL *dp;
  int raw;

  dp=filter;
  vp1=calcbufferL[currentcalcbuffer]+calcbufferoffset;
  vp2=calcbufferR[currentcalcbuffer]+calcbufferoffset;
// actual_v+actual_write_pos;

  i=32/2;
  switch (calcbufferoffset)
  {
    case  0:for(;i;i--,vp1+=15,vp2+=15){
              OS;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  1:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  2:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  3:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  4:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  5:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  6:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  7:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  8:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case  9:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;OP;
	      SAVE;}break;
    case 10:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;OP;
	      SAVE;}break;
    case 11:for(;i;i--,vp1+=15,vp2+=15){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;OP;
	      SAVE;}break;
    case 12:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;OP;
	      SAVE;}break;
    case 13:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;OP;
	      SAVE;}break;
    case 14:for(;i;i--,vp1+=15,vp2+=15){
	      OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;XX;
	      SAVE;}break;
    case 15:for(;i;i--,vp1+=31,vp2+=31){
              OS;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;OP;
	      SAVE;}break;
  }
}


void MPEGaudio::subbandsynthesis_2(REAL *fractionL,REAL *fractionR)
{
  computebuffer_2(fractionL,calcbufferL);
  if(!outputstereo)generatesingle_2();
  else
  {
    computebuffer_2(fractionR,calcbufferR);
    generate_2();
  }

  if(calcbufferoffset<15)calcbufferoffset++;
  else calcbufferoffset=0;

  currentcalcbuffer^=1;
}
