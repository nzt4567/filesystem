#include "includes.h"

using namespace std;

#ifdef DEBUG

int           g_LogicalSector;
TBlkDev   *   g_Device;
int       *   g_Table_Fat;
TFileName *   g_Table_FileName;
TFd       *   g_Table_Fd;
bool          g_Mounted;
int           g_FreeSector;
int           g_LastFile;

int     FsCreate            ( struct TBlkDev * dev )
{
    if (dev == NULL)
        return FS_ERROR;
    
    SetLogicalSector(dev->m_Sectors);
    
    if (g_Mounted == 1)
        return FS_ERROR;
    
//    LogicalSectors - kolik logickych sektoru na disku budu adresovat, zaroven kolik jich tam 
//    a zaroven kolik je polozek FAT tabulky
    
//    FatSectors - na kolik sektoru se vejde FAT tabulka, je to zaokrouhleny nahoru aby to tam m_write mohl zapsat
    
//    FileNameSectors - viz FatSectors jenom jina tabulka

//    MagicSectors - MAGIC zabira konstantne jeden int, takze je uplne jedno jaka je velikost logickyho sektoru, na 1 se to proste vejde

//    Table_FileNameElems - kolik polozek do tabulky jmen zalokovat - sice platnejch je vzdycky 128 ptz nemuze byt vic souboru, ale 
//    kdyz ma logickej sektor napr 8kb tak ta tabulka zabere vic nez 128*sizeof(TFileName) a tohle misto je taky potreba zaalokovat, 
//    aby m_write necetl data pro zapis z pameti ktera neni moje

//    Table_FatElems - viz Table_FileNameElems, akorat pro FAT tabulku
    
    int    SectorsWritten       = 0;
    int    LogicalSectors       = dev->m_Sectors / g_LogicalSector;
    int    FatSectors           = Ceil( (double) (LogicalSectors * sizeof(int)) / (double) (SECTOR_SIZE * g_LogicalSector) );
    int    FileNameSectors      = Ceil( (double) (DIR_ENTRIES_MAX * sizeof(TFileName) / (double) (SECTOR_SIZE * g_LogicalSector) ));
    int    MagicSectors         = 1;
    int    Table_FileNamesElems = Ceil( (double) (FileNameSectors * SECTOR_SIZE * g_LogicalSector) / (double) sizeof(TFileName) );
    int    Table_FatElems       = Ceil( (double) (FatSectors      * SECTOR_SIZE * g_LogicalSector) / (double) sizeof(int) );
    
    cout << "FsCreate(): real sectors: " << dev->m_Sectors << endl;
    cout << "FsCreate(): logical sectors: " << LogicalSectors << endl;
    cout << "FsCreate(): fat sectors: " << FatSectors << endl;
    cout << "FsCreate(): filename sectors: " << FileNameSectors << endl;
    cout << "FsCreate(): magic sectors: " << MagicSectors << endl;
    cout << "FsCreate(): filename polozek: " << Table_FileNamesElems << endl;
    cout << "FsCreate(): fat polozek: " << Table_FatElems << endl;
    
    if (SetMagic(dev) != 1)
        return FS_ERROR;
    
    
//    Vyrobim a inicilizuju - memsetem pamet pro ty dve tabulky, ktery na disku budou 
//    inicializuju proto, ze m_Write zapisuje do zaokrouhleni na velikost logickyho sektoru 
//    no a zapisoval by neinicializovanou pamet tam co je jakoby jenom padding pro obe dve
//    tyhle tabulky - kdyz nevychazej na velikost logickyho sektoru - coz vetsinou nebudou,
//    nebo aspon v ostrym nasazeni ne. 
            
    TFileName * Table_FileNames  = new TFileName [ Table_FileNamesElems ];
    int       * Table_Fat        = new int       [ Table_FatElems ];
    
    memset(Table_FileNames, 0, Table_FileNamesElems  * sizeof(TFileName));
    memset(Table_Fat, 0, Table_FatElems * sizeof(int));
  
    
//    Pak se tabulky uvedou do vychoziho stavu, kdy je disk prazdnej az na ty sektory kde lezi
//    MAGIC, tabulka jmen souboru a fat tabulka.
    
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++) 
    {
        for (register int k = 0; k < FILENAME_LEN_MAX; k++)
            Table_FileNames[i].m_FileName[k] = EMPTY_CHAR;
        
        Table_FileNames[i].m_RealSize    = FREE_FILE;
        Table_FileNames[i].m_FirstSector = FREE_SECTOR;
    }
     
    int FullSectors = FatSectors + FileNameSectors + MagicSectors;
    for (register int i = 0; i < FullSectors; i++)
        Table_Fat[i] = LAST_FILE_SECTOR;
    
   
