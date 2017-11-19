#include <string.h>
#include <time.h>
#include <math.h>
#include <gmp.h>
#include "ptypes.h"

#include "gmp_main.h"
#include "primality.h"
#include "prime_iterator.h"
#include "ecpp.h"
#include "factor.h"

#define FUNC_gcd_ui 1
#define FUNC_mpz_logn 1
#include "utility.h"

static mpz_t _bgcd;
static mpz_t _bgcd2;
static mpz_t _bgcd3;
#define BGCD_PRIMES       168
#define BGCD_LASTPRIME    997
#define BGCD_NEXTPRIME   1009
#define BGCD2_PRIMES     1229
#define BGCD2_NEXTPRIME 10007
#define BGCD3_PRIMES     4203
#define BGCD3_NEXTPRIME 40009

#define NSMALLPRIMES 168
static const unsigned short sprimes[NSMALLPRIMES] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,127,131,137,139,149,151,157,163,167,173,179,181,191,193,197,199,211,223,227,229,233,239,241,251,257,263,269,271,277,281,283,293,307,311,313,317,331,337,347,349,353,359,367,373,379,383,389,397,401,409,419,421,431,433,439,443,449,457,461,463,467,479,487,491,499,503,509,521,523,541,547,557,563,569,571,577,587,593,599,601,607,613,617,619,631,641,643,647,653,659,661,673,677,683,691,701,709,719,727,733,739,743,751,757,761,769,773,787,797,809,811,821,823,827,829,839,853,857,859,863,877,881,883,887,907,911,919,929,937,941,947,953,967,971,977,983,991,997};

#define TSTAVAL(arr, val)   (arr[(val) >> 6] & (1U << (((val)>>1) & 0x1F)))
#define SETAVAL(arr, val)   arr[(val) >> 6] |= 1U << (((val)>>1) & 0x1F)


void _GMP_init(void)
{
  /* For real use of randomness we need to be seeded properly.
   * This gives us a start until someone calls seed_csprng().
   * We could try to improve this duct-tape in various ways. */
  unsigned long seed = time(NULL);
  init_randstate(seed);
  prime_iterator_global_startup();
  mpz_init(_bgcd);
  _GMP_pn_primorial(_bgcd, BGCD_PRIMES);   /* mpz_primorial_ui(_bgcd, 1000) */
  mpz_init_set_ui(_bgcd2, 0);
  mpz_init_set_ui(_bgcd3, 0);
  _init_factor();
}

void _GMP_destroy(void)
{
  prime_iterator_global_shutdown();
  clear_randstate();
  mpz_clear(_bgcd);
  mpz_clear(_bgcd2);
  mpz_clear(_bgcd3);
  destroy_ecpp_gcds();
}


static const unsigned char next_wheel[30] =
  {1,7,7,7,7,7,7,11,11,11,11,13,13,17,17,17,17,19,19,23,23,23,23,29,29,29,29,29,29,1};
static const unsigned char prev_wheel[30] =
  {29,29,1,1,1,1,1,1,7,7,7,7,11,11,13,13,13,13,17,17,19,19,19,19,23,23,23,23,23,23};
static const unsigned char wheel_advance[30] =
  {1,6,5,4,3,2,1,4,3,2,1,2,1,4,3,2,1,2,1,4,3,2,1,6,5,4,3,2,1,2};
static const unsigned char wheel_retreat[30] =
  {1,2,1,2,3,4,5,6,1,2,3,4,1,2,1,2,3,4,1,2,1,2,3,4,1,2,3,4,5,6};



/*****************************************************************************/

static int is_tiny_prime(uint32_t n) {
  uint32_t f, limit;
  if (n < 11) {
    if (n == 2 || n == 3 || n == 5 || n == 7)      return 2;
    else                                           return 0;
  }
  if (!(n%2) || !(n%3) || !(n%5) || !(n%7))        return 0;
  if (n <  121) /* 11*11 */                        return 2;
  if (!(n%11) || !(n%13) || !(n%17) || !(n%19) ||
       !(n%23) || !(n%29) || !(n%31) || !(n%37) ||
       !(n%41) || !(n%43) || !(n%47) || !(n%53))   return 0;
  if (n < 3481) /* 59*59 */                        return 2;

  f = 59;
  limit = (uint32_t) (sqrt((double)n));
  while (f <= limit) {
    if ( !(n%f) || !(n%(f+2)) || !(n%(f+8)) || !(n%(f+12)) ) return 0;  f += 14;
    if ( !(n%f) || !(n%(f+4)) || !(n%(f+6)) || !(n%(f+10)) ) return 0;  f += 16;
  }
  return 2;
}

int primality_pretest(mpz_t n)
{
  if (mpz_cmp_ui(n, 100000) < 0)
    return is_tiny_prime((uint32_t)mpz_get_ui(n));

  /* Check for tiny divisors */
  if (mpz_even_p(n)) return 0;
  if (sizeof(unsigned long) < 8) {
    if (mpz_gcd_ui(NULL, n, 3234846615UL) != 1) return 0;           /*  3-29 */
  } else {
    if (mpz_gcd_ui(NULL, n, 4127218095UL*3948078067UL)!=1) return 0;/*  3-53 */
    if (mpz_gcd_ui(NULL, n, 4269855901UL*1673450759UL)!=1) return 0;/* 59-101 */
  }

  {
    UV log2n = mpz_sizeinbase(n,2);
    mpz_t t;
    mpz_init(t);

    /* Do a GCD with all primes < 1009 */
    mpz_gcd(t, n, _bgcd);
    if (mpz_cmp_ui(t, 1))
      { mpz_clear(t); return 0; }

    /* No divisors under 1009 */
    if (mpz_cmp_ui(n, BGCD_NEXTPRIME*BGCD_NEXTPRIME) < 0)
      { mpz_clear(t); return 2; }

    /* If we're reasonably large, do a gcd with more primes */
    if (log2n > 700) {
      if (mpz_sgn(_bgcd3) == 0) {
        _GMP_pn_primorial(_bgcd3, BGCD3_PRIMES);
        mpz_divexact(_bgcd3, _bgcd3, _bgcd);
      }
      mpz_gcd(t, n, _bgcd3);
      if (mpz_cmp_ui(t, 1))
        { mpz_clear(t); return 0; }
    } else if (log2n > 300) {
      if (mpz_sgn(_bgcd2) == 0) {
        _GMP_pn_primorial(_bgcd2, BGCD2_PRIMES);
        mpz_divexact(_bgcd2, _bgcd2, _bgcd);
      }
      mpz_gcd(t, n, _bgcd2);
      if (mpz_cmp_ui(t, 1))
        { mpz_clear(t); return 0; }
    }
    mpz_clear(t);
    /* Do more trial division if we think we should.
     * According to Menezes (section 4.45) as well as Park (ISPEC 2005),
     * we want to select a trial limit B such that B = E/D where E is the
     * time for our primality test (one M-R test) and D is the time for
     * one trial division.  Example times on my machine came out to
     *   log2n = 840375, E= 6514005000 uS, D=1.45 uS, E/D = 0.006 * log2n
     *   log2n = 465618, E= 1815000000 uS, D=1.05 uS, E/D = 0.008 * log2n
     *   log2n = 199353, E=  287282000 uS, D=0.70 uS, E/D = 0.01  * log2n
     *   log2n =  99678, E=   56956000 uS, D=0.55 uS, E/D = 0.01  * log2n
     *   log2n =  33412, E=    4289000 uS, D=0.30 uS, E/D = 0.013 * log2n
     *   log2n =  13484, E=     470000 uS, D=0.21 uS, E/D = 0.012 * log2n
     * Our trial division could also be further improved for large inputs.
     */
    if (log2n > 16000) {
      double dB = (double)log2n * (double)log2n * 0.005;
      if (BITS_PER_WORD == 32 && dB > 4200000000.0) dB = 4200000000.0;
      if (_GMP_trial_factor(n, BGCD3_NEXTPRIME, (UV)dB))  return 0;
    } else if (log2n > 4000) {
      if (_GMP_trial_factor(n, BGCD3_NEXTPRIME, 80*log2n))  return 0;
    } else if (log2n > 1600) {
      if (_GMP_trial_factor(n, BGCD3_NEXTPRIME, 30*log2n))  return 0;
    }
  }
  return 1;
}


/*****************************************************************************/

/* Controls how many numbers to sieve.  Little time impact. */
#define NPS_MERIT  30.0
/* Controls how many primes to use.  Big time impact. */
#define NPS_DEPTH(log2n, log2log2n) \
  (log2n < 100) ? 1000 : \
  (BITS_PER_WORD == 32 && log2n > 9000U) ? UVCONST(2500000000) : \
  (BITS_PER_WORD == 64 && log2n > 4294967294U) ? UVCONST(9300000000000000000) :\
  ((log2n * (log2n >> 5) * (UV)((log2log2n)*1.5)) >> 1)

static void next_prime_with_sieve(mpz_t n) {
  UV i, log2n, log2log2n, width, depth;
  uint32_t* comp;
  mpz_t t, base;
  log2n = mpz_sizeinbase(n, 2);
  for (log2log2n = 1, i = log2n; i >>= 1; ) log2log2n++;
  width = (UV) (NPS_MERIT/1.4427 * (double)log2n + 0.5);
  depth = NPS_DEPTH(log2n, log2log2n);

  if (width & 1) width++;                     /* Make width even */
  mpz_add_ui(n, n, mpz_even_p(n) ? 1 : 2);    /* Set n to next odd */
  mpz_init(t);  mpz_init(base);
  while (1) {
    mpz_set(base, n);
    comp = partial_sieve(base, width, depth); /* sieve range to depth */
    for (i = 1; i <= width; i += 2) {
      if (!TSTAVAL(comp, i)) {
        mpz_add_ui(t, base, i);               /* We found a candidate */
        if (_GMP_BPSW(t)) {
          mpz_set(n, t);
          mpz_clear(t);  mpz_clear(base);
          Safefree(comp);
          return;
        }
      }
    }
    Safefree(comp);       /* A huge gap found, so sieve another range */
    mpz_add_ui(n, n, width);
  }
}

static void prev_prime_with_sieve(mpz_t n) {
  UV i, j, log2n, log2log2n, width, depth;
  uint32_t* comp;
  mpz_t t, base;
  log2n = mpz_sizeinbase(n, 2);
  for (log2log2n = 1, i = log2n; i >>= 1; ) log2log2n++;
  width = (UV) (NPS_MERIT/1.4427 * (double)log2n + 0.5);
  depth = NPS_DEPTH(log2n, log2log2n);

  mpz_sub_ui(n, n, mpz_even_p(n) ? 1 : 2);       /* Set n to prev odd */
  width = 64 * ((width+63)/64);                /* Round up to next 64 */
  mpz_init(t);  mpz_init(base);
  while (1) {
    mpz_sub_ui(base, n, width-2);
    /* gmp_printf("sieve from %Zd to %Zd width %lu\n", base, n, width); */
    comp = partial_sieve(base, width, depth); /* sieve range to depth */
    /* if (mpz_odd_p(base)) croak("base off after partial");
       if (width & 1) croak("width is odd after partial"); */
    for (j = 1; j < width; j += 2) {
      i = width - j;
      if (!TSTAVAL(comp, i)) {
        mpz_add_ui(t, base, i);               /* We found a candidate */
        if (_GMP_BPSW(t)) {
          mpz_set(n, t);
          mpz_clear(t);  mpz_clear(base);
          Safefree(comp);
          return;
        }
      }
    }
    Safefree(comp);       /* A huge gap found, so sieve another range */
    mpz_sub_ui(n, n, width);
  }
}

/* Modifies argument */
void _GMP_next_prime(mpz_t n)
{
  if (mpz_cmp_ui(n, 29) < 0) { /* small inputs */

    UV m = mpz_get_ui(n);
    m = (m < 2) ? 2 : (m < 3) ? 3 : (m < 5) ? 5 : next_wheel[m];
    mpz_set_ui(n, m);

  } else if (mpz_sizeinbase(n,2) > 120) {

    next_prime_with_sieve(n);

  } else {

    UV m23 = mpz_fdiv_ui(n, 223092870UL);  /* 2*3*5*7*11*13*17*19*23 */
    UV m = m23 % 30;
    do {
      UV skip = wheel_advance[m];
      mpz_add_ui(n, n, skip);
      m23 += skip;
      m = next_wheel[m];
    } while ( !(m23% 7) || !(m23%11) || !(m23%13) || !(m23%17) ||
              !(m23%19) || !(m23%23) || !_GMP_is_prob_prime(n) );

  }
}

