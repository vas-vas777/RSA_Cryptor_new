
#include "stdafx.h"
#include "DlgCryptingStatus.h"
#include "File_Cryptor.h"
#include "LONG_INT_RSA.h"
#include "FileUtility.h"
#include "ErrHandler.h"
#include "LONG_INT_UnitTest.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
LOGFONT lFont;
HFONT hFont= NULL;
HWND hListBox= NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void DsplStatus(TCHAR *StatusStr)
{
	SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)StatusStr); 
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
			

	HDC hDcNull;
	
	hDcNull= GetWindowDC(0);
	GetObject(GetStockObject(ANSI_VAR_FONT), sizeof(LOGFONT), &lFont); 

	lstrcpy(lFont.lfFaceName, _T("Courier New"));  
	lFont.lfWeight= FW_NORMAL; 
	lFont.lfHeight= -MulDiv(14, GetDeviceCaps(hDcNull, LOGPIXELSY), 72);
	hFont= CreateFontIndirect(&lFont); 
	ReleaseDC(0, hDcNull);
	
	MSG msg;
	HACCEL hAccelTable;
	
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RSA_CRYPTOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RSA_CRYPTOR));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RSA_CRYPTOR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RSA_CRYPTOR);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int x, y;
	int CryptoRslt= 0;
	
	hInst = hInstance; // Store instance handle in our global variable

	x= GetSystemMetrics(SM_CXSCREEN); 
	y= GetSystemMetrics(SM_CYSCREEN); 

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		100, 50, x- 200, y- 100, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hMainWnd= hWnd;
	InitWorkDir();
	CryptoRslt= RSA_Init(); // InitCrypto();	
	if (CryptoRslt != 0)
		DsplErrDescr(CryptoRslt);
	
	Test_Init();
	
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HMENU hMenu= NULL;
	RECT Rect;
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int CryptoRslt;
	
	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		case ID_FILE_ENCRYPT:
			if (!SelectFile(_T("Select a file to encrypt"), _T("documents (*.doc;*.docx;*.txt)\0*.doc;*.docx;*.txt\0images (*.bmp;*.jpg;*.png)\0*.bmp;*.jpg;*.png\0binary files 9.exe;*.dll)\0*.exe;*.dll\0All files (*.*)\0*.*\0\0"), true, hWnd))
				break;
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Wait, please")); 		
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Encrypting...")); 		
			::UpdateWindow(hListBox);

			DoCrypting(hInst, hWnd, true);
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			::MessageBox(hWnd, _T("Encrypting was complited"), _T("Inormation"), MB_ICONINFORMATION | MB_OK);
		break;
		case ID_FILE_DECRYPT:	
			if (!SelectFile(_T("Select a file to decrypt"), _T("encrypted files(*.rsa\0*.rsa\0\0"), false, hWnd))
				break;
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Wait, please")); 		
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Decrypting...")); 			
			::UpdateWindow(hListBox);
		
			DoCrypting(hInst, hWnd, false);
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			::MessageBox(hWnd, _T("Decrypting was complited"), _T("Inormation"), MB_ICONINFORMATION | MB_OK);
		break;

		case ID_LONG_ADDITIONSUBSTRACTION:
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			Test1();
			Test2();
		break;
		case ID_LONG_MULTIPLICATIONDIVISION:
			SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
			Test3();
			Test4();
		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		DeleteObject(hFont);
		RSA_Clear();
		Test_Clear();
		PostQuitMessage(0);
		break;
		case WM_CREATE:
			hMenu = GetMenu(hWnd); 

			GetClientRect(hWnd, &Rect);
			hListBox= CreateWindow(_T("LISTBOX"), NULL, WS_CHILD | WS_VISIBLE | LBS_NOSEL, 15, 5, Rect.right- 15, Rect.bottom- 5, hWnd, NULL, hInst, NULL);

		break;	

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
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
