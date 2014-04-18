
#include "includes.h"

using namespace std;

#ifdef DEBUG

int     FileOpen            ( const char     * fileName,
                              int              writeMode )
{
//    Checking...... 
    if (g_Mounted != 1)
    {
        cout << "FileOpen(): nepripojeny disk error." << endl;
        return FILE_ERROR;
    }
        
//    Projedu Fd tabulku a kouknu se podle pointeru m_File, kterej by mel ukazovat v pripade plny
//    polozky na nejakou strukturu z tabulky jmen. Prazdna polozka v tabulce fd (volnej fd) je
//    signalizovana tim, ze ten pointer nikam neukazuje - NULL. Kdyz se mi nepodari najit zadnej
//    kterej ukazuje na NULL mam moc otevrenejch souboru a dalsi neberu. Dal to taky zkontroluje
//    jestli nahodou nechci otevrit soubor co ma vic znaku ve jmene nez je povoleno. 
    int fd = GetFd(fileName);
    
    if (fd == -1)
    {
        cout << "FileOpen(): moc otevrenych souboru / moc dlouhe jmeno error." << endl;
        return FILE_ERROR;
    }
    cout << "FileOpen(): filedeskriptor: " << fd << endl;
    
//    Kdyz jsem teda dosel k tomu, ze bude mozny soubor otevrit, potrebuju se podivat jestli uz v tabulce je
//    abych mohl rozhodovat o tom ostatnim. To udela SearchFileName - vrati mi index do tabulky jmen, na 
//    kterym drime soubor se stejnym jmenem jako se snazim otevrit a kdyz tam zadnej takovej soubor neni,
//    vrati -1. Proto tu -1 potom testuju zejo....
    
    int fileNamePos = SearchFileName(fileName);
    
    if (fileNamePos == -1 && writeMode == 0)
    {
      cout << "FileOpen(): soubor otevirany pro cteni neexistuje error. " << endl;
      return FILE_ERROR;
    }
       
    cout << "FileOpen(): pozice souboru v tabulce filename: " << fileNamePos << endl;
    
//    Nejdriv vyresim to nejjednodusi - tzn otevreni souboru pro cteni. To spociva v podstate 
//    jenom v nastaveni pointeru ve strukture TFd na odpovidajici prvek z tabulky jmen.
//    A samozrejmne si poznamenam, ze mam soubor otevrenej pro cteni, kdyby do nej chtel potom
//    nekdo zakerne psat....
    if (writeMode == 0)
    {
        cout << "  FileOpen(): soubor otevren pro cteni"  << endl;
        g_Table_Fd[fd].m_File = g_Table_FileName + fileNamePos;
        g_Table_Fd[fd].m_Mode = 'R';
        g_Table_Fd[fd].m_HaveRead = 0;        
        
        cout << "  FileOpen(): testovaci vypis obsahu jmena, velikosti a prvniho sektoru"  << endl;
        cout << "  FileOpen(): Filename: ";
        
        for (register int i = 0; i < FILENAME_LEN_MAX; i++)
            cout << g_Table_Fd[fd].m_File->m_FileName[i];
        cout << endl;
        cout << "  FileOpen(): Mode: " << g_Table_Fd[fd].m_Mode << endl;
        cout << "  FileOpen(): Realsize: " << g_Table_Fd[fd].m_File->m_RealSize << endl;
        cout << "  FileOpen(): FisrtSector: " << g_Table_Fd[fd].m_File->m_FirstSector << endl;
        cout << "  FileOpen(): ending... " << endl;
        return fd;
    }

//    Od nejjednodusiho k tezsimu, i kdyz je to furt pohadka oproti tomu co me ceka pri write...
//    Moznost kdy oteviram soubor pro cteni uz jsem poresil, takze ted uz chci kazdopadne otevrit
//    soubor pro zapis - a tady je pripad kdy v tabulce jmen tohle jmeno jeste neni, coz by celkem
//    nutne melo znamenat, ze chci takovej soubor vytvorit novej.
    if (fileNamePos == -1)
    {
        cout << "   FileOpen(): vytvareni noveho souboru se jmenem "  << fileName <<endl;
        int freeFile = SearchFreeFile();

//        Musim zkontrolovat, ze je v tabulce jmen jeste misto - tzn. ze mam na disku
//        min jak 128 souboru. To presne udela SearchFreeFile a kdyz vrati -1 tak je
//        to doufam jasny (gzzz, zadny misto v tabulce uz neni, moc souboru).
        if (freeFile == -1)
        {
             cout << "   FileOpen(): moc souboru v tabulce filename error."  << endl;
             return FILE_ERROR;
        }
        cout << "   FileOpen(): pozice pro novy soubor v tabulce filename: " << freeFile  << endl;    
        
        int freeSector = SearchFreeSector();
//        No a misto v tabulce souboru jeste neni uplne vyhra, ptz na kazdej soubor musi bejt aspon jeden
//        prazdnej sektor zejo, takze se pomoci SearchFreeSector kouknu do FAT tabulky a najdu nejakej
//        free sektor - kdyz neni, vraci mi to -1 a soubor nebude vytvoren.
        if (freeSector == -1)
        {
            cout << "   FileOpen(): disk plny error."  << endl;
            return FILE_ERROR;
        }
        cout << "   FileOpen(): prvni sektor noveho souboru: " << freeSector  << endl;
           
//        No a tady je ten pripad, kdy se to uspesne povede. Prvni sektor souboru
//        v FAT tabulce bude mit hodnotu LAST_FILE_SECTOR, ptz je zaroven i posledni.
//        A jinak se do tabulky jmen prida ze prvni sektor je ten kterej sem nasel
//        jako volnej tady o patro vys a ze opravdova velikost je 0, ptz v souboru
//        zatim nic neni. Vyplni se jmeno a pointer na polozku v tabulce jmen se 
//        nastavi do tabulky fd, aby se se souborem dalo pracovat. Psat komentare,
//        je hrozne vycerpavajici....
        
        int fileNameLen = strlen(fileName);
        g_Table_Fat[freeSector] = LAST_FILE_SECTOR;
        g_Table_FileName[freeFile].m_FirstSector = freeSector;
        g_Table_FileName[freeFile].m_RealSize = FREE_FILE;
        
        for (register int i = 0; i < fileNameLen; i++)
            g_Table_FileName[freeFile].m_FileName[i] = fileName[i];
        
        g_Table_Fd[fd].m_File = g_Table_FileName + freeFile;
        g_Table_Fd[fd].m_Mode = 'W';
        g_Table_Fd[fd].m_HaveRead = -1;
        
        cout << "   FileOpen(): ending... "  << endl;
        
        return fd;
    }
    
//    No a tohle je posledni pripad co muze nastat, oteviram soubor pro zapis a pritom 
//    uz jeho jmeno existuje v tabulce jmen, takze soubor uz je na disku -> chci mu smazat
//    vsechny jeho dosavadni sektory, nastavit zero size a priradit nejakej sektor volnej.
            
    cout << "   FileOpen(): Zkracovani souboru " << fileName << " na nulovou delku" << endl;
    DeleteFileSectors(fileNamePos);
    
    // Tohle bude krasny, ale bude to pouzitelny jenom v pripade pouzivani
    // SearchFileSectors. Kdy a jak to pouzit - viz SearchFileSectors.

//    int   fileSectorsNum = 0;    
//    int * fileSectors    = SearchFileSectors(fileNamePos, &fileSectorsNum);
//    
//    cout << "   FileOpen(): Indexy do fat tabulky - sektory ktere budou smazany: " << endl;
//    for (register int i = 0; i < fileSectorsNum; i++)
//        cout << fileSectors[i] << " " << endl;
//    
//    for (register int i = 0; i < fileSectorsNum; i++)
//        g_Table_Fat[ fileSectors[i] ] = FREE_SECTOR;
//    free ( fileSectors );
     
    int freeSector = SearchFreeSector();    
    if (freeSector == -1)
    {
        cout << "   FileOpen(): disk plny error."  << endl;
        return FILE_ERROR;
    }
    cout << "   FileOpen(): prvni sektor noveho souboru: " << freeSector  << endl;
        
    g_Table_Fat[freeSector] = LAST_FILE_SECTOR;
    g_Table_FileName[fileNamePos].m_FirstSector = freeSector;
    g_Table_FileName[fileNamePos].m_RealSize = FREE_FILE;
        
    g_Table_Fd[fd].m_File = g_Table_FileName + fileNamePos;
    g_Table_Fd[fd].m_Mode = 'W';
    g_Table_Fd[fd].m_HaveRead = -1;
    
    cout << "FileOpen(): ending...." << endl;
 
    return fd;
}