/* Modifies argument */
void _GMP_prev_prime(mpz_t n)
{
  UV m, m23;
  if (mpz_cmp_ui(n, 29) <= 0) { /* small inputs */

    m = mpz_get_ui(n);
    m = (m < 3) ? 0 : (m < 4) ? 2 : (m < 6) ? 3 : (m < 8) ? 5 : prev_wheel[m];
    mpz_set_ui(n, m);

  } else if (mpz_sizeinbase(n,2) > 200) {

    prev_prime_with_sieve(n);

  } else {

    m23 = mpz_fdiv_ui(n, 223092870UL);  /* 2*3*5*7*11*13*17*19*23 */
    m = m23 % 30;
    m23 += 223092870UL;  /* No need to re-mod inside the loop */
    do {
      UV skip = wheel_retreat[m];
      mpz_sub_ui(n, n, skip);
      m23 -= skip;
      m = prev_wheel[m];
    } while ( !(m23% 7) || !(m23%11) || !(m23%13) || !(m23%17) ||
              !(m23%19) || !(m23%23) || !_GMP_is_prob_prime(n) );

  }
}

void surround_primes(mpz_t n, UV* prev, UV* next, UV skip_width) {
  UV i, j, log2n, log2log2n, width, depth, fprev, fnext, search_merits;
  uint32_t* comp;
  mpz_t t, base;
  int neven, found;

  log2n = mpz_sizeinbase(n, 2);
  for (log2log2n = 1, i = log2n; i >>= 1; ) log2log2n++;

  if (log2n < 64) {
    mpz_init(t);
    mpz_set(t, n);
    _GMP_prev_prime(t);
    mpz_sub(t, n, t);
    *prev = mpz_get_ui(t);
    mpz_set(t, n);
    _GMP_next_prime(t);
    mpz_sub(t, t, n);
    *next = mpz_get_ui(t);
    mpz_clear(t);
    return;
  }

  mpz_init(t);
  mpz_init(base);
  fprev = fnext = 0;
  neven = mpz_even_p(n);
  j = 1 + !neven;         /* distance from n we're looking. */

  for (found = 0, search_merits = 20; !found; search_merits *= 2) {
    double logn = mpz_logn(n);

    if (BITS_PER_WORD == 32 && log2n >   7000)
      depth = UVCONST(2500000000);
    else if (BITS_PER_WORD == 64 && log2n > 200000)
      depth = UVCONST(6000000000000);
    else if (log2n > 900)
      depth = (UV) ((-.05L+(log2n/8000.0L)) * logn * logn * log(logn));
    else
      depth = NPS_DEPTH(log2n, log2log2n);

    width = (UV) (search_merits * logn + 0.5);
    width = 64 * ((width+63)/64);    /* Round up to next 64 */
    if (neven) width++;              /* base will always be odd */
    mpz_sub_ui(base, n, width);
    /* printf("merits %lu  width %lu  depth %lu  skip_width %lu\n", search_merits, width, depth, skip_width); */

    /* gmp_printf("partial sieve width %lu  depth %lu\n", 2*width+1, depth); */
    comp = partial_sieve(base, 2*width+1, depth);

    for (; j < width; j += 2) {
      if (!fprev) {
        if (!TSTAVAL(comp, width+1-j)) {
          mpz_sub_ui(t, n, j);
          if ( (skip_width == 0) ? _GMP_BPSW(t) : miller_rabin_ui(t,2) ) {
            fprev = j;
            if (fnext || (skip_width != 0 && j <= skip_width))
              break;
          }
        }
      }
      if (!fnext) {
        if (!TSTAVAL(comp, width+1+j)) {
          mpz_add_ui(t, n, j);
          if ( (skip_width == 0) ? _GMP_BPSW(t) : miller_rabin_ui(t,2) ) {
            fnext = j;
            if (fprev || (skip_width != 0 && j <= skip_width))
              break;
          }
        }
      }
    }

    Safefree(comp);
    if ( (fprev && fnext) ||
         (skip_width != 0 && j <= skip_width && (fprev || fnext)) )
      found = 1;
  }

  mpz_clear(base);
  mpz_clear(t);

  *prev = fprev;
  *next = fnext;
}

/*****************************************************************************/


#define LAST_TRIPLE_PROD \
  ((ULONG_MAX <= 4294967295UL) ? UVCONST(1619) : UVCONST(2642231))
#define LAST_DOUBLE_PROD \
  ((ULONG_MAX <= 4294967295UL) ? UVCONST(65521) : UVCONST(4294967291))
void _GMP_pn_primorial(mpz_t prim, UV n)
{
  UV i = 0, al = 0, p = 2;
  mpz_t* A;

  if (n <= 4) {                 /* tiny input */

    p = (n == 0) ? 1 : (n == 1) ? 2 : (n == 2) ? 6 : (n == 3) ? 30 : 210;
    mpz_set_ui(prim, p);

  } else if (n < 200) {         /* simple linear multiply */

    PRIME_ITERATOR(iter);
    mpz_set_ui(prim, 1);
    while (n-- > 0) {
      if (n > 0) { p *= prime_iterator_next(&iter); n--; }
      mpz_mul_ui(prim, prim, p);
      p = prime_iterator_next(&iter);
    }
    prime_iterator_destroy(&iter);

  } else {                      /* tree mult array of products of 8 UVs */

    PRIME_ITERATOR(iter);
    New(0, A, n, mpz_t);
    while (n-- > 0) {
      if (p <= LAST_TRIPLE_PROD && n > 0)
        { p *= prime_iterator_next(&iter); n--; }
      if (p <= LAST_DOUBLE_PROD && n > 0)
        { p *= prime_iterator_next(&iter); n--; }
      /* each array entry holds the product of 8 UVs */
      if ((i & 7) == 0) mpz_init_set_ui( A[al++], p );
      else              mpz_mul_ui(A[al-1],A[al-1], p );
      i++;
      p = prime_iterator_next(&iter);
    }
    mpz_product(A, 0, al-1);
    mpz_set(prim, A[0]);
    for (i = 0; i < al; i++)  mpz_clear(A[i]);
    Safefree(A);
    prime_iterator_destroy(&iter);

  }
}
void _GMP_primorial(mpz_t prim, UV n)
{
#if (__GNU_MP_VERSION > 5) || (__GNU_MP_VERSION == 5 && __GNU_MP_VERSION_MINOR >= 1)
  mpz_primorial_ui(prim, n);
#else
  if (n <= 4) {

    UV p = (n == 0) ? 1 : (n == 1) ? 1 : (n == 2) ? 2 : (n == 3) ? 6 : 6;
    mpz_set_ui(prim, p);

  } else {

    mpz_t *A;
    UV nprimes, i, al;
    UV *primes = sieve_to_n(n, &nprimes);

    /* Multiply native pairs until we overflow the native type */
    while (nprimes > 1 && ULONG_MAX/primes[0] > primes[nprimes-1]) {
      i = 0;
      while (nprimes > i+1 && ULONG_MAX/primes[i] > primes[nprimes-1])
        primes[i++] *= primes[--nprimes];
    }

    if (nprimes <= 8) {
      /* Just multiply if there are only a few native values left */
      mpz_set_ui(prim, primes[0]);
      for (i = 1; i < nprimes; i++)
        mpz_mul_ui(prim, prim, primes[i]);
    } else {
      /* Create n/4 4-way products, then use product tree */
      New(0, A, nprimes/4 + 1, mpz_t);
      for (i = 0, al = 0; i < nprimes; al++) {
        mpz_init_set_ui(A[al], primes[i++]);
        if (i < nprimes) mpz_mul_ui(A[al], A[al], primes[i++]);
        if (i < nprimes) mpz_mul_ui(A[al], A[al], primes[i++]);
        if (i < nprimes) mpz_mul_ui(A[al], A[al], primes[i++]);
      }
      mpz_product(A, 0, al-1);
      mpz_set(prim, A[0]);
      for (i = 0; i < al; i++)  mpz_clear(A[i]);
      Safefree(A);
    }
    Safefree(primes);

  }
#endif
}

/*****************************************************************************/

/* Put result into char with correct number of digits */
static char* _str_real(mpf_t f, unsigned long prec) {
  char* out;
  unsigned long k;
  int neg = (mpf_sgn(f) < 0);

  if (neg)
    mpf_neg(f, f);

  for (k = 0;  mpf_cmp_ui(f, 1000000000U) >= 0;  k += 9)
    mpf_div_ui(f, f, 1000000000U);
  for (;  mpf_cmp_ui(f, 1) >= 0;  k++)
    mpf_div_ui(f, f, 10);

  New(0, out, 10+((k>prec) ? k : prec), char);
  gmp_sprintf(out, "%.*Ff", prec, f);
  if (out[0] == '0') {
    memmove(out, out+2, prec);
  } else { /* We rounded up.  Treat like 0.1 with one larger k */
    memmove(out+1, out+2, prec);
    k++;
  }

  if (k >= prec) { /* No decimal */
    if (k-prec < 10) {
      memset(out+prec, '0', k-prec);
      prec=k-1;
    } else {
      out[prec++] = 'E';
      prec += sprintf(out+prec, "%lu", k-prec+1);
    }
  } else {        /* insert decimal in correct place */
    memmove(out+k+1, out+k, prec-k);
    out[k] = '.';
  }
  out[prec+1]='\0';
  if (neg) {
    memmove(out+1, out, prec+2);
    out[0] = '-';
  }
  return out;
}

static char* _frac_real(mpz_t num, mpz_t den, unsigned long prec) {
#if 0
  char* out;
  mpf_t fnum, fden, res;
  unsigned long numbits = mpz_sizeinbase(num,  2);
  unsigned long denbits = mpz_sizeinbase(den,  2);
  unsigned long numdigs = mpz_sizeinbase(num, 10);
  unsigned long dendigs = mpz_sizeinbase(den, 10);

  mpf_init2(fnum, 1 + numbits);  mpf_set_z(fnum, num);
  mpf_init2(fden, 1 + denbits);  mpf_set_z(fden, den);
  mpf_init2(res, (unsigned long) (8 + (numbits-denbits+1) + prec*3.4) );
  mpf_div(res, fnum, fden);
  mpf_clear(fnum);  mpf_clear(fden);

  New(0, out, (10+numdigs-dendigs)+prec, char);
  gmp_sprintf(out, "%.*Ff", (int)(prec), res);
  mpf_clear(res);

  return out;
#else
  char* out;
  mpf_t fnum, fden;
  unsigned long bits = 32+(unsigned long)(prec*3.32193);
  mpf_init2(fnum, bits);   mpf_set_z(fnum, num);
  mpf_init2(fden, bits);   mpf_set_z(fden, den);
  mpf_div(fnum, fnum, fden);
  out = _str_real(fnum, prec);
  mpf_clear(fden);
  mpf_clear(fnum);
  return out;
#endif
}


/*********************     Riemann Zeta and Riemann R     *********************/

static void _bern_real_zeta(mpf_t bn, mpz_t zn, unsigned long prec);
static unsigned long zeta_n = 0;
static mpz_t* zeta_d = 0;

static void _borwein_d(unsigned long D) {
  mpz_t t1, t2, t3, sum;
  unsigned long i, n = 3 + (1.31 * D);

  if (zeta_n >= n)
    return;

  if (zeta_n > 0) {
    for (i = 0; i <= zeta_n; i++)
      mpz_clear(zeta_d[i]);
    Safefree(zeta_d);
  }

  n += 10;   /* Add some in case we want a few more digits later */
  zeta_n = n;
  New(0, zeta_d, n+1, mpz_t);
  mpz_init(t1); mpz_init(t2); mpz_init(t3);

  mpz_init_set_ui(sum, 1);
  mpz_init_set(zeta_d[0], sum);

  mpz_fac_ui(t1, n);
  mpz_fac_ui(t2, n);
  for (i = 1; i <= n; i++) {
    mpz_mul_ui(t1, t1, 2*(n+i-1));    /* We've pulled out a 2 from t1 and t2 */
    mpz_divexact_ui(t2, t2, n-i+1);
    mpz_mul_ui(t2, t2, (2*i-1) * i);
    mpz_divexact(t3, t1, t2);
    mpz_add(sum, sum, t3);
    mpz_init_set(zeta_d[i], sum);
  }
  mpz_clear(sum); mpz_clear(t3); mpz_clear(t2); mpz_clear(t1);
}

/* MPFR does some shortcuts, then does an in-place version of Borwein 1991.
 * It's quite clever, and has the advantage of not using the statics.  Our
 * code can be a little faster in some cases, slower in others.  They
 * certainly have done more rigorous error bounding, allowing fewer guard
 * bits and earlier loop exits.
 *
 * For real values, we use our home-grown mpf_pow function, which is very
 * slow at high precisions and again very crude compared to MPFR.
 * For non-integer values this won't compare at all in speed to MPFR or Pari.
 */

