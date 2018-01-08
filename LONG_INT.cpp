#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exception>

#include "LONG_INT.h"

/* Macros */

#define _li_invert_unit(x) _li_invert_3by2 ((x), 0)
#define li_odd_p(z)   (((z)->RealLen != 0) & (int) (z)->Units[0])
#define li_even_p(z)  (! li_odd_p (z))

#define LONG_INT_UNIT_MAX (~ (LI_UNIT_T) 0)
#define LI_UNIT_HIGHBIT ((LI_UNIT_T) 1 << (BITS_PER_LONG_INT_UNIT - 1))

#define LI_HUNIT_BIT ((LI_UNIT_T) 1 << (BITS_PER_LONG_INT_UNIT / 2))
#define LI_LUNIT_MASK (LI_HUNIT_BIT - 1)

#define LI_ULONG_BITS (sizeof(unsigned long) * CHAR_BIT)
#define LI_ULONG_HIGHBIT ((unsigned long) 1 << (LI_ULONG_BITS - 1))

#define LI_ABS(x) ((x) >= 0 ? (x) : -(x))
#define LI_NEG_CAST(T,x) (-((T)((x) + 1) - 1))

#define LI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LI_MAX(a, b) ((a) > (b) ? (a) : (b))

#define LI_CMP(a,b) (((a) > (b)) - ((a) < (b)))

#define gmp_assert_nocarry(x) do { \
    LI_UNIT_T __cy = (x);	   \
    assert (__cy == 0);		   \
  } while (0)

#define gmp_clz(count, x) do {						\
    LI_UNIT_T __clz_x = (x);						\
    unsigned __clz_c;							\
    for (__clz_c = 0;							\
	 (__clz_x & ((LI_UNIT_T) 0xff << (BITS_PER_LONG_INT_UNIT - 8))) == 0;	\
	 __clz_c += 8)							\
      __clz_x <<= 8;							\
    for (; (__clz_x & LI_UNIT_HIGHBIT) == 0; __clz_c++)		\
      __clz_x <<= 1;							\
    (count) = __clz_c;							\
  } while (0)

#define gmp_ctz(count, x) do {						\
    LI_UNIT_T __ctz_x = (x);						\
    unsigned __ctz_c = 0;						\
    gmp_clz (__ctz_c, __ctz_x & - __ctz_x);				\
    (count) = BITS_PER_LONG_INT_UNIT - 1 - __ctz_c;				\
  } while (0)

#define gmp_add_ssaaaa(sh, sl, ah, al, bh, bl) \
  do {									\
    LI_UNIT_T __x;							\
    __x = (al) + (bl);							\
    (sh) = (ah) + (bh) + (__x < (al));					\
    (sl) = __x;								\
  } while (0)

#define gmp_sub_ddmmss(sh, sl, ah, al, bh, bl) \
  do {									\
    LI_UNIT_T __x;							\
    __x = (al) - (bl);							\
    (sh) = (ah) - (bh) - ((al) < (bl));					\
    (sl) = __x;								\
  } while (0)

#define gmp_umul_ppmm(w1, w0, u, v)					\
  do {									\
    LI_UNIT_T __x0, __x1, __x2, __x3;					\
    unsigned __ul, __vl, __uh, __vh;					\
    LI_UNIT_T __u = (u), __v = (v);					\
									\
    __ul = __u & LI_LUNIT_MASK;					\
    __uh = __u >> (BITS_PER_LONG_INT_UNIT / 2);					\
    __vl = __v & LI_LUNIT_MASK;					\
    __vh = __v >> (BITS_PER_LONG_INT_UNIT / 2);					\
									\
    __x0 = (LI_UNIT_T) __ul * __vl;					\
    __x1 = (LI_UNIT_T) __ul * __vh;					\
    __x2 = (LI_UNIT_T) __uh * __vl;					\
    __x3 = (LI_UNIT_T) __uh * __vh;					\
									\
    __x1 += __x0 >> (BITS_PER_LONG_INT_UNIT / 2);/* this can't give carry */	\
    __x1 += __x2;		/* but this indeed can */		\
    if (__x1 < __x2)		/* did we get it? */			\
      __x3 += LI_HUNIT_BIT;	/* yes, add it in the proper pos. */	\
									\
    (w1) = __x3 + (__x1 >> (BITS_PER_LONG_INT_UNIT / 2));			\
    (w0) = (__x1 << (BITS_PER_LONG_INT_UNIT / 2)) + (__x0 & LI_LUNIT_MASK);	\
  } while (0)

#define gmp_udiv_qrnnd_preinv(q, r, nh, nl, d, di)			\
  do {									\
    LI_UNIT_T _qh, _ql, _r, _mask;					\
    gmp_umul_ppmm (_qh, _ql, (nh), (di));				\
    gmp_add_ssaaaa (_qh, _ql, _qh, _ql, (nh) + 1, (nl));		\
    _r = (nl) - _qh * (d);						\
    _mask = -(LI_UNIT_T) (_r > _ql); /* both > and >= are OK */		\
    _qh += _mask;							\
    _r += _mask & (d);							\
    if (_r >= (d))							\
      {									\
	_r -= (d);							\
	_qh++;								\
      }									\
									\
    (r) = _r;								\
    (q) = _qh;								\
  } while (0)

#define gmp_udiv_qr_3by2(q, r1, r0, n2, n1, n0, d1, d0, dinv)		\
  do {									\
    LI_UNIT_T _q0, _t1, _t0, _mask;					\
    gmp_umul_ppmm ((q), _q0, (n2), (dinv));				\
    gmp_add_ssaaaa ((q), _q0, (q), _q0, (n2), (n1));			\
									\
    /* Compute the two most significant units of n - q'd */		\
    (r1) = (n1) - (d1) * (q);						\
    gmp_sub_ddmmss ((r1), (r0), (r1), (n0), (d1), (d0));		\
    gmp_umul_ppmm (_t1, _t0, (d0), (q));				\
    gmp_sub_ddmmss ((r1), (r0), (r1), (r0), _t1, _t0);			\
    (q)++;								\
									\
    /* Conditionally adjust q and the remainders */			\
    _mask = - (LI_UNIT_T) ((r1) >= _q0);				\
    (q) += _mask;							\
    gmp_add_ssaaaa ((r1), (r0), (r1), (r0), _mask & (d1), _mask & (d0)); \
    if ((r1) >= (d1))							\
      {									\
	if ((r1) > (d1) || (r0) >= (d0))				\
	  {								\
	    (q)++;							\
	    gmp_sub_ddmmss ((r1), (r0), (r1), (r0), (d1), (d0));	\
	  }								\
      }									\
  } while (0)

/* Swap macros. */
#define LI_UNIT_T_SWAP(x, y)						\
  do {									\
    LI_UNIT_T __LI_UNIT_T_swap__tmp = (x);				\
    (x) = (y);								\
    (y) = __LI_UNIT_T_swap__tmp;					\
  } while (0)
#define MP_SIZE_T_SWAP(x, y)						\
  do {									\
    LI_SIZE_T __LI_SIZE_T_swap__tmp = (x);				\
    (x) = (y);								\
    (y) = __LI_SIZE_T_swap__tmp;					\
  } while (0)
#define MP_BITCNT_T_SWAP(x,y)			\
  do {						\
    LI_BIT_CNT_T __LI_BIT_CNT_T_swap__tmp = (x);	\
    (x) = (y);					\
    (y) = __LI_BIT_CNT_T_swap__tmp;		\
  } while (0)
#define MP_PTR_SWAP(x, y)						\
  do {									\
    LI_UNIT_PTR __LI_UNIT_PTR_swap__tmp = (x);					\
    (x) = (y);								\
    (y) = __LI_UNIT_PTR_swap__tmp;						\
  } while (0)
#define MP_SRCPTR_SWAP(x, y)						\
  do {									\
    LI_UNIT_CPTR __LI_UNIT_CPTR_swap__tmp = (x);				\
    (x) = (y);								\
    (y) = __LI_UNIT_CPTR_swap__tmp;					\
  } while (0)

#define _li_PTR_SWAP(xp,xs, yp,ys)					\
  do {									\
    MP_PTR_SWAP (xp, yp);						\
    MP_SIZE_T_SWAP (xs, ys);						\
  } while(0)
#define _li_SRCPTR_SWAP(xp,xs, yp,ys)					\
  do {									\
    MP_SRCPTR_SWAP (xp, yp);						\
    MP_SIZE_T_SWAP (xs, ys);						\
  } while(0)

#define MPZ_PTR_SWAP(x, y)						\
  do {									\
    LI_PTR __LI_PTR_swap__tmp = (x);					\
    (x) = (y);								\
    (y) = __LI_PTR_swap__tmp;						\
  } while (0)
#define MPZ_SRCPTR_SWAP(x, y)						\
  do {									\
    LI_CPTR __LI_CPTR_swap__tmp = (x);				\
    (x) = (y);								\
    (y) = __LI_CPTR_swap__tmp;					\
  } while (0)


/* Memory allocation and other helper functions. */
static void li_exception(const char *msg)
{
	//  fDsplStatus (stderr, "%s\n", msg);
	//  abort();
	throw std::exception(msg);
}

static void *li_default_alloc(size_t size)
{
	void *p;

	assert(size > 0);

	p = malloc(size);
	if (!p)
		li_exception("li_default_alloc: Virtual memory exhausted.");

	return p;
}

static void *li_default_realloc(void *old, size_t old_size, size_t new_size)
{
	void * p;

	p = realloc(old, new_size);

	if (!p)
		li_exception("li_default_realloc: Virtual memory exhausted.");

	return p;
}

static void li_default_free(void *p, size_t size)
{

	free(p);
}

static void * (*li_allocate_func) (size_t) = li_default_alloc;
static void * (*li_reallocate_func) (void *, size_t, size_t) = li_default_realloc;
static void(*li_free_func) (void *, size_t) = li_default_free;

void
mp_get_memory_functions(void *(**alloc_func) (size_t),
	void *(**realloc_func) (void *, size_t, size_t),
	void(**free_func) (void *, size_t))
{
	if (alloc_func)
		*alloc_func = li_allocate_func;

	if (realloc_func)
		*realloc_func = li_reallocate_func;

	if (free_func)
		*free_func = li_free_func;
}

void
mp_set_memory_functions(void *(*alloc_func) (size_t),
	void *(*realloc_func) (void *, size_t, size_t),
	void(*free_func) (void *, size_t))
{
	if (!alloc_func)
		alloc_func = li_default_alloc;
	if (!realloc_func)
		realloc_func = li_default_realloc;
	if (!free_func)
		free_func = li_default_free;

	li_allocate_func = alloc_func;
	li_reallocate_func = realloc_func;
	li_free_func = free_func;
}

#define gmp_xalloc(size) ((*li_allocate_func)((size)))
#define gmp_free(p) ((*li_free_func) ((p), 0))

