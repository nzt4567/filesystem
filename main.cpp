#include "includes.h"

using namespace std;
/* Filesystem - sample usage.
 *
 * The testing of the fs driver requires a backend (simulating the underlying disk).
 * Next, tests of your fs implemetnation are needed. To help you with the implementation,
 * a sample backend is implemented in this file. It provides a quick-and-dirty
 * implementation of the underlying disk (simulated in a file) and a few Fs... function calls.
 *
 * The implementation in the real testing environment is different. The sample below is a
 * minimalistic disk backend which matches the required interface.
 *
 * You will have to add some FS testing. There is a few Fs... functions called from within
 * main(), however, the tests are incomplete. Once again, this is only a starting point.
 */

#define DISK_SECTORS 2048
static FILE  * g_Fp = NULL;

//-------------------------------------------------------------------------------------------------
/** Sample sector reading function. The function will be called by your fs driver implementation.
 * Notice, the function is not called directly. Instead, the function will be invoked indirectly
 * through function pointer in the TBlkDev structure.
 */
int                diskRead                                ( int               sectorNr,
                                                             void            * data,
                                                             int               sectorCnt )
 {
   if ( g_Fp == NULL ) return 0;
   if ( sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS ) return 0;
   fseek ( g_Fp, sectorNr * SECTOR_SIZE, SEEK_SET );
   return fread ( data, SECTOR_SIZE, sectorCnt, g_Fp );
 }
//-------------------------------------------------------------------------------------------------
/** Sample sector writing function. Similar to diskRead
 */
int                diskWrite                               ( int               sectorNr,
                                                             const void      * data,
                                                             int               sectorCnt )
 {
   if ( g_Fp == NULL ) return 0;
   if ( sectorCnt <= 0 || sectorNr + sectorCnt > DISK_SECTORS ) return 0;
   fseek ( g_Fp, sectorNr * SECTOR_SIZE, SEEK_SET );
   return fwrite ( data, SECTOR_SIZE, sectorCnt, g_Fp );
 }
//-------------------------------------------------------------------------------------------------
/** A function which creates the file needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above. It could be understand as
 * "buying a new disk".
 */
TBlkDev          * createDisk                              ( void )
 {
   char       buffer[SECTOR_SIZE];
   TBlkDev  * res = NULL;
   int        i;

   memset    ( buffer, 1, sizeof ( buffer ) );

   g_Fp = fopen ( "/tmp/disk_content", "w+b" );
   if ( ! g_Fp ) return NULL;

   for ( i = 0; i < DISK_SECTORS; i ++ )
    if ( fwrite ( buffer, sizeof ( buffer ), 1, g_Fp ) != 1 )
     return NULL;

   res              = new TBlkDev;
   res -> m_Sectors = DISK_SECTORS;
   res -> m_Read    = diskRead;
   res -> m_Write   = diskWrite;
   return res;
 }
//-------------------------------------------------------------------------------------------------
/** A function which opens the files needed for the sector reading/writing functions above.
 * This function is only needed for the particular implementation above. It could be understand as
 * "turning the computer on".
 */
TBlkDev          * openDisk                                ( void )
 {
   TBlkDev  * res = NULL;
   int        i;
   char       fn[100];

   g_Fp = fopen ( "/tmp/disk_content", "r+b" );
   if ( ! g_Fp ) return NULL;
   fseek ( g_Fp, 0, SEEK_END );
   if ( ftell ( g_Fp ) != DISK_SECTORS * SECTOR_SIZE )
    {
      fclose ( g_Fp );
      g_Fp = NULL;
      return NULL;
    }

   res              = new TBlkDev;
   res -> m_Sectors = DISK_SECTORS;
   res -> m_Read    = diskRead;
   res -> m_Write   = diskWrite;
   return res;
 }
//-------------------------------------------------------------------------------------------------
/** A function which releases resources allocated by openDisk/createDisk
 */
void               doneDisk                                ( TBlkDev         * dev )
 {
   delete dev;
   if ( g_Fp )
    {
      fclose ( g_Fp );
      g_Fp  = NULL;
    }
 }
//-------------------------------------------------------------------------------------------------

