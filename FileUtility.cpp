#include <windows.h>
#include <tchar.h>

//#pragma warning(disable: 4996)

TCHAR KeyFN[MAX_PATH+ 1];
int KeyDirLen= 0;
static TCHAR ListOfFN[2048];
static TCHAR SrcFN[MAX_PATH+ 1];
static TCHAR DstFN[MAX_PATH+ 1];
static TCHAR WorkDir[MAX_PATH+ 1];
static int SrcDirLen= 0;
static int DstDirLen= 0;
static int WorkDirLen= 0;

static TCHAR *pSrcFile= NULL;

HANDLE hSrcF= NULL, hDstF= NULL;

bool SelectFile(TCHAR *DlgTitle, TCHAR *FileExts, bool SelectToEncrypt, HWND hOwnerWnd)
{
OPENFILENAME OSrcFNInfo;
DWORD ErrCode;


	ListOfFN[0]= 0;
	SrcFN[0]= 0;
	SrcDirLen= 0;
	pSrcFile= NULL;
	
  memset(&OSrcFNInfo,0,sizeof(OPENFILENAME));
  OSrcFNInfo.lStructSize = sizeof(OPENFILENAME);
  OSrcFNInfo.hwndOwner = hOwnerWnd;
  OSrcFNInfo.lpstrFile = ListOfFN;
  OSrcFNInfo.nMaxFile = MAX_PATH; 
  OSrcFNInfo.lpstrFilter = FileExts;				

  OSrcFNInfo.lpstrTitle = DlgTitle;					
  OSrcFNInfo.lpstrInitialDir = NULL; //SrcFN;
  OSrcFNInfo.Flags =  OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

  if (!GetOpenFileName(&OSrcFNInfo))
	{
		ErrCode= CommDlgExtendedError();
    SrcFN[0]= 0;
		switch (ErrCode)
		{
			case 0:
			break;
			case CDERR_DIALOGFAILURE: 
			break;
			case CDERR_FINDRESFAILURE: 
			break;
			case CDERR_INITIALIZATION: 
			break;
			case CDERR_LOADRESFAILURE: 
			break;
			case CDERR_LOADSTRFAILURE: 
			break;
			case CDERR_LOCKRESFAILURE: 
			break;
			case CDERR_MEMALLOCFAILURE: 
			break;
			case CDERR_MEMLOCKFAILURE: 
			break;
			case CDERR_NOHINSTANCE: 
			break;
			case CDERR_NOHOOK: 
			break;
			case CDERR_NOTEMPLATE:
			break;
			case CDERR_STRUCTSIZE: 
			break;
			case FNERR_BUFFERTOOSMALL:
        ::MessageBox(hOwnerWnd, _T("Too many files selected.Try again"), _T("Error"), MB_OK);
			break;
			case FNERR_INVALIDFILENAME: 
			break;
			case FNERR_SUBCLASSFAILURE:
			break;
			default:	
        ::MessageBox(hOwnerWnd, _T("GetOpenFileName failed"), _T("Unknown Error Code"), MB_OK);
		}
		return false;
	}
	else
	{
	}

	if (SelectToEncrypt)
	{
		_tcscpy(&DstFN[WorkDirLen], _T("\\Enc\\"));
	}
	else
		_tcscpy(&DstFN[WorkDirLen], _T("\\Dec\\"));
	DstDirLen= _tcslen(DstFN);

	_tcscpy_s(SrcFN, ListOfFN);
	SrcDirLen= _tcslen(SrcFN);
	if ((OSrcFNInfo.nFileOffset- SrcDirLen) == 1)
	{
		SrcFN[SrcDirLen]= '\\';
		SrcDirLen++;
	}
	else
		SrcDirLen= OSrcFNInfo.nFileOffset;
	pSrcFile= ListOfFN+ OSrcFNInfo.nFileOffset;
	return true;
}

int PrepareFiles(bool PreoareToEncrypt, TCHAR **ppShortFN)
{
	int i= 0;
	
	if (*pSrcFile == 0)
		return -1;
		
	if (ppShortFN != NULL)
		*ppShortFN= pSrcFile;
			
	do
	{
		SrcFN[SrcDirLen+ i]= *pSrcFile;
		DstFN[DstDirLen+ i]= *pSrcFile;
		pSrcFile++;
		i++;
	}	
	while (*pSrcFile != 0);
	SrcFN[SrcDirLen+ i]= 0;
	DstFN[DstDirLen+ i]= 0;
	pSrcFile++;
	
	if (PreoareToEncrypt)
	{
		_tcscat(DstFN, _T(".rsa"));
	}
	else
	{
		DstFN[DstDirLen+ i- 4]= 0;
	}

	hSrcF = CreateFile(SrcFN, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
							FILE_ATTRIBUTE_NORMAL, NULL);
  if (hSrcF == INVALID_HANDLE_VALUE)
  {
		hSrcF= NULL;
		return 20;
  }
  hDstF = CreateFile(DstFN, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 
              FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDstF == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hSrcF);
		hSrcF= NULL;
		hDstF= NULL;
		return 21;
	}
	
// LockFile 
	SetFilePointer(hSrcF, 0, 0, FILE_BEGIN);
	SetFilePointer(hDstF, 0, 0, FILE_BEGIN);

	return 0;
}

void CloseFiles()
{
	if (hSrcF != NULL)
	{
		CloseHandle(hSrcF);
		hSrcF= NULL;
	}
	if (hDstF != NULL)
	{
		CloseHandle(hDstF);
		hDstF= NULL;
	}
}

void InitWorkDir()
{
	::GetCurrentDirectory(MAX_PATH, WorkDir);
	WorkDirLen= _tcslen(WorkDir);

	_tcscpy_s(DstFN, WorkDir);

	_tcscpy_s(KeyFN, WorkDir);
	_tcscat_s(KeyFN, _T("\\Keys\\"));
	::CreateDirectory(KeyFN, NULL);
	KeyDirLen= _tcslen(KeyFN);

	_tcscat_s(WorkDir, _T("\\Enc\\"));
	::CreateDirectory(WorkDir, NULL);

	_tcscpy(&WorkDir[WorkDirLen], _T("\\Dec\\"));
	::CreateDirectory(WorkDir, NULL);
}