//    No a vychozi stav se z pameti zapise na prvnich X logickejch sektoru disku
//    Od toho MagicSectors * g_LogicalSector zapisuju ptz SetMagic od 0 zapsal MAGIC formuli,
//    takze kdybych to tam pral od 0 tak bych si ji smaznul a nebylo by poznat ze je disk
//    zformatovanej.
    
   if ( (SectorsWritten = dev->m_Write(MagicSectors * g_LogicalSector, Table_FileNames, FileNameSectors * g_LogicalSector)) != FileNameSectors * g_LogicalSector)
   {
       cout << "zapis filename error." << endl;
       delete [] Table_FileNames;
       delete [] Table_Fat;
       return FS_ERROR;
   }
    delete [] Table_FileNames;
    
    if ( (SectorsWritten = dev->m_Write(FileNameSectors * g_LogicalSector + MagicSectors * g_LogicalSector , Table_Fat, FatSectors * g_LogicalSector )) != FatSectors * g_LogicalSector )
   {
       cout << "zapis fat error." << endl;
       delete [] Table_Fat;
       return FS_ERROR;
   }
    delete [] Table_Fat;
    cout << "FsCreate(): ending.... " << endl << endl;
    return 1;
}

int     FsMount              ( struct TBlkDev* dev  )
{
    SetLogicalSector(dev->m_Sectors);
    
    if (g_Mounted == 1)
        return FS_ERROR;
    
    if ( CheckMagic(dev) != 1 )
        return FS_ERROR;
    
    int    SectorsRead          = 0;
    int    LogicalSectors       = dev->m_Sectors / g_LogicalSector;
    int    FatSectors           = Ceil( (double) (LogicalSectors * sizeof(int)) / (double) (SECTOR_SIZE * g_LogicalSector) );
    int    FileNameSectors      = Ceil( (double) (DIR_ENTRIES_MAX * sizeof(TFileName) / (double) (SECTOR_SIZE * g_LogicalSector) ));
    int    MagicSectors         = 1;
    int    Table_FileNamesElems = Ceil( (double) (FileNameSectors * SECTOR_SIZE * g_LogicalSector) / (double) sizeof(TFileName) );
    int    Table_FatElems       = Ceil( (double) (FatSectors      * SECTOR_SIZE * g_LogicalSector) / (double) sizeof(int) );
    
    cout << "FsMount(): real sectors: " << dev->m_Sectors << endl;
    cout << "FsMount(): logical sectors: " << LogicalSectors << endl;
    cout << "FsMount(): fat sectors: " << FatSectors << endl;
    cout << "FsMount(): filename sectors: " << FileNameSectors << endl;
    cout << "FsMount(): magic sectors: " << MagicSectors << endl;
    cout << "FsMount(): filename polozek: " << Table_FileNamesElems << endl;
    cout << "FsMount(): fat polozek: " << Table_FatElems << endl;
    
    TFileName * Table_FileNames  = new TFileName [ Table_FileNamesElems ];
    int       * Table_Fat        = new int       [ Table_FatElems ];   
    
//    Tak polozky az do ted maji stejnej vyznam jako ve formatu, jenom si tady do ty pameti ty tabulky z disku nacitam,
//    coz je logicky, kdyby mount formatoval bylo by vsechno porno fuc 
    
    
    if ( (SectorsRead = dev->m_Read(MagicSectors * g_LogicalSector, Table_FileNames, FileNameSectors * g_LogicalSector)) != FileNameSectors * g_LogicalSector)
   {
        cout << "nacitani filename error." << endl;
       delete [] Table_FileNames;
       delete [] Table_Fat;
       return FS_ERROR;
   }
    
    if ( (SectorsRead = dev->m_Read(FileNameSectors * g_LogicalSector + MagicSectors * g_LogicalSector, Table_Fat, FatSectors * g_LogicalSector )) != FatSectors * g_LogicalSector )
   {
       cout << "nacitani fat error." << endl; 
       delete [] Table_Fat;
       return FS_ERROR;
   }
    
//    Mount pro me udela jeste jednu moc peknou vec, a to tu ze vytvori tabulku filedescriptoru
//    Ta se mi bude hodit prakticky pri vsech operacich se souborama a tim ze ji vytvori uz 
//    mount se o to ve FileOpen nemusim zajimat. m_File je pointr na odpovidajici soubor v 
//    tabulce jmen souboru - kde je taky jeho prvni sektor a prava velikost. NULL tam davam,
//    abych nejak oznacil ze fd na tyhle pozici je volnej - tady sem totiz pouzil super tricek
//    a index do tyhle tabulky fd je zaroven cislo toho fd. Snad je jasny proc - sem super.
    
    TFd *   Table_Fd = new TFd[OPEN_FILES_MAX];
    
    for (register int i = 0; i < OPEN_FILES_MAX; i++)
    {
            Table_Fd[i].m_File = NULL;
            Table_Fd[i].m_Mode = EMPTY_CHAR; // jo a mode je jak je soubor otevrenej, R/W, tj doufam taky jasny
    }
    
//    Nastavim globalni promenny, ktery budou potrebovat ostatni funkce aby mohly nejak s tim diskem mluvit
//    a zaroven oznacim disk jako pripojenej, coz budu vsude v pozdejsich funkcich kontrolovat
    g_Device            = new TBlkDev();
    g_Device->m_Read    = dev->m_Read;
    g_Device->m_Write   = dev->m_Write;
    g_Device->m_Sectors = dev->m_Sectors;
    g_Table_Fat         = Table_Fat;
    g_Table_FileName    = Table_FileNames;
    g_Table_Fd          = Table_Fd;
    g_LastFile          = 0;
    g_FreeSector        = 0;
    g_Mounted           = 1;
    
    cout << "FsMount(): Filename tabulka: " << endl;
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++)
    {
        cout << "Polozka : " << dec << i << endl;
        cout << "   Filename: ";
        for (register int k = 0; k < FILENAME_LEN_MAX; k++)
            cout << hex << g_Table_FileName[i].m_FileName[k];
        cout << dec << endl;
        cout << "   Velikost: " << g_Table_FileName[i].m_RealSize << endl;
        cout << "   Prvni sektor: " <<  g_Table_FileName[i].m_FirstSector << endl;
    }
    
    cout << "FsMount(): Fat tabulka: " << endl; 
    for (register int i = 0; i < LogicalSectors; i++)
    {
        cout << "Polozka: " << dec <<  i << endl;
        cout << "   Hodnota: " << hex << g_Table_Fat[i] << endl;
    }
      
    cout << "FsMount(): FD tabulka: " << endl;
    for (register int i = 0; i < OPEN_FILES_MAX; i++)
    {
        cout << "Polozka: " << dec << i << endl;
        cout << "   Mod: " << g_Table_Fd[i].m_Mode << endl;
        cout << "   File: " << g_Table_Fd[i].m_File << endl;
        
    }
    cout << "FsMount(): ending...." << endl;
    return 1;
}