static void _zeta(mpf_t z, mpf_t f, unsigned long prec)
{
  unsigned long k, S, p;
  mpf_t s, tf, term;
  mpz_t t1;

  if (mpf_cmp_ui(f,1) == 0) {
   mpf_set_ui(z, 0);
   return;
 }

  /* Shortcut if we know all prec terms are zeros. */
  if (mpf_cmp_ui(f, 1+3.3219281*prec) >= 0 || mpf_cmp_ui(f, mpf_get_prec(z)) > 0) {
    mpf_set_ui(z,1);
    return;
  }

  S = (mpf_integer_p(f) && mpf_fits_ulong_p(f))  ?  mpf_get_ui(f)  :  0;

  /* Negative integers using Bernoulli */
  if (S == 0 && mpf_integer_p(f) && mpf_fits_slong_p(f) && mpf_sgn(f) != 0) {
    S = -mpf_get_si(f);
    if (!(S & 1)) { /* negative even integers are zero */
      mpf_set_ui(z,0);
    } else {        /* negative odd integers are -B_(n+1)/(n+1) */
      mpz_t n;
      mpz_init_set_ui(n, S+1);
      _bern_real_zeta(z, n, prec);
      mpf_div_ui(z, z, S+1);
      mpf_neg(z,z);
      mpz_clear(n);
    }
    return;
  }

  mpf_init2(s,    mpf_get_prec(z));   mpf_set(s, f);
  mpf_init2(tf,   mpf_get_prec(z));
  mpf_init2(term, mpf_get_prec(z));
  mpz_init(t1);

  if (S && S <= 14 && !(S & 1)) {         /* Small even S can be done with Pi */
    unsigned long div[]={0,6,90,945,9450,93555,638512875,18243225};
    const_pi(z, prec);
    mpf_pow_ui(z, z, S);
    if (S == 12) mpf_mul_ui(z, z, 691);
    if (S == 14) mpf_mul_ui(z, z, 2);
    mpf_div_ui(z, z, div[S/2]);
  } else if (mpf_cmp_ui(f, 3+prec*2.15) > 0) {  /* Only one term (3^s < prec) */
    if (S) {
      mpf_set_ui(term, 1);
      mpf_mul_2exp(term, term, S);
    } else {
      mpf_set_ui(term, 2);
      mpf_pow(term, term, s);
    }
    mpf_sub_ui(tf, term, 1);
    mpf_div(z, term, tf);
  } else if ( (mpf_cmp_ui(f,20) > 0 && mpf_cmp_ui(f, prec/3.5) > 0) ||
              (prec > 500 && (mpz_ui_pow_ui(t1, 8*prec, S), mpz_sizeinbase(t1,2) > (20+3.3219281*prec))) ) {
    /* Basic formula, for speed (also note only valid for > 1) */
    PRIME_ITERATOR(iter);
    mpf_set_ui(z, 1);
    for (p = 2; p <= 1000000000; p = prime_iterator_next(&iter)) {
      if (S) {
        mpz_ui_pow_ui(t1, p, S);
        mpf_set_z(term, t1);
      } else {
        mpf_set_ui(tf, p);
        mpf_pow(term, tf, s);
        mpz_set_f(t1, term);
      }
      if (mpz_sizeinbase(t1,2) > (20+3.3219281*prec)) break;
      mpf_sub_ui(tf, term, 1);
      mpf_div(term, term, tf);
      mpf_mul(z, z, term);
    }
    prime_iterator_destroy(&iter);
  } else {
    /* TODO: negative non-integer inputs past -20 or so are very wrong. */
    _borwein_d( (mpf_cmp_d(f,-3.0) >= 0)  ?  prec  :  80+2*prec );

    mpf_set_ui(z, 0);
    for (k = 0; k <= zeta_n-1; k++) {
      if (S) {
        mpz_ui_pow_ui(t1, k+1, S);
        mpf_set_z(term, t1);
      } else {
        mpf_set_ui(tf, k+1);
        mpf_pow(term, tf, s);
      }

      mpz_sub(t1, zeta_d[k], zeta_d[zeta_n]);
      mpf_set_z(tf, t1);

      mpf_div(term, tf, term);

      if (k&1) mpf_sub(z, z, term);
      else     mpf_add(z, z, term);
    }

    mpf_set_z(tf, zeta_d[zeta_n]);
    mpf_div(z, z, tf);

    if (S) {
      mpf_set_ui(tf, 1);
      mpf_div_2exp(tf, tf, S-1);
    } else {
      mpf_set_ui(term, 2);
      mpf_ui_sub(tf, 1, s);
      mpf_pow(tf, term, tf);
    }

    mpf_ui_sub(tf, 1, tf);
    mpf_div(z, z, tf);

    mpf_neg(z, z);
  }
  mpz_clear(t1);
  mpf_clear(term); mpf_clear(tf); mpf_clear(s);
}

static void _zetaint(mpf_t z, unsigned long s, unsigned long prec)
{
  mpf_t f;

  if (s <= 1) {
    mpf_set_ui(z, 0);
  } else if (s >= (1+3.3219281*prec) || s > mpf_get_prec(z)) {
    /* Shortcut if we know all prec terms are zeros. */
    mpf_set_ui(z,1);
  } else {
    mpf_init2(f, mpf_get_prec(z));
    mpf_set_ui(f, s);
    _zeta(z, f, prec);
    mpf_clear(f);
  }
}

static void _riemann_r(mpf_t r, mpf_t n, unsigned long prec)
{
  mpf_t logn, sum, term, part_term, tol, tf;
  unsigned long k, bits = mpf_get_prec(n);

  mpf_init2(logn,      bits);
  mpf_init2(sum,       bits);
  mpf_init2(term,      bits);
  mpf_init2(part_term, bits);
  mpf_init2(tol,       bits);
  mpf_init2(tf,        bits);

  mpf_log(logn, n);
  mpf_set_ui(tol, 10);  mpf_pow_ui(tol, tol, prec);  mpf_ui_div(tol,1,tol);

#if 1 /* Standard Gram Series */
  mpf_set_ui(part_term, 1);
  mpf_set_ui(sum, 1);
  for (k = 1; k < 10000; k++) {
    mpf_mul(part_term, part_term, logn);
    mpf_div_ui(part_term, part_term, k);

    _zetaint(tf, k+1, prec+1);
    mpf_mul_ui(tf, tf, k);
    mpf_div(term, part_term, tf);
    mpf_add(sum, sum, term);

    mpf_abs(term, term);
    mpf_mul(tf, sum, tol);
    if (mpf_cmp(term, tf) <= 0) break;
  }
#else  /* Accelerated (about half the number of terms needed) */
  /* See:   http://mathworld.wolfram.com/GramSeries.html (5)
   * Ramanujan's G (3) and its restatements (5) and (6) are equal,
   * but G is only asymptotically equal to R (restated in (4) as per Gram).
   * To avoid confusion we won't use this.  Too bad, as it can be 2x faster.
   */
  mpf_set(part_term, logn);
  mpf_set_ui(sum, 0);
  _zetaint(tf, 2, prec);
  mpf_div(term, part_term, tf);
  mpf_add(sum, sum, term);
  for (k = 2; k < 1000000; k++) {
    if (mpf_cmp(part_term, tol) <= 0) break;

    mpf_mul(tf, logn, logn);
    if (k < 32768) {
      mpf_div_ui(tf, tf, (2*k-2) * (2*k-1));
    } else {
      mpf_div_ui(tf, tf, 2*k-2);  mpf_div_ui(tf, tf, 2*k-1);
    }
    mpf_mul(part_term, part_term, tf);

    _zetaint(tf, 2*k, prec);
    mpf_mul_ui(tf, tf, 2*k-1);
    mpf_div(term, part_term, tf);
    mpf_add(sum, sum, term);
  }
  mpf_mul_ui(sum, sum, 2);
  mpf_add_ui(sum, sum, 1);
#endif

  mpf_set(r, sum);

  mpf_clear(tf); mpf_clear(tol); mpf_clear(part_term);
  mpf_clear(term); mpf_clear(sum); mpf_clear(logn);
}

/***********************     Constants: Euler, Pi      ***********************/

/* See:
 *   http://numbers.computation.free.fr/Constants/Gamma/gamma.pdf
 *   https://www.ginac.de/CLN/binsplit.pdf (3.1)
 *   https://www-fourier.ujf-grenoble.fr/~demailly/manuscripts/gamma_gazmath_eng.pdf
 *   Pari/GP trans1.c
 *
 * Mortici and Chen (2013) have a O(n^-12) method, but it still too slow.
 * https://link.springer.com/content/pdf/10.1186/1029-242X-2013-222.pdf
 *
 * The Stieltjes zeta method isn't terrible but too slow for large n.
 *
 * We'll use the series method as Pari does.  We should use binary splitting,
 * as it still isn't really fast.  For high precision it's about 2x slower
 * than Pari due to mpf_log / mpf_exp.
 */
void const_euler(mpf_t gamma, unsigned long prec)
{
  const double log2 = 0.693147180559945309417232121458176568L;
  const unsigned long maxsqr = (1UL << (4*sizeof(unsigned long))) - 1;
  unsigned long bits = ceil(40 + prec * 3.322);
  unsigned long x = ceil((2 + bits) * log2/4);
  unsigned long N = ceil(1 + 3.591121477*x - 0.195547*log(x));
  unsigned long xx = x*x;
  unsigned long k;
  mpf_t u, v, a, b, fxx;

  if (prec <= 100) {
    mpf_set_str(gamma, "0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495", 10);
    return;
  }

  mpf_init2(u,    bits);
  mpf_init2(v,    bits);
  mpf_init2(a,    bits);
  mpf_init2(b,    bits);

  mpf_set_ui(u, x);
  mpf_log(u, u);       /* <-- About 20% of the time is spent here. */
  mpf_neg(u, u);
  mpf_set(a, u);
  mpf_set_ui(b, 1);
  mpf_set_ui(v, 1);

  if (x <= maxsqr && N <= maxsqr) {
    /*  v_k = x^2 / k^2  |  u_k = x^2 / k + v_k) / k  */
    for (k = 1; k <= N; k++) {
      mpf_mul_ui(b, b, xx);
      mpf_div_ui(b, b, k*k);
      mpf_mul_ui(a, a, xx);
      mpf_div_ui(a, a, k);
      mpf_add(a, a, b);
      mpf_div_ui(a, a, k);
      mpf_add(u, u, a);
      mpf_add(v, v, b);
    }
  } else {
    mpf_init2(fxx,bits);
    mpf_set_ui(fxx, x);
    mpf_mul(fxx, fxx, fxx);
    for (k = 1; k <= N; k++) {
      mpf_mul(b,b,fxx);
      if (k <= maxsqr) { mpf_div_ui(b,b,k*k); }
      else             { mpf_div_ui(b,b,k);  mpf_div_ui(b,b,k); }
      mpf_mul(a,a,fxx);
      mpf_div_ui(a, a, k);
      mpf_add(a, a, b);
      mpf_div_ui(a, a, k);
      mpf_add(u, u, a);
      mpf_add(v, v, b);
    }
    mpf_clear(fxx);
  }
  mpf_div(gamma, u, v);
  mpf_clear(u); mpf_clear(v); mpf_clear(a); mpf_clear(b);
}

void const_pi(mpf_t pi, unsigned long prec)
{
  mpf_t t, an, bn, tn, prev_an;
  unsigned long k, bits = ceil(prec * 3.322);

  if (prec <= 100) {
    mpf_set_str(pi, "3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798215", 10);
    return;
  }

  mpf_init2(t,       10+bits);
  mpf_init2(an,      10+bits);
  mpf_init2(bn,      10+bits);
  mpf_init2(tn,      10+bits);
  mpf_init2(prev_an, 10+bits);

  mpf_set_d(an, 1);
  mpf_set_d(bn, 0.5);
  mpf_set_d(tn, 0.25);
  mpf_sqrt(bn, bn);
                                    /* Comments from Brent 1976 */
  for (k = 0; (prec >> (k+1)) > 0; k++) {
    mpf_set(prev_an, an);           /* Y <- A */
    mpf_add(t, an, bn);
    mpf_div_ui(an, t, 2);           /* A <- (A+B)/2 */
    mpf_mul(t, bn, prev_an);
    mpf_sqrt(bn, t);                /* B <- (BY)^(1/2) */
    mpf_sub(prev_an, prev_an, an);
    mpf_mul(t, prev_an, prev_an);
    mpf_mul_2exp(t, t, k);
    mpf_sub(tn, tn, t);             /* T <- T - 2^k (A-Y)^2 */
#if 0 /* Instead of doing the comparison, we assume doubling per iteration */
    mpf_sub(t, an, bn);
    mpf_mul_2exp(t, t, bits);
    if (mpf_cmp_ui(t,1) <= 0) break;
#endif
  }
  mpf_add(t, an, bn);
  mpf_mul(an, t, t);
  mpf_mul_2exp(t, tn, 2);
  mpf_div(pi, an, t);               /* return (A+B)^2 / 4T */
  mpf_clear(tn); mpf_clear(bn); mpf_clear(an);
  mpf_clear(prev_an); mpf_clear(t);
}

/*****************     Exponential / Logarithmic Integral     *****************/