int     FileClose(int fd)
{
    if (g_Mounted != 1)
    {
        cout << "FileClose(): nepripojeny disk error." << endl;
        return FILE_ERROR;
    }
    
    if (fd < 0 || fd > 7)
        return FILE_ERROR;
    
    if (g_Table_Fd[fd].m_File != NULL)
    {
        g_Table_Fd[fd].m_File = NULL;
        g_Table_Fd[fd].m_Mode = EMPTY_CHAR;
        g_Table_Fd[fd].m_HaveRead = -1;
        return 0;
    }
    return FILE_ERROR;
}

int     GetFd(const char * fileName)
{
  int fd = -1;
  
  int FileNameLen = 0;
    if ( (FileNameLen = strlen(fileName)) > FILENAME_LEN_MAX )
        return fd;
  
    for (register int i = 0; i < OPEN_FILES_MAX; i++)
        if (g_Table_Fd[i].m_File == NULL)
        {
            fd = i;
            break;
        }
  
  return fd;
}

int     SearchFileName(const char* fileName)
{
//    Skvela funkce, ktera projede tabulku jmen a pri shode jmena co dostane jako 
//    parametr a nejakeho jmena z tabulky jmen vrati na jake pozici v tabulce jmen
//    tohle jmeno je. Kdyz tam jmeno neni, vrati -1;
    int fileNamePos = -1;
    
    char * s1 = new char[29];
    char * s2 = new char[29];
    strncpy(s2, fileName, 29);
    
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++)
    {
        strncpy(s1, g_Table_FileName[i].m_FileName, 29);
        if (strcmp(s1, s2) == 0)
        {
            fileNamePos = i;
            break;
        }
    
    }
    
    delete [] s1;
    delete [] s2;
    
    return fileNamePos;
}

