#include "stdafx.h"
#include "Commctrl.h"
#include "resource.h"
#include "LONG_INT_RSA.h"
#include "FileUtility.h"
#include "ErrHandler.h"

HWND hDlgCryptingStatus=NULL;
HWND hWndFN= NULL;
HWND hWndProgressBar= NULL;
HANDLE hThrdCrypting;
bool DoEncrypting= false;
//int ProgressStep= 0;

DWORD WINAPI DoCryptingThrdFunct(LPVOID pParam)
{
	int CryptoRslt= 0;
//	bool DoEncrypt= (bool)pParam;
	TCHAR *pFN;
	
	while (true)
	{
		CryptoRslt= PrepareFiles(DoEncrypting, &pFN);
		::SetWindowText(hWndFN, pFN);
		UpdateWindow(hWndFN);
//		ProgressStep= 0;
		SendMessage(hWndProgressBar, PBM_SETPOS, 0, 0);
		UpdateWindow(hWndProgressBar);
		
		if (CryptoRslt == -1) // all files were encrypted
			break;
		if (CryptoRslt != 0)
		{
			DsplErrDescr(CryptoRslt);
		}
		else
		{
			if (DoEncrypting)
				CryptoRslt= EncryptSelectedFile();
			else
				CryptoRslt= DecryptSelectedFile();
			if (CryptoRslt != 0)
				DsplErrDescr(CryptoRslt);
		}
	}
	
	SendMessage(hDlgCryptingStatus, WM_CLOSE, 0, 0);
	CloseHandle(hThrdCrypting);
	return 0;
}

void InitStatus(TCHAR *FN, int Range)
{
	::SetWindowText(hWndFN, FN);
}

void DsplPogressStep()
{
//	ProgressStep++;
//	SendMessage(hWndProgressBar, PBM_SETPOS, (WPARAM)ProgressStep, 0);
//	UpdateWindow(hWndProgressBar);
	SendMessage(hWndProgressBar, PBM_STEPIT, 0, 0);
}

INT_PTR CALLBACK DoCryptingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
			hDlgCryptingStatus= hDlg;
			hWndFN= GetDlgItem(hDlg, IDC_STATIC_FN);
			hWndProgressBar= GetDlgItem(hDlg, IDC_PROGRESS1);
			SendMessage(hWndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(hWndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0); 
			hThrdCrypting= ::CreateThread(NULL, 0, DoCryptingThrdFunct, 0, 0, NULL);
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		break;
	}
	return (INT_PTR)FALSE;
}

void DoCrypting(HINSTANCE hInst, HWND hParant, bool Encrypt)
{
	DoEncrypting= Encrypt;
	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_PROGRESS), hParant, DoCryptingDlgProc);
}