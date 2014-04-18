#include "includes.h"


using namespace std;

#ifdef DEBUG

int     FileWrite           ( int              fd,   
                              const void     * buffer,  
                              int              len )
{
    int    bytesWritten = 0;
    
    // tahle kontrola by snad mela vyhodit vsechen vstupovej bordel, proslo to, takze asi jo
    if (fd < 0 || fd > 7 || g_Mounted != 1 || len <= 0 || g_Table_Fd[fd].m_File == NULL || g_Table_Fd[fd].m_Mode != 'W') 
    {
        cout << "Neplatnej fd/Umounted/Nulova velikost/zavrenej soubor/spatnej mod error." << endl;
        return bytesWritten;
    }
     
    // realSectors znaci pocet celejch zabranejch sektoru souborem
    // a realOffset je offset v poslednim sektoru, kolik bajtu z toho sektoru je platnejch
    // nextSector je zatim prvni sektor souboru, jednotlivy funkce to menej podle toho kam zapisou
    // kazdopadne by to dycky melo rikat cislo posledniho sektoru souboru (uz zase nevim jestli posledni free nebo este full, asi full)
    
    int realSectors = g_Table_Fd[fd].m_File->m_RealSize / (g_LogicalSector * SECTOR_SIZE);
    int realOffset  = g_Table_Fd[fd].m_File->m_RealSize % (g_LogicalSector * SECTOR_SIZE);
    int nextSector  = g_Table_Fd[fd].m_File->m_FirstSector;
    int bytesLeft   = len;
    int skipSectors = realSectors;
    char * helpBuffer = (char *) buffer;
    
    // tj kvuli tomu kdyz mam velikost 0 a chci nacist napr. 3 bajty -> aby to skocilo do CriticalSector
    // i kdyz by si s tim mozna poradil i LastSector
    if (g_Table_Fd[fd].m_File->m_RealSize == 0)
        realOffset = (g_LogicalSector * SECTOR_SIZE);
    
    // tak to proste je, kdyz je offset nulovej, potrebuju preskocit o sektor min, maths & kresleni
    if (realOffset == 0)
        skipSectors = realSectors - 1;
    
    cout << "FileWrite(): oteviram si soubor " << g_Table_Fd[fd].m_File->m_FileName << " (" <<  g_Table_Fd[fd].m_File->m_RealSize << " B) - firstSector: " << g_Table_Fd[fd].m_File->m_FirstSector << endl;
    cout << "FileWrite(): realSectors: " << realSectors << " (soubor zabira tolik logickych sektoru) " << endl;
    cout << "FileWrite(): realOffset: " << realOffset << " (offset konce souboru v ramci posledniho sektoru) " <<endl;
    cout << "FileWrite(): nextSector: " << nextSector << " (zde oznacuje prvni index do fat tabulky - cislo prvniho sektoru souboru)" <<endl;
    cout << "FileWrite(): bytesLeft: " << bytesLeft << " (kolik mam na tenhle zapis zapsat do souboru bajtu) " << endl;
    cout << "FileWrite(): skipSectors: " << skipSectors << " (tolik sektoru musim preskocit abych dosel ke spravnemu konci souboru) " << endl;
    
    // preskoceni uz platnejch sektoru, ptz na ty nic zapsat nechci
    for (register int i = 0; i < skipSectors; i++)
        nextSector = g_Table_Fat[nextSector];
    cout << "FileWrite(): nextSector po cyklu - tzn posledni sektor souboru: " << nextSector << endl;
    
    // je to easy, to bejt musi, jinak preskakovani nevyslo
    if (g_Table_Fat[nextSector] != LAST_FILE_SECTOR)
    {
        cout << "FileWrite(): fatal error. chyba v preskakavani sektoru. sektor neni posledni." << endl;
        return bytesWritten;
    }
    
    // volani CriticalSector pro vyrovnani offsetu v ramci sektoru
    // ta hodnota -1 znamena ze se posralo hned neco na zacatku CriticalSectoru
    // a tak se nic nezapsalo - jestli si to spravne pamatuju 
    if (realOffset != 0)
        if ( (bytesWritten = CriticalSectorW(fd, nextSector, bytesLeft, &helpBuffer)) == -1 )
        {
            bytesWritten = 0;
            cout << "FileWrite(): CriticalSection() to posral.... " << endl;
            return bytesWritten;
        }
    cout << "FileWrite(): CriticalSection() vratil: " << bytesWritten << endl;
    cout << "FileWrite(): bajtu zapsano: " << bytesWritten << endl;
    cout << "FileWrite(): zbyvajicich bajtu k zapisu: " << bytesLeft << endl;
    
    // klasicka kontrola jestli uz treba nechceme skoncit
    if (bytesLeft <= 0)
        return bytesWritten;
    
    // kdyz mame nejaky cely sektory tak je chci rovnou sypat k LV do bufferu
    // ptz na to si nebudu prece prcat svoji memory ;)
    if ( (bytesLeft / (g_LogicalSector * SECTOR_SIZE)) >= 1 )
    {
        int temp = 0;
        cout << "FileWrite(): do souboru se da zapsat  " << (bytesLeft / (g_LogicalSector * SECTOR_SIZE)) << " celych logickych sekotru, volam FullSector() " << endl;
        
        // nula by byla divna zejo, jelikoz tady vyslo ze sou nejaky cely sektory k zapsani
        // a potom by se vratilo ze se zapsalo 0 ? no tak to se to v FullSectorW posralo.... 
        if ( (temp = FullSectorW(fd, nextSector, bytesLeft, &helpBuffer)) == 0 )
        {
            cout << "FileWrite(): FullSector() to posral.... i kdyz tohle neni tak uplne posiracka, mohlo proste behem zapisu dojit misto, ale nemelo by se to stat" << endl;
            return bytesWritten;
        }
        cout << "FileWrite(): FullSector() vratil " << temp << endl;
        bytesWritten += temp;
        cout << "FileWrite(): bajtu zapsano: " << bytesWritten << endl;
        cout << "FileWrite(): zbyvajicich bajtu k zapisu: " << bytesLeft << endl;
        
        // end checking, klasa... 
        if (bytesLeft <= 0)
            return bytesWritten;
    }
    
    // reseni offset na konci sektoru, taky myslim ze sem to vymyslel tak
    // ze by to melo resit zapis min nez na celej sektor bajtu kdyz ma ten
    // soubor zarovnanou velikost na sektor presne
    int temp = 0; 
    if ( (temp = LastSectorW(fd, nextSector, bytesLeft, &helpBuffer)) == 0 )
    {
         cout << "FileWrite(): LastSector() to posral... " << endl;
         return bytesWritten;
    }
    
    bytesWritten += temp;
    cout << "FileWrite(): ending... " << endl;
    return bytesWritten;
}