static void _li_r(mpf_t r, mpf_t n, unsigned long prec)
{
  mpz_t factorial;
  mpf_t logn, sum, inner_sum, term, p, q, tol;
  unsigned long j, k, bits = mpf_get_prec(n);

  mpf_init2(logn,      bits);
  mpf_log(logn, n);

  mpf_init2(sum,       bits);
  mpf_init2(inner_sum, bits);
  mpf_init2(term,      bits);
  mpf_init2(p,         bits);
  mpf_init2(q,         bits);
  mpf_init2(tol,       bits);

  mpf_set_ui(tol, 10);  mpf_pow_ui(tol, tol, prec);  mpf_ui_div(tol,1,tol);

  mpz_init_set_ui(factorial, 1);

  mpf_set_si(p, -1);
  for (j = 1, k = 0; j < 1000000; j++) {
    mpz_mul_ui(factorial, factorial, j);
    mpf_mul(p, p, logn);
    mpf_neg(p, p);
    for (; k <= (j - 1) / 2; k++) {
      mpf_set_ui(q, 1);
      mpf_div_ui(q, q, 2*k+1);
      mpf_add(inner_sum, inner_sum, q);
    }
    mpf_set_z(q, factorial);
    mpf_mul_2exp(q, q, j-1);
    mpf_mul(term, p, inner_sum);
    mpf_div(term, term, q);
    mpf_add(sum, sum, term);

    mpf_abs(term, term);
    mpf_mul(q, sum, tol);
    mpf_abs(q, q);
    if (mpf_cmp(term, q) <= 0) break;
  }
  mpf_sqrt(q, n);
  mpf_mul(r, sum, q);

  mpf_abs(logn,logn);
  mpf_log(q, logn);
  mpf_add(r, r, q);

  /* Find out roughly how many digits of C we need, then get it and add */
  mpf_set(q, r);
  for (k = prec; mpf_cmp_ui(q, 1024*1024) >= 0; k -= 6)
    mpf_div_2exp(q, q, 20);
  const_euler(q, k);
  mpf_add(r, r, q);

  mpz_clear(factorial);
  mpf_clear(tol);
  mpf_clear(q);
  mpf_clear(p);
  mpf_clear(term);
  mpf_clear(inner_sum);
  mpf_clear(sum);
  mpf_clear(logn);
}

static void _ei_r(mpf_t r, mpf_t n, unsigned long prec)
{
  mpf_exp(r, n);
  _li_r(r, r, prec);
}

char* zetareal(mpf_t z, unsigned long prec)
{
  size_t est_digits = 10+prec;
  char* out;
  if (mpf_cmp_ui(z,1) == 0) return 0;
  if (mpz_sgn(z) < 0) est_digits += -mpf_get_si(z);
  _zeta(z, z, prec);
  New(0, out, est_digits, char);
  gmp_sprintf(out, "%.*Ff", (int)(prec), z);
  return out;
}

char* riemannrreal(mpf_t r, unsigned long prec)
{
  if (mpf_cmp_ui(r,0) <= 0) return 0;
  _riemann_r(r, r, prec);
  return _str_real(r, prec);
}

char* lireal(mpf_t r, unsigned long prec)
{
  if (mpf_cmp_ui(r,0) < 0) return 0;
  if (mpf_cmp_ui(r,1) == 0) return 0;
  _li_r(r, r, prec);
  return _str_real(r, prec);
}
char* eireal(mpf_t r, unsigned long prec)
{
  if (mpf_cmp_ui(r,0) == 0) return 0;
  _ei_r(r, r, prec);
  return _str_real(r, prec);
}

char* logreal(mpf_t r, unsigned long prec)
{
  mpf_log(r, r);
  return _str_real(r, prec);
}
char* expreal(mpf_t r, unsigned long prec)
{
  mpf_exp(r, r);
  return _str_real(r, prec);
}
char* powreal(mpf_t r, mpf_t x, unsigned long prec)
{
  mpf_pow(r, r, x);
  return _str_real(r, prec);
}
char* agmreal(mpf_t a, mpf_t b, unsigned long prec)
{
  if (mpz_sgn(a) == 0 || mpz_sgn(b) == 0) {
     mpf_set_ui(a,0);
  } else if (mpz_sgn(a) < 0 || mpz_sgn(b) < 0) {
     return 0;  /* NaN */
  }
  mpf_agm(a, a, b);
  return _str_real(a, prec);
}

char* eulerconst(unsigned long prec) {
  char* out;
  mpf_t gamma;
  unsigned long bits = 7 + (unsigned long)(prec*3.32193);

  mpf_init2(gamma, bits);
  const_euler(gamma, prec);
  New(0, out, prec+4, char);
  gmp_sprintf(out, "%.*Ff", (int)(prec), gamma);
  mpf_clear(gamma);
  return out;
}
char* piconst(unsigned long prec) {
  char* out;
  mpf_t pi;
  unsigned long bits = 7 + (unsigned long)(prec*3.32193);

  mpf_init2(pi, bits);
  const_pi(pi, prec);
  New(0, out, prec+4, char);
  gmp_sprintf(out, "%.*Ff", (int)(prec-1), pi);
  mpf_clear(pi);
  return out;
}


/***************************        Harmonic        ***************************/

static void _harmonic(mpz_t a, mpz_t b, mpz_t t) {
  mpz_sub(t, b, a);
  if (mpz_cmp_ui(t, 1) == 0) {
    mpz_set(b, a);
    mpz_set_ui(a, 1);
  } else {
    mpz_t q, r;
    mpz_add(t, a, b);
    mpz_tdiv_q_2exp(t, t, 1);
    mpz_init_set(q, t); mpz_init_set(r, t);
    _harmonic(a, q, t);
    _harmonic(r, b, t);
    mpz_mul(a, a, b);
    mpz_mul(t, q, r);
    mpz_add(a, a, t);
    mpz_mul(b, b, q);
    mpz_clear(q); mpz_clear(r);
  }
}

void harmfrac(mpz_t num, mpz_t den, mpz_t zn)
{
  mpz_t t;
  mpz_init(t);
  mpz_add_ui(den, zn, 1);
  mpz_set_ui(num, 1);
  _harmonic(num, den, t);
  mpz_gcd(t, num, den);
  mpz_divexact(num, num, t);
  mpz_divexact(den, den, t);
  mpz_clear(t);
}

char* harmreal(mpz_t zn, unsigned long prec) {
  char* out;
  mpz_t num, den;

  mpz_init(num); mpz_init(den);
  harmfrac(num, den, zn);
  out = _frac_real(num, den, prec);
  mpz_clear(den); mpz_clear(num);

  return out;
}

/**************************        Bernoulli        **************************/

static void _bern_real_zeta(mpf_t bn, mpz_t zn, unsigned long prec)
{
  unsigned long s = mpz_get_ui(zn);
  mpf_t tf;

  if (s & 1) {
    mpf_set_d(bn, (s == 1) ? 0.5 : 0.0);
    return;
  }

  mpf_init2(tf, mpf_get_prec(bn));

  /* For large values with low precision, we should look at approximations.
   *   http://www.ebyte.it/library/downloads/2008_MTH_Nemes_GammaApproximationUpdate.pdf
   *   http://www.luschny.de/math/primes/bernincl.html
   *   http://arxiv.org/pdf/math/0702300.pdf
   */

  _zetaint(bn, s, prec);

  /* We should be using an approximation here, e.g. Pari's mpfactr.  For
   * large values this is the majority of time taken for this function. */
  { mpz_t t; mpz_init(t); mpz_fac_ui(t, s); mpf_set_z(tf, t); mpz_clear(t);}
  mpf_mul(bn, bn, tf);
  /* bn = s! * zeta(s) */

  const_pi(tf, prec);
  mpf_mul_ui(tf, tf, 2);
  mpf_pow_ui(tf, tf, s);
  mpf_div(bn, bn, tf);
  /* bn = s! * zeta(s) / (2Pi)^s */

  mpf_mul_2exp(bn, bn, 1);
  if ((s & 3) == 0) mpf_neg(bn, bn);
  /* bn = (-1)^(n-1) * 2 * s! * zeta(s) / (2Pi)^s */
  mpf_clear(tf);
}


static void _bernfrac_comb(mpz_t num, mpz_t den, mpz_t zn, mpz_t t)
{
  unsigned long k, j, n = mpz_get_ui(zn);
  mpz_t* T;

  if (n <= 1 || (n & 1)) {
    mpz_set_ui(num, (n<=1) ? 1 : 0);
    mpz_set_ui(den, (n==1) ? 2 : 1);
    return;
  }

  /* Denominator */
  mpz_set_ui(t, 1);
  mpz_mul_2exp(den, t, n);    /* den = U = 1 << n  */
  mpz_sub_ui(t, den, 1);      /* t = U-1            */
  mpz_mul(den, den, t);       /* den = U*(U-1)      */

  n >>= 1;

  /* Luschny's version of the "Brent-Harvey" method */
  /* Algorithm TangentNumbers from https://arxiv.org/pdf/1108.0286.pdf */
  New(0, T, n+1, mpz_t);
  for (k = 1; k <= n; k++)  mpz_init(T[k]);
  mpz_set_ui(T[1], 1);

  for (k = 2; k <= n; k++)
    mpz_mul_ui(T[k], T[k-1], k-1);

  for (k = 2; k <= n; k++) {
    for (j = k; j <= n; j++) {
      mpz_mul_ui(t, T[j], j-k+2);
      mpz_mul_ui(T[j], T[j-1], j-k);
      mpz_add(T[j], T[j], t);
    }
  }

  /* (14), also last line of Algorithm FastTangentNumbers from paper */
  mpz_mul_ui(num, T[n], n);
  mpz_mul_si(num, num, (n & 1) ? 2 : -2);

  for (k = 1; k <= n; k++)  mpz_clear(T[k]);
  Safefree(T);
}


static void _bernfrac_zeta(mpz_t num, mpz_t den, mpz_t zn, mpz_t t)
{
  unsigned long prec, n = mpz_get_ui(zn);
  double nbits;
  mpf_t bn, tf;
  /* Compute integer numerator by getting the real bn first. */

  if (n <= 1 || (n & 1)) {
    mpz_set_ui(num, (n<=1) ? 1 : 0);
    mpz_set_ui(den, (n==1) ? 2 : 1);
    return;
  }
  if (n == 2) { mpz_set_ui(num, 1); mpz_set_ui(den, 6); return; }

  /* Calculate denominator */
  {
    int i, ndivisors;
    mpz_t *D;

    mpz_set_ui(t, n >> 1);
    D = divisor_list(&ndivisors, t);
    mpz_set_ui(den, 6);
    for (i = 1; i < ndivisors; i++) {
      mpz_mul_2exp(t,D[i],1);  mpz_add_ui(t,t,1);
      if (_GMP_is_prime(t))
        mpz_mul(den, den, t);
    }
    for (i = 0; i < ndivisors; i++)
      mpz_clear(D[i]);
    Safefree(D);
  }

  /* Estimate number of bits, from Pari, also see Stein 2006 */
  nbits = mpz_logn(den) + (n+0.5) * log((double)n) - n*2.8378770664093454835606594728L + 1.712086L;
  nbits /= log(2);
  nbits += 32;
  prec = (unsigned long)(nbits/3.32193 + 1);

  mpf_init2(bn, nbits);
  mpf_init2(tf, nbits);
  _bern_real_zeta(bn, zn, prec);
  mpf_set_z(tf, den);
  mpf_mul(bn, bn, tf);

  mpf_set_d(tf, (mpf_sgn(bn) < 0) ? -0.5 : 0.5);
  mpf_add(bn, bn, tf);

  mpz_set_f(num, bn);

  mpf_clear(tf);
  mpf_clear(bn);
}


void bernfrac(mpz_t num, mpz_t den, mpz_t zn)
{
  mpz_t t;
  mpz_init(t);

  if (mpz_cmp_ui(zn,46) < 0) {
    _bernfrac_comb(num, den, zn, t);
  } else {
    _bernfrac_zeta(num, den, zn, t);
  }

  mpz_gcd(t, num, den);
  mpz_divexact(num, num, t);
  mpz_divexact(den, den, t);
  mpz_clear(t);
}

char* bernreal(mpz_t zn, unsigned long prec) {
  char* out;

  if (mpz_cmp_ui(zn,40) < 0) {
    mpz_t num, den, t;
    mpz_init(num); mpz_init(den); mpz_init(t);
    _bernfrac_comb(num, den, zn, t);
    out = _frac_real(num, den, prec);
    mpz_clear(t); mpz_clear(den); mpz_clear(num);
  } else {
    mpf_t z;
    unsigned long bits = 32+(unsigned long)(prec*3.32193);
    mpf_init2(z, bits);
    _bern_real_zeta(z, zn, prec);
    out = _str_real(z, prec);
    mpf_clear(z);
  }
  return out;
}

/***************************       Lambert W       ***************************/

