#include <Windows.h>
#include <tchar.h>
#include <Wincrypt.h>
#pragma comment(lib, "Advapi32.lib")

#pragma warning (disable: 4996)

#include "FileUtility.h"

HCRYPTPROV hCryptoProv = NULL;
HCRYPTKEY hRSAKey = NULL;
HCRYPTKEY hPrivateKey = NULL;
HCRYPTKEY hPublicKey = NULL;
HCRYPTKEY hSymmetricKey= NULL;
TCHAR ContainerName[]= _T("NuttyLab_RSA_ENHANCED_KEY_Container");

bool IsRSACrypting= true;

#define PUBLIC_KEY_FN			_T("Public.key")
#define PRIVATE_KEY_FN		_T("Private.key")
#define SYMMETRIC_KEY_FN	_T("Simmetric.key")

int LoadKeyFromFile(HCRYPTKEY *phKey, const TCHAR *FN)
{
	HANDLE hKeyF= NULL;
	BYTE *pKeyData= NULL;
	DWORD DataLen, FileLenHi;
	
	_tcscpy(&KeyFN[KeyDirLen], FN); 

	hKeyF = CreateFile(KeyFN, GENERIC_READ, 0, NULL, OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL, NULL);
  if (hKeyF == INVALID_HANDLE_VALUE)
  {
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			return -1;
		hKeyF= NULL;
		return 14;
  }
	DataLen= ::GetFileSize(hKeyF, &FileLenHi);
	if (DataLen == 0)
	{
		CloseHandle(hKeyF);
		return 15;
	}
	pKeyData= (BYTE *)malloc(DataLen);
	if (pKeyData == NULL)
	{
		CloseHandle(hKeyF);
		return 16;
	}
	if (!ReadFile(hKeyF, pKeyData, DataLen, &FileLenHi, NULL))
	{
		free(pKeyData);
		CloseHandle(hKeyF);
		return 17;
	}
	CloseHandle(hKeyF);
  if (!CryptImportKey(hCryptoProv, pKeyData, DataLen, 0, 0, phKey))
	{
		free(pKeyData);
		return 18;
	}
	free(pKeyData);
	return 0;
}

int SaveKeyToFile(const TCHAR *FN, BYTE *pKeyData, int DataLen)
{
	HANDLE hKeyF= NULL;
	DWORD BytesCnt= 0;
	
	_tcscpy(&KeyFN[KeyDirLen], FN); 

	hKeyF =  CreateFile(KeyFN, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
              FILE_ATTRIBUTE_NORMAL, NULL);
  if (hKeyF == INVALID_HANDLE_VALUE)
  {
		hKeyF= NULL;
		return 14;
  }
  if (!WriteFile(hKeyF, pKeyData, DataLen, &BytesCnt, NULL))
  {
		CloseHandle(hKeyF);
		return 35;
  }
	CloseHandle(hKeyF);
	return 0;
}