int     FsUmount             ( void                 )
{
//    Jako prvni, jak sem rikal, zkontroluju jestli nahodou neodpojuju nepripojenej disk
//    a hned potom jestli nahodou neni nenaformatovanej, ptz to by byl taky error
            
    if (g_Mounted != 1)
        return FS_ERROR;
    
    if (CheckMagic(g_Device) != 1 )
        return FS_ERROR;
    
    int    SectorsWritten  = 0;
    int    LogicalSectors  = Ceil( (double) g_Device->m_Sectors / (double) g_LogicalSector );
    int    FatSectors      = Ceil( (double) (LogicalSectors * sizeof(int)) / (double) (SECTOR_SIZE * g_LogicalSector) );
    int    FileNameSectors = Ceil( (double) (DIR_ENTRIES_MAX * sizeof(TFileName) / (double) (SECTOR_SIZE * g_LogicalSector) ));
    int    MagicSectors    = 1;
    
    cout << "FsUmount(): real sectors: " << g_Device->m_Sectors << endl;
    cout << "FsUmount(): logical sectors: " << LogicalSectors << endl;
    cout << "FsUmount(): fat sectors: " << FatSectors << endl;
    cout << "FsUmount(): filename sectors: " << FileNameSectors << endl;
    cout << "FsUmount(): magic sectors: " << MagicSectors << endl;
    
    
    // Provedu vypis Fd tabulky a uvolnim ji z pameti
    cout << "FsUmount(): FD tabulka: " << endl;
    for (register int i = 0; i < OPEN_FILES_MAX; i++)
    {
        cout << "Polozka: " << dec << i << endl;
        cout << "   Mod: " << g_Table_Fd[i].m_Mode << endl;
        cout << "   File: " << g_Table_Fd[i].m_File << endl;
        
    }
    delete [] g_Table_Fd;
    
    // To steny udelam s tabulkou jmen....
    cout << "FsUmount(): Filename tabulka: " << endl;
    for (register int i = 0; i < DIR_ENTRIES_MAX; i++)
    {
        cout << "Polozka : " << dec << i << endl;
        cout << "   Filename: ";
        for (register int k = 0; k < FILENAME_LEN_MAX; k++)
            cout << hex << g_Table_FileName[i].m_FileName[k];
        cout << dec << endl;
        cout << "   Velikost: " << g_Table_FileName[i].m_RealSize << endl;
        cout << "   Prvni sektor: " << g_Table_FileName[i].m_FirstSector << endl;
    }
    
    //... a tu jeste navic zapisu na disk, aby mi ty souburky hned nezmizely
    if ( (SectorsWritten = g_Device->m_Write(MagicSectors * g_LogicalSector, g_Table_FileName, FileNameSectors * g_LogicalSector)) != FileNameSectors * g_LogicalSector)
    {
       delete [] g_Table_FileName;
       delete [] g_Table_Fat;
       return FS_ERROR;
    }
    delete [] g_Table_FileName;
    
    // No a nakonec to provedu i s FAT tabulkou...
    cout << "FsUmount(): Fat tabulka: " << endl; 
    for (register int i = 0; i < LogicalSectors; i++)
    {
        cout << "Polozka: " << dec << i << endl;
        cout << "   Hodnota: " << hex << g_Table_Fat[i] << endl;
    }
    
    if ( (SectorsWritten = g_Device->m_Write(FileNameSectors * g_LogicalSector + MagicSectors * g_LogicalSector, g_Table_Fat, FatSectors * g_LogicalSector )) != FatSectors * g_LogicalSector )
    {
       delete [] g_Table_Fat;
       return FS_ERROR;
    }
    delete [] g_Table_Fat;
  
    // Uplne ve finishi uvolnim strukturu pro komunikaci s hdd a oznacim disk jako odpojenej. 
    // Od ted se neda volat zadna funkce na praci s diskem.
    delete g_Device;
    g_LogicalSector = 0;
    g_LastFile      = 0;
    g_FreeSector    = 0;
    g_Mounted = 0;
    cout << "FsUmount(): ending..." << endl;
    return 1;
}