static void _lambertw(mpf_t w, mpf_t x, unsigned long prec)
{
  int i;
  unsigned long bits = 96+mpf_get_prec(x);  /* More bits for intermediate */
  mpf_t t, w1, zn, qn, en, tol;

  if (mpf_cmp_d(x, -0.36787944117145) < 0)
    croak("Invalid input to LambertW:  x must be >= -1/e");
  if (mpf_sgn(x) == 0)
    { mpf_set(w, x); return; }

  /* Use Fritsch rather than Halley. */
  mpf_init2(t,   bits);
  mpf_init2(w1,  bits);
  mpf_init2(zn,  bits);
  mpf_init2(qn,  bits);
  mpf_init2(en,  bits);
  mpf_init2(tol, bits);

  /* Initial estimate */
  if (mpf_cmp_d(x, -0.06) < 0) {  /* Pade(3,2) */
    mpf_set_d(t, 5.4365636569180904707205749);
    mpf_mul(t, t, x);
    mpf_add_ui(t, t, 2);
    if (mpf_sgn(t) <= 0) { mpf_set_ui(t, 0); } else { mpf_sqrt(t, t); }
    mpf_mul(zn, t, t);
    mpf_mul(qn, zn, t);

    mpf_set_d(w, -1);
    mpf_set_d(w1, 1.0L/6.0L);      mpf_mul(w1, w1,  t);  mpf_add(w, w, w1);
    mpf_set_d(w1, 257.0L/720.0L);  mpf_mul(w1, w1, zn);  mpf_add(w, w, w1);
    mpf_set_d(w1, 13.0L/720.0L);   mpf_mul(w1, w1, zn);  mpf_add(w, w, w1);
    mpf_set(en, w);  /* numerator */

    mpf_set_d(w, 1);
    mpf_set_d(w1, 5.0L/6.0L);      mpf_mul(w1, w1,  t);  mpf_add(w, w, w1);
    mpf_set_d(w1, 103.0L/720.0L);  mpf_mul(w1, w1, zn);  mpf_add(w, w, w1);

    mpf_div(w, en, w);
  } else if (mpf_cmp_d(x, 1.363) < 0) {  /* Winitzki 2003 */
    mpf_add_ui(t, x, 1);
    mpf_log(w1, t);
    mpf_add_ui(zn, w1, 1);
    mpf_log(zn, zn);
    mpf_add_ui(qn, w1, 2);
    mpf_div(t, zn, qn);
    mpf_ui_sub(t, 1, t);
    mpf_mul(w, w1, t);
  } else if (mpf_cmp_d(x, 3.7) < 0) {  /* Vargas 2013 modified */
    mpf_log(w, x);
    mpf_log(w1, w);
    mpf_div(t, w1, w);
    mpf_ui_sub(t, 1, t);
    mpf_log(t, t);
    mpf_div_ui(t, t, 2);
    mpf_sub(w, w, w1);
    mpf_sub(w, w, t);
  } else {  /* Corless et al. 1993 */
    mpf_t l1, l2, d1, d2, d3;
    mpf_init2(l1,bits); mpf_init2(l2,bits);
    mpf_init2(d1,bits); mpf_init2(d2,bits); mpf_init2(d3,bits);

    mpf_log(l1, x);
    mpf_log(l2, l1);
    mpf_mul(d1, l1, l1);  mpf_mul_ui(d1, d1, 2);
    mpf_mul(d2, l1, d1);  mpf_mul_ui(d2, d2, 3);
    mpf_mul(d3, l1, d2);  mpf_mul_ui(d3, d3, 2);

    mpf_sub(w, l1, l2);

    mpf_div(t, l2, l1);
    mpf_add(w, w, t);

    mpf_sub_ui(t, l2, 2);
    mpf_mul(t, t, l2);
    mpf_div(t, t, d1);
    mpf_add(w, w, t);

    mpf_mul_ui(t, l2, 2);
    mpf_sub_ui(t, t, 9);
    mpf_mul(t, t, l2);
    mpf_add_ui(t, t, 6);
    mpf_mul(t, t, l2);
    mpf_div(t, t, d2);
    mpf_add(w, w, t);

    mpf_mul_ui(t, l2, 3);
    mpf_sub_ui(t, t, 22);
    mpf_mul(t, t, l2);
    mpf_add_ui(t, t, 36);
    mpf_mul(t, t, l2);
    mpf_sub_ui(t, t, 12);
    mpf_mul(t, t, l2);
    mpf_div(t, t, d3);
    mpf_add(w, w, t);

    mpf_clear(l1); mpf_clear(l2);
    mpf_clear(d1); mpf_clear(d2); mpf_clear(d3);
  }

  /* Divide prec by 2 since t should be have 4x number of zeros each round */
  mpf_set_ui(tol, 10);
  mpf_pow_ui(tol, tol, (mpf_cmp_d(x, -.36) < 0) ? prec : prec/2);
  mpf_ui_div(tol,1,tol);

  for (i = 0; i < 500 && mpz_sgn(w) != 0; i++) {
    mpf_add_ui(w1, w, 1);

    mpf_div(t, x, w);
    mpf_log(zn, t);
    mpf_sub(zn, zn, w);

    mpf_mul_ui(t, zn, 2);
    mpf_div_ui(t, t, 3);
    mpf_add(t, t, w1);
    mpf_mul(t, t, w1);
    mpf_mul_ui(qn, t, 2);

    mpf_sub(en, qn, zn);
    mpf_mul_ui(t, zn, 2);
    mpf_sub(t, qn, t);
    mpf_div(en, en, t);
    mpf_div(t, zn, w1);
    mpf_mul(en, en, t);

    mpf_mul(t, w, en);
    mpf_add(w, w, t);

    mpf_abs(t, t);
    if (mpf_cmp(t, tol) <= 0) break;
    if (mpf_cmp_d(w,-1) <= 0) break;
  }

  if (mpf_cmp_d(w, -1) <= 0)
    mpf_set_si(w, -1);

  mpf_clear(en); mpf_clear(qn); mpf_clear(zn);
  mpf_clear(w1); mpf_clear(t); mpf_clear(tol);
}

char* lambertwreal(mpf_t x, unsigned long prec) {
  char* out;
  mpf_t w;
  unsigned long bits = 64+(unsigned long)(prec*3.32193);

  mpf_init2(w, bits);
  _lambertw(w, x, 10+prec);
  out = _str_real(w, prec);
  mpf_clear(w);
  return out;
}

/*****************************************************************************/

void stirling(mpz_t r, unsigned long n, unsigned long m, UV type)
{
  mpz_t t, t2;
  unsigned long j;
  if (type < 1 || type > 3) croak("stirling type must be 1, 2, or 3");
  if (n == m) {
    mpz_set_ui(r, 1);
  } else if (n == 0 || m == 0 || m > n) {
    mpz_set_ui(r,0);
  } else if (m == 1) {
    switch (type) {
      case 1:  mpz_fac_ui(r, n-1);  if (!(n&1)) mpz_neg(r, r); break;
      case 2:  mpz_set_ui(r, 1); break;
      case 3:
      default: mpz_fac_ui(r, n); break;
    }
  } else {
    mpz_init(t);  mpz_init(t2);
    mpz_set_ui(r,0);
    if (type == 3) { /* Lah: binomial(n k) * binomial(n-1 k-1) * (n-k)!*/
      mpz_bin_uiui(t, n, m);
      mpz_bin_uiui(t2, n-1, m-1);
      mpz_mul(r, t, t2);
      mpz_fac_ui(t2, n-m);
      mpz_mul(r, r, t2);
    } else if (type == 2) {
      mpz_t binom;
      mpz_init_set_ui(binom, m);
      mpz_ui_pow_ui(r, m, n);
      /* Use symmetry to halve the number of loops */
      for (j = 1; j <= ((m-1)>>1); j++) {
        mpz_ui_pow_ui(t, j, n);
        mpz_ui_pow_ui(t2, m-j, n);
        if (m&1) mpz_sub(t, t2, t);
        else     mpz_add(t, t2, t);
        mpz_mul(t, t, binom);
        if (j&1) mpz_sub(r, r, t);
        else     mpz_add(r, r, t);
        mpz_mul_ui(binom, binom, m-j);
        mpz_divexact_ui(binom, binom, j+1);
      }
      if (!(m&1)) {
        mpz_ui_pow_ui(t, j, n);
        mpz_mul(t, t, binom);
        if (j&1) mpz_sub(r, r, t);
        else     mpz_add(r, r, t);
      }
      mpz_clear(binom);
      mpz_fac_ui(t, m);
      mpz_divexact(r, r, t);
    } else {
      mpz_bin_uiui(t,  n-1+1, n-m+1);
      mpz_bin_uiui(t2, n-m+n, n-m-1);
      mpz_mul(t2, t2, t);
      for (j = 1; j <= n-m; j++) {
        stirling(t, n-m+j, j, 2);
        mpz_mul(t, t, t2);
        if (j & 1)      mpz_sub(r, r, t);
        else            mpz_add(r, r, t);
        mpz_mul_ui(t2, t2, n+j);
        mpz_divexact_ui(t2, t2, n-m+j+1);
        mpz_mul_ui(t2, t2, n-m-j);
        mpz_divexact_ui(t2, t2, n+j+1);
      }
    }
    mpz_clear(t2);  mpz_clear(t);
  }
}

/* Goetgheluck method.  Also thanks to Peter Luschny. */
void binomial(mpz_t r, UV n, UV k)
{
  UV fi, nk, sqrtn, piN, prime, i, j;
  UV* primes;
  mpz_t* mprimes;

  if (k > n)            { mpz_set_ui(r, 0); return; }
  if (k == 0 || k == n) { mpz_set_ui(r, 1); return; }

  if (k > n/2)  k = n-k;

  sqrtn = (UV) (sqrt((double)n));
  fi = 0;
  nk = n-k;
  primes = sieve_to_n(n, &piN);

#define PUSHP(p) \
 do { \
   if ((j++ % 8) == 0) mpz_init_set_ui(mprimes[fi++], p); \
   else                mpz_mul_ui(mprimes[fi-1], mprimes[fi-1], p); \
 } while (0)

  New(0, mprimes, (piN+7)/8, mpz_t);

  for (i = 0, j = 0; i < piN; i++) {
    prime = primes[i];
    if (prime > nk) {
      PUSHP(prime);
    } else if (prime > n/2) {
      /* nothing */
    } else if (prime > sqrtn) {
      if (n % prime < k % prime)
        PUSHP(prime);
    } else {
      UV N = n, K = k, p = 1, s = 0;
      while (N > 0) {
        s = (N % prime) < (K % prime + s) ? 1 : 0;
        if (s == 1)  p *= prime;
        N /= prime;
        K /= prime;
      }
      if (p > 1)
        PUSHP(p);
    }
  }
  Safefree(primes);
  mpz_product(mprimes, 0, fi-1);
  mpz_set(r, mprimes[0]);
  for (i = 0; i < fi; i++)
    mpz_clear(mprimes[i]);
  Safefree(mprimes);
}

void factorialmod(mpz_t r, UV N, mpz_t m)
{
  mpz_t t;
  UV i, D = N;

  if (mpz_cmp_ui(m,N) <= 0 || mpz_cmp_ui(m,1) <= 0) {
    mpz_set_ui(r,0);
    return;
  }

  mpz_init(t);
  mpz_tdiv_q_2exp(t, m, 1);
  if (mpz_cmp_ui(t, N) < 0 && _GMP_is_prime(m))
    D = mpz_get_ui(m) - N - 1;

  if (D < 2 && N > D) {
    if (D == 0) mpz_sub_ui(r, m, 1);
    else        mpz_set_ui(r, 1);
    mpz_clear(t);
    return;
  }

  if (D == N && D > 5000000) {   /* TODO: tune this threshold */
    mpz_t *factors;
    int j, nfactors, *exponents, reszero;
    nfactors = factor(m, &factors, &exponents);
    /* Find max factor */
    mpz_set_ui(t, 0);
    for (j = 0; j < nfactors; j++) {
      if (exponents[j] > 1)
        mpz_pow_ui(factors[j], factors[j], exponents[j]);
      if (mpz_cmp(factors[j], t) > 0)
        mpz_set(t, factors[j]);
    }
    reszero = (mpz_cmp_ui(t, N) <= 0);
    clear_factors(nfactors, &factors, &exponents);
    if (reszero) { mpz_clear(t); mpz_set_ui(r,0); return; }
  }

  mpz_set_ui(t,1);
  for (i = 2; i <= D && mpz_sgn(t); i++) {
    mpz_mul_ui(t, t, i);
    if ((i & 15) == 0) mpz_mod(t, t, m);
  }
  mpz_mod(r, t, m);

  if (D != N && mpz_sgn(r)) {
    if (!(D&1)) mpz_sub(r, m, r);
    mpz_invert(r, r, m);
  }

  mpz_clear(t);
}