int     CriticalSectorW     ( const int&   fd,
                              const int&   nextSector,
                                    int&   bytesLeft,
                                    char** buffer      )
{
    int    sectorsIO      = 0;
    int    toWrite        = 0;
    char * criticalSector = new char[SECTOR_SIZE * g_LogicalSector];
    char * help           = criticalSector;
    int    realOffset     = g_Table_Fd[fd].m_File->m_RealSize % (g_LogicalSector * SECTOR_SIZE);
    int    sectorFree     = SECTOR_SIZE * g_LogicalSector;
    
    if ( (sectorsIO = g_Device->m_Read(nextSector * g_LogicalSector, criticalSector, g_LogicalSector)) != g_LogicalSector )
    {
        cout << "   CriticalSector(): nepodarilo se korektne nacist kriticky sektor z disku error" << endl;
        cout << "   CriticalSector(): nacteno bylo: " << sectorsIO << " FYZICKYCH sektoru." << endl;
        delete [] criticalSector;
        return FILE_ERROR;
    }
    
    if (g_Table_Fd[fd].m_File->m_RealSize != 0)
    {
       help += realOffset;
       sectorFree -= realOffset;
    }
    
    if (bytesLeft <= sectorFree)
        toWrite = bytesLeft;
    else 
        toWrite = sectorFree;
    
    cout << "   CriticalSector(): po rozhodovani, pred memcpy a zapisem na disk to vypada takhle: " << endl;
    cout << "   CriticalSector(): realOffset: " << realOffset << " (u souboru se size 0 se muze lisit oproti offsetu v FileWrite())" << endl;
    cout << "   CriticalSector(): sectorFree: " << sectorFree << " (volnyho mista v ramci kritickyho sektoru)" << endl;
    cout << "   CriticalSector(): bytesLeft: " << bytesLeft << " (celkove bajtu k zapsani)" <<endl;
    cout << "   CriticalSector(): toWrite: " << toWrite << " (bajtu k zapsani do kritickeho sektoru)" <<endl;
    cout << "   CriticalSector(): nextSector: " << nextSector << " (cislo kritickeho sektoru)" << endl;
    cout << "   CriticalSector(): realSize: " << g_Table_Fd[fd].m_File->m_RealSize << " (opravdova velikost souboru pred zapisem)" << endl;
    
    memcpy(help, *buffer, toWrite);
    
    if ( (sectorsIO = g_Device->m_Write(nextSector * g_LogicalSector, criticalSector, g_LogicalSector)) != g_LogicalSector )
    {
        delete [] criticalSector;
        cout << "   CriticalSector(): nepodarilo se zapsat vsechna data zpatky na disk error" << endl;
        cout << "   CriticalSector(): zapsano bylo: " << sectorsIO << " FYZICKYCH sektoru." << endl;
        if ( (g_Table_Fd[fd].m_File->m_RealSize + toWrite) <= sectorsIO * SECTOR_SIZE)
        {
            cout << "   CriticalSector(): Ale vypada to, ze vsechna podstatna data byla zapsana... " << endl;
            bytesLeft -= toWrite;
            g_Table_Fd[fd].m_File->m_RealSize += toWrite;
            *buffer += toWrite;
            return toWrite;
        }
        cout << "   CriticalSector(): a este ke vsemu se tam neveslo vsechno co melo! " << endl;
        cout << "   CriticalSector(): veslo se " <<  sectorsIO * SECTOR_SIZE << " bajtu" << endl;
        bytesLeft -= sectorsIO * SECTOR_SIZE;
        g_Table_Fd[fd].m_File->m_RealSize += sectorsIO * SECTOR_SIZE;
        *buffer += sectorsIO * SECTOR_SIZE;
        return sectorsIO * SECTOR_SIZE;
    }
    
    bytesLeft -= toWrite;
    g_Table_Fd[fd].m_File->m_RealSize += toWrite;
    *buffer += toWrite;
    
    cout << "   CriticalSector(): Po pravdepodobnem uspesnem zapsani na disk... " << endl;
    cout << "   CriticalSector(): bytesLeft: " << bytesLeft << " (jeste tolik bajtu zbyva celkove pro tenhle FileWrite() k zapisu)" << endl;
    cout << "   CriticalSector(): realSize: " << g_Table_Fd[fd].m_File->m_RealSize << " (soucasna skutecna velikost souboru)" << endl;
    cout << "   CriticalSector(): ending.... " << endl;
    delete [] criticalSector;
    return toWrite;
}