// Jelikoz kod vypada ze funguje, ehm, dostal sem 40 ze 40 hnedka z prvni...
// tak sem nahodim komentare, kdybych se k tomu chtel nekdy vratit... 
// a poslu si to na PT kdyby mi spadly vsechny servery a vsechny zalohy 
// smetlo tornado a vubec katastrofy nejsou jenom v USA... a este trochu upravim 
// ty podminky, myslim ze by se ta cyklomaticka dala este trosku stahnout

int main ( void )
 {
   TBlkDev * dev;
   int       i, fd1, fd2, fd3, retCode;
   char      buffer [SECTOR_SIZE];
   TFile     info;
   
   //dev = createDisk ();
   dev = openDisk ();
   retCode = FsCreate ( dev );
   
   retCode = FsMount ( dev );
   printf("Mount Retcode: %d\n", retCode);

   fd1 = FileOpen("do_velikosti_sektoru", 0);
   printf("FileDescriptor: %d\n", fd1);
   
   char * testplace  = new char[3000];
   char * dealocator = testplace;
   
   char * written   = new char[3000];
   char * dealocator_2 = written;
   
   memset(testplace, 2, 200);
   i = FileRead(fd1, (void *) written, 200);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 200; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 200 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 200 bajtu ok" << endl;
   testplace += 200;
   written += 200;
   
   
   memset(testplace, 3, 511);
   i = FileRead(fd1, (void *) written, 511);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 511; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 511 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 511 bajtu ok" << endl;
   testplace += 511;
   written += 511;

   memset(testplace, 4, 0);
   i = FileRead(fd1, (void *) written, 0);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 0; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 2 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 0 bajtu ok" << endl;
   testplace += 0;
   written += 0;
   
   memset(testplace, 5, 2);
   i = FileRead(fd1, (void *) written, 2);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 2; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 2 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 2 bajtu ok" << endl;
   testplace += 2;
   written += 2;
   
   memset(testplace, 6, 100);
   i = FileRead(fd1, (void *) written, 100);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 100; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 100 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 100 bajtu ok" << endl;
   testplace += 100;
   written += 100;
   
   
   memset(testplace, 7, 23);
   i = FileRead(fd1, (void *) written, 23);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 23; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 23 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 23 bajtu ok" << endl;
   testplace += 23;
   written += 23;
   
   memset(testplace, 8, 111);
   i = FileRead(fd1, (void *) written, 111);
   cout << "FileRead navratova hodnota: " << i << endl;
   
   for (register int i = 0; i < 111; i++)
       if (testplace[i] != written[i])
       {
           cout << "Cteni 111 bajtu " << endl;
           cout << "Lisime se na bajtu " << endl;
           return 1;
       }
   cout << "Cteni 111 bajtu ok" << endl;
   testplace += 111;
   written += 111;
   
//   delete [] dealocator;
//   delete [] dealocator_2;
//   //------------------------------------------------------------------------------------------------------------------------------
//   fd2 = FileOpen("nasobky_velikosti_sektoru", 0);
//   printf("FileDescriptor: %d\n", fd2);
//   
//   testplace  = new char[300000];
//   dealocator = testplace;
//   
//   written   = new char[300000];
//   dealocator_2 = written;
//   
//   memset(testplace, 2, 1024);
//   i = FileRead(fd2, (void *) written, 1024);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 1024; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 1024 bajtu " << endl;
//           cout << "Lisime se na bajtu " << i << endl;
//           return 1;
//       }
//   cout << "Cteni 1024 bajtu ok" << endl;
//   testplace += 1024;
//   written += 1024;
//   
//   
//   memset(testplace, 3, 512);
//   i = FileRead(fd2, (void *) written, 512);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 512; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 512 bajtu " << endl;
//           cout << "Lisime se na bajtu " << i << endl;
//           cout << "testplace je tu " << dec << testplace[i] << " a precteno je "<< dec << written[i] << endl;
//           return 1;
//       }
//   cout << "Cteni 512 bajtu ok" << endl;
//   testplace += 512;
//   written += 512;
//  
//   memset(testplace, 4, 512);
//   i = FileRead(fd2, (void *) written, 512);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 512; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 512 bajtu " << endl;
//           cout << "Lisime se na bajtu pica pica" << endl;
//           return 1;
//       }
//   cout << "Cteni 512 bajtu ok" << endl;
//   testplace += 512;
//   written += 512;
//   
//   memset(testplace, 5, 2048);
//   i = FileRead(fd2, (void *) written, 2048);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 2048; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 2048 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 2048 bajtu ok" << endl;
//   testplace += 2048;
//   written += 2048;
//   
//   memset(testplace, 6, 2048);
//   i = FileRead(fd2, (void *) written, 2048);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 2048; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 2048 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 2048 bajtu ok" << endl;
//   testplace += 2048;
//   written += 2048;
//   
//   
//   memset(testplace, 7, 4096);
//   i = FileRead(fd2, (void *) written, 4096);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 4096; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 4096 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 4096 bajtu ok" << endl;
//   testplace += 4096;
//   written += 4096;
//   
//   memset(testplace, 8, 8192);
//   i = FileRead(fd2, (void *) written, 8192);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 8192; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 8192 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 8192 bajtu ok" << endl;
//   testplace += 8192;
//   written += 8192;
//   
//   delete [] dealocator;
//   delete [] dealocator_2;
//   //-----------------------------------------------------------------------------------------------------------
//   
//   fd3 = FileOpen("vetsi_nez_sektor", 0);
//   printf("FileDescriptor: %d\n", fd3);
//   
//   testplace  = new char[300000];
//   dealocator = testplace;
//   
//   written   = new char[300000];
//   dealocator_2 = written;
//   
//   memset(testplace, 2, 1025);
//   i = FileRead(fd3, (void *) written, 1025);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 1025; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 1025 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 1025 bajtu ok" << endl;
//   testplace += 1025;
//   written += 1025;
//   
//   memset(testplace, 3, 1111);
//   i = FileRead(fd3, (void *) written, 1111);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 1111; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 1111 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 1111 bajtu ok" << endl;
//   testplace += 1111;
//   written += 1111;
//   
//   memset(testplace, 4, 8532);
//   i = FileRead(fd3, (void *) written, 8532);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 8532; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 8532 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 8532 bajtu ok" << endl;
//   testplace += 8532;
//   written += 8532;
//   
//
//   memset(testplace, 5, 3214);
//   i = FileRead(fd3, (void *) written, 3214);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 3214; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 3214 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 3214 bajtu ok" << endl;
//   testplace += 3214;
//   written += 3214;
//
//   memset(testplace, 6, 5172);
//   i = FileRead(fd3, (void *) written, 5172);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 5172; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 5172 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 5172 bajtu ok" << endl;
//   testplace += 5172;
//   written += 5172; 
//   
//   memset(testplace, 7, 791);
//   i = FileRead(fd3, (void *) written, 791);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 791; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 791 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 791 bajtu ok" << endl;
//   testplace += 791;
//   written += 791;    
//   
//   memset(testplace, 8, 11512);
//   i = FileRead(fd3, (void *) written, 11512);
//   cout << "FileRead navratova hodnota: " << i << endl;
//   
//   for (register int i = 0; i < 11512; i++)
//       if (testplace[i] != written[i])
//       {
//           cout << "Cteni 11512 bajtu " << endl;
//           cout << "Lisime se na bajtu " << endl;
//           return 1;
//       }
//   cout << "Cteni 11512 bajtu ok" << endl;
//   testplace += 11512;
//   written += 11512;   
//   
//   delete [] dealocator;
//   delete [] dealocator_2;
//   
//   FileClose(fd1);
//   FileClose(fd2);
//   FileClose(fd3);
//   
//   if (FileFindFirst(&info))
//       do
//       {
//           cout <<"Soubor : " << info.m_FileName << " ma velikost " << info.m_FileSize << endl;
//           FileDelete(info.m_FileName);
//       } while (FileFindNext(&info));
//                      
   retCode = FsUmount ();
   printf("Umount Retcode: %d\n", retCode);

   doneDisk ( dev );

   return 0;
 }