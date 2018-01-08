#pragma once

extern TCHAR KeyFN[MAX_PATH+ 1];
extern int KeyDirLen;

extern HANDLE hSrcF, hDstF;

bool SelectFile(TCHAR *DlgTitle, TCHAR *FileExts, bool SelectToEncrypt, HWND hOwnerWnd= NULL);
int PrepareFiles(bool PreoareToEncrypt, TCHAR **ppShortFN= NULL);
void CloseFiles();
void InitWorkDir();