int     FullSectorW         ( const int&   fd,
                                    int&   nextSector,
                                    int&   bytesLeft,
                                    char** buffer      )
{
    int   FullSectors = bytesLeft / (SECTOR_SIZE * g_LogicalSector);
    int   toWrite     = 0;
    int   freeSector  = -1;
    int   sectorsIO   = 0;
    
    for (register int i = 0; i < FullSectors; i++)
    {
        if ( (freeSector = SearchFreeSector()) == -1)
            return toWrite;
        
        if ( (sectorsIO = g_Device->m_Write(freeSector * g_LogicalSector, (void *) *buffer, g_LogicalSector)) != g_LogicalSector )
        {
            cout << "   FullSector(): na zarizeni se nevesly vsechny pozadovane bloky / chyba IO." << endl;
            cout << "   FullSector(): kazdopadne to je pruser jak svina, ptz to se nikdy nemelo stat" << endl;
            return toWrite + sectorsIO * SECTOR_SIZE;
        }
        
        g_Table_Fat[nextSector] = freeSector;
        cout << "   FullSector(): zapisuju do fat tabulky: na index " << nextSector << " davam hodnotu " << freeSector << endl;
        nextSector = freeSector;
        g_Table_Fat[nextSector] = LAST_FILE_SECTOR;
        cout << "   FullSector(): posledni sektor souboru " << g_Table_Fd[fd].m_File->m_FileName << " je " << nextSector << endl;
        toWrite += g_LogicalSector * SECTOR_SIZE;
        cout << "   FullSector(): zvedam toWrite o " << g_LogicalSector * SECTOR_SIZE << " na " << toWrite << endl;
        *buffer += g_LogicalSector * SECTOR_SIZE;
        g_Table_Fd[fd].m_File->m_RealSize += g_LogicalSector * SECTOR_SIZE;
        cout << "   FullSector(): zvedam realSize o " << g_LogicalSector * SECTOR_SIZE << " na " << g_Table_Fd[fd].m_File->m_RealSize << endl;
        bytesLeft -= g_LogicalSector * SECTOR_SIZE;
        cout << "   FullSector(): zmensuju bytesLeft o " << g_LogicalSector * SECTOR_SIZE << " na " << bytesLeft << endl;
    }
    
    cout << "   FullSector(): ending... " << endl;
    return toWrite;
}

