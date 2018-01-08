extern bool IsRSACrypting;

int InitCrypto();
void CloseCrypto();
int EncryptSelectedFile(/*HANDLE SrcFile, HANDLE dstFile*/);
int DecryptSelectedFile(/*HANDLE SrcFile, HANDLE dstFile*/);