static LI_UNIT_PTR
gmp_xalloc_units(LI_SIZE_T size)
{
	return (LI_UNIT_PTR)gmp_xalloc(size * sizeof(LI_UNIT_T));
}

static LI_UNIT_PTR
gmp_xrealloc_units(LI_UNIT_PTR old, LI_SIZE_T size)
{
	assert(size > 0);
	return (LI_UNIT_PTR)(*li_reallocate_func) (old, 0, size * sizeof(LI_UNIT_T));
}


/* MPN interface */

void
_li_copyi(LI_UNIT_PTR d, LI_UNIT_CPTR s, LI_SIZE_T n)
{
	LI_SIZE_T i;
	for (i = 0; i < n; i++)
		d[i] = s[i];
}

void
_li_copyd(LI_UNIT_PTR d, LI_UNIT_CPTR s, LI_SIZE_T n)
{
	while (--n >= 0)
		d[n] = s[n];
}

int
_li_cmp(LI_UNIT_CPTR ap, LI_UNIT_CPTR bp, LI_SIZE_T n)
{
	while (--n >= 0)
	{
		if (ap[n] != bp[n])
			return ap[n] > bp[n] ? 1 : -1;
	}
	return 0;
}

static int
_li_cmp4(LI_UNIT_CPTR ap, LI_SIZE_T an, LI_UNIT_CPTR bp, LI_SIZE_T bn)
{
	if (an != bn)
		return an < bn ? -1 : 1;
	else
		return _li_cmp(ap, bp, an);
}

static LI_SIZE_T
_li_normalized_size(LI_UNIT_CPTR xp, LI_SIZE_T n)
{
	while (n > 0 && xp[n - 1] == 0)
		--n;
	return n;
}

int
_li_zero_p(LI_UNIT_CPTR rp, LI_SIZE_T n)
{
	return _li_normalized_size(rp, n) == 0;
}

void
_li_zero(LI_UNIT_PTR rp, LI_SIZE_T n)
{
	while (--n >= 0)
		rp[n] = 0;
}


LI_UNIT_T _li_add_1(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_SIZE_T n, LI_UNIT_T b)
{
	LI_SIZE_T i;

	assert(n > 0);
	i = 0;
	do
	{
		LI_UNIT_T r = ap[i] + b;
		// Carry out 
		b = (r < b);
		rp[i] = r;
	} while (++i < n);

	return b;
}

LI_UNIT_T _li_add_n(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_UNIT_CPTR bp, LI_SIZE_T n)
{
	LI_SIZE_T i;
	LI_UNIT_T cy;

	for (i = 0, cy = 0; i < n; i++)
	{
		LI_UNIT_T a, b, r;
		a = ap[i]; b = bp[i];
		r = a + cy;
		cy = (r < cy);
		r += b;
		cy += (r < b);
		rp[i] = r;
	}
	return cy;
}

LI_UNIT_T
_li_add(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_SIZE_T an, LI_UNIT_CPTR bp, LI_SIZE_T bn)
{
	LI_UNIT_T cy;

	assert(an >= bn);

	cy = _li_add_n(rp, ap, bp, bn);
	if (an > bn)
		cy = _li_add_1(rp + bn, ap + bn, an - bn, cy);
	return cy;
}

LI_UNIT_T
_li_sub_1(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_SIZE_T n, LI_UNIT_T b)
{
	LI_SIZE_T i;

	assert(n > 0);

	i = 0;
	do
	{
		LI_UNIT_T a = ap[i];
		// Carry out 
		LI_UNIT_T cy = a < b;
		rp[i] = a - b;
		b = cy;
	} while (++i < n);

	return b;
}

LI_UNIT_T
_li_sub_n(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_UNIT_CPTR bp, LI_SIZE_T n)
{
	LI_SIZE_T i;
	LI_UNIT_T cy;

	for (i = 0, cy = 0; i < n; i++)
	{
		LI_UNIT_T a, b;
		a = ap[i]; b = bp[i];
		b += cy;
		cy = (b < cy);
		cy += (a < b);
		rp[i] = a - b;
	}
	return cy;
}

LI_UNIT_T
_li_sub(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_SIZE_T an, LI_UNIT_CPTR bp, LI_SIZE_T bn)
{
	LI_UNIT_T cy;

	assert(an >= bn);

	cy = _li_sub_n(rp, ap, bp, bn);
	if (an > bn)
		cy = _li_sub_1(rp + bn, ap + bn, an - bn, cy);
	return cy;
}

LI_UNIT_T
_li_mul_1(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n, LI_UNIT_T vl)
{
	LI_UNIT_T ul, cl, hpl, lpl;

	assert(n >= 1);

	cl = 0;
	do
	{
		ul = *up++;
		gmp_umul_ppmm(hpl, lpl, ul, vl);

		lpl += cl;
		cl = (lpl < cl) + hpl;

		*rp++ = lpl;
	} while (--n != 0);

	return cl;
}

LI_UNIT_T
_li_addmul_1(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n, LI_UNIT_T vl)
{
	LI_UNIT_T ul, cl, hpl, lpl, rl;

	assert(n >= 1);

	cl = 0;
	do
	{
		ul = *up++;
		gmp_umul_ppmm(hpl, lpl, ul, vl);

		lpl += cl;
		cl = (lpl < cl) + hpl;

		rl = *rp;
		lpl = rl + lpl;
		cl += lpl < rl;
		*rp++ = lpl;
	} while (--n != 0);

	return cl;
}

LI_UNIT_T
_li_submul_1(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n, LI_UNIT_T vl)
{
	LI_UNIT_T ul, cl, hpl, lpl, rl;

	assert(n >= 1);

	cl = 0;
	do
	{
		ul = *up++;
		gmp_umul_ppmm(hpl, lpl, ul, vl);

		lpl += cl;
		cl = (lpl < cl) + hpl;

		rl = *rp;
		lpl = rl - lpl;
		cl += lpl > rl;
		*rp++ = lpl;
	} while (--n != 0);

	return cl;
}

LI_UNIT_T
_li_mul(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T un, LI_UNIT_CPTR vp, LI_SIZE_T vn)
{
	assert(un >= vn);
	assert(vn >= 1);

	// We first multiply by the low order unit. This result can be
	//   stored, not added, to rp. We also avoid a loop for zeroing this way. 

	rp[un] = _li_mul_1(rp, up, un, vp[0]);

	// Now accumulate the product of up[] and the next higher unit from vp[]. 

	while (--vn >= 1)
	{
		rp += 1, vp += 1;
		rp[un] = _li_addmul_1(rp, up, un, vp[0]);
	}
	return rp[un];
}

void
_li_mul_n(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_UNIT_CPTR bp, LI_SIZE_T n)
{
	_li_mul(rp, ap, n, bp, n);
}

void
_li_sqr(LI_UNIT_PTR rp, LI_UNIT_CPTR ap, LI_SIZE_T n)
{
	_li_mul(rp, ap, n, ap, n);
}

LI_UNIT_T
_li_lshift(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n, unsigned int cnt)
{
	LI_UNIT_T high_unit, low_unit;
	unsigned int tnc;
	LI_UNIT_T retval;

	assert(n >= 1);
	assert(cnt >= 1);
	assert(cnt < BITS_PER_LONG_INT_UNIT);

	up += n;
	rp += n;

	tnc = BITS_PER_LONG_INT_UNIT - cnt;
	low_unit = *--up;
	retval = low_unit >> tnc;
	high_unit = (low_unit << cnt);

	while (--n != 0)
	{
		low_unit = *--up;
		*--rp = high_unit | (low_unit >> tnc);
		high_unit = (low_unit << cnt);
	}
	*--rp = high_unit;

	return retval;
}

LI_UNIT_T
_li_rshift(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n, unsigned int cnt)
{
	LI_UNIT_T high_unit, low_unit;
	unsigned int tnc;
	LI_UNIT_T retval;

	assert(n >= 1);
	assert(cnt >= 1);
	assert(cnt < BITS_PER_LONG_INT_UNIT);

	tnc = BITS_PER_LONG_INT_UNIT - cnt;
	high_unit = *up++;
	retval = (high_unit << tnc);
	low_unit = high_unit >> cnt;

	while (--n != 0)
	{
		high_unit = *up++;
		*rp++ = low_unit | (high_unit << tnc);
		low_unit = high_unit >> cnt;
	}
	*rp = low_unit;

	return retval;
}

static LI_BIT_CNT_T
_li_common_scan(LI_UNIT_T unit, LI_SIZE_T i, LI_UNIT_CPTR up, LI_SIZE_T un,
	LI_UNIT_T ux)
{
	unsigned cnt;

	assert(ux == 0 || ux == LONG_INT_UNIT_MAX);
	assert(0 <= i && i <= un);

	while (unit == 0)
	{
		i++;
		if (i == un)
			return (ux == 0 ? ~(LI_BIT_CNT_T)0 : un * BITS_PER_LONG_INT_UNIT);
		unit = ux ^ up[i];
	}
	gmp_ctz(cnt, unit);
	return (LI_BIT_CNT_T)i * BITS_PER_LONG_INT_UNIT + cnt;
}

LI_BIT_CNT_T
_li_scan1(LI_UNIT_CPTR ptr, LI_BIT_CNT_T bit)
{
	LI_SIZE_T i;
	i = bit / BITS_PER_LONG_INT_UNIT;

	return _li_common_scan(ptr[i] & (LONG_INT_UNIT_MAX << (bit % BITS_PER_LONG_INT_UNIT)),
		i, ptr, i, 0);
}

LI_BIT_CNT_T
_li_scan0(LI_UNIT_CPTR ptr, LI_BIT_CNT_T bit)
{
	LI_SIZE_T i;
	i = bit / BITS_PER_LONG_INT_UNIT;

	return _li_common_scan(~ptr[i] & (LONG_INT_UNIT_MAX << (bit % BITS_PER_LONG_INT_UNIT)),
		i, ptr, i, LONG_INT_UNIT_MAX);
}

void
_li_com(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n)
{
	while (--n >= 0)
		*rp++ = ~*up++;
}

LI_UNIT_T
_li_neg(LI_UNIT_PTR rp, LI_UNIT_CPTR up, LI_SIZE_T n)
{
	while (*up == 0)
	{
		*rp = 0;
		if (!--n)
			return 0;
		++up; ++rp;
	}
	*rp = -*up;
	_li_com(++rp, ++up, --n);
	return 1;
}


/* MPN division interface. */