int     LastSectorW         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            bytesLeft,
                                      char**          buffer        )
{
    int    toWrite    = 0;
    int    sectorsIO  = 0;
    int    freeSector = -1;
    char * criticalSector = new char[SECTOR_SIZE * g_LogicalSector];
    
    cout << "   LastSector(): posledni sektor souboru " << g_Table_Fd[fd].m_File->m_FileName << " by mel byt " << nextSector << endl;
    cout << "   LastSector(): a vypada to ze chceme najit volnej sektor a zapsat do nej " << bytesLeft << " bajtu " << endl;
    
    if ( (freeSector = SearchFreeSector()) == -1 )
    {
        cout << "   LastSector(): nedostatek mista na disku na zapsani zbytku souboru " << endl; 
        delete [] criticalSector;
        return toWrite;
    }
    
    if (bytesLeft >= SECTOR_SIZE * g_LogicalSector)
    {
        cout << "   LastSector(): spatna zbyvajici velikost k zapisu error " << endl; 
        delete [] criticalSector;
        return toWrite;
    }
    
    memset(criticalSector, 0, SECTOR_SIZE * g_LogicalSector);
    memcpy(criticalSector, *buffer, bytesLeft);
    
    cout << "   LastSector(): pamet pro sektor inicializovana a pripravena k zapisu " << endl;
    cout << "   LastSector(): sektor na ktery chceme zapisovat:  "<< freeSector << endl;
    cout << "   LastSector(): opravdova velikost souboru nyni je " << g_Table_Fd[fd].m_File->m_RealSize << endl;
    
    if ( (sectorsIO = g_Device->m_Write(freeSector * g_LogicalSector, criticalSector, g_LogicalSector)) != g_LogicalSector )
    {
        delete [] criticalSector;
        cout << "   LastSector(): nepodarilo se zapsat vsechny sektory! jako vzdycky je tohle pruser jako poleno! " << endl;
        cout << "   LastSector(): zapsano bylo " << sectorsIO << " FYZICKYCH sektoru" << endl;
        if (bytesLeft <= sectorsIO  * SECTOR_SIZE)
        {
            cout << "   LastSector(): ale pravdepodobne se tam vsechno podstatny jeste veslo " << endl;
            g_Table_Fat[nextSector] = freeSector;
            g_Table_Fat[freeSector] = LAST_FILE_SECTOR;
            
            g_Table_Fd[fd].m_File->m_RealSize += bytesLeft;
            toWrite += bytesLeft;
            *buffer += bytesLeft;
            bytesLeft = 0;
            return toWrite;
        }
        
        if (sectorsIO * SECTOR_SIZE >= 1)
        {
            cout << "   LastSector(): a neveslo se tam vsechno dulezity, ale veslo se tam aspon neco " << endl; 
            g_Table_Fat[nextSector] = freeSector;
            g_Table_Fat[freeSector] = LAST_FILE_SECTOR;
            g_Table_Fd[fd].m_File->m_RealSize += sectorsIO * SECTOR_SIZE;
            toWrite += sectorsIO * SECTOR_SIZE;
            *buffer += sectorsIO * SECTOR_SIZE;
            bytesLeft -= sectorsIO * SECTOR_SIZE;
            return toWrite;
        }
        
        cout << "   LastSector(): Tohle je uplne v prdeli, ptz se tam nevesel ani bajt, takze nic, to je taky totalni pruser " << endl;
        return toWrite;
    }
    
    delete [] criticalSector;
    g_Table_Fat[nextSector] = freeSector;
    g_Table_Fat[freeSector] = LAST_FILE_SECTOR;
            
    g_Table_Fd[fd].m_File->m_RealSize += bytesLeft;
    toWrite += bytesLeft;
    *buffer += bytesLeft;
    bytesLeft -= bytesLeft;
    
    cout << "   LastSector(): Vypada to, ze se nic nezvrtlo a sektor byl uspesne zapsan " << endl;
    cout << "   LastSector(): posledni sektor souboru po zapisu je: " << freeSector << endl;
    cout << "   LastSector(): velikost souboru po zapisu je: " << g_Table_Fd[fd].m_File->m_RealSize << endl;
    cout << "   LastSector(): zapsano bylo uspesne: " << toWrite << " bajtu a " << bytesLeft << " jich zbyva k zapisu " << endl;
    return toWrite;
}