void    SetLogicalSector    ( const int&             sectors )
{
//    Prostoduse vypocitam velikost logickyho sektoru na zaklade 
//    poctu skutecnejch sektoru. Tohle jeste v ostry verzi zmenim,
//    tak aby nebyla adresace jenom 8192B a 512B ale i vic, nemel
//    by to byt problem.
    if (sectors >= FAT_TAB_SECTORS_MAX)
         g_LogicalSector = SECTOR_SIZE_8192B;
    else g_LogicalSector = SECTOR_SIZE_512B;
       
    return;
}

int     Ceil                ( const double&           number  )
{
    // Pouzil bych ceil z math.h, ale zadny takovy, napis si sam... 
    int help = number;
    
    if (help == number)
        return number;
    
    return help+1;
}

int     SetMagic            ( const struct TBlkDev* dev )
{
//    Dulezita vec pri tvorbe fs je, oznacit nejak disk jako naformatovanej, 
//    abych treba mountem nezacal jentak cist nejakej bordel a nechtel s tim
//    pracovat. Od toho je MAGIC....
    int    SectorsWritten = 0;
    int    MagicElems     = Ceil( (double) (SECTOR_SIZE * g_LogicalSector) / (double) sizeof(int) );
    int *  Magic          = new int [ MagicElems ];
    
    memset(Magic, 0, MagicElems * sizeof(int));
    *Magic = MAGIC;
    
    if ( (SectorsWritten = dev->m_Write(0, Magic, g_LogicalSector)) != g_LogicalSector )
    {
        cout << "zapis magic error." << endl;
        delete [] Magic;
        return FS_ERROR;
    }
    delete [] Magic;
    return 1;
}

int     CheckMagic          ( const struct TBlkDev* dev )
{
//    Bla bla, proste to zkontroluju jestli je ten disk zformatovanej, jasny ne?
    int    SectorsRead  = 0;
    int    MagicElems   = Ceil( (double) (SECTOR_SIZE * g_LogicalSector) / (double) sizeof(int) );
    int *  Magic        = new int [ MagicElems ];

    
    if ( (SectorsRead = dev->m_Read(0, Magic, g_LogicalSector)) != g_LogicalSector )
    {
        cout << "cteni magic error." << endl;
        delete [] Magic;
        return FS_ERROR;
    }
    
    if (*Magic != MAGIC)
    {
        cout << "neformatovany disk error." << endl;
        delete [] Magic;
        return FS_ERROR;
    }
    delete [] Magic;
    return 1;
}

#endif