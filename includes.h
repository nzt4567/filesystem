
#ifndef INCLUDES_H
#define	INCLUDES_H

#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#define FILENAME_LEN_MAX    28
#define DIR_ENTRIES_MAX     128
#define OPEN_FILES_MAX      8
#define SECTOR_SIZE         512
#define DEVICE_SIZE_MAX     ( 1024 * 1024 * 1024 )
#define DEVICE_SIZE_MIN     ( 8 * 1024 * 1024 )

struct TFile
 {
   char            m_FileName[FILENAME_LEN_MAX + 1];
   int             m_FileSize;
 };

struct TBlkDev
 {
   int             m_Sectors;
   int          (* m_Read)( int, void *, int );
   int          (* m_Write)( int, const void *, int );
 };

#endif /* __PROGTEST__ */

#define FS_ERROR                    0
#define DELETE_ERROR                0
#define FILE_ERROR                  -1
#define FILENAME_TAB_SECTORS        9
#define FAT_TAB_SECTORS_MAX         131072
#define SECTOR_SIZE_16384B          32
#define SECTOR_SIZE_8192B           16
#define SECTOR_SIZE_4096B           8
#define SECTOR_SIZE_2048B           4
#define SECTOR_SIZE_512B            1
#define LAST_FILE_SECTOR            0xFFFFFFFF
#define FREE_SECTOR                 0x00000000
#define FREE_FILE                   0x00000000
#define MAGIC                       0x55AA55AA
#define EMPTY_CHAR                  0x00
#define DEBUG
 
 
 struct TFileName
 {
     char           m_FileName[FILENAME_LEN_MAX];
     int            m_RealSize;
     int            m_FirstSector;
 };
 
 struct TFd
 {
     TFileName *   m_File;
     char          m_Mode;
     int           m_HaveRead;
 };
 
extern int                g_LogicalSector;
extern TBlkDev   *        g_Device;
extern int       *        g_Table_Fat;
extern TFileName *        g_Table_FileName;
extern TFd       *        g_Table_Fd;
extern bool               g_Mounted;
extern int                g_FreeSector;
extern int                g_LastFile;

//int *   SearchFileSectors   ( int    fileNamePos,
//                              int *  sectors_num   );

int     Ceil                ( const   double&         number        );
void    SetLogicalSector    ( const   int&            sectors       );
int     SetMagic            ( const   struct TBlkDev* dev           );
int     CheckMagic          ( const   struct TBlkDev* dev           );
int     GetFd               ( const   char*           fileName      );
int     SearchFileName      ( const   char*           fileName      );
int     SearchFreeFile      (         void                          );
int     SearchFreeSector    (         void                          );
void    DeleteFileSectors   ( const   int&            fileNamePos   );

int     CriticalSectorW     ( const   int&            fd,
                              const   int&            nextSector,
                                      int&            bytesLeft,
                                      char**          buffer        );

int     FullSectorW         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            bytesLeft,
                                      char**          buffer        );

int     LastSectorW         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            bytesLeft,
                                      char**          buffer        );

int     CriticalSectorR     ( const   int&            fd,
                              const   int&            nextSector,
                                      int&            readLeft,
                                      char**          buffer        );

int     FullSectorR         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            readLeft,
                                      char**          buffer        );

int     LastSectorR         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            readLeft,
                                      char**          buffer        );

int     FsCreate            (         struct TBlkDev* dev           );
int     FsMount             (         struct TBlkDev* dev           );
int     FsUmount            (         void                          );

int     FileOpen            ( const   char*           fileName,
                                      int             writeMode     );

int     FileRead            (         int             fd,
                                      void*           buffer,
                                      int             len           );

int     FileWrite           (         int             fd,   
                              const   void*           buffer,  
                                      int             len           );

int     FileClose           (         int             fd            ); 
int     FileDelete          ( const   char*           fileName      );
int     FileFindFirst       (         struct TFile*   info          );
int     FileFindNext        (         struct TFile*   info          );
int     FileSize            ( const   char*           fileName      );

#endif	/* INCLUDES_H */