void partitions(mpz_t npart, UV n)
{
  mpz_t psum, *part;
  UV *pent, i, j, k, d = (UV) sqrt(n+1);

  if (n <= 3) {
    mpz_set_ui(npart, (n == 0) ? 1 : n);
    return;
  }

  New(0, pent, 2*d+2, UV);
  pent[0] = 0;
  pent[1] = 1;
  for (i = 1; i <= d; i++) {
    pent[2*i  ] = ( i   *(3*i+1)) / 2;
    pent[2*i+1] = ((i+1)*(3*i+2)) / 2;
  }
  New(0, part, n+1, mpz_t);
  mpz_init_set_ui(part[0], 1);
  mpz_init(psum);
  for (j = 1; j <= n; j++) {
    mpz_set_ui(psum, 0);
    for (k = 1; pent[k] <= j; k++) {
      if ((k+1) & 2) mpz_add(psum, psum, part[ j - pent[k] ]);
      else           mpz_sub(psum, psum, part[ j - pent[k] ]);
    }
    mpz_init_set(part[j], psum);
  }

  mpz_set(npart, part[n]);

  mpz_clear(psum);
  for (i = 0; i <= n; i++)
    mpz_clear(part[i]);
  Safefree(part);
  Safefree(pent);
}


void consecutive_integer_lcm(mpz_t m, UV B)
{
  UV i, p, p_power, pmin;
  mpz_t t[8];
  PRIME_ITERATOR(iter);

  /* Create eight sub-products to combine at the end. */
  for (i = 0; i < 8; i++)  mpz_init_set_ui(t[i], 1);
  i = 0;
  /* For each prime, multiply m by p^floor(log B / log p), which means
   * raise p to the largest power e such that p^e <= B.
   */
  if (B >= 2) {
    p_power = 2;
    while (p_power <= B/2)
      p_power *= 2;
    mpz_mul_ui(t[i&7], t[i&7], p_power);  i++;
  }
  p = prime_iterator_next(&iter);
  while (p <= B) {
    pmin = B/p;
    if (p > pmin)
      break;
    p_power = p*p;
    while (p_power <= pmin)
      p_power *= p;
    mpz_mul_ui(t[i&7], t[i&7], p_power);  i++;
    p = prime_iterator_next(&iter);
  }
  while (p <= B) {
    mpz_mul_ui(t[i&7], t[i&7], p);  i++;
    p = prime_iterator_next(&iter);
  }
  /* combine the eight sub-products */
  for (i = 0; i < 4; i++)  mpz_mul(t[i], t[2*i], t[2*i+1]);
  for (i = 0; i < 2; i++)  mpz_mul(t[i], t[2*i], t[2*i+1]);
  mpz_mul(m, t[0], t[1]);
  for (i = 0; i < 8; i++)  mpz_clear(t[i]);
  prime_iterator_destroy(&iter);
}


/* a=0, return power.  a>1, return bool if an a-th power */
UV is_power(mpz_t n, UV a)
{
  if (mpz_cmp_ui(n,3) <= 0 && a == 0)
    return 0;
  else if (a == 1)
    return 1;
  else if (a == 2)
    return mpz_perfect_square_p(n);
  else {
    UV result;
    mpz_t t;
    mpz_init(t);
    result = (a == 0)  ?  power_factor(n, t)  :  (UV)mpz_root(t, n, a);
    mpz_clear(t);
    return result;
  }
}

UV prime_power(mpz_t prime, mpz_t n)
{
  UV k;
  if (mpz_even_p(n)) {
    k = mpz_scan1(n, 0);
    if (k+1 == mpz_sizeinbase(n, 2)) {
      mpz_set_ui(prime, 2);
      return k;
    }
    return 0;
  }
  if (_GMP_is_prob_prime(n)) {
    mpz_set(prime, n);
    return 1;
  }
  k = power_factor(n, prime);
  if (k && !_GMP_is_prob_prime(prime))
    k = 0;
  return k;
}


void exp_mangoldt(mpz_t res, mpz_t n)
{
  if (prime_power(res, n) < 1)
    mpz_set_ui(res, 1);
}

int is_carmichael(mpz_t n)
{
  mpz_t nm1, t, *factors;
  int i, res, nfactors, *exponents;

  /* small or even */
  if (mpz_cmp_ui(n,561) < 0 || mpz_even_p(n))  return 0;

  /* divisible by small square */
  for (i = 1; i < 9; i++)
    if (mpz_divisible_ui_p(n, sprimes[i] * sprimes[i]))
      return 0;

  mpz_init(nm1);
  mpz_sub_ui(nm1, n, 1);

  /* Korselt's criterion for small divisors */
  for (i = 2; i < 20; i++)
    if (mpz_divisible_ui_p(n, sprimes[i]) && !mpz_divisible_ui_p(nm1, sprimes[i]-1))
      { mpz_clear(nm1); return 0; }

  mpz_init_set_ui(t, 2);
  mpz_powm(t, t, nm1, n);
  if (mpz_cmp_ui(t, 1) != 0) {   /* if 2^(n-1) mod n != 1, fail */
    mpz_clear(t);  mpz_clear(nm1);
    return 0;
  }

  /* If large enough, use probable test */
  if (mpz_sizeinbase(n,10) > 50) {
    res = !_GMP_is_prime(n);  /* It must be a composite */
    for (i = 20; res && i <= 100; i++) {
      UV p = sprimes[i];
      UV gcd = mpz_gcd_ui(NULL, n, p);
      if (gcd == 1) {
        /* For each non prime factor it passes a Fermat test */
        if (mpz_set_ui(t,p), mpz_powm(t,t,nm1,n), mpz_cmp_ui(t, 1) != 0)
          res = 0;
      } else {
        /* For each prime factor it meets Korselt criterion */
        if (gcd != p || !mpz_divisible_ui_p(nm1, p-1))
          res = 0;
      }
    }
    mpz_clear(t);  mpz_clear(nm1);
    return res;
  }

  nfactors = factor(n, &factors, &exponents);
  res = (nfactors > 2);                       /* must have 3+ factors */
  for (i = 0; res && i < nfactors; i++) {     /* must be square free  */
    if (exponents[i] > 1)
      res = 0;
  }
  for (i = 0; res && i < nfactors; i++) {     /* p-1 | n-1 for all p */
    mpz_sub_ui(t, factors[i], 1);
    if (!mpz_divisible_p(nm1, t))
      res = 0;
  }
  clear_factors(nfactors, &factors, &exponents);
  mpz_clear(t);  mpz_clear(nm1);
  return res;
}

int is_fundamental(mpz_t n)
{
  int r, neg = (mpz_sgn(n) < 0), ret = 0;
  if (neg) mpz_neg(n,n);

  r = mpz_fdiv_ui(n,16);
  if (r != 0) {
    int r4 = r & 3;
    if (!neg && r4 == 1 && moebius(n) != 0) ret = 1;
    if ( neg && r4 == 3 && moebius(n) != 0) ret = 1;
    if (r4 == 0 && ((!neg && r != 4) || (neg && r != 12))) {
      mpz_t t;
      mpz_init(t);
      mpz_tdiv_q_2exp(t, n, 2);
      if (moebius(t) != 0)
        ret = 1;
      mpz_clear(t);
    }
  }
  if (neg) mpz_neg(n,n);
  return ret;
}

int _totpred(mpz_t n, mpz_t maxd)
{
    int i, res, ndivisors;
    mpz_t N, r, d, p, *D;

    if (mpz_odd_p(n)) return 0;
    if (mpz_cmp_ui(n,2) == 0) return 1;

    mpz_init(N);  mpz_init(p);
    mpz_tdiv_q_2exp(N, n, 1);
    res = 0;
    mpz_add_ui(p, n, 1);
    if (mpz_cmp(N, maxd) < 0 && _GMP_is_prime(p)) {
      res = 1;
    } else {
      mpz_init(d);  mpz_init(r);
      D = divisor_list(&ndivisors, N);
      for (i = 0; res == 0 && i < ndivisors && mpz_cmp(D[i],maxd) < 0; i++) {
        mpz_set(d, D[i]);
        mpz_mul_2exp(p,d,1);  mpz_add_ui(p,p,1);
        if (!_GMP_is_prime(p))
          continue;
        mpz_divexact(r, N, d);
        while (1) {
          if (mpz_cmp(r, p) == 0 || _totpred(r, d)) {
            res = 1;
            break;
          }
          if (!mpz_divisible_p(r, p))
            break;
          mpz_divexact(r, r, p);
        }
      }
      mpz_clear(r);  mpz_clear(d);
      for (i = 0; i < ndivisors; i++)
        mpz_clear(D[i]);
      Safefree(D);
    }
    mpz_clear(p);  mpz_clear(N);
    return res;
}

int is_totient(mpz_t n)
{
  if (mpz_sgn(n) == 0 || mpz_odd_p(n))
    return !mpz_cmp_ui(n,1) ? 1 : 0;
  return _totpred(n, n);
}

void polygonal_nth(mpz_t r, mpz_t n, UV k)
{
  mpz_t D, t;
  UV R;

  if (k < 3 || mpz_sgn(n) < 0) { mpz_set_ui(r,0); return; }
  if (mpz_cmp_ui(n,1) <= 0)    { mpz_set_ui(r,1); return; }

  if (k == 4) {
    if (mpz_perfect_square_p(n)) mpz_sqrt(r, n);
    else                         mpz_set_ui(r, 0);
    return;
  }

  mpz_init(D);
  if (k == 3) {
    mpz_mul_2exp(D, n, 3);
    mpz_add_ui(D, D, 1);
  } else if (k == 5) {
    mpz_mul_ui(D, n, (8*k-16));
    mpz_add_ui(D, D, 1);
  } else {
    mpz_mul_ui(D, n, (8*k-16));
    mpz_init_set_ui(t, k-4);
    mpz_mul(t, t, t);
    mpz_add(D, D, t);
    mpz_clear(t);
  }
  if (mpz_perfect_square_p(D)) {
    mpz_sqrt(D, D);
    if (k == 3)  mpz_sub_ui(D, D, 1);
    else         mpz_add_ui(D, D, k-4);
    R = 2*k-4;
    if (mpz_divisible_ui_p(D, R)) {
      mpz_divexact_ui(r, D, R);
      mpz_clear(D);
      return;
    }
  }
  mpz_clear(D);
  mpz_set_ui(r, 0);
}


static void word_tile(uint32_t* source, uint32_t from, uint32_t to) {
  while (from < to) {
    uint32_t words = (2*from > to) ? to-from : from;
    memcpy(source+from, source, sizeof(uint32_t)*words);
    from += words;
  }
}
static void sievep_ui(uint32_t* comp, UV pos, UV p, UV len, int verbose) {
  if (!(pos & 1)) pos += p;
  if (verbose > 3) {
    for ( ; pos < len; pos += 2*p ) {
      if (!TSTAVAL(comp, pos)) {
        printf("factor: %"UVuf" at %"UVuf"\n", p, pos);
        SETAVAL(comp, pos);
      }
    }
  } else {
    for ( ; pos < len; pos += 2*p )
      SETAVAL(comp, pos);
  }
}
/* Find first multiple of p after start */
#define sievep(comp, start, p, len, verbose) \
  sievep_ui(comp, (p) - mpz_fdiv_ui((start),(p)), p, len, verbose)