int     FileRead            (         int             fd,
                                      void*           buffer,
                                      int             len           )
{
    int   bytesRead   = 0;
    int   toRead      = len;
    int   nextSector  = g_Table_Fd[fd].m_File->m_FirstSector;
    int   readSectors = g_Table_Fd[fd].m_HaveRead / (SECTOR_SIZE * g_LogicalSector);
    int   readOffset  = g_Table_Fd[fd].m_HaveRead % (SECTOR_SIZE * g_LogicalSector);
    int   skipSectors = readSectors;
    char* helpBuffer  = (char *) buffer; 
    
    if (fd < 0 || fd > 7 || g_Mounted != 1 || len <= 0 || g_Table_Fd[fd].m_File == NULL || g_Table_Fd[fd].m_Mode != 'R' || g_Table_Fd[fd].m_HaveRead >= g_Table_Fd[fd].m_File->m_RealSize) 
    {
        cout << "Neplatnej fd/Umounted/Nulova velikost/zavrenej soubor/spatnej mod error." << endl;
        return bytesRead;
    }
    
    if ( toRead > (g_Table_Fd[fd].m_File->m_RealSize - g_Table_Fd[fd].m_HaveRead) )
         toRead = g_Table_Fd[fd].m_File->m_RealSize - g_Table_Fd[fd].m_HaveRead;

    if (toRead <= 0)
        return bytesRead;
    
    if (g_Table_Fd[fd].m_HaveRead == 0)
        readOffset = SECTOR_SIZE * g_LogicalSector;
    
    if (readOffset == 0)
        skipSectors = readSectors - 1;
    
    for (register int i = 0; i < skipSectors; i++)
        nextSector = g_Table_Fat[nextSector];
    cout << "FileRead(): nextSector po cyklu - tzn sektor ze ktereho budu cist: " << nextSector << endl;
    
    if (nextSector == LAST_FILE_SECTOR)
    {
        cout << "FileRead(): chyba v preskakovani error" << endl;
        return bytesRead;
    }
    
    
    if (readOffset != 0)
        if ( (bytesRead = CriticalSectorR(fd, nextSector, toRead, &helpBuffer)) == 0 )
        {
            cout << "FileRead(): CriticalSectorR to posral a nic nenacetl " << endl;
            return bytesRead;
        }
    
    if (toRead <= 0)
        return bytesRead;
    
    if (toRead / (SECTOR_SIZE * g_LogicalSector) >= 1)
    {
        int temp = 0;
        
        if ( (temp = FullSectorR(fd, nextSector, toRead, &helpBuffer)) == 0 )
        {
            cout << "FileRead(): FullSectorR to posral " << endl;
            return bytesRead;
        }
        
        bytesRead += temp;
        
        if (toRead <= 0)
            return bytesRead;
    }
    
    int temp = 0;
    
    if ( (temp = LastSectorR(fd, nextSector, toRead, &helpBuffer)) == 0 )
    {
        cout << "FileRead(): LastSectorR to posral " << endl;
        return bytesRead;
    }
    
    bytesRead += temp;
    
    if (toRead != 0)
        cout << "FileRead(): jeste bych mel neco nacist ale nenactu, takze to je taky v prdeli " << endl;
    
    return bytesRead;
}


