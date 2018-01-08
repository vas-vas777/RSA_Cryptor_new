#include <Windows.h>
#include <tchar.h>



HWND hMainWnd= NULL;

void DsplErrDescr(int ErrInd)
{
TCHAR *lpMsgBuf;
LONG ErrCode;
TCHAR ErrDescr[2048];
TCHAR Function[128];

	ErrCode= GetLastError();
	FormatMessage( 
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    ErrCode,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    (LPTSTR)&lpMsgBuf,
    0,
    NULL);

	switch (ErrInd)
	{
		case 1:
	    _tcscpy_s(Function, _T("CryptSetProvider"));
		break;
		case 2:
	    _tcscpy_s(Function, _T("CryptAcquireContext"));
		break;
		case 3:
	    _tcscpy_s(Function, _T("CryptGenKey"));
		break;
		case 4:
			 _tcscpy_s(Function, _T("CryptExportKey(PUBLICKEYBLOB)"));
		break;
		case 5:
			_tcscpy_s(Function, _T("malloc"));
		break;
		case 6:
			_tcscpy_s(Function, _T("CryptExportKey(PUBLICKEYBLOB)"));
		break;
		case 7:
			_tcscpy_s(Function, _T("CryptImportKey(&hPublicKey)"));
		break;
		case 8:
			_tcscpy_s(Function, _T("CryptExportKey(PRIVATEKEYBLOB)"));
		break;
		case 9:
			_tcscpy_s(Function, _T("malloc"));
		break;
		case 10:
			_tcscpy_s(Function, _T("CryptExportKey(PRIVATEKEYBLOB)"));
		break;
		case 11:
			_tcscpy_s(Function, _T("CryptImportKey(&hPrivateKey)"));
		break;
		case 12:
			_tcscpy_s(Function, _T(""));
		break;
		case 13:
			_tcscpy_s(Function, _T(""));
		break;
		case 14:
			_tcscpy_s(Function, _T("CreateFile(KeyFN)"));
		break;
		case 15:
			_tcscpy_s(Function, _T("GetFileSize(hKeyF)"));
		break;
		case 16:
			_tcscpy_s(Function, _T("malloc(FileLenLo)"));
		break;
		case 17:
			_tcscpy_s(Function, _T("ReadFile(hKeyF)"));
		break;
		case 18:
			_tcscpy_s(Function, _T("CryptImportKey"));
		break;
		case 19:
			_tcscpy_s(Function, _T(""));
		break;
		case 20:
			_tcscpy_s(Function, _T("CreateFile(SrcFN)"));
		break;
		case 21:
			_tcscpy_s(Function, _T("CreateFile(DstFN)"));
		break;
		case 22:
			_tcscpy_s(Function, _T(""));
		break;
		case 23:
			_tcscpy_s(Function, _T(""));
		break;
		case 24:
			_tcscpy_s(Function, _T("CryptGenKey(CALG_RC4)"));
		break;
		case 25:
			_tcscpy_s(Function, _T("CryptExportKey(SIMPLEBLOB)"));
		break;
		case 26:
			_tcscpy_s(Function, _T(""));
		break;
		case 27:
			_tcscpy_s(Function, _T(""));
		break;
		case 29:
			_tcscpy_s(Function, _T("CryptGetKeyParam"));
		break;
		case 30:
			_tcscpy_s(Function, _T("CryptEncrypt(GetBuffSize))"));
		break;
		case 31:
			_tcscpy_s(Function, _T("malloc()"));
		break;
		case 32:
			_tcscpy_s(Function, _T("ReadFile"));
		break;
		case 33:
			_tcscpy_s(Function, _T("CryptEncrypt"));
		break;
		case 34:
			_tcscpy_s(Function, _T("CryptDecrypt. SetFilePointer"));
		break;
		case 35:
			_tcscpy_s(Function, _T("Internal crypting exception"));
		break;
		case 36:
			_tcscpy_s(Function, _T("Internal crypting error"));
		break;
		case 37:
			_tcscpy_s(Function, _T("CryptDecrypt. SetFileValidData"));
		break;
	}

  _stprintf_s(ErrDescr, _T("%s\r\n\t -Err #%u(0x%.08X)\r\n%ws"), Function, ErrCode, ErrCode, lpMsgBuf);
  MessageBox(hMainWnd, ErrDescr, _T("Error"), MB_OK | MB_ICONINFORMATION );

  LocalFree( lpMsgBuf );
}

 