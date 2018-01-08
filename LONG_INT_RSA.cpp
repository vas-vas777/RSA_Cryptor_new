#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <exception>
#include "LONG_INT_RSA.h"
#include "FileUtility.h"
#include "DlgCryptingStatus.h"



#define RSA_KEYS_FN		_T("rsaKeys.bin")

#define KEYLENGTH							512
#define KEYLENGTH_IN_BYTES		KEYLENGTH / 8 

static LI_T E, D, N;
static int EncryptBlockLenInBytes= 0;
static int DecryptBlockLenInBytes= 0;
static int CryptBlockLenInBits= 0;

static LI_T Src, Dst, Tmp;

void Random(LI_PTR Dst)
{
	int i;
	unsigned long Val; //, Val1;
	
	Dst->RealLen= 0;

	for (i= 0; i < Dst->NumOfUnitsAllocated; i++)
	{
		Val= 0;
		do
		{ Val= rand(); }
		while (Val == 0);

		Dst->Units[i]= Val;
		Dst->RealLen++;
	}
// set the most significant bit to be sure the value is really 512 bit long	
	Dst->Units[i- 1]|= 0x80000000;
}

void NextPrime(LI_T a, LI_T r)
{
LI_T b;

	li_init2(b, r->NumOfUnitsAllocated* BITS_PER_LONG_INT_UNIT);
	do 
	{
		do
		{
			Random(b);
		}
		while (li_probab_prime_p(b, 25) == 0);
	}
	while (li_cmp(a, b) == 0);
	li_set(r, b);
	li_clear(b);
}

void GetMaxDiviser(LI_T a, LI_T b, LI_T r)
{
  LI_T aa;
  LI_T bb;
  LI_T c;

  
  li_init(aa);
  li_init(bb);
  li_init(c);

  
  li_set(aa, a);
  li_set(bb, b);
  

  while (li_cmp_ui(bb, 0) != 0)
  {
//    c = a % d;
		li_cdiv_r(c, aa, bb);
    li_set(aa, bb);
    li_set(bb, c);
  }
  li_set(r, aa);
  
  li_clear(aa);
  li_clear(bb);
  li_clear(c);
}

void extEuclid (LI_PTR a, LI_PTR  b, LI_PTR y)
{
  LI_T r, q, q_inv, tmp, a11, a12, a21, a22;
  LI_T A11, A12, A21 ,A22;

	
	li_init(r); li_init2(q, 512); li_init(q_inv); li_init(tmp); 
	li_init(a11); li_init(a12); li_init(a21); li_init(a22);
	li_init(A11); li_init(A12); li_init(A21); li_init(A22);
	
	li_set_ui(a11, 1);
	li_set_ui(a12, 0);
	li_set_ui(a21, 0);
	li_set_ui(a22, 1);


  while ( li_cmp_ui(b, 0) > 0)
  {
    li_mod(r, a, b); //  r = a%b;
    li_cdiv_q(q, a, b); // q = a/b;

		if (li_cmp_ui(r, 0) == 0) 
			break;
			
    li_set(A11, a12);
//    A12 = a11+a12*-q;

		li_mul_si(A12, q, -1);
		li_mul(tmp, A12, a12);
		li_add(A12, tmp, a11);
		
    li_set(A21, a22);
//    A22 = a21+a22*-q;
		li_mul(tmp, a21, q);
		li_sub(A21, a21, tmp);
    li_set(A22, a22);

//    a11 = A11, a12 = A12, a21 = A21, a22 = A22;
//    a = b;
//    b = r;
    li_set(a11, A11); li_set(a12, A12); li_set(a21, A21); li_set(a22, A22);
    li_set(a, b);
    li_set(b, r);


  }
//  y = a22;
	li_set(y, a22);
	
	li_clear(r); li_clear(q); li_clear(q_inv); li_clear(tmp); 
	li_clear(a11); li_clear(a12); li_clear(a21); li_clear(a22);
	li_clear(A11); li_clear(A12); li_clear(A21); li_clear(A22);
	
}