int     CriticalSectorR     ( const   int&            fd,
                              const   int&            sector,
                                      int&            readLeft,
                                      char**          buffer        )
{
    int    sectorsIO  = 0;
    int    toRead     = 0;
    int    readOffset = g_Table_Fd[fd].m_HaveRead % (SECTOR_SIZE * g_LogicalSector);
    char*  criticalSector = new char[ SECTOR_SIZE * g_LogicalSector ];
    char*  help = criticalSector;
    int    sectorValid = SECTOR_SIZE * g_LogicalSector;
    
    if (g_Table_Fd[fd].m_HaveRead != 0)
    {
        help += readOffset;
        sectorValid -= readOffset;
    }
    
    if (readLeft <= sectorValid)
        toRead = readLeft;
    else 
        toRead = sectorValid;
    
    if ((sectorsIO = g_Device->m_Read(sector * g_LogicalSector, criticalSector, g_LogicalSector)) != g_LogicalSector)
    {
        cout << "CriticalSectorR(): nepodarilo se sektor nacist, to je celkem v hajzlu zejo, to z nej proste nic neprectu " << endl;
        if ( (sectorsIO * SECTOR_SIZE) >= toRead )
        {
            cout << "CriticalSectorR(): ale asi se nacetlo dost na to abych nacetl co chci, takze nactu co muzu" << endl;
            memcpy(*buffer, help, toRead);
    
            g_Table_Fd[fd].m_HaveRead += toRead;
            *buffer += toRead;
            readLeft -= toRead;
            delete [] criticalSector;
    
            return toRead;
        
        }
        delete [] criticalSector;
        return 0; // tohle je na hovno ale nebudu na to zakladat promennou, proste sem nic nenacetl kdyz se nenahodil ten sektor
    }
    
    
    cout << "CriticalSectorR(): readLeft: " << readLeft << endl;
    cout << "CriticalSectorR(): sectorValid: " << sectorValid << endl;
    cout << "CriticalSectorR(): budu cist " << toRead << " bajtu ze sektoru " << sector << endl;
    
    memcpy(*buffer, help, toRead);
    
    g_Table_Fd[fd].m_HaveRead += toRead;
    *buffer += toRead;
    readLeft -= toRead;
    delete [] criticalSector;
    
    return toRead;
}