int InitCrypto()
{
// The RSA public-key key exchange algorithm
#define ENCRYPT_ALGORITHM         CALG_RSA_KEYX              
// The high order WORD 0x0200 (decimal 512) 
// determines the key length in bits.
#define KEYLENGTH                 0x02000000

	int Rslt= 0, i= 0;
	DWORD KeyLen;
	BYTE *pKeyData;
	 
  if (!CryptAcquireContext(&hCryptoProv, ContainerName, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET)) // CRYPT_MACHINE_KEYSET))
  {
		if (GetLastError() != NTE_EXISTS) 
			return 2;
		if(!CryptAcquireContext(&hCryptoProv, ContainerName, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_DELETEKEYSET)) 
			return 2;
		if(!CryptAcquireContext(&hCryptoProv, ContainerName, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET))   
			return 2;
  }

// Import keys from file
	Rslt= LoadKeyFromFile(&hPublicKey, PUBLIC_KEY_FN);
	if (Rslt != 0)
	{
		if (Rslt != -1)
			return Rslt;
		i++;	
	}
	Rslt= LoadKeyFromFile(&hPrivateKey, PRIVATE_KEY_FN);
	if (Rslt != 0)
	{
		if (Rslt != -1)
			return Rslt;
		i++;	
	}
	Rslt= LoadKeyFromFile(&hSymmetricKey, SYMMETRIC_KEY_FN);
	if (Rslt != 0)
	{
		if (Rslt != -1)
			return Rslt;
		i++;	
	}
	DWORD Param, BytesCnt= 4;		
	if (!CryptGetKeyParam(hSymmetricKey, KP_KEYLEN, (LPBYTE) &Param, &BytesCnt, 0)) 
	{
		Rslt= 29;
	}
	
	if (i == 0)
		return 0;
// if files not found generate new keys		 
  if (!CryptGenKey(hCryptoProv, CALG_RSA_KEYX, KEYLENGTH | CRYPT_EXPORTABLE, &hRSAKey))
  {
    return 3;
  }
 // get private and public key handles
  
  if (!CryptExportKey(hRSAKey, NULL, PUBLICKEYBLOB, 0, NULL, &KeyLen))
		return 4;
  pKeyData = (BYTE *)malloc(KeyLen);
  if (pKeyData == NULL)
		return 5;
  if (!CryptExportKey(hRSAKey, NULL, PUBLICKEYBLOB, 0, pKeyData, &KeyLen))
	{
		free(pKeyData);
		return 6;
  }
  if (!CryptImportKey(hCryptoProv, pKeyData, KeyLen, 0, 0, &hPublicKey))
	{
		free(pKeyData);
		return 7;
	}
	Rslt= SaveKeyToFile(PUBLIC_KEY_FN, pKeyData, KeyLen);
	if (Rslt != 0)
	{
		free(pKeyData);
		return Rslt;
	}
	free(pKeyData);
	if (!CryptExportKey(hRSAKey, NULL, PRIVATEKEYBLOB, 0, NULL, &KeyLen))
		return 8;
	pKeyData = (BYTE *)malloc(KeyLen);
  if (pKeyData == NULL)
  	return 9;
	if (!CryptExportKey(hRSAKey, NULL, PRIVATEKEYBLOB, 0, pKeyData, &KeyLen))
	{
		free(pKeyData);
		return 10;
	}
  if (!CryptImportKey(hCryptoProv, pKeyData, KeyLen, 0, 0, &hPrivateKey))
	{
		free(pKeyData);
		return 11;
	}
	Rslt= SaveKeyToFile(PRIVATE_KEY_FN, pKeyData, KeyLen);
	if (Rslt != 0)
	{
		free(pKeyData);
		return Rslt;
	}
	free(pKeyData);
 
  if (!CryptGenKey(hCryptoProv, CALG_RC4, CRYPT_EXPORTABLE, &hSymmetricKey))
		return 24; 
	if (!CryptExportKey(hSymmetricKey, hRSAKey, SIMPLEBLOB, 0, NULL, &KeyLen))
		return 8;
	pKeyData = (BYTE *)malloc(KeyLen);
  if (pKeyData == NULL)
  	return 25;
	if (!CryptExportKey(hSymmetricKey, hRSAKey, SIMPLEBLOB, 0, pKeyData, &KeyLen))
	{
		free(pKeyData);
		return 25;
	}
	Rslt= SaveKeyToFile(SYMMETRIC_KEY_FN, pKeyData, KeyLen);
	if (Rslt != 0)
	{
		free(pKeyData);
		return Rslt;
	}
	free(pKeyData);
		
	if (!CryptGetKeyParam(hSymmetricKey, KP_KEYLEN, (LPBYTE) &Param, &BytesCnt, 0)) 
	{
		Rslt= 29;
	}

	
  return 0;
}

void CloseCrypto()
{
  if (hRSAKey != NULL) CryptDestroyKey(hRSAKey);
  if (hPublicKey != NULL) CryptDestroyKey(hPublicKey);
  if (hPrivateKey != NULL) CryptDestroyKey(hPrivateKey);
  if (hSymmetricKey != NULL) CryptDestroyKey(hSymmetricKey);
  if (hCryptoProv != NULL) CryptReleaseContext(hCryptoProv, 0);
}