uint32_t* partial_sieve(mpz_t start, UV length, UV maxprime)
{
  uint32_t* comp;
  UV p, wlen, pwlen;
  int _verbose = get_verbose_level();
  PRIME_ITERATOR(iter);

  /* mpz_init(t);
     mpz_add_ui(t, start, (length & 1) ? length-1 : length-2);
     gmp_printf("partial sieve start %Zd  length %lu mark %Zd to %Zd\n", start, length, start, t); */
  MPUassert(mpz_odd_p(start), "partial sieve given even start");
  MPUassert(length > 0, "partial sieve given zero length");
  mpz_sub_ui(start, start, 1);
  if (length & 1) length++;

  /* Allocate odds-only array in uint32_t units */
  wlen = (length+63)/64;
  New(0, comp, wlen, uint32_t);
  p = prime_iterator_next(&iter);

  /* Mark 3, 5, ... by tiling as long as we can. */
  pwlen = (wlen < 3) ? wlen : 3;
  memset(comp, 0x00, pwlen*sizeof(uint32_t));
  while (p <= maxprime) {
    sievep(comp, start, p, pwlen*64, _verbose);
    p = prime_iterator_next(&iter);
    if (pwlen*p >= wlen) break;
    word_tile(comp, pwlen, pwlen*p);
    pwlen *= p;
  }
  word_tile(comp, pwlen, wlen);

  /* Sieve up to their limit.
   *
   * Simple code for this:
   *    for ( ; p <= maxprime; p = prime_iterator_next(&iter))
   *      sievep(comp, start, p, length);
   * We'll save some time for large start values by doubling up primes.
   */
  {
    UV p1, p2;
    UV doublelim = (1UL << (sizeof(unsigned long) * 4)) - 1;
    UV ulim = (maxprime > ULONG_MAX) ? ULONG_MAX : maxprime;
    if (doublelim > maxprime) doublelim = maxprime;
    /* Do 2 primes at a time.  Fewer mpz remainders. */
    for ( p1 = p, p2 = prime_iterator_next(&iter);
          p2 <= doublelim;
          p1 = prime_iterator_next(&iter), p2 = prime_iterator_next(&iter) ) {
      UV p1p2 = p1 * p2;
      UV ddiv = mpz_fdiv_ui(start, p1p2);
      sievep_ui(comp, p1 - (ddiv % p1), p1, length, _verbose);
      sievep_ui(comp, p2 - (ddiv % p2), p2, length, _verbose);
    }
    if (p1 <= maxprime) sievep(comp, start, p1, length, _verbose);
    for (p = p2; p <= ulim; p = prime_iterator_next(&iter))
      sievep(comp, start, p, length, _verbose);
    if (p < maxprime) {
      /* UV is 64-bit, GMP's ui functions are 32-bit.  Sigh. */
      UV lastp, pos;
      mpz_t mp, rem;
      mpz_init(rem);
      mpz_init_set_ui(mp, (p >> 16) >> 16);
      mpz_mul_2exp(mp, mp, 32);
      mpz_add_ui(mp, mp, p & 0xFFFFFFFFUL);
      for (lastp = p;  p <= maxprime;  lastp=p, p=prime_iterator_next(&iter)) {
        mpz_add_ui(mp, mp, p-lastp);                 /* Calc mp = p */
        mpz_fdiv_r(rem, start, mp);                  /* Calc start % mp */
        if (mpz_cmp_ui(rem, ULONG_MAX) <= 0) {       /* pos = p - (start % p) */
          pos = p - mpz_get_ui(rem);
        } else {
          p1 = mpz_fdiv_q_ui(rem, rem, 2147483648UL);
          p1 += ((UV)mpz_get_ui(rem)) << 31;
          pos = p - p1;
        }
        sievep_ui(comp, pos, p, length, _verbose);
      }
      mpz_clear(mp);
      mpz_clear(rem);
    }
  }

  prime_iterator_destroy(&iter);
  return comp;
}


/*****************************************************************************/

void count_primes(mpz_t count, mpz_t lo, mpz_t hi)
{
  uint32_t* comp;
  UV i, cnt, hbits, depth, width;
  mpz_t shi, t;

  mpz_set_ui(count, 0);
  if (mpz_cmp_ui(lo,2) <= 0) {
    if (mpz_cmp_ui(hi,2) >= 0) mpz_add_ui(count,count,1);
    mpz_set_ui(lo,3);
  }
  if (mpz_cmp(lo,hi) > 0) return;

  mpz_init(t);
  if (mpz_add_ui(t, lo, 100000), mpz_cmp(t,hi) > 0) {
    mpz_sub_ui(lo,lo,1);
    for (cnt = 0; mpz_cmp(lo, hi) <= 0; _GMP_next_prime(lo))
      cnt++;
    mpz_add_ui(count,count,cnt-1);
    mpz_clear(t);
    return;
  }

  /* Determine a reasonable depth for sieve. */
  hbits = mpz_sizeinbase(hi,2);
  depth = (hbits < 100) ? 50000000UL : ((UV)hbits)*UVCONST(500000);
  if (hbits < 64) {
    mpz_sqrt(t, hi);
    if (mpz_cmp_ui(t,depth) < 0)
      depth = mpz_get_ui(t);
  }

  /* Count small inputs directly.  We should do this faster. */
  if (mpz_cmp_ui(lo, depth) <= 0) {
    mpz_sub_ui(lo,lo,1);
    for (cnt = 0; mpz_cmp_ui(lo, depth) <= 0; _GMP_next_prime(lo))
      cnt++;
    mpz_add_ui(count,count,cnt-1);
  }

  /* Ensure endpoints are odd */
  if (mpz_even_p(lo)) mpz_add_ui(lo,lo,1);
  if (mpz_even_p(hi)) mpz_sub_ui(hi,hi,1);

  mpz_init(shi);
  while (mpz_cmp(lo,hi) <= 0) {
    mpz_add_ui(shi, lo, 1000000000UL);
    if (mpz_cmp(shi, hi) > 0)
      mpz_set(shi, hi);
    mpz_sub(t, shi, lo);
    width = mpz_get_ui(t) + 1;

    comp = partial_sieve(lo, width, depth);
    for (i = 1, cnt = 0; i <= width; i += 2) {
      if (!TSTAVAL(comp, i) && (mpz_add_ui(t, lo, i), _GMP_BPSW(t)))
        cnt++;
    }
    Safefree(comp);

    mpz_add_ui(lo, shi, 2);
    mpz_add_ui(count, count, cnt);
  }
  mpz_clear(shi);
  mpz_clear(t);
}


typedef struct {
  uint32_t nmax;
  uint32_t nsize;
  UV* list;
} vlist;
#define INIT_VLIST(v) \
  v.nsize = 0; \
  v.nmax = 1024; \
  New(0, v.list, v.nmax, UV);
#define RESIZE_VLIST(v, size) \
  do { if (v.nmax < size) Renew(v.list, v.nmax = size, UV); } while (0)
#define PUSH_VLIST(v, n) \
  do { \
    if (v.nsize >= v.nmax) \
      Renew(v.list, v.nmax += 1024, UV); \
    v.list[v.nsize++] = n; \
  } while (0)

#define ADDVAL32(v, n, max, val) \
  do { if (n >= max) Renew(v, max += 1024, UV);  v[n++] = val; } while (0)
#define SWAPL32(l1, n1, m1,  l2, n2, m2) \
  { UV t_, *u_ = l1;  l1 = l2;  l2 = u_; \
            t_ = n1;  n1 = n2;  n2 = t_; \
            t_ = m1;  m1 = m2;  m2 = t_; }

UV* sieve_primes(mpz_t inlow, mpz_t high, UV k, UV *rn) {
  mpz_t t, low;
  int test_primality = 0, k_primality = 0, force_full = 0, width2, hbits;
  uint32_t* comp;
  vlist retlist;

  if (mpz_cmp_ui(inlow, 2) < 0) mpz_set_ui(inlow, 2);
  if (mpz_cmp(inlow, high) > 0) { *rn = 0; return 0; }

  mpz_init(t);
  mpz_sub(t, high, inlow);
  width2 = mpz_sizeinbase(t,2);
  hbits = mpz_sizeinbase(high,2);

  mpz_sqrt(t, high);           /* No need for k to be > sqrt(high) */
  /* If auto-setting k or k >= sqrt(n), pick a good depth and test primality */
  if (k == 0 || mpz_cmp_ui(t, k) <= 0) {
    test_primality = 1;
    k = (hbits < 100) ? 50000000UL : ((UV)hbits)*UVCONST(500000);
    /* For small widths we're much better off with smaller sieves. */
    if (width2 <= 23)  k /= 2;
    if (width2 <= 21)  k /= 2;
    if (width2 <= 19)  k /= 2;
    if (width2 <= 17)  k /= 2;
    if (width2 <= 15)  k /= 2;
    if (width2 <= 13)  k /= 2;
    if (width2 <= 11)  k /= 2;
  }
  /* For smallist n and large ranges, force full sieve */
  if (test_primality && hbits >= 40 && hbits <  56 && (width2 * 2.8) >= hbits)
    force_full = 1;
  if (test_primality && hbits >= 56 && hbits <  64 && (width2 * 2.6) >= hbits)
    force_full = 1;
  if (test_primality && hbits >= 64 && hbits <= 82 && (width2 * 2.5) >= hbits && sizeof(unsigned long) > 4)
    force_full = 1;
  /* If k >= sqrtn, sieving is enough.  Use k=sqrtn, turn off post-sieve test */
  if (force_full || mpz_cmp_ui(t, k) <= 0) {
    k = mpz_get_ui(t);
    k_primality = 1;           /* Our sieve is complete */
    test_primality = 0;        /* Don't run BPSW */
  }

  INIT_VLIST(retlist);

  /* If we want small primes, do it quickly */
  if ( (k_primality || test_primality) && mpz_cmp_ui(high,2000000000U) <= 0 ) {
    UV ulow = mpz_get_ui(inlow), uhigh = mpz_get_ui(high);
    if (uhigh < 1000000U || uhigh/ulow >= 4) {
      UV n, Pi, *primes;
      primes = sieve_to_n(mpz_get_ui(high), &Pi);
      RESIZE_VLIST(retlist, Pi);
      for (n = 0; n < Pi; n++) {
        if (primes[n] >= ulow)
          PUSH_VLIST(retlist, primes[n]-ulow);
      }
      Safefree(primes);
      mpz_clear(t);
      *rn = retlist.nsize;
      return retlist.list;
    }
  }

  mpz_init_set(low, inlow);
  if (k < 2) k = 2;   /* Should have been handled by quick return */

  /* Include all primes up to k, since they will get filtered */
  if (mpz_cmp_ui(low, k) <= 0) {
    UV n, Pi, *primes, ulow = mpz_get_ui(low);
    primes = sieve_to_n(k, &Pi);
    RESIZE_VLIST(retlist, Pi);
    for (n = 0; n < Pi; n++) {
      if (primes[n] >= ulow)
        PUSH_VLIST(retlist, primes[n]-ulow);
    }
    Safefree(primes);
  }

  if (mpz_even_p(low))           mpz_add_ui(low, low, 1);
  if (mpz_even_p(high))          mpz_sub_ui(high, high, 1);

  if (mpz_cmp(low, high) <= 0) {
    UV i, length, offset;
    mpz_sub(t, high, low); length = mpz_get_ui(t) + 1;
    /* Get bit array of odds marked with composites(k) marked with 1 */
    comp = partial_sieve(low, length, k);
    mpz_sub(t, low, inlow); offset = mpz_get_ui(t);
    for (i = 1; i <= length; i += 2) {
      if (!TSTAVAL(comp, i)) {
        if (!test_primality || (mpz_add_ui(t,low,i),_GMP_BPSW(t)))
          PUSH_VLIST(retlist, i - offset);
      }
    }
    Safefree(comp);
  }

  mpz_clear(low);
  mpz_clear(t);
  *rn = retlist.nsize;
  return retlist.list;
}

UV* sieve_twin_primes(mpz_t low, mpz_t high, UV twin, UV *rn) {
  mpz_t t;
  UV i, length, k, starti = 1, skipi = 2;
  uint32_t* comp;
  vlist retlist;

  /* Twin offset will always be even, so we will never return 2 */
  MPUassert( !(twin & 1), "twin prime offset is even" );
  if (mpz_cmp_ui(low, 3) <= 0) mpz_set_ui(low, 3);

  if (mpz_even_p(low))  mpz_add_ui(low, low, 1);
  if (mpz_even_p(high)) mpz_sub_ui(high, high, 1);

  i = twin % 6;
  if (i == 2 || i == 4) { skipi = 6; starti = ((twin%6)==2) ? 5 : 1; }

  /* If no way to return any more results, leave now */
  if (mpz_cmp(low, high) > 0 || (i == 1 || i == 3 || i == 5))
    { *rn = 0; return 0; }

  INIT_VLIST(retlist);
  mpz_init(t);

  /* Use a much higher k value than we do for primes */
  k = 80000 * mpz_sizeinbase(high,2);
  /* No need for k to be > sqrt(high) */
  mpz_sqrt(t, high);
  if (mpz_cmp_ui(t, k) < 0)
    k = mpz_get_ui(t);

  /* Handle small primes that will get sieved out */
  if (mpz_cmp_ui(low, k) <= 0) {
    UV p, ulow = mpz_get_ui(low);
    PRIME_ITERATOR(iter);
    for (p = 2; p <= k; p = prime_iterator_next(&iter)) {
      if (p < ulow) continue;
      if (mpz_set_ui(t, p+twin), _GMP_BPSW(t))
        PUSH_VLIST(retlist, p-ulow+1);
    }
    prime_iterator_destroy(&iter);
  }

  mpz_sub(t, high, low);
  length = mpz_get_ui(t) + 1;
  starti = ((starti+skipi) - mpz_fdiv_ui(low,skipi) + 1) % skipi;

  /* Get bit array of odds marked with composites(k) marked with 1 */
  comp = partial_sieve(low, length + twin, k);
  for (i = starti; i <= length; i += skipi) {
    if (!TSTAVAL(comp, i) && !TSTAVAL(comp, i+twin)) {
      /* Add to list if both t,t+2 pass MR and if both pass ES Lucas */
      if ( (mpz_add_ui(t,low,i),  miller_rabin_ui(t,2)) &&
           (mpz_add_ui(t,t,twin), miller_rabin_ui(t,2)) &&
           (mpz_add_ui(t,low,i),  _GMP_is_lucas_pseudoprime(t,2)) &&
           (mpz_add_ui(t,t,twin), _GMP_is_lucas_pseudoprime(t,2)) )
        PUSH_VLIST(retlist, i);
    }
  }
  Safefree(comp);
  mpz_clear(t);
  *rn = retlist.nsize;
  return retlist.list;
}