/* The 3/2 inverse is defined as

m = floor( (B^3-1) / (B u1 + u0)) - B
*/
LI_UNIT_T
_li_invert_3by2(LI_UNIT_T u1, LI_UNIT_T u0)
{
	LI_UNIT_T r, p, m, ql;
	unsigned ul, uh, qh;

	assert(u1 >= LI_UNIT_HIGHBIT);

	/* For notation, let b denote the half-unit base, so that B = b^2.
	Split u1 = b uh + ul. */
	ul = u1 & LI_LUNIT_MASK;
	uh = u1 >> (BITS_PER_LONG_INT_UNIT / 2);

	/* Approximation of the high half of quotient. Differs from the 2/1
	inverse of the half unit uh, since we have already subtracted
	u0. */
	qh = ~u1 / uh;

	/* Adjust to get a half-unit 3/2 inverse, i.e., we want

	qh' = floor( (b^3 - 1) / u) - b = floor ((b^3 - b u - 1) / u
	= floor( (b (~u) + b-1) / u),

	and the remainder

	r = b (~u) + b-1 - qh (b uh + ul)
	= b (~u - qh uh) + b-1 - qh ul

	Subtraction of qh ul may underflow, which implies adjustments.
	But by normalization, 2 u >= B > qh ul, so we need to adjust by
	at most 2.
	*/

	r = ((~u1 - (LI_UNIT_T)qh * uh) << (BITS_PER_LONG_INT_UNIT / 2)) | LI_LUNIT_MASK;

	p = (LI_UNIT_T)qh * ul;
	/* Adjustment steps taken from udiv_qrnnd_c */
	if (r < p)
	{
		qh--;
		r += u1;
		if (r >= u1) /* i.e. we didn't get carry when adding to r */
			if (r < p)
			{
				qh--;
				r += u1;
			}
	}
	r -= p;

	/* Low half of the quotient is

	ql = floor ( (b r + b-1) / u1).

	This is a 3/2 division (on half-units), for which qh is a
	suitable inverse. */

	p = (r >> (BITS_PER_LONG_INT_UNIT / 2)) * qh + r;
	/* Unlike full-unit 3/2, we can add 1 without overflow. For this to
	work, it is essential that ql is a full LI_UNIT_T. */
	ql = (p >> (BITS_PER_LONG_INT_UNIT / 2)) + 1;

	/* By the 3/2 trick, we don't need the high half unit. */
	r = (r << (BITS_PER_LONG_INT_UNIT / 2)) + LI_LUNIT_MASK - ql * u1;

	if (r >= (p << (BITS_PER_LONG_INT_UNIT / 2)))
	{
		ql--;
		r += u1;
	}
	m = ((LI_UNIT_T)qh << (BITS_PER_LONG_INT_UNIT / 2)) + ql;
	if (r >= u1)
	{
		m++;
		r -= u1;
	}

	/* Now m is the 2/1 invers of u1. If u0 > 0, adjust it to become a
	3/2 inverse. */
	if (u0 > 0)
	{
		LI_UNIT_T th, tl;
		r = ~r;
		r += u0;
		if (r < u0)
		{
			m--;
			if (r >= u1)
			{
				m--;
				r -= u1;
			}
			r -= u1;
		}
		gmp_umul_ppmm(th, tl, u0, m);
		r += th;
		if (r < th)
		{
			m--;
			m -= ((r > u1) | ((r == u1) & (tl > u0)));
		}
	}

	return m;
}

struct gmp_div_inverse
{
	/* Normalization shift count. */
	unsigned shift;
	/* Normalized divisor (d0 unused for _li_div_qr_1) */
	LI_UNIT_T d1, d0;
	/* Inverse, for 2/1 or 3/2. */
	LI_UNIT_T di;
};

static void _li_div_qr_1_invert(struct gmp_div_inverse *inv, LI_UNIT_T d)
{
	unsigned shift;

	assert(d > 0);
	gmp_clz(shift, d);
	inv->shift = shift;
	inv->d1 = d << shift;
	inv->di = _li_invert_unit(inv->d1);
}

static void _li_div_qr_2_invert(struct gmp_div_inverse *inv, LI_UNIT_T d1, LI_UNIT_T d0)
{
	unsigned shift;

	assert(d1 > 0);
	gmp_clz(shift, d1);
	inv->shift = shift;
	if (shift > 0)
	{
		d1 = (d1 << shift) | (d0 >> (BITS_PER_LONG_INT_UNIT - shift));
		d0 <<= shift;
	}
	inv->d1 = d1;
	inv->d0 = d0;
	inv->di = _li_invert_3by2(d1, d0);
}

static void _li_div_qr_invert(struct gmp_div_inverse *inv, LI_UNIT_CPTR dp, LI_SIZE_T dn)
{
	assert(dn > 0);

	if (dn == 1)
		_li_div_qr_1_invert(inv, dp[0]);
	else if (dn == 2)
		_li_div_qr_2_invert(inv, dp[1], dp[0]);
	else
	{
		unsigned shift;
		LI_UNIT_T d1, d0;

		d1 = dp[dn - 1];
		d0 = dp[dn - 2];
		assert(d1 > 0);
		gmp_clz(shift, d1);
		inv->shift = shift;
		if (shift > 0)
		{
			d1 = (d1 << shift) | (d0 >> (BITS_PER_LONG_INT_UNIT - shift));
			d0 = (d0 << shift) | (dp[dn - 3] >> (BITS_PER_LONG_INT_UNIT - shift));
		}
		inv->d1 = d1;
		inv->d0 = d0;
		inv->di = _li_invert_3by2(d1, d0);
	}
}

/* Not matching current public gmp interface, rather corresponding to
the sbpi1_div_* functions. */
static LI_UNIT_T
_li_div_qr_1_preinv(LI_UNIT_PTR qp, LI_UNIT_CPTR np, LI_SIZE_T nn,
	const struct gmp_div_inverse *inv)
{
	LI_UNIT_T d, di;
	LI_UNIT_T r;
	LI_UNIT_PTR tp = NULL;

	if (inv->shift > 0)
	{
		tp = gmp_xalloc_units(nn);
		r = _li_lshift(tp, np, nn, inv->shift);
		np = tp;
	}
	else
		r = 0;

	d = inv->d1;
	di = inv->di;
	while (--nn >= 0)
	{
		LI_UNIT_T q;

		gmp_udiv_qrnnd_preinv(q, r, r, np[nn], d, di);
		if (qp)
			qp[nn] = q;
	}
	if (inv->shift > 0)
		gmp_free(tp);

	return r >> inv->shift;
}

static LI_UNIT_T
_li_div_qr_1(LI_UNIT_PTR qp, LI_UNIT_CPTR np, LI_SIZE_T nn, LI_UNIT_T d)
{
	assert(d > 0);

	/* Special case for powers of two. */
	if ((d & (d - 1)) == 0)
	{
		LI_UNIT_T r = np[0] & (d - 1);
		if (qp)
		{
			if (d <= 1)
				_li_copyi(qp, np, nn);
			else
			{
				unsigned shift;
				gmp_ctz(shift, d);
				_li_rshift(qp, np, nn, shift);
			}
		}
		return r;
	}
	else
	{
		struct gmp_div_inverse inv;
		_li_div_qr_1_invert(&inv, d);
		return _li_div_qr_1_preinv(qp, np, nn, &inv);
	}
}

static void
_li_div_qr_2_preinv(LI_UNIT_PTR qp, LI_UNIT_PTR rp, LI_UNIT_CPTR np, LI_SIZE_T nn,
	const struct gmp_div_inverse *inv)
{
	unsigned shift;
	LI_SIZE_T i;
	LI_UNIT_T d1, d0, di, r1, r0;
	LI_UNIT_PTR tp;

	assert(nn >= 2);
	shift = inv->shift;
	d1 = inv->d1;
	d0 = inv->d0;
	di = inv->di;

	if (shift > 0)
	{
		tp = gmp_xalloc_units(nn);
		r1 = _li_lshift(tp, np, nn, shift);
		np = tp;
	}
	else
		r1 = 0;

	r0 = np[nn - 1];

	i = nn - 2;
	do
	{
		LI_UNIT_T n0, q;
		n0 = np[i];
		gmp_udiv_qr_3by2(q, r1, r0, r1, r0, n0, d1, d0, di);

		if (qp)
			qp[i] = q;
	} while (--i >= 0);

	if (shift > 0)
	{
		assert((r0 << (BITS_PER_LONG_INT_UNIT - shift)) == 0);
		r0 = (r0 >> shift) | (r1 << (BITS_PER_LONG_INT_UNIT - shift));
		r1 >>= shift;

		gmp_free(tp);
	}

	rp[1] = r1;
	rp[0] = r0;
}

#if 0
static void
_li_div_qr_2(LI_UNIT_PTR qp, LI_UNIT_PTR rp, LI_UNIT_CPTR np, LI_SIZE_T nn,
	LI_UNIT_T d1, LI_UNIT_T d0)
{
	struct gmp_div_inverse inv;
	assert(nn >= 2);

	_li_div_qr_2_invert(&inv, d1, d0);
	_li_div_qr_2_preinv(qp, rp, np, nn, &inv);
}
#endif

static void
_li_div_qr_pi1(LI_UNIT_PTR qp,
	LI_UNIT_PTR np, LI_SIZE_T nn, LI_UNIT_T n1,
	LI_UNIT_CPTR dp, LI_SIZE_T dn,
	LI_UNIT_T dinv)
{
	LI_SIZE_T i;

	LI_UNIT_T d1, d0;
	LI_UNIT_T cy, cy1;
	LI_UNIT_T q;

	assert(dn > 2);
	assert(nn >= dn);

	d1 = dp[dn - 1];
	d0 = dp[dn - 2];

	assert((d1 & LI_UNIT_HIGHBIT) != 0);
	//Iteration variable is the index of the q unit.
	//
	// We divide <n1, np[dn-1+i], np[dn-2+i], np[dn-3+i],..., np[i]>
	// by            <d1,          d0,        dp[dn-3],  ..., dp[0] >
	//

	i = nn - dn;
	do
	{
		LI_UNIT_T n0 = np[dn - 1 + i];

		if (n1 == d1 && n0 == d0)
		{
			q = LONG_INT_UNIT_MAX;
			_li_submul_1(np + i, dp, dn, q);
			n1 = np[dn - 1 + i];	// update n1, last loop's value will now be invalid 
		}
		else
		{
			gmp_udiv_qr_3by2(q, n1, n0, n1, n0, np[dn - 2 + i], d1, d0, dinv);

			cy = _li_submul_1(np + i, dp, dn - 2, q);

			cy1 = n0 < cy;
			n0 = n0 - cy;
			cy = n1 < cy1;
			n1 = n1 - cy1;
			np[dn - 2 + i] = n0;

			if (cy != 0)
			{
				n1 += d1 + _li_add_n(np + i, np + i, dp, dn - 1);
				q--;
			}
		}

		if (qp)
			qp[i] = q;
	} while (--i >= 0);

	np[dn - 1] = n1;
}

static void
_li_div_qr_preinv(LI_UNIT_PTR qp, LI_UNIT_PTR np, LI_SIZE_T nn,
	LI_UNIT_CPTR dp, LI_SIZE_T dn,
	const struct gmp_div_inverse *inv)
{
	assert(dn > 0);
	assert(nn >= dn);

	if (dn == 1)
		np[0] = _li_div_qr_1_preinv(qp, np, nn, inv);
	else if (dn == 2)
		_li_div_qr_2_preinv(qp, np, np, nn, inv);
	else
	{
		LI_UNIT_T nh;
		unsigned shift;

		assert(inv->d1 == dp[dn - 1]);
		assert(inv->d0 == dp[dn - 2]);
		assert((inv->d1 & LI_UNIT_HIGHBIT) != 0);

		shift = inv->shift;
		if (shift > 0)
			nh = _li_lshift(np, np, nn, shift);
		else
			nh = 0;

		_li_div_qr_pi1(qp, np, nn, nh, dp, dn, inv->di);

		if (shift > 0)
			gmp_assert_nocarry(_li_rshift(np, np, dn, shift));
	}
}

static void
_li_div_qr(LI_UNIT_PTR qp, LI_UNIT_PTR np, LI_SIZE_T nn, LI_UNIT_CPTR dp, LI_SIZE_T dn)
{
	struct gmp_div_inverse inv;
	LI_UNIT_PTR tp = NULL;

	assert(dn > 0);
	assert(nn >= dn);

	_li_div_qr_invert(&inv, dp, dn);
	if (dn > 2 && inv.shift > 0)
	{
		tp = gmp_xalloc_units(dn);
		gmp_assert_nocarry(_li_lshift(tp, dp, dn, inv.shift));
		dp = tp;
	}
	_li_div_qr_preinv(qp, np, nn, dp, dn, &inv);
	if (tp)
		gmp_free(tp);
}


/* MPN base conversion. */
static unsigned
_li_base_power_of_two_p(unsigned b)
{
	switch (b)
	{
	case 2: return 1;
	case 4: return 2;
	case 8: return 3;
	case 16: return 4;
	case 32: return 5;
	case 64: return 6;
	case 128: return 7;
	case 256: return 8;
	default: return 0;
	}
}

void
li_init(LI_T r)
{
	static const LI_UNIT_T dummy_unit = NULL; // 0xc1a0;

	r->NumOfUnitsAllocated = 0;
	r->RealLen = 0;
	r->Units = (LI_UNIT_PTR)&dummy_unit;
}

/* The utility of this function is a bit limited, since many functions
assigns the result variable using li_swap. */
void
li_init2(LI_T r, LI_BIT_CNT_T bits)
{
	LI_SIZE_T rn;

	bits -= (bits != 0);		/* Round down, except if 0 */
	rn = 1 + bits / BITS_PER_LONG_INT_UNIT;

	r->NumOfUnitsAllocated = rn;
	r->RealLen = 0;
	// !!!!  
	r->Units = gmp_xalloc_units(rn);
	/*
	r->Units= (LI_UNIT_T *)malloc(bits / CHAR_BIT);
	if (r->Units == NULL)
	li_exception("li_default_alloc: Virtual memory exhausted.");
	memset(r->Units, 0, bits / CHAR_BIT);
	*/
}

void
li_clear(LI_T r)
{
	if (r->NumOfUnitsAllocated)
		gmp_free(r->Units);
}

static LI_UNIT_PTR
li_realloc(LI_T r, LI_SIZE_T size)
{
	size = LI_MAX(size, 1);

	if (r->NumOfUnitsAllocated)
		r->Units = gmp_xrealloc_units(r->Units, size);
	else
		r->Units = gmp_xalloc_units(size);
	r->NumOfUnitsAllocated = size;

	if (LI_ABS(r->RealLen) > size)
		r->RealLen = 0;

	return r->Units;
}

/* Realloc for an LI_T WHAT if it has less than NEEDED units.  */
#define MPZ_REALLOC(z,n) ((n) > (z)->NumOfUnitsAllocated			\
			  ? li_realloc(z,n)			\
			  : (z)->Units)

/* MPZ assignment and basic conversions. */
void
li_set_si(LI_T r, signed long int x)
{
	if (x >= 0)
		li_set_ui(r, x);
	else /* (x < 0) */
	{
		r->RealLen = -1;
		MPZ_REALLOC(r, 1)[0] = LI_NEG_CAST(unsigned long int, x);
	}
}

void
li_set_ui(LI_T r, unsigned long int x)
{
	if (x > 0)
	{
		r->RealLen = 1;
		MPZ_REALLOC(r, 1)[0] = x;
	}
	else
		r->RealLen = 0;
}

void
li_set(LI_T r, const LI_T x)
{
	/* Allow the NOP r == x */
	if (r != x)
	{
		LI_SIZE_T n;
		LI_UNIT_PTR rp;

		n = LI_ABS(x->RealLen);
		rp = MPZ_REALLOC(r, n);

		_li_copyi(rp, x->Units, n);
		r->RealLen = x->RealLen;
	}
}

void
li_init_set_si(LI_T r, signed long int x)
{
	li_init(r);
	li_set_si(r, x);
}

void
li_init_set_ui(LI_T r, unsigned long int x)
{
	li_init(r);
	li_set_ui(r, x);
}

void
li_init_set(LI_T r, const LI_T x)
{
	li_init(r);
	li_set(r, x);
}

int
li_fits_slong_p(const LI_T u)
{
	LI_SIZE_T us = u->RealLen;

	if (us == 1)
		return u->Units[0] < LI_UNIT_HIGHBIT;
	else if (us == -1)
		return u->Units[0] <= LI_UNIT_HIGHBIT;
	else
		return (us == 0);
}

int
li_fits_ulong_p(const LI_T u)
{
	LI_SIZE_T us = u->RealLen;

	return (us == (us > 0));
}

unsigned long int
li_get_ui(const LI_T u)
{
	return u->RealLen == 0 ? 0 : u->Units[0];
}

long int
li_get_si(const LI_T u)
{
	if (u->RealLen < 0)
		/* This expression is necessary to properly handle 0x80000000 */
		return -1 - (long)((u->Units[0] - 1) & ~LI_UNIT_HIGHBIT);
	else
		return (long)(li_get_ui(u) & ~LI_UNIT_HIGHBIT);
}

size_t
li_size(const LI_T u)
{
	return LI_ABS(u->RealLen);
}

LI_UNIT_T
li_getunitn(const LI_T u, LI_SIZE_T n)
{
	if (n >= 0 && n < LI_ABS(u->RealLen))
		return u->Units[n];
	else
		return 0;
}

void
li_realloc2(LI_T x, LI_BIT_CNT_T n)
{
	li_realloc(x, 1 + (n - (n != 0)) / BITS_PER_LONG_INT_UNIT);
}

LI_UNIT_CPTR
li_units_read(LI_CPTR x)
{
	return x->Units;
}

LI_UNIT_PTR
li_units_modify(LI_T x, LI_SIZE_T n)
{
	assert(n > 0);
	return MPZ_REALLOC(x, n);
}

LI_UNIT_PTR
li_units_write(LI_T x, LI_SIZE_T n)
{
	return li_units_modify(x, n);
}

void
li_units_finish(LI_T x, LI_SIZE_T xs)
{
	LI_SIZE_T xn;
	xn = _li_normalized_size(x->Units, LI_ABS(xs));
	x->RealLen = xs < 0 ? -xn : xn;
}

LI_CPTR
li_roinit_n(LI_T x, LI_UNIT_CPTR xp, LI_SIZE_T xs)
{
	x->NumOfUnitsAllocated = 0;
	x->Units = (LI_UNIT_PTR)xp;
	li_units_finish(x, xs);
	return x;
}


/* Conversions and comparison to double. */
void
li_set_d(LI_T r, double x)
{
	int sign;
	LI_UNIT_PTR rp;
	LI_SIZE_T rn, i;
	double B;
	double Bi;
	LI_UNIT_T f;

	/* x != x is true when x is a NaN, and x == x * 0.5 is true when x is
	zero or infinity. */
	if (x != x || x == x * 0.5)
	{
		r->RealLen = 0;
		return;
	}

	sign = x < 0.0;
	if (sign)
		x = -x;

	if (x < 1.0)
	{
		r->RealLen = 0;
		return;
	}
	B = 2.0 * (double)LI_UNIT_HIGHBIT;
	Bi = 1.0 / B;
	for (rn = 1; x >= B; rn++)
		x *= Bi;

	rp = MPZ_REALLOC(r, rn);

	f = (LI_UNIT_T)x;
	x -= f;
	assert(x < 1.0);
	i = rn - 1;
	rp[i] = f;
	while (--i >= 0)
	{
		x = B * x;
		f = (LI_UNIT_T)x;
		x -= f;
		assert(x < 1.0);
		rp[i] = f;
	}

	r->RealLen = sign ? -rn : rn;
}

void
li_init_set_d(LI_T r, double x)
{
	li_init(r);
	li_set_d(r, x);
}

double
li_get_d(const LI_T u)
{
	LI_SIZE_T un;
	double x;
	double B = 2.0 * (double)LI_UNIT_HIGHBIT;

	un = LI_ABS(u->RealLen);

	if (un == 0)
		return 0.0;

	x = u->Units[--un];
	while (un > 0)
		x = B*x + u->Units[--un];

	if (u->RealLen < 0)
		x = -x;

	return x;
}

int
li_cmpabs_d(const LI_T x, double d)
{
	LI_SIZE_T xn;
	double B, Bi;
	LI_SIZE_T i;

	xn = x->RealLen;
	d = LI_ABS(d);

	if (xn != 0)
	{
		xn = LI_ABS(xn);

		B = 2.0 * (double)LI_UNIT_HIGHBIT;
		Bi = 1.0 / B;

		/* Scale d so it can be compared with the top unit. */
		for (i = 1; i < xn; i++)
			d *= Bi;

		if (d >= B)
			return -1;

		/* Compare floor(d) to top unit, subtract and cancel when equal. */
		for (i = xn; i-- > 0;)
		{
			LI_UNIT_T f, xl;

			f = (LI_UNIT_T)d;
			xl = x->Units[i];
			if (xl > f)
				return 1;
			else if (xl < f)
				return -1;
			d = B * (d - f);
		}
	}
	return -(d > 0.0);
}

int
li_cmp_d(const LI_T x, double d)
{
	if (x->RealLen < 0)
	{
		if (d >= 0.0)
			return -1;
		else
			return -li_cmpabs_d(x, d);
	}
	else
	{
		if (d < 0.0)
			return 1;
		else
			return li_cmpabs_d(x, d);
	}
}


/* MPZ comparisons and the like. */
int
li_sgn(const LI_T u)
{
	return LI_CMP(u->RealLen, 0);
}

int
li_cmp_si(const LI_T u, long v)
{
	LI_SIZE_T usize = u->RealLen;

	if (usize < -1)
		return -1;
	else if (v >= 0)
		return li_cmp_ui(u, v);
	else if (usize >= 0)
		return 1;
	else /* usize == -1 */
		return LI_CMP(LI_NEG_CAST(LI_UNIT_T, v), u->Units[0]);
}

int
li_cmp_ui(const LI_T u, unsigned long v)
{
	LI_SIZE_T usize = u->RealLen;

	if (usize > 1)
		return 1;
	else if (usize < 0)
		return -1;
	else
		return LI_CMP(li_get_ui(u), v);
}

int
li_cmp(const LI_T a, const LI_T b)
{
	LI_SIZE_T asize = a->RealLen;
	LI_SIZE_T bsize = b->RealLen;

	if (asize != bsize)
		return (asize < bsize) ? -1 : 1;
	else if (asize >= 0)
		return _li_cmp(a->Units, b->Units, asize);
	else
		return _li_cmp(b->Units, a->Units, -asize);
}

int
li_cmpabs_ui(const LI_T u, unsigned long v)
{
	if (LI_ABS(u->RealLen) > 1)
		return 1;
	else
		return LI_CMP(li_get_ui(u), v);
}

int
li_cmpabs(const LI_T u, const LI_T v)
{
	return _li_cmp4(u->Units, LI_ABS(u->RealLen),
		v->Units, LI_ABS(v->RealLen));
}

void
li_abs(LI_T r, const LI_T u)
{
	li_set(r, u);
	r->RealLen = LI_ABS(r->RealLen);
}

void
li_neg(LI_T r, const LI_T u)
{
	li_set(r, u);
	r->RealLen = -r->RealLen;
}

void
li_swap(LI_T u, LI_T v)
{
	MP_SIZE_T_SWAP(u->RealLen, v->RealLen);
	MP_SIZE_T_SWAP(u->NumOfUnitsAllocated, v->NumOfUnitsAllocated);
	MP_PTR_SWAP(u->Units, v->Units);
}


/* MPZ addition and subtraction */

/* Adds to the absolute value. Returns new size, but doesn't store it. */


/* Subtract from the absolute value. Returns new size, (or -1 on underflow),
but doesn't store it. */
static LI_SIZE_T
li_abs_sub_ui(LI_T r, const LI_T a, unsigned long b)
{
	LI_SIZE_T an = LI_ABS(a->RealLen);
	LI_UNIT_PTR rp;

	if (an == 0)
	{
		MPZ_REALLOC(r, 1)[0] = b;
		return -(b > 0);
	}
	rp = MPZ_REALLOC(r, an);
	if (an == 1 && a->Units[0] < b)
	{
		rp[0] = b - a->Units[0];
		return -1;
	}
	else
	{
		gmp_assert_nocarry(_li_sub_1(rp, a->Units, an, b));
		return _li_normalized_size(rp, an);
	}
}

static LI_SIZE_T li_abs_add_ui(LI_T r, const LI_T a, unsigned long b)
{
	LI_SIZE_T an;
	LI_UNIT_PTR rp;
	LI_UNIT_T cy;

	an = LI_ABS(a->RealLen);
	if (an == 0)
	{
		MPZ_REALLOC(r, 1)[0] = b;
		return b > 0;
	}

	rp = MPZ_REALLOC(r, an + 1);

	cy = _li_add_1(rp, a->Units, an, b);
	rp[an] = cy;
	an += cy;

	return an;
}


void li_add_ui(LI_T r, const LI_T a, unsigned long b)
{
	if (a->RealLen >= 0)
		r->RealLen = li_abs_add_ui(r, a, b);
	else
		r->RealLen = -li_abs_sub_ui(r, a, b);
}

void li_sub_ui(LI_T r, const LI_T a, unsigned long b)
{
	if (a->RealLen < 0)
		r->RealLen = -li_abs_add_ui(r, a, b);
	else
		r->RealLen = li_abs_sub_ui(r, a, b);
}

void li_ui_sub(LI_T r, unsigned long a, const LI_T b)
{
	if (b->RealLen < 0)
		r->RealLen = li_abs_add_ui(r, b, a);
	else
		r->RealLen = -li_abs_sub_ui(r, b, a);
}

static LI_SIZE_T li_abs_add(LI_T r, const LI_T a, const LI_T b)
{
	LI_SIZE_T an = LI_ABS(a->RealLen);
	LI_SIZE_T bn = LI_ABS(b->RealLen);
	LI_UNIT_PTR rp;
	LI_UNIT_T cy;

	if (an < bn)
	{
		MPZ_SRCPTR_SWAP(a, b);
		MP_SIZE_T_SWAP(an, bn);
	}

	rp = MPZ_REALLOC(r, an + 1);
	cy = _li_add(rp, a->Units, an, b->Units, bn);

	rp[an] = cy;

	return an + cy;
}

static LI_SIZE_T li_abs_sub(LI_T r, const LI_T a, const LI_T b)
{
	LI_SIZE_T an = LI_ABS(a->RealLen);
	LI_SIZE_T bn = LI_ABS(b->RealLen);
	int cmp;
	LI_UNIT_PTR rp;

	cmp = _li_cmp4(a->Units, an, b->Units, bn);
	if (cmp > 0)
	{
		rp = MPZ_REALLOC(r, an);
		gmp_assert_nocarry(_li_sub(rp, a->Units, an, b->Units, bn));
		return _li_normalized_size(rp, an);
	}
	else if (cmp < 0)
	{
		rp = MPZ_REALLOC(r, bn);
		gmp_assert_nocarry(_li_sub(rp, b->Units, bn, a->Units, an));
		return -_li_normalized_size(rp, bn);
	}
	else
		return 0;
}

void li_add(LI_T r, const LI_T a, const LI_T b)
{
	LI_SIZE_T rn;

	if ((a->RealLen ^ b->RealLen) >= 0)
		rn = li_abs_add(r, a, b);
	else
		rn = li_abs_sub(r, a, b);

	r->RealLen = a->RealLen >= 0 ? rn : -rn;
}

void
li_sub(LI_T r, const LI_T a, const LI_T b)
{
	LI_SIZE_T rn;

	if ((a->RealLen ^ b->RealLen) >= 0)
		rn = li_abs_sub(r, a, b);
	else
		rn = li_abs_add(r, a, b);

	r->RealLen = a->RealLen >= 0 ? rn : -rn;
}

/* MPZ multiplication */
void
li_mul_si(LI_T r, const LI_T u, long int v)
{
	if (v < 0)
	{
		li_mul_ui(r, u, LI_NEG_CAST(unsigned long int, v));
		li_neg(r, r);
	}
	else
		li_mul_ui(r, u, (unsigned long int) v);
}

void
li_mul_ui(LI_T r, const LI_T u, unsigned long int v)
{
	LI_SIZE_T un, us;
	LI_UNIT_PTR tp;
	LI_UNIT_T cy;

	us = u->RealLen;

	if (us == 0 || v == 0)
	{
		r->RealLen = 0;
		return;
	}

	un = LI_ABS(us);

	tp = MPZ_REALLOC(r, un + 1);
	cy = _li_mul_1(tp, u->Units, un, v);
	tp[un] = cy;

	un += (cy > 0);
	r->RealLen = (us < 0) ? -un : un;
}

void
li_mul(LI_T r, const LI_T u, const LI_T v)
{
	int sign;
	LI_SIZE_T un, vn, rn;
	LI_T t;
	LI_UNIT_PTR tp;

	if (r->NumOfUnitsAllocated < 0)
		un = 0;

	un = u->RealLen;
	vn = v->RealLen;

	if (un == 0 || vn == 0)
	{
		r->RealLen = 0;
		return;
	}

	sign = (un ^ vn) < 0;

	un = LI_ABS(un);
	vn = LI_ABS(vn);

	li_init2(t, (un + vn) * BITS_PER_LONG_INT_UNIT);

	tp = t->Units;
	if (un >= vn)
		_li_mul(tp, u->Units, un, v->Units, vn);
	else
		_li_mul(tp, v->Units, vn, u->Units, un);

	rn = un + vn;
	rn -= tp[rn - 1] == 0;

	t->RealLen = sign ? -rn : rn;
	li_swap(r, t);
	li_clear(t);
}

void
li_mul_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T bits)
{
	LI_SIZE_T un, rn;
	LI_SIZE_T units;
	unsigned shift;
	LI_UNIT_PTR rp;

	un = LI_ABS(u->RealLen);
	if (un == 0)
	{
		r->RealLen = 0;
		return;
	}

	units = bits / BITS_PER_LONG_INT_UNIT;
	shift = bits % BITS_PER_LONG_INT_UNIT;

	rn = un + units + (shift > 0);
	rp = MPZ_REALLOC(r, rn);
	if (shift > 0)
	{
		LI_UNIT_T cy = _li_lshift(rp + units, u->Units, un, shift);
		rp[rn - 1] = cy;
		rn -= (cy == 0);
	}
	else
		_li_copyd(rp + units, u->Units, un);

	_li_zero(rp, units);

	r->RealLen = (u->RealLen < 0) ? -rn : rn;
}

void
li_addmul_ui(LI_T r, const LI_T u, unsigned long int v)
{
	LI_T t;
	li_init(t);
	li_mul_ui(t, u, v);
	li_add(r, r, t);
	li_clear(t);
}

void
li_submul_ui(LI_T r, const LI_T u, unsigned long int v)
{
	LI_T t;
	li_init(t);
	li_mul_ui(t, u, v);
	li_sub(r, r, t);
	li_clear(t);
}

void
li_addmul(LI_T r, const LI_T u, const LI_T v)
{
	LI_T t;
	li_init(t);
	li_mul(t, u, v);
	li_add(r, r, t);
	li_clear(t);
}

void
li_submul(LI_T r, const LI_T u, const LI_T v)
{
	LI_T t;
	li_init(t);
	li_mul(t, u, v);
	li_sub(r, r, t);
	li_clear(t);
}


/* MPZ division */
enum li_div_round_mode { LONG_INT_DIV_FLOOR, LONG_INT_DIV_CEIL, LONG_INT_DIV_TRUNC };

/* Allows q or r to be zero. Returns 1 iff remainder is non-zero. */

static int
li_div_qr(LI_T q, LI_T r,
	const LI_T n, const LI_T d, enum li_div_round_mode mode)
{
	LI_SIZE_T ns, ds, nn, dn, qs;
	ns = n->RealLen;
	ds = d->RealLen;

	if (ds == 0)
		li_exception("li_div_qr: Divide by zero.");

	if (ns == 0)
	{
		if (q)
			q->RealLen = 0;
		if (r)
			r->RealLen = 0;
		return 0;
	}

	nn = LI_ABS(ns);
	dn = LI_ABS(ds);

	qs = ds ^ ns;

	if (nn < dn)
	{
		if (mode == LONG_INT_DIV_CEIL && qs >= 0)
		{
			// q = 1, r = n - d 
			if (r)
				li_sub(r, n, d);
			if (q)
				li_set_ui(q, 1);
		}
		else if (mode == LONG_INT_DIV_FLOOR && qs < 0)
		{
			// q = -1, r = n + d 
			if (r)
				li_add(r, n, d);
			if (q)
				li_set_si(q, -1);
		}
		else
		{
			// q = 0, r = d 
			if (r)
				li_set(r, n);
			if (q)
				q->RealLen = 0;
		}
		return 1;
	}
	else
	{
		LI_UNIT_PTR np, qp;
		LI_SIZE_T qn, rn;
		LI_T tq, tr;

		li_init_set(tr, n);
		np = tr->Units;

		qn = nn - dn + 1;

		if (q)
		{
			li_init2(tq, qn * BITS_PER_LONG_INT_UNIT);
			qp = tq->Units;
		}
		else
			qp = NULL;

		_li_div_qr(qp, np, nn, d->Units, dn);

		if (qp)
		{
			qn -= (qp[qn - 1] == 0);

			tq->RealLen = qs < 0 ? -qn : qn;
		}
		rn = _li_normalized_size(np, dn);
		tr->RealLen = ns < 0 ? -rn : rn;

		if (mode == LONG_INT_DIV_FLOOR && qs < 0 && rn != 0)
		{
			if (q)
				li_sub_ui(tq, tq, 1);
			if (r)
				li_add(tr, tr, d);
		}
		else if (mode == LONG_INT_DIV_CEIL && qs >= 0 && rn != 0)
		{
			if (q)
				li_add_ui(tq, tq, 1);
			if (r)
				li_sub(tr, tr, d);
		}

		if (q)
		{
			li_swap(tq, q);
			li_clear(tq);
		}
		if (r)
			li_swap(tr, r);

		li_clear(tr);

		return rn != 0;
	}
}

void
li_cdiv_qr(LI_T q, LI_T r, const LI_T n, const LI_T d)
{
	li_div_qr(q, r, n, d, LONG_INT_DIV_CEIL);
}
/*
void
li_fdiv_qr (LI_T q, LI_T r, const LI_T n, const LI_T d)
{
li_div_qr (q, r, n, d, LONG_INT_DIV_FLOOR);
}
*/
void
LI_Tdiv_qr(LI_T q, LI_T r, const LI_T n, const LI_T d)
{
	li_div_qr(q, r, n, d, LONG_INT_DIV_TRUNC);
}

void
li_cdiv_q(LI_T q, const LI_T n, const LI_T d)
{
	li_div_qr(q, NULL, n, d, LONG_INT_DIV_CEIL);
}
/*
void
li_fdiv_q (LI_T q, const LI_T n, const LI_T d)
{
li_div_qr (q, NULL, n, d, LONG_INT_DIV_FLOOR);
}

void
LI_Tdiv_q (LI_T q, const LI_T n, const LI_T d)
{
li_div_qr (q, NULL, n, d, LONG_INT_DIV_TRUNC);
}
*/
void
li_cdiv_r(LI_T r, const LI_T n, const LI_T d)
{
	li_div_qr(NULL, r, n, d, LONG_INT_DIV_CEIL);
}
/*
void
li_fdiv_r (LI_T r, const LI_T n, const LI_T d)
{
li_div_qr (NULL, r, n, d, LONG_INT_DIV_FLOOR);
}
*/
void LI_Tdiv_r(LI_T r, const LI_T n, const LI_T d)
{
	li_div_qr(NULL, r, n, d, LONG_INT_DIV_TRUNC);
}

void
li_mod(LI_T r, const LI_T n, const LI_T d)
{
	li_div_qr(NULL, r, n, d, d->RealLen >= 0 ? LONG_INT_DIV_FLOOR : LONG_INT_DIV_CEIL);
}

static void
li_div_q_2exp(LI_T q, const LI_T u, LI_BIT_CNT_T bit_index,
	enum li_div_round_mode mode)
{
	LI_SIZE_T un, qn;
	LI_SIZE_T unit_cnt;
	LI_UNIT_PTR qp;
	int adjust;

	un = u->RealLen;
	if (un == 0)
	{
		q->RealLen = 0;
		return;
	}
	unit_cnt = bit_index / BITS_PER_LONG_INT_UNIT;
	qn = LI_ABS(un) - unit_cnt;
	bit_index %= BITS_PER_LONG_INT_UNIT;

	if (mode == ((un > 0) ? LONG_INT_DIV_CEIL : LONG_INT_DIV_FLOOR)) /* un != 0 here. */
																	 /* Note: Below, the final indexing at unit_cnt is valid because at
																	 that point we have qn > 0. */
		adjust = (qn <= 0
			|| !_li_zero_p(u->Units, unit_cnt)
			|| (u->Units[unit_cnt]
				& (((LI_UNIT_T)1 << bit_index) - 1)));
	else
		adjust = 0;

	if (qn <= 0)
		qn = 0;
	else
	{
		qp = MPZ_REALLOC(q, qn);

		if (bit_index != 0)
		{
			_li_rshift(qp, u->Units + unit_cnt, qn, bit_index);
			qn -= qp[qn - 1] == 0;
		}
		else
		{
			_li_copyi(qp, u->Units + unit_cnt, qn);
		}
	}

	q->RealLen = qn;

	if (adjust)
		li_add_ui(q, q, 1);
	if (un < 0)
		li_neg(q, q);
}

static void
li_div_r_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T bit_index,
	enum li_div_round_mode mode)
{
	LI_SIZE_T us, un, rn;
	LI_UNIT_PTR rp;
	LI_UNIT_T mask;

	us = u->RealLen;
	if (us == 0 || bit_index == 0)
	{
		r->RealLen = 0;
		return;
	}
	rn = (bit_index + BITS_PER_LONG_INT_UNIT - 1) / BITS_PER_LONG_INT_UNIT;
	assert(rn > 0);

	rp = MPZ_REALLOC(r, rn);
	un = LI_ABS(us);

	mask = LONG_INT_UNIT_MAX >> (rn * BITS_PER_LONG_INT_UNIT - bit_index);

	if (rn > un)
	{
		/* Quotient (with truncation) is zero, and remainder is
		non-zero */
		if (mode == ((us > 0) ? LONG_INT_DIV_CEIL : LONG_INT_DIV_FLOOR)) /* us != 0 here. */
		{
			/* Have to negate and sign extend. */
			LI_SIZE_T i;

			gmp_assert_nocarry(!_li_neg(rp, u->Units, un));
			for (i = un; i < rn - 1; i++)
				rp[i] = LONG_INT_UNIT_MAX;

			rp[rn - 1] = mask;
			us = -us;
		}
		else
		{
			/* Just copy */
			if (r != u)
				_li_copyi(rp, u->Units, un);

			rn = un;
		}
	}
	else
	{
		if (r != u)
			_li_copyi(rp, u->Units, rn - 1);

		rp[rn - 1] = u->Units[rn - 1] & mask;

		if (mode == ((us > 0) ? LONG_INT_DIV_CEIL : LONG_INT_DIV_FLOOR)) /* us != 0 here. */
		{
			/* If r != 0, compute 2^{bit_count} - r. */
			_li_neg(rp, rp, rn);

			rp[rn - 1] &= mask;

			/* us is not used for anything else, so we can modify it
			here to indicate flipped sign. */
			us = -us;
		}
	}
	rn = _li_normalized_size(rp, rn);
	r->RealLen = us < 0 ? -rn : rn;
}

void
li_cdiv_q_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_q_2exp(r, u, cnt, LONG_INT_DIV_CEIL);
}

void
li_fdiv_q_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_q_2exp(r, u, cnt, LONG_INT_DIV_FLOOR);
}

void
LI_Tdiv_q_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_q_2exp(r, u, cnt, LONG_INT_DIV_TRUNC);
}

void
li_cdiv_r_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_r_2exp(r, u, cnt, LONG_INT_DIV_CEIL);
}

void
li_fdiv_r_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_r_2exp(r, u, cnt, LONG_INT_DIV_FLOOR);
}

void
LI_Tdiv_r_2exp(LI_T r, const LI_T u, LI_BIT_CNT_T cnt)
{
	li_div_r_2exp(r, u, cnt, LONG_INT_DIV_TRUNC);
}

void
li_divexact(LI_T q, const LI_T n, const LI_T d)
{
	gmp_assert_nocarry(li_div_qr(q, NULL, n, d, LONG_INT_DIV_TRUNC));
}

int
li_divisible_p(const LI_T n, const LI_T d)
{
	return li_div_qr(NULL, NULL, n, d, LONG_INT_DIV_TRUNC) == 0;
}

int
li_congruent_p(const LI_T a, const LI_T b, const LI_T m)
{
	LI_T t;
	int res;

	/* a == b (mod 0) iff a == b */
	if (li_sgn(m) == 0)
		return (li_cmp(a, b) == 0);

	li_init(t);
	li_sub(t, a, b);
	res = li_divisible_p(t, m);
	li_clear(t);

	return res;
}

static unsigned long
li_div_qr_ui(LI_T q, LI_T r,
	const LI_T n, unsigned long d, enum li_div_round_mode mode)
{
	LI_SIZE_T ns, qn;
	LI_UNIT_PTR qp;
	LI_UNIT_T rl;
	LI_SIZE_T rs;

	ns = n->RealLen;
	if (ns == 0)
	{
		if (q)
			q->RealLen = 0;
		if (r)
			r->RealLen = 0;
		return 0;
	}

	qn = LI_ABS(ns);
	if (q)
		qp = MPZ_REALLOC(q, qn);
	else
		qp = NULL;

	rl = _li_div_qr_1(qp, n->Units, qn, d);
	assert(rl < d);

	rs = rl > 0;
	rs = (ns < 0) ? -rs : rs;

	if (rl > 0 && ((mode == LONG_INT_DIV_FLOOR && ns < 0)
		|| (mode == LONG_INT_DIV_CEIL && ns >= 0)))
	{
		if (q)
			gmp_assert_nocarry(_li_add_1(qp, qp, qn, 1));
		rl = d - rl;
		rs = -rs;
	}

	if (r)
	{
		MPZ_REALLOC(r, 1)[0] = rl;
		r->RealLen = rs;
	}
	if (q)
	{
		qn -= (qp[qn - 1] == 0);
		assert(qn == 0 || qp[qn - 1] > 0);

		q->RealLen = (ns < 0) ? -qn : qn;
	}

	return rl;
}

unsigned long
li_cdiv_qr_ui(LI_T q, LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, r, n, d, LONG_INT_DIV_CEIL);
}

unsigned long
li_fdiv_qr_ui(LI_T q, LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, r, n, d, LONG_INT_DIV_FLOOR);
}

unsigned long
LI_Tdiv_qr_ui(LI_T q, LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, r, n, d, LONG_INT_DIV_TRUNC);
}

unsigned long
li_cdiv_q_ui(LI_T q, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, NULL, n, d, LONG_INT_DIV_CEIL);
}

unsigned long
li_fdiv_q_ui(LI_T q, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, NULL, n, d, LONG_INT_DIV_FLOOR);
}

unsigned long
LI_Tdiv_q_ui(LI_T q, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(q, NULL, n, d, LONG_INT_DIV_TRUNC);
}

unsigned long
li_cdiv_r_ui(LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, r, n, d, LONG_INT_DIV_CEIL);
}
unsigned long
li_fdiv_r_ui(LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, r, n, d, LONG_INT_DIV_FLOOR);
}
unsigned long
LI_Tdiv_r_ui(LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, r, n, d, LONG_INT_DIV_TRUNC);
}

unsigned long
li_cdiv_ui(const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, NULL, n, d, LONG_INT_DIV_CEIL);
}

unsigned long
li_fdiv_ui(const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, NULL, n, d, LONG_INT_DIV_FLOOR);
}

unsigned long
LI_Tdiv_ui(const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, NULL, n, d, LONG_INT_DIV_TRUNC);
}

unsigned long
li_mod_ui(LI_T r, const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, r, n, d, LONG_INT_DIV_FLOOR);
}

void
li_divexact_ui(LI_T q, const LI_T n, unsigned long d)
{
	gmp_assert_nocarry(li_div_qr_ui(q, NULL, n, d, LONG_INT_DIV_TRUNC));
}

int
li_divisible_ui_p(const LI_T n, unsigned long d)
{
	return li_div_qr_ui(NULL, NULL, n, d, LONG_INT_DIV_TRUNC) == 0;
}

/* GCD */
static LI_UNIT_T
_li_gcd_11(LI_UNIT_T u, LI_UNIT_T v)
{
	unsigned shift;

	assert((u | v) > 0);

	if (u == 0)
		return v;
	else if (v == 0)
		return u;

	gmp_ctz(shift, u | v);

	u >>= shift;
	v >>= shift;

	if ((u & 1) == 0)
		LI_UNIT_T_SWAP(u, v);

	while ((v & 1) == 0)
		v >>= 1;

	while (u != v)
	{
		if (u > v)
		{
			u -= v;
			do
				u >>= 1;
			while ((u & 1) == 0);
		}
		else
		{
			v -= u;
			do
				v >>= 1;
			while ((v & 1) == 0);
		}
	}
	return u << shift;
}

unsigned long
li_gcd_ui(LI_T g, const LI_T u, unsigned long v)
{
	LI_SIZE_T un;

	if (v == 0)
	{
		if (g)
			li_abs(g, u);
	}
	else
	{
		un = LI_ABS(u->RealLen);
		if (un != 0)
			v = _li_gcd_11(_li_div_qr_1(NULL, u->Units, un, v), v);

		if (g)
			li_set_ui(g, v);
	}

	return v;
}

static LI_BIT_CNT_T
li_make_odd(LI_T r)
{
	LI_BIT_CNT_T shift;

	assert(r->RealLen > 0);
	/* Count trailing zeros, equivalent to _li_scan1, because we know that there is a 1 */
	shift = _li_common_scan(r->Units[0], 0, r->Units, 0, 0);
	LI_Tdiv_q_2exp(r, r, shift);

	return shift;
}

void
li_gcd(LI_T g, const LI_T u, const LI_T v)
{
	LI_T tu, tv;
	LI_BIT_CNT_T uz, vz, gz;

	if (u->RealLen == 0)
	{
		li_abs(g, v);
		return;
	}
	if (v->RealLen == 0)
	{
		li_abs(g, u);
		return;
	}

	li_init(tu);
	li_init(tv);

	li_abs(tu, u);
	uz = li_make_odd(tu);
	li_abs(tv, v);
	vz = li_make_odd(tv);
	gz = LI_MIN(uz, vz);

	if (tu->RealLen < tv->RealLen)
		li_swap(tu, tv);

	LI_Tdiv_r(tu, tu, tv);
	if (tu->RealLen == 0)
	{
		li_swap(g, tv);
	}
	else
		for (;;)
		{
			int c;

			li_make_odd(tu);
			c = li_cmp(tu, tv);
			if (c == 0)
			{
				li_swap(g, tu);
				break;
			}
			if (c < 0)
				li_swap(tu, tv);

			if (tv->RealLen == 1)
			{
				LI_UNIT_T vl = tv->Units[0];
				LI_UNIT_T ul = LI_Tdiv_ui(tu, vl);
				li_set_ui(g, _li_gcd_11(ul, vl));
				break;
			}
			li_sub(tu, tu, tv);
		}
	li_clear(tu);
	li_clear(tv);
	li_mul_2exp(g, g, gz);
}

/* Logical operations and bit manipulation. */

/* Numbers are treated as if represented in two's complement (and
infinitely sign extended). For a negative values we get the two's
complement from -x = ~x + 1, where ~ is bitwise complement.
Negation transforms

xxxx10...0

into

yyyy10...0

where yyyy is the bitwise complement of xxxx. So least significant
bits, up to and including the first one bit, are unchanged, and
the more significant bits are all complemented.

To change a bit from zero to one in a negative number, subtract the
corresponding power of two from the absolute value. This can never
underflow. To change a bit from one to zero, add the corresponding
power of two, and this might overflow. E.g., if x = -001111, the
two's complement is 110001. Clearing the least significant bit, we
get two's complement 110000, and -010000. */

int
LI_Tstbit(const LI_T d, LI_BIT_CNT_T bit_index)
{
	LI_SIZE_T unit_index;
	unsigned shift;
	LI_SIZE_T ds;
	LI_SIZE_T dn;
	LI_UNIT_T w;
	int bit;

	ds = d->RealLen;
	dn = LI_ABS(ds);
	unit_index = bit_index / BITS_PER_LONG_INT_UNIT;
	if (unit_index >= dn)
		return ds < 0;

	shift = bit_index % BITS_PER_LONG_INT_UNIT;
	w = d->Units[unit_index];
	bit = (w >> shift) & 1;

	if (ds < 0)
	{
		/* d < 0. Check if any of the bits below is set: If so, our bit
		must be complemented. */
		if (shift > 0 && (w << (BITS_PER_LONG_INT_UNIT - shift)) > 0)
			return bit ^ 1;
		while (--unit_index >= 0)
			if (d->Units[unit_index] > 0)
				return bit ^ 1;
	}
	return bit;
}

static void li_abs_add_bit(LI_T d, LI_BIT_CNT_T bit_index)
{
	LI_SIZE_T dn, unit_index;
	LI_UNIT_T bit;
	LI_UNIT_PTR dp;

	dn = LI_ABS(d->RealLen);

	unit_index = bit_index / BITS_PER_LONG_INT_UNIT;
	bit = (LI_UNIT_T)1 << (bit_index % BITS_PER_LONG_INT_UNIT);

	if (unit_index >= dn)
	{
		LI_SIZE_T i;
		/* The bit should be set outside of the end of the number.
		We have to increase the size of the number. */
		dp = MPZ_REALLOC(d, unit_index + 1);

		dp[unit_index] = bit;
		for (i = dn; i < unit_index; i++)
			dp[i] = 0;
		dn = unit_index + 1;
	}
	else
	{
		LI_UNIT_T cy;

		dp = d->Units;

		cy = _li_add_1(dp + unit_index, dp + unit_index, dn - unit_index, bit);
		if (cy > 0)
		{
			dp = MPZ_REALLOC(d, dn + 1);
			dp[dn++] = cy;
		}
	}

	d->RealLen = (d->RealLen < 0) ? -dn : dn;
}

static void li_abs_sub_bit(LI_T d, LI_BIT_CNT_T bit_index)
{
	LI_SIZE_T dn, unit_index;
	LI_UNIT_PTR dp;
	LI_UNIT_T bit;

	dn = LI_ABS(d->RealLen);
	dp = d->Units;

	unit_index = bit_index / BITS_PER_LONG_INT_UNIT;
	bit = (LI_UNIT_T)1 << (bit_index % BITS_PER_LONG_INT_UNIT);

	assert(unit_index < dn);

	gmp_assert_nocarry(_li_sub_1(dp + unit_index, dp + unit_index,
		dn - unit_index, bit));
	dn = _li_normalized_size(dp, dn);
	d->RealLen = (d->RealLen < 0) ? -dn : dn;
}

void
li_setbit(LI_T d, LI_BIT_CNT_T bit_index)
{
	if (!LI_Tstbit(d, bit_index))
	{
		if (d->RealLen >= 0)
			li_abs_add_bit(d, bit_index);
		else
			li_abs_sub_bit(d, bit_index);
	}
}

LI_BIT_CNT_T li_scan1(const LI_T u, LI_BIT_CNT_T starting_bit)
{
	LI_UNIT_PTR up;
	LI_SIZE_T us, un, i;
	LI_UNIT_T unit, ux;

	us = u->RealLen;
	un = LI_ABS(us);
	i = starting_bit / BITS_PER_LONG_INT_UNIT;

	/* Past the end there's no 1 bits for u>=0, or an immediate 1 bit
	for u<0. Notice this test picks up any u==0 too. */
	if (i >= un)
		return (us >= 0 ? ~(LI_BIT_CNT_T)0 : starting_bit);

	up = u->Units;
	ux = 0;
	unit = up[i];

	if (starting_bit != 0)
	{
		if (us < 0)
		{
			ux = _li_zero_p(up, i);
			unit = ~unit + ux;
			ux = -(LI_UNIT_T)(unit >= ux);
		}

		/* Mask to 0 all bits before starting_bit, thus ignoring them. */
		unit &= (LONG_INT_UNIT_MAX << (starting_bit % BITS_PER_LONG_INT_UNIT));
	}

	return _li_common_scan(unit, i, up, un, ux);
}

void
li_gcdext(LI_T g, LI_T s, LI_T t, const LI_T u, const LI_T v)
{
	LI_T tu, tv, s0, s1, t0, t1;
	LI_BIT_CNT_T uz, vz, gz;
	LI_BIT_CNT_T power;

	if (u->RealLen == 0)
	{
		/* g = 0 u + sgn(v) v */
		signed long sign = li_sgn(v);
		li_abs(g, v);
		if (s)
			li_set_ui(s, 0);
		if (t)
			li_set_si(t, sign);
		return;
	}

	if (v->RealLen == 0)
	{
		/* g = sgn(u) u + 0 v */
		signed long sign = li_sgn(u);
		li_abs(g, u);
		if (s)
			li_set_si(s, sign);
		if (t)
			li_set_ui(t, 0);
		return;
	}

	li_init(tu);
	li_init(tv);
	li_init(s0);
	li_init(s1);
	li_init(t0);
	li_init(t1);

	li_abs(tu, u);
	uz = li_make_odd(tu);
	li_abs(tv, v);
	vz = li_make_odd(tv);
	gz = LI_MIN(uz, vz);

	uz -= gz;
	vz -= gz;

	/* Cofactors corresponding to odd gcd. gz handled later. */
	if (tu->RealLen < tv->RealLen)
	{
		li_swap(tu, tv);
		MPZ_SRCPTR_SWAP(u, v);
		MPZ_PTR_SWAP(s, t);
		MP_BITCNT_T_SWAP(uz, vz);
	}

	/* Maintain
	*
	* u = t0 tu + t1 tv
	* v = s0 tu + s1 tv
	*
	* where u and v denote the inputs with common factors of two
	* eliminated, and det (s0, t0; s1, t1) = 2^p. Then
	*
	* 2^p tu =  s1 u - t1 v
	* 2^p tv = -s0 u + t0 v
	*/

	/* After initial division, tu = q tv + tu', we have
	*
	* u = 2^uz (tu' + q tv)
	* v = 2^vz tv
	*
	* or
	*
	* t0 = 2^uz, t1 = 2^uz q
	* s0 = 0,    s1 = 2^vz
	*/

	li_setbit(t0, uz);
	LI_Tdiv_qr(t1, tu, tu, tv);
	li_mul_2exp(t1, t1, uz);

	li_setbit(s1, vz);
	power = uz + vz;

	if (tu->RealLen > 0)
	{
		LI_BIT_CNT_T shift;
		shift = li_make_odd(tu);
		li_mul_2exp(t0, t0, shift);
		li_mul_2exp(s0, s0, shift);
		power += shift;

		for (;;)
		{
			int c;
			c = li_cmp(tu, tv);
			if (c == 0)
				break;

			if (c < 0)
			{
				/* tv = tv' + tu
				*
				* u = t0 tu + t1 (tv' + tu) = (t0 + t1) tu + t1 tv'
				* v = s0 tu + s1 (tv' + tu) = (s0 + s1) tu + s1 tv' */

				li_sub(tv, tv, tu);
				li_add(t0, t0, t1);
				li_add(s0, s0, s1);

				shift = li_make_odd(tv);
				li_mul_2exp(t1, t1, shift);
				li_mul_2exp(s1, s1, shift);
			}
			else
			{
				li_sub(tu, tu, tv);
				li_add(t1, t0, t1);
				li_add(s1, s0, s1);

				shift = li_make_odd(tu);
				li_mul_2exp(t0, t0, shift);
				li_mul_2exp(s0, s0, shift);
			}
			power += shift;
		}
	}

	/* Now tv = odd part of gcd, and -s0 and t0 are corresponding
	cofactors. */

	li_mul_2exp(tv, tv, gz);
	li_neg(s0, s0);

	/* 2^p g = s0 u + t0 v. Eliminate one factor of two at a time. To
	adjust cofactors, we need u / g and v / g */

	li_divexact(s1, v, tv);
	li_abs(s1, s1);
	li_divexact(t1, u, tv);
	li_abs(t1, t1);

	while (power-- > 0)
	{
		/* s0 u + t0 v = (s0 - v/g) u - (t0 + u/g) v */
		if (li_odd_p(s0) || li_odd_p(t0))
		{
			li_sub(s0, s0, s1);
			li_add(t0, t0, t1);
		}
		li_divexact_ui(s0, s0, 2);
		li_divexact_ui(t0, t0, 2);
	}

	/* Arrange so that |s| < |u| / 2g */
	li_add(s1, s0, s1);
	if (li_cmpabs(s0, s1) > 0)
	{
		li_swap(s0, s1);
		li_sub(t0, t0, t1);
	}
	if (u->RealLen < 0)
		li_neg(s0, s0);
	if (v->RealLen < 0)
		li_neg(t0, t0);

	li_swap(g, tv);
	if (s)
		li_swap(s, s0);
	if (t)
		li_swap(t, t0);

	li_clear(tu);
	li_clear(tv);
	li_clear(s0);
	li_clear(s1);
	li_clear(t0);
	li_clear(t1);
}

int
li_invert(LI_T r, const LI_T u, const LI_T m)
{
	LI_T g, tr;
	int invertible;

	if (u->RealLen == 0 || li_cmpabs_ui(m, 1) <= 0)
		return 0;

	li_init(g);
	li_init(tr);

	li_gcdext(g, tr, NULL, u, m);
	invertible = (li_cmp_ui(g, 1) == 0);

	if (invertible)
	{
		if (tr->RealLen < 0)
		{
			if (m->RealLen >= 0)
				li_add(tr, tr, m);
			else
				li_sub(tr, tr, m);
		}
		li_swap(r, tr);
	}

	li_clear(g);
	li_clear(tr);
	return invertible;
}

/* Higher level operations (sqrt, pow and root) */

void
li_pow_ui(LI_T r, const LI_T b, unsigned long e)
{
	unsigned long bit;
	LI_T tr;
	li_init_set_ui(tr, 1);

	bit = LI_ULONG_HIGHBIT;
	do
	{
		li_mul(tr, tr, tr);
		if (e & bit)
			li_mul(tr, tr, b);
		bit >>= 1;
	} while (bit > 0);

	li_swap(r, tr);
	li_clear(tr);
}

void
li_ui_pow_ui(LI_T r, unsigned long bunit, unsigned long e)
{
	LI_T b;
	li_pow_ui(r, li_roinit_n(b, &bunit, 1), e);
}

void
li_powm(LI_T r, const LI_T b, const LI_T e, const LI_T m)
{
	LI_T tr;
	LI_T base;
	LI_SIZE_T en, mn;
	LI_UNIT_CPTR mp;
	struct gmp_div_inverse minv;
	unsigned shift;
	LI_UNIT_PTR tp = NULL;

	en = LI_ABS(e->RealLen);
	mn = LI_ABS(m->RealLen);
	if (mn == 0)
		li_exception("li_powm: Zero modulo.");

	if (en == 0)
	{
		li_set_ui(r, 1);
		return;
	}

	mp = m->Units;
	_li_div_qr_invert(&minv, mp, mn);
	shift = minv.shift;

	if (shift > 0)
	{
		/* To avoid shifts, we do all our reductions, except the final
		one, using a *normalized* m. */
		minv.shift = 0;

		tp = gmp_xalloc_units(mn);
		gmp_assert_nocarry(_li_lshift(tp, mp, mn, shift));
		mp = tp;
	}

	li_init(base);

	if (e->RealLen < 0)
	{
		if (!li_invert(base, b, m))
			li_exception("li_powm: Negative exponent and non-invertible base.");
	}
	else
	{
		LI_SIZE_T bn;
		li_abs(base, b);

		bn = base->RealLen;
		if (bn >= mn)
		{
			_li_div_qr_preinv(NULL, base->Units, base->RealLen, mp, mn, &minv);
			bn = mn;
		}

		/* We have reduced the absolute value. Now take care of the
		sign. Note that we get zero represented non-canonically as
		m. */
		if (b->RealLen < 0)
		{
			LI_UNIT_PTR bp = MPZ_REALLOC(base, mn);
			gmp_assert_nocarry(_li_sub(bp, mp, mn, bp, bn));
			bn = mn;
		}
		base->RealLen = _li_normalized_size(base->Units, bn);
	}
	li_init_set_ui(tr, 1);

	while (--en >= 0)
	{
		LI_UNIT_T w = e->Units[en];
		LI_UNIT_T bit;

		bit = LI_UNIT_HIGHBIT;
		do
		{
			li_mul(tr, tr, tr);
			if (w & bit)
				li_mul(tr, tr, base);
			if (tr->RealLen > mn)
			{
				_li_div_qr_preinv(NULL, tr->Units, tr->RealLen, mp, mn, &minv);
				tr->RealLen = _li_normalized_size(tr->Units, mn);
			}
			bit >>= 1;
		} while (bit > 0);
	}

	/* Final reduction */
	if (tr->RealLen >= mn)
	{
		minv.shift = shift;
		_li_div_qr_preinv(NULL, tr->Units, tr->RealLen, mp, mn, &minv);
		tr->RealLen = _li_normalized_size(tr->Units, mn);
	}
	if (tp)
		gmp_free(tp);

	li_swap(r, tr);
	li_clear(tr);
	li_clear(base);
}

void
li_powm_ui(LI_T r, const LI_T b, unsigned long eunit, const LI_T m)
{
	LI_T e;
	li_powm(r, b, li_roinit_n(e, &eunit, 1), m);
}

/* Primality testing */
static int
gmp_millerrabin(const LI_T n, const LI_T nm1, LI_T y,
	const LI_T q, LI_BIT_CNT_T k)
{
	assert(k > 0);

	/* Caller must initialize y to the base. */
	li_powm(y, y, q, n);

	if (li_cmp_ui(y, 1) == 0 || li_cmp(y, nm1) == 0)
		return 1;

	while (--k > 0)
	{
		li_powm_ui(y, y, 2, n);
		if (li_cmp(y, nm1) == 0)
			return 1;
		/* y == 1 means that the previous y was a non-trivial square root
		of 1 (mod n). y == 0 means that n is a power of the base.
		In either case, n is not prime. */
		if (li_cmp_ui(y, 1) <= 0)
			return 0;
	}
	return 0;
}

/* This product is 0xc0cfd797, and fits in 32 bits. */
#define LONG_INT_PRIME_PRODUCT \
  (3UL*5UL*7UL*11UL*13UL*17UL*19UL*23UL*29UL)

/* Bit (p+1)/2 is set, for each odd prime <= 61 */
#define LONG_INT_PRIME_MASK 0xc96996dcUL

int
li_probab_prime_p(const LI_T n, int reps)
{
	LI_T nm1;
	LI_T q;
	LI_T y;
	LI_BIT_CNT_T k;
	int is_prime;
	int j;

	/* Note that we use the absolute value of n only, for compatibility
	with the real GMP. */
	if (li_even_p(n))
		return (li_cmpabs_ui(n, 2) == 0) ? 2 : 0;

	/* Above test excludes n == 0 */
	assert(n->RealLen != 0);

	if (li_cmpabs_ui(n, 64) < 0)
		return (LONG_INT_PRIME_MASK >> (n->Units[0] >> 1)) & 2;

	if (li_gcd_ui(NULL, n, LONG_INT_PRIME_PRODUCT) != 1)
		return 0;

	/* All prime factors are >= 31. */
	if (li_cmpabs_ui(n, 31 * 31) < 0)
		return 2;

	/* Use Miller-Rabin, with a deterministic sequence of bases, a[j] =
	j^2 + j + 41 using Euler's polynomial. We potentially stop early,
	if a[j] >= n - 1. Since n >= 31*31, this can happen only if reps >
	30 (a[30] == 971 > 31*31 == 961). */

	li_init(nm1);
	li_init(q);
	li_init(y);

	/* Find q and k, where q is odd and n = 1 + 2**k * q.  */
	nm1->RealLen = li_abs_sub_ui(nm1, n, 1);
	k = li_scan1(nm1, 0);
	LI_Tdiv_q_2exp(q, nm1, k);

	for (j = 0, is_prime = 1; is_prime & (j < reps); j++)
	{
		li_set_ui(y, (unsigned long)j*j + j + 41);
		if (li_cmp(y, nm1) >= 0)
		{
			/* Don't try any further bases. This "early" break does not affect
			the result for any reasonable reps value (<=5000 was tested) */
			assert(j >= 30);
			break;
		}
		is_prime = gmp_millerrabin(n, nm1, y, q, k);
	}
	li_clear(nm1);
	li_clear(q);
	li_clear(y);

	return is_prime;
}

