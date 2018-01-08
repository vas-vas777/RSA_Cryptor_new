#ifndef ___LONG_INT_H__
#define ___LONG_INT_H__

// For size_t 
#include <stddef.h>

#if defined (__cplusplus)
extern "C" {
#endif

	typedef unsigned long LI_UNIT_T;
	typedef long LI_SIZE_T;
	typedef unsigned long LI_BIT_CNT_T;

#define BYTES_PER_LONG_INT_UNIT				sizeof(LI_UNIT_T) 
#define BITS_PER_LONG_INT_UNIT				(BYTES_PER_LONG_INT_UNIT * CHAR_BIT)

	typedef LI_UNIT_T *LI_UNIT_PTR;
	typedef const LI_UNIT_T *LI_UNIT_CPTR;

	typedef struct _LONG_INT_STRUCT
	{
		// Number of Units allocated and pointed to by the Inits field
		int NumOfUnitsAllocated;
		// abs(RealLen) is the number of units the last field points to.  If RealLen is  negative this is a negative number  
		int RealLen;
		// Pointer to the units  
		LI_UNIT_T *Units;
	}
	LONG_INT_STRUCT;

	typedef LONG_INT_STRUCT LI_T[1];

	typedef LONG_INT_STRUCT *LI_PTR;
	typedef const LONG_INT_STRUCT *LI_CPTR;

	void _li_copyi(LI_UNIT_PTR, LI_UNIT_CPTR, LI_SIZE_T);
	void _li_copyd(LI_UNIT_PTR, LI_UNIT_CPTR, LI_SIZE_T);
	void _li_zero(LI_UNIT_PTR, LI_SIZE_T);

	int _li_cmp(LI_UNIT_CPTR, LI_UNIT_CPTR, LI_SIZE_T);
	int _li_zero_p(LI_UNIT_CPTR, LI_SIZE_T);

	void li_init(LI_T);
	void li_init2(LI_T, LI_BIT_CNT_T);
	void li_clear(LI_T);

	void li_set_ui(LI_T, unsigned long int);
	void li_set(LI_T, const LI_T);

	int li_sgn(const LI_T);
	int li_cmp_ui(const LI_T, unsigned long);
	int li_cmp(const LI_T, const LI_T);

	void li_add_ui(LI_T, const LI_T, unsigned long);
	void li_add(LI_T, const LI_T, const LI_T);
	void li_sub_ui(LI_T, const LI_T, unsigned long);
	void li_sub(LI_T, const LI_T, const LI_T);

	void li_mul_si(LI_T, const LI_T, long int);
	void li_mul_ui(LI_T, const LI_T, unsigned long int);
	void li_mul(LI_T, const LI_T, const LI_T);
	void li_cdiv_q(LI_T, const LI_T, const LI_T);
	void li_cdiv_r(LI_T, const LI_T, const LI_T);
	void li_mod(LI_T, const LI_T, const LI_T);

	unsigned long li_cdiv_q_ui(LI_T, const LI_T, unsigned long);
	void li_gcd(LI_T, const LI_T, const LI_T);
	void li_gcdext(LI_T, LI_T, LI_T, const LI_T, const LI_T);

	void li_powm(LI_T, const LI_T, const LI_T, const LI_T);

	int li_probab_prime_p(const LI_T, int);

#if defined (__cplusplus)
}
#endif
#endif /* ___LONG_INT_H__ */
