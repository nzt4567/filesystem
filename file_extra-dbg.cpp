#include "includes.h"

using namespace std;

#ifdef DEBUG

int     FileSize            ( const   char*           fileName      )
{
    int   fileNamePos = -1;
    
    if ((fileNamePos = SearchFileName(fileName)) != -1)
        return g_Table_FileName[fileNamePos].m_RealSize;
    
    return fileNamePos;
}

int     FileFindFirst       (         struct TFile*   info          )
{
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++)
        if (g_Table_FileName[i].m_FirstSector != FREE_FILE)
        {
            g_LastFile = i+1;
            info->m_FileSize = g_Table_FileName[i].m_RealSize;
            strncpy(info->m_FileName, g_Table_FileName[i].m_FileName, 29);
            return 1;
        }    
    return 0;
}

int     FileFindNext        (         struct TFile*   info          )
{
    for (g_LastFile; g_LastFile < DIR_ENTRIES_MAX; g_LastFile++)
    {
        if (g_Table_FileName[g_LastFile].m_FirstSector != FREE_FILE)
        {
            info->m_FileSize = g_Table_FileName[g_LastFile].m_RealSize;
            strncpy(info->m_FileName, g_Table_FileName[g_LastFile].m_FileName, 29);
            g_LastFile++;
            return 1;
        }
    }
    return 0;
}

int     FileDelete          ( const   char*           fileName      )
{
    if (g_Mounted != 1)
        return DELETE_ERROR;
    
    int   fileNamePos = -1;
    
    if ((fileNamePos = SearchFileName(fileName)) == -1)
        return DELETE_ERROR;
    
    DeleteFileSectors(fileNamePos);
    
    // takhle sem v formatovani rozhodl o podobe prazdnyho file
    // takze by bylo dobry, aby to mazani dodrzovalo
    for (register int i = 0; i < FILENAME_LEN_MAX; i++)
            g_Table_FileName[fileNamePos].m_FileName[i] = EMPTY_CHAR;
        
    g_Table_FileName[fileNamePos].m_RealSize    = FREE_FILE;
    g_Table_FileName[fileNamePos].m_FirstSector = FREE_SECTOR;
    
    return 1;
}




#endif