int     SearchFreeFile()
{
//    Volny misto v tabulce jmen poznavam tak, ze polozka m_FirstSector se rovna
//    FREE_FILE, ptz zadnymu souboru nemuze byt nikdy jako cislo sektoru 
//    prirazeno FREE_FILE
    int freePos = -1;
    
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++)
        if ( g_Table_FileName[i].m_FirstSector == FREE_FILE )
        {
            freePos = i;
            break;
        }
    return freePos;
}

int     SearchFreeSector()
{
//    Oznacovani volnyho sektoru je ukradeny z opravdovy FATky, takze 
//    kdyz se nejaka hodnota v FAT tabulce rovna FREE_SECTOR tak oznacuje 
//    sektor, kterej je free.
    int logicalSectors = g_Device->m_Sectors / g_LogicalSector;
    
    for (register int i = 0; i < logicalSectors; i++)
    {
        if (g_FreeSector == logicalSectors) 
            g_FreeSector = 0;
        
        if (g_Table_Fat[g_FreeSector] == FREE_SECTOR)
            return g_FreeSector;
        
        g_FreeSector++;
    }
    cout << "SearchFreeSector(): Nenasel jsem zadnej volnej sektor! Co je do pice? " << endl;
    return FILE_ERROR;
}

//int *   SearchFileSectors(int fileNamePos, int* sectors_num)
//{
//    // Puvodni idea - vrati pole s indexama do fatky, indexy oznacujou 
//    // sektory na kterejch soubor je a soubor se ve fileopen zkrati - 
//    // na indexy se hodej nuly takze se sektory oznacej jako volny.
//    // ma to jedinou chybu, pri souboru pres vsechny sektory mi utece pamet
//    
//    int     sector = g_Table_FileName[fileNamePos].m_FirstSector;
//    int *   fileSectors = NULL;
//    
//    while (g_Table_Fat[sector] != LAST_FILE_SECTOR)
//    {
//        fileSectors = (int *) realloc(fileSectors, ++(*sectors_num));
//        fileSectors[ *sectors_num - 1] = sector;
//        sector = g_Table_Fat[sector];
//    }
//    
//    fileSectors = (int *) realloc(fileSectors, ++(*sectors_num));
//    fileSectors[ *sectors_num - 1] = sector;
//    
//    
//    // Problem s pretahnutim pameti de vyresit bud funkci DeleteFileSectors
//    // ktera proste zadny pole indexu nevraci, ale sektory rovnou odpali a
//    // oznaci jako prazdny - tohle reseni se ted pouziva.
//    
//    // Pravdepodobne inteligentnejsi je reseni, kdy se bude lip delit 
//    // disk na logicky sektory aby FAT nemela pres 256kb a pak muzou byt 
//    // v pameti klidne dve. Tohle reseni taky bude potreba pouzit v momente,
//    // kdy bude write/read nebo nejaka dalsi funkce napsana v budoucnu 
//    // tu funkci na nalezeni vsech sektoru souboru potrebovat. 
//    // Zmena velikosti logickyho sektoru viz SetLogicalSector.
//    
//    // OBROVSKEJ WARNING: FUNKCE NENI ODLADENA, PRESEL SEM NA DELETESECTORS
//    // A TOHLE UZ NEODLADIL, TAKZE ODLADIT JESTLI TO BUDE POTREBA POUZIT
//    
//   return fileSectors;
//}

void    DeleteFileSectors(const int& fileNamePos)
{
    // Funkce slouzi jako reseni nedostatku pameti pri delani truncate na soubor.
    // Ale krom toho proste smaze soubor kterej lezi v tabulce jmen na pozici fileNamePos.
    
    int     sector = g_Table_FileName[fileNamePos].m_FirstSector;
    cout << "DeleteFileSectors(): Prvni sektor souboru na pozici " << fileNamePos <<" je " << sector << endl; 
    
    while (g_Table_Fat[sector] != LAST_FILE_SECTOR)
    {
        int help = g_Table_Fat[sector];
        g_Table_Fat[sector] = FREE_SECTOR;
        cout << "DeleteFileSectors(): Mazani sektoru: " << sector << endl;
        sector = help;
    }
    cout << "DeleteFileSectors(): Posledni sektor souboru na pozici " << fileNamePos << " je " << sector << endl;
    g_Table_Fat[sector] = FREE_SECTOR;

    cout << "DeleteFileSectors(): ending... "<< endl;
    return;
}

#endif