void RSA_GenerateKeys()
{
  LI_T p;	 li_init2(p, KEYLENGTH/ 2);
  LI_T q;	 li_init2(q, KEYLENGTH/ 2);
  LI_T euler, diviser, tmp0, tmp1, tmp2;

	li_init2(euler, KEYLENGTH);
	li_init2(diviser, KEYLENGTH);
	li_init2(tmp0, KEYLENGTH);
	li_init2(tmp1, KEYLENGTH);
	li_init2(tmp2, KEYLENGTH);

  li_init2(D, KEYLENGTH- 4);
	li_init2(E, KEYLENGTH- 4);
	li_init2(N, KEYLENGTH);
			
	srand( (unsigned)time( NULL ) );
	do
	{
		Random(p);
	}
	while (li_probab_prime_p(p, 25) == 0);
	do
	{
		Random(q);
	}
	while (li_probab_prime_p(q, 25) == 0);
// N= p * q
	li_mul(N, p, q);
//	euler = (p - 1) * (q - 1);
	li_sub_ui(tmp0, p, 1);
	li_sub_ui(tmp1, q, 1);
	li_mul(euler, tmp0, tmp1);
	
//getting private and public exponents
  do
  {
		Random(D);
		li_sub_ui(D, D, 1);
    do
    {
			li_add_ui(D, D, 1);
			li_gcd(diviser, D, euler);

    }
    while (li_cmp_ui(diviser, 1) != 0);


		li_set(tmp0, euler);
		li_set(tmp1, D);
		li_set(tmp2, E);

	li_gcdext(p, q, E, euler, D);

	if(li_sgn(E) < 0)
		li_add(D, E, euler);


  }
  while (li_cmp_ui(E, 0) < 0);

	
L_EXIT:
	li_clear(p);
	li_clear(q);
	li_clear(euler);
	li_clear(diviser);
	li_clear(tmp0);
	li_clear(tmp1);
	li_clear(tmp2);
}

int SaveKeysToFile()
{
	HANDLE hKeyF= NULL;
	BYTE *pKeyData= NULL;
	DWORD DataLen, BytesCnt;
	
	_tcscpy(&KeyFN[KeyDirLen], RSA_KEYS_FN); 
	
	hKeyF= CreateFile(KeyFN, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hKeyF == INVALID_HANDLE_VALUE)
  {
		hKeyF= NULL;
		return 14;
  }
// write public key  

  DataLen= E->RealLen;
  DataLen*= BYTES_PER_LONG_INT_UNIT;
  if (!WriteFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
  pKeyData= (BYTE *)E->Units;
  if (!WriteFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
// write private key  

  DataLen= D->RealLen;
  DataLen*= BYTES_PER_LONG_INT_UNIT;
  if (!WriteFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
  pKeyData= (BYTE *)D->Units;
  if (!WriteFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
// write exponent  

  DataLen= N->RealLen;
  DataLen*= BYTES_PER_LONG_INT_UNIT;
  if (!WriteFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
  pKeyData= (BYTE *)N->Units;
  if (!WriteFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
  
	CloseHandle(hKeyF);
	return 0;
}

int RSA_LoadKeysFromFile()
{
	HANDLE hKeyF= NULL;
	BYTE *pKeyData= NULL;
	DWORD DataLen, BytesCnt;
	
	_tcscpy(&KeyFN[KeyDirLen], RSA_KEYS_FN); 

	hKeyF= CreateFile(KeyFN, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hKeyF == INVALID_HANDLE_VALUE)
  {
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			return -1;
		hKeyF= NULL;
		return 14;
  }
// read public key  
	if (!ReadFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
	::li_init2(E, DataLen* 8);

	E->RealLen= DataLen / BYTES_PER_LONG_INT_UNIT;
	pKeyData= (BYTE *)E->Units;
	if (!ReadFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
// read private key
	if (!ReadFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}

	::li_init2(D, DataLen* 8);

	D->RealLen= DataLen / BYTES_PER_LONG_INT_UNIT;
	pKeyData= (BYTE *)D->Units;
	if (!ReadFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
// read exponent
	if (!ReadFile(hKeyF, &DataLen, 4, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
	::li_init2(N, DataLen* 8);

	N->RealLen= DataLen / BYTES_PER_LONG_INT_UNIT;
	pKeyData= (BYTE *)N->Units;
	if (!ReadFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
	CloseHandle(hKeyF);
	return 0;
}

void RSA_Clear()
{
	li_clear(Src);
	li_clear(Dst);
	li_clear(Tmp);

	::li_clear(E);
	::li_clear(D);
	::li_clear(N);
}

int RSA_Init()
{
	int Rslt;
	
	Rslt= RSA_LoadKeysFromFile();
	switch (Rslt) 
	{
		case -1:
			RSA_GenerateKeys();
		  Rslt= SaveKeysToFile();	
		break;
		case 0:
		break;
		default:
			return Rslt;
	}

	EncryptBlockLenInBytes= (N->RealLen- 1)* BYTES_PER_LONG_INT_UNIT;
	DecryptBlockLenInBytes= (N->RealLen)* BYTES_PER_LONG_INT_UNIT;
	CryptBlockLenInBits= DecryptBlockLenInBytes* 8;

	li_init2(Src, CryptBlockLenInBits); 
	li_init2(Dst, CryptBlockLenInBits* 2+ 32); 
	li_init2(Tmp, CryptBlockLenInBits* 2+ 32); 

	return Rslt;
}

int RSA_Crypt(bool DoEncrypt)
{
	int Rslt= 0;
	int i, Remainder= 0;

	bool Continue= true;
	unsigned __int32 BytesToRead, BytesToWrite, BytesCnt;
	LONGLONG  FileLen= 0, BlocksToRead= 0, BlockCnt= 0;
	LONGLONG ProgressStep= 0, ProgressCnt= 0;
	DWORD LoDWord, HiDWord;
	char *pByte; 
			
	if (DoEncrypt)
	{
		BytesToRead= EncryptBlockLenInBytes;
		BytesToWrite= DecryptBlockLenInBytes;
		if (!WriteFile(hDstF, &BlockCnt, sizeof(LONGLONG), (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 33;
			goto L_EXIT;
		}
		if (!WriteFile(hDstF, &Remainder, sizeof(int), (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 33;
			goto L_EXIT;
		}
	}
	else
	{
		BytesToRead= DecryptBlockLenInBytes;
		BytesToWrite= EncryptBlockLenInBytes;
		if (!ReadFile(hSrcF, &BlocksToRead, sizeof(LONGLONG), (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 32;
			goto L_EXIT;
		}
		if (!ReadFile(hSrcF, &Remainder, sizeof(int), (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 32;
			goto L_EXIT;
		}

	}
	Src->RealLen= BytesToRead / BYTES_PER_LONG_INT_UNIT; // BITS_PER_LONG_INT_UNIT;	
	
	LoDWord= GetFileSize(hSrcF, &HiDWord); 	
	ProgressStep= HiDWord;
	ProgressStep= ProgressStep << 32;
	ProgressStep|= LoDWord; 
	ProgressStep/= BytesToRead;
	if (ProgressStep < 100)
		ProgressStep= 1;
	else	
		ProgressStep= ProgressStep / 100;
	
try
{	
	do
	{
		if (!ReadFile(hSrcF, Src->Units, BytesToRead, (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 32;
			break; 
		}
		if (BytesCnt == 0)
			break;

		ProgressCnt++;
		if (ProgressCnt == ProgressStep)
		{
			ProgressCnt= 0;
			DsplPogressStep();
		}
		BlockCnt++;
		if (DoEncrypt)	
		{
			if (BytesCnt < BytesToRead)
			{
				Continue= false;
				i= BytesToRead- BytesCnt;
				Remainder= BytesCnt;

				pByte= (char *)Src->Units;
				pByte+= BytesCnt;
				do
				{
					*pByte= 0;
					pByte++;
					i--;
				}
				while (i > 0);
			}
			li_powm(Dst, Src, E, N);
// fix exe dll 0000 bytes
			i= BytesToWrite / BYTES_PER_LONG_INT_UNIT;
			if (Dst->RealLen < i)
			{
				if (Dst->NumOfUnitsAllocated < i)
				{
					li_set(Tmp, Dst);
					li_init2(Dst, i* BITS_PER_LONG_INT_UNIT);
					li_set(Dst, Tmp);
				}
				if (Dst->RealLen == 0)
					Dst->Units[0]= 0;
				do
				{
					i--;
					Dst->Units[i]= 0;
				}
				while (i > Dst->RealLen);
			}
		}
		else
		{
			li_powm(Dst, Src, D, N);
		}
// fix  0000 bytes
		i= BytesToWrite / BYTES_PER_LONG_INT_UNIT;
		if (Dst->RealLen < i)
		{
			if (Dst->NumOfUnitsAllocated < i)
			{
				li_set(Tmp, Dst);
				li_init2(Dst, i* BITS_PER_LONG_INT_UNIT);
				li_set(Dst, Tmp);
			}
			if (Dst->RealLen == 0)
				Dst->Units[0]= 0;
			do
			{
				i--;
				Dst->Units[i]= 0;
			}
			while (i > Dst->RealLen);
		}
// fix remainder for decryption
		if (BlockCnt == BlocksToRead)		
		{
			if (Remainder != 0)
			{
				BytesToWrite= Remainder;
			}
		}
		if (!WriteFile(hDstF, Dst->Units, BytesToWrite, (DWORD *)&BytesCnt, NULL))
		{
			Rslt= 33;
			break; 
		}
		
	}
	while (Continue);
	if (Remainder != 0)
		if (DoEncrypt)	
		{
			if (SetFilePointer(hDstF, 0, NULL, FILE_BEGIN ) != 0)		
			{
				Rslt= 34;
			}
			else
			if (!WriteFile(hDstF, &BlockCnt, sizeof(LONGLONG), (DWORD *)&BytesCnt, NULL))
			{
				Rslt= 33;
			}
			if (!WriteFile(hDstF, &Remainder, sizeof(int), (DWORD *)&BytesCnt, NULL))
			{
				Rslt= 33;
			}
		}
		else
		{

		}
}
catch (std::exception e)
{
	Rslt= 35;
}	

L_EXIT:	
	CloseFiles();
	
	return Rslt;
}

int EncryptSelectedFile()
{
	return RSA_Crypt(true);
}

int DecryptSelectedFile()
{
	return RSA_Crypt(false);
}