#define addmodded(r,a,b,n)  do { r = a + b; if (r >= n) r -= n; } while(0)

UV* sieve_cluster(mpz_t low, mpz_t high, uint32_t* cl, UV nc, UV *rn) {
  mpz_t t, savelow;
  vlist retlist;
  UV i, ppr, nres, allocres;
  uint32_t const targres = 4000000;
  uint32_t const maxpi = 168;
  UV *residues, *cres;
  uint32_t pp_0, pp_1, pp_2, *resmod_0, *resmod_1, *resmod_2;
  uint32_t rem_0, rem_1, rem_2, remadd_0, remadd_1, remadd_2;
  uint32_t pi, startpi = 1;
  uint32_t lastspr = sprimes[maxpi-1];
  uint32_t c, smallnc;
  UV ibase = 0, num_mr = 0, num_lucas = 0;
  char crem_0[53*59], crem_1[61*67], crem_2[71*73], *VPrem;
  int run_pretests = 0;
  int _verbose = get_verbose_level();

  if (nc == 1) return sieve_primes(low, high, 0, rn);
  if (nc == 2) return sieve_twin_primes(low, high, cl[1], rn);

  if (mpz_even_p(low))           mpz_add_ui(low, low, 1);
  if (mpz_even_p(high))          mpz_sub_ui(high, high, 1);

  if (mpz_cmp(low, high) > 0) { *rn = 0; return 0; }

  INIT_VLIST(retlist);
  mpz_init(t);

  /* Handle small values that would get sieved away */
  if (mpz_cmp_ui(low, lastspr) <= 0) {
    UV ui_low = mpz_get_ui(low);
    UV ui_high = (mpz_cmp_ui(high,lastspr) > 0) ? lastspr : mpz_get_ui(high);
    for (pi = 0; pi < maxpi; pi++) {
      UV p = sprimes[pi];
      if (p > ui_high) break;
      if (p < ui_low) continue;
      for (c = 1; c < nc; c++)
        if (!(mpz_set_ui(t, p+cl[c]), _GMP_is_prob_prime(t))) break;
      if (c == nc)
        PUSH_VLIST(retlist, p-ui_low+1);
    }
  }
  if (mpz_odd_p(low)) mpz_sub_ui(low, low, 1);
  if (mpz_cmp_ui(high, lastspr) <= 0) {
    mpz_clear(t);
    *rn = retlist.nsize;
    return retlist.list;
  }

  /* Determine the primorial size and acceptable residues */
  New(0, residues, allocres = 1024, UV);
  {
    UV remr, *res2, allocres2, nres2, maxppr;
    /* Calculate residues for a small primorial */
    for (pi = 2, ppr = 1, i = 0;  i <= pi;  i++) ppr *= sprimes[i];
    remr = mpz_fdiv_ui(low, ppr);
    nres = 0;
    for (i = 1; i <= ppr; i += 2) {
      UV remi = remr + i;
      for (c = 0; c < nc; c++) {
        if (gcd_ui(remi + cl[c], ppr) != 1) break;
      }
      if (c == nc)
        ADDVAL32(residues, nres, allocres, i);
    }
    /* Raise primorial size until we have plenty of residues */
    New(0, res2, allocres2 = 1024, UV);
    mpz_sub(t, high, low);
    maxppr = (mpz_sizeinbase(t,2) >= BITS_PER_WORD) ? UV_MAX : (UVCONST(1) << mpz_sizeinbase(t,2));
#if BITS_PER_WORD == 64
    while (pi++ < 14) {
#else
    while (pi++ < 8) {
#endif
      uint32_t j, p = sprimes[pi];
      UV r, newppr = ppr * p;
      if (nres == 0 || nres > targres/(p/2) || newppr > maxppr) break;
      if (_verbose > 1) printf("cluster sieve found %"UVuf" residues mod %"UVuf"\n", nres, ppr);
      remr = mpz_fdiv_ui(low, newppr);
      nres2 = 0;
      for (i = 0; i < p; i++) {
        for (j = 0; j < nres; j++) {
          r = i*ppr + residues[j];
          for (c = 0; c < nc; c++) {
            UV v = remr + r + cl[c];
            if ((v % p) == 0) break;
          }
          if (c == nc)
            ADDVAL32(res2, nres2, allocres2, r);
        }
      }
      ppr = newppr;
      SWAPL32(residues, nres, allocres,  res2, nres2, allocres2);
    }
    startpi = pi;
    Safefree(res2);
  }
  if (_verbose) printf("cluster sieve using %"UVuf" residues mod %"UVuf"\n", nres, ppr);

  /* Return if not admissible, maybe with a single small value */
  if (nres == 0) {
    Safefree(residues);
    mpz_clear(t);
    *rn = retlist.nsize;
    return retlist.list;
  }

  mpz_init_set(savelow, low);
  if (mpz_sizeinbase(low, 2) > 310) run_pretests = 1;
  if (run_pretests && mpz_sgn(_bgcd2) == 0) {
    _GMP_pn_primorial(_bgcd2, BGCD2_PRIMES);
    mpz_divexact(_bgcd2, _bgcd2, _bgcd);
  }

  /* Pre-mod the residues with first two primes for fewer modulos every chunk */
  {
    uint32_t p1 = sprimes[startpi+0], p2 = sprimes[startpi+1];
    uint32_t p3 = sprimes[startpi+2], p4 = sprimes[startpi+3];
    uint32_t p5 = sprimes[startpi+4], p6 = sprimes[startpi+5];
    pp_0 = p1*p2; pp_1 = p3*p4; pp_2 = p5*p6;
    memset(crem_0, 1, pp_0);
    memset(crem_1, 1, pp_1);
    memset(crem_2, 1, pp_2);
    /* Mark remainders that indicate a composite for this residue. */
    for (i = 0; i < p1; i++) { crem_0[i*p1]=0; crem_0[i*p2]=0; }
    for (     ; i < p2; i++) { crem_0[i*p1]=0;                }
    for (i = 0; i < p3; i++) { crem_1[i*p3]=0; crem_1[i*p4]=0; }
    for (     ; i < p4; i++) { crem_1[i*p3]=0;                }
    for (i = 0; i < p5; i++) { crem_2[i*p5]=0; crem_2[i*p6]=0; }
    for (     ; i < p6; i++) { crem_2[i*p5]=0;                }
    for (c = 1; c < nc; c++) {
      uint32_t c1=cl[c], c2=cl[c], c3=cl[c], c4=cl[c], c5=cl[c], c6=cl[c];
      if (c1 >= p1) c1 %= p1;
      if (c2 >= p2) c2 %= p2;
      for (i = 1; i <= p1; i++) { crem_0[i*p1-c1]=0; crem_0[i*p2-c2]=0; }
      for (     ; i <= p2; i++) { crem_0[i*p1-c1]=0;                   }
      if (c3 >= p3) c3 %= p3;
      if (c4 >= p4) c4 %= p4;
      for (i = 1; i <= p3; i++) { crem_1[i*p3-c3]=0; crem_1[i*p4-c4]=0; }
      for (     ; i <= p4; i++) { crem_1[i*p3-c3]=0;                   }
      if (c5 >= p5) c5 %= p5;
      if (c6 >= p6) c6 %= p6;
      for (i = 1; i <= p5; i++) { crem_2[i*p5-c5]=0; crem_2[i*p6-c6]=0; }
      for (     ; i <= p6; i++) { crem_2[i*p5-c5]=0;                   }
    }
    New(0, resmod_0, nres, uint32_t);
    New(0, resmod_1, nres, uint32_t);
    New(0, resmod_2, nres, uint32_t);
    for (i = 0; i < nres; i++) {
      resmod_0[i] = residues[i] % pp_0;
      resmod_1[i] = residues[i] % pp_1;
      resmod_2[i] = residues[i] % pp_2;
    }
  }

  /* Precalculate acceptable residues for more primes */
  MPUassert( lastspr <= 1024, "cluster sieve internal" );
  New(0, VPrem, maxpi * 1024, char);
  memset(VPrem, 1, maxpi * 1024);
  for (pi = startpi+6; pi < maxpi; pi++)
    VPrem[pi*1024] = 0;
  for (pi = startpi+6, smallnc = 0; pi < maxpi; pi++) {
    uint32_t p = sprimes[pi];
    char* prem = VPrem + pi*1024;
    while (smallnc < nc && cl[smallnc] < p)   smallnc++;
    for (c = 1; c < smallnc; c++) prem[p-cl[c]] = 0;
    for (     ; c <      nc; c++) prem[p-(cl[c]%p)] = 0;
  }

  New(0, cres, nres, UV);

  rem_0 = mpz_fdiv_ui(low,pp_0);  remadd_0 = ppr % pp_0;
  rem_1 = mpz_fdiv_ui(low,pp_1);  remadd_1 = ppr % pp_1;
  rem_2 = mpz_fdiv_ui(low,pp_2);  remadd_2 = ppr % pp_2;

  /* Loop over their range in chunks of size 'ppr' */
  while (mpz_cmp(low, high) <= 0) {

    uint32_t r, nr, remr, ncres;
    unsigned long ui_low = (mpz_sizeinbase(low,2) > 8*sizeof(unsigned long)) ? 0 : mpz_get_ui(low);

    /* Reduce the allowed residues for this chunk using more primes */

    { /* Start making a list of this chunk's residues using three pairs */
      for (r = 0, ncres = 0; r < nres; r++) {
        addmodded(remr, rem_0, resmod_0[r], pp_0);
        if (crem_0[remr]) {
          addmodded(remr, rem_1, resmod_1[r], pp_1);
          if (crem_1[remr]) {
            addmodded(remr, rem_2, resmod_2[r], pp_2);
            if (crem_2[remr]) {
              cres[ncres++] = residues[r];
            }
          }
        }
      }
      addmodded(rem_0, rem_0, remadd_0, pp_0);
      addmodded(rem_1, rem_1, remadd_1, pp_1);
      addmodded(rem_2, rem_2, remadd_2, pp_2);
    }

    /* Sieve through more primes one at a time, removing residues. */
    for (pi = startpi+6; pi < maxpi && ncres > 0; pi++) {
      uint32_t p = sprimes[pi];
      uint32_t rem = (ui_low) ? (ui_low % p) : mpz_fdiv_ui(low,p);
      char* prem = VPrem + pi*1024;
      /* Check divisibility of each remaining residue with this p */
      if (startpi <= 9 || cres[ncres-1] < 4294967295U) {   /* Residues are 32-bit */
        for (r = 0, nr = 0; r < ncres; r++) {
          if (prem[ (rem+(uint32_t)cres[r]) % p ])
            cres[nr++] = cres[r];
        }
      } else {              /* Residues are 64-bit */
        for (r = 0, nr = 0; r < ncres; r++) {
          if (prem[ (rem+cres[r]) % p ])
            cres[nr++] = cres[r];
        }
      }
      ncres = nr;
    }
    if (_verbose > 2) printf("cluster sieve range has %u residues left\n", ncres);

    /* Now check each of the remaining residues for inclusion */
    for (r = 0; r < ncres; r++) {
      i = cres[r];
      mpz_add_ui(t, low, i);
      if (mpz_cmp(t, high) > 0) break;
      /* Pretest each element if the input is large enough */
      if (run_pretests) {
        for (c = 0; c < nc; c++)
          if (mpz_add_ui(t, low, i+cl[c]), mpz_gcd(t,t,_bgcd2), mpz_cmp_ui(t,1)) break;
        if (c != nc) continue;
      }
      /* PRP test.  Split BPSW in two for faster rejection. */
      for (c = 0; c < nc; c++)
        if (! (mpz_add_ui(t, low, i+cl[c]), num_mr++, miller_rabin_ui(t,2)) ) break;
      if (c != nc) continue;
      for (c = 0; c < nc; c++)
        if (! (mpz_add_ui(t, low, i+cl[c]), num_lucas++, _GMP_is_lucas_pseudoprime(t,2)) ) break;
      if (c != nc) continue;
      PUSH_VLIST(retlist, ibase + i);
    }
    ibase += ppr;
    mpz_add_ui(low, low, ppr);
  }

  if (_verbose) printf("cluster sieve ran %"UVuf" MR and %"UVuf" Lucas tests (pretests %s)\n", num_mr, num_lucas, run_pretests ? "on" : "off");
  mpz_set(low, savelow);
  Safefree(cres);
  Safefree(VPrem);
  Safefree(resmod_0);
  Safefree(resmod_1);
  Safefree(resmod_2);
  Safefree(residues);
  mpz_clear(savelow);
  mpz_clear(t);
  *rn = retlist.nsize;
  return retlist.list;
}