int     FullSectorR         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            readLeft,
                                      char**          buffer        )
{
    int  haveRead = 0;
    int  readSectors = readLeft / (SECTOR_SIZE * g_LogicalSector);
    int  sectorIO = 0;
    
    
    for (register int i = 0; i < readSectors; i++)
    {
        if (g_Table_Fat[nextSector] == LAST_FILE_SECTOR)
        {
            cout << "FullSectorR(): chyba ve vypoctech, sektor " << nextSector << " uz je posledni v souboru a ja chci nacitat jeste nejakej dalsi..." << endl;
            return haveRead;
        }
        
        nextSector = g_Table_Fat[nextSector];
        cout << "FullSectorR(): nacitam data ze sektoru " << nextSector << endl;
        
        if ((sectorIO = g_Device->m_Read(nextSector * g_LogicalSector, *buffer, g_LogicalSector)) != g_LogicalSector)
        {
            cout << "FullSectorR(): nacteni sektoru " << i << " se nepovedlo. " << endl;
            cout << "FullSectorR(): to je taky jeden z ukoncovacich pruseru " << endl;
            
            haveRead += sectorIO * SECTOR_SIZE;
            g_Table_Fd[fd].m_HaveRead += sectorIO * SECTOR_SIZE;
            *buffer += sectorIO * SECTOR_SIZE;
            readLeft -= sectorIO * SECTOR_SIZE;
            
            return haveRead + sectorIO * SECTOR_SIZE;
        }
        
        haveRead += g_LogicalSector * SECTOR_SIZE;
        g_Table_Fd[fd].m_HaveRead += g_LogicalSector * SECTOR_SIZE;
        *buffer += g_LogicalSector * SECTOR_SIZE;
        readLeft -= g_LogicalSector * SECTOR_SIZE;
    }

    return haveRead;
}

int     LastSectorR         ( const   int&            fd,
                                      int&            nextSector,
                                      int&            readLeft,
                                      char**          buffer        )
{
    int   sectorsIO = 0;
    int   haveRead = 0;
    char* criticalSector = new char[ SECTOR_SIZE * g_LogicalSector ];
    
    
    if (g_Table_Fat[nextSector] == LAST_FILE_SECTOR)
    {
        delete [] criticalSector;
        cout << "LastSectorR(): chyba, sektor kterej je uz prectenej byl posledni error" << endl;
        return haveRead;
    }
    
    if (readLeft >= g_LogicalSector * SECTOR_SIZE)
    {
        delete [] criticalSector;
        cout << "LastSectorR(): spatna velikost ke cteni error" << endl;
        return haveRead;
    }
    
    nextSector = g_Table_Fat[nextSector];
    
    if ( (sectorsIO = g_Device->m_Read(nextSector * g_LogicalSector, criticalSector, g_LogicalSector)) != g_LogicalSector )
    {
        cout << "LastSectorR(): nacteni v prdeli error "  << endl;
        if (sectorsIO * SECTOR_SIZE >= readLeft)
        {
            cout << "LastSectorR(): ale vsechno podstatny se nacetlo, peru to tam" << endl;
            memcpy(*buffer, criticalSector, readLeft);
            delete [] criticalSector;
            *buffer += readLeft;
            haveRead += readLeft;
            g_Table_Fd[fd].m_HaveRead += readLeft;
            readLeft -= readLeft;
            return haveRead;
        }
        delete [] criticalSector;
        return haveRead;
    }
    
    memcpy(*buffer, criticalSector, readLeft);
    
    delete [] criticalSector;
    
    *buffer += readLeft;
    haveRead += readLeft;
    g_Table_Fd[fd].m_HaveRead += readLeft;
    readLeft -= readLeft;
    return haveRead;
}


#endif