
#ifndef NEW_PLC
#define NEW_PLC
#endif

#ifdef FIXED_POINT
#define frac_div(a,b) ((celt_word32)((32768*65535+32767)*(float)(a)/(b)))
#else
#define frac_div(a,b) ((float)(a)/(b))
#endif


float _celt_lpc(
      celt_word16       *_lpc, /* out: [0...p-1] LPC coefficients      */
const celt_word32 *ac,  /* in:  [0...p] autocorrelation values  */
int          p
)
{
   int i, j;  
   celt_word32 r;
   celt_word32 error = ac[0];
#ifdef FIXED_POINT
   celt_word32 lpc[LPC_ORDER];
#else
   float *lpc = _lpc;
#endif

   for (i = 0; i < p; i++)
      lpc[i] = 0;
   if (ac[0] != 0)
   {
      for (i = 0; i < p; i++) {
         /* Sum up this iteration's reflection coefficient */
         celt_word32 rr = 0;
         for (j = 0; j < i; j++)
            rr += MULT32_32_Q31(lpc[j],ac[i - j]);
         rr += SHR32(ac[i + 1],3);
         //r = -RC_SCALING*1.*SHL32(rr,3)/(error+1e-15);
         r = -frac_div(SHL32(rr,3), error);
         /*  Update LPC coefficients and total error */
         lpc[i] = SHR32(r,3);
         for (j = 0; j < (i+1)>>1; j++)
         {
            celt_word32 tmp1, tmp2;
            tmp1 = lpc[j];
            tmp2 = lpc[i-1-j];
            lpc[j]     = tmp1 + MULT32_32_Q31(r,tmp2);
            lpc[i-1-j] = tmp2 + MULT32_32_Q31(r,tmp1);
         }

         error = error - MULT32_32_Q31(MULT32_32_Q31(r,r),error);
         if (error<.00001*ac[0])
            break;
      }
   }
#ifdef FIXED_POINT
   for (i=0;i<p;i++)
      _lpc[i] = ROUND16(lpc[i],16);
#endif
   return error;
}

void fir(const celt_word16 *x,
         const celt_word16 *num,
         celt_word16 *y,
         int N,
         int ord,
         celt_word16 *mem)
{
   int i,j;

   for (i=0;i<N;i++)
   {
      celt_word32 sum = SHL32(EXTEND32(x[i]), SIG_SHIFT);
      for (j=0;j<ord;j++)
      {
         sum += MULT16_16(num[j],mem[j]);
      }
      for (j=ord-1;j>=1;j--)
      {
         mem[j]=mem[j-1];
      }
      mem[0] = x[i];
      y[i] = ROUND16(sum, SIG_SHIFT);
   }
}

void iir(const celt_word32 *x,
         const celt_word16 *den,
         celt_word32 *y,
         int N,
         int ord,
         celt_word16 *mem)
{
   int i,j;
   for (i=0;i<N;i++)
   {
      celt_word32 sum = x[i];
      for (j=0;j<ord;j++)
      {
         sum -= MULT16_16(den[j],mem[j]);
      }
      for (j=ord-1;j>=1;j--)
      {
         mem[j]=mem[j-1];
      }
      mem[0] = ROUND16(sum,SIG_SHIFT);
      y[i] = sum;
   }
}

void _celt_autocorr(
                   const celt_word16 *x,   /*  in: [0...n-1] samples x   */
                   celt_word32       *ac,  /* out: [0...lag-1] ac values */
                   const celt_word16       *window,
                   int          overlap,
                   int          lag, 
                   int          n
                  )
{
   float d;
   int i;
   float scale=1;
   VARDECL(float, xx);
   SAVE_STACK;
   ALLOC(xx, n, float);
   for (i=0;i<n;i++)
      xx[i] = x[i];
   for (i=0;i<overlap;i++)
   {
      xx[i] *= (1./Q15ONE)*window[i];
      xx[n-i-1] *= (1./Q15ONE)*window[i];
   }
#ifdef FIXED_POINT
   {
      float ac0=0;
      for(i=0;i<n;i++)
         ac0 += x[i]*x[i];
      ac0+=10;
      scale = 2000000000/ac0;
   }
#endif
   while (lag>=0)
   {
      for (i = lag, d = 0; i < n; i++) 
         d += x[i] * x[i-lag];
      ac[lag] = d*scale;
      /*printf ("%f ", ac[lag]);*/
      lag--;
   }
   /*printf ("\n");*/
   ac[0] += 10;

   RESTORE_STACK;
}