int RSA_EncryptDecryptSelectedFile(bool fEncrypt)
{
static DWORD BuffLen= 0, BytesToEnc= 0, BytesToDec= 0;
int Rslt= 0;
DWORD	BytesToRead, WriteCnt= 0, BytesCnt= 0;
bool finished = false;
BYTE *pBuff= NULL; //*pSrcBuff= NULL, *pOstBuff= NULL;
DWORD Param;

__try
{
	if (BuffLen == 0)
	{
		BytesCnt= 4;		
		if (!CryptGetKeyParam(hPrivateKey, KP_KEYLEN, (LPBYTE) &Param, &BytesCnt, 0)) 
		{
			Rslt= 29;
			 __leave;
		}
		BuffLen= Param;

/*
		BytesCnt= 4;		
		if (!CryptGetKeyParam(hRSAKey, KP_BLOCKLEN, (LPBYTE) &Param, &BytesCnt, 0)) 
		{
			Rslt= 29;
			 __leave;
		}
		BytesCnt= 4;		
		if (!CryptGetKeyParam(hRSAKey, KP_PERMISSIONS, (LPBYTE) &Param, &BytesCnt, 0))
		{
			Rslt= 29;
			 __leave;
		}
		if (Param & CRYPT_DECRYPT) 
			MessageBeep(0);
		if (Param & CRYPT_ENCRYPT)
			MessageBeep(0);
*/		
		
		BuffLen= (BuffLen+ 7) / 8;
		BytesToEnc= BuffLen- 11;
		BytesToDec= BytesToEnc;
	
		if (!CryptEncrypt(hPublicKey, NULL, TRUE, 0, NULL, &BytesToDec, 0))
		{
			Rslt= 30;
			 __leave;
		}
	}
	pBuff= (BYTE *)malloc(BuffLen);
	if (pBuff == NULL)
	{
		Rslt= 31;
		__leave;
	}
	if (fEncrypt)
		BytesToRead= BytesToEnc;
	else
		BytesToRead= BytesToDec;
	do
	{
		BytesCnt= 0;
		if (!ReadFile(hSrcF, pBuff, BytesToRead, &BytesCnt, NULL))
		{
			Rslt= 32;
			__leave; 
		}
		if (BytesCnt == 0)
		{
			break;
		}
		if (BytesCnt < BytesToRead)
			finished= true;
		if (fEncrypt)
		{
			if (!CryptEncrypt(hPublicKey, 0, true, 0, pBuff, &BytesCnt, BuffLen))
			{
				Rslt= 33;
				 __leave;
			}
		}
	 else
	 {
//			BytesToRead= 64;
			if (!CryptDecrypt(hPrivateKey, 0, true, 0, pBuff, &BytesCnt))
			{
				Rslt= 34;
				 __leave;
			}
	 }
	 // Write the encrypted/decrypted data to the output file.
	 if (!WriteFile(hDstF, pBuff, BytesCnt, &WriteCnt, NULL))
	 {
			Rslt= 35;
		__leave;
	 }
	}
	while (!finished);
}
__finally
{
	CloseFiles();
}	
	if (pBuff != NULL)
		free(pBuff);
	return Rslt;
}

int RC4_EncryptDecryptSelectedFile(bool fEncrypt)
{
static DWORD BuffLen= 1024; // BytesToEnc= 0, BytesToDec= 0;
int Rslt= 0;
DWORD	BytesToRead= 1024, WriteCnt= 0, BytesCnt= 0;
bool finished = false;
BYTE *pBuff= NULL; //*pSrcBuff= NULL, *pOstBuff= NULL;


__try
{
	pBuff= (BYTE *)malloc(BuffLen);
	if (pBuff == NULL)
	{
		Rslt= 31;
		__leave;
	}
	do
	{
		BytesCnt= 0;
		if (!ReadFile(hSrcF, pBuff, BytesToRead, &BytesCnt, NULL))
		{
			Rslt= 32;
			__leave; 
		}
		if (BytesCnt == 0)
		{
			break;
		}
		if (BytesCnt < BytesToRead)
			finished= true;
		if (fEncrypt)
		{
			if (!CryptEncrypt(hSymmetricKey, 0, true, 0, pBuff, &BytesCnt, BuffLen))
			{
				Rslt= 33;
				 __leave;
			}
		}
	 else
	 {
//			BytesToRead= 64;
			if (!CryptDecrypt(hSymmetricKey, 0, true, 0, pBuff, &BytesCnt))
			{
				Rslt= 34;
				 __leave;
			}
	 }
	 // Write the encrypted/decrypted data to the output file.
	 if (!WriteFile(hDstF, pBuff, BytesCnt, &WriteCnt, NULL))
	 {
			Rslt= 35;
		__leave;
	 }
	}
	while (!finished);
}
__finally
{
	CloseFiles();
}	
	if (pBuff != NULL)
		free(pBuff);
	return Rslt;
}

int EncryptSelectedFile(/*HANDLE SrcFile, HANDLE dstFile*/)
{
	if (IsRSACrypting)
		return RSA_EncryptDecryptSelectedFile(true);
	else
		return RC4_EncryptDecryptSelectedFile(true);
}

int DecryptSelectedFile(/*HANDLE SrcFile, HANDLE dstFile*/)
{
	if (IsRSACrypting)
		return RSA_EncryptDecryptSelectedFile(false);
	else
		return RC4_EncryptDecryptSelectedFile(false);
}
