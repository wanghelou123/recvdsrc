#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <cerrno>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "IniFile.h"

#ifdef _MSC_VER
#pragma   warning(disable:4996)
#endif

#define INI_SEC_BEGIN   '['
#define INI_SEC_END     ']'
#define INI_COMMENT     '#'
#define INI_FS          '='
#define INI_SPACE       0x20
#define EMPTYSTR        "";

#define INI_SECTION     'S' /*SECTION LINE FLAG*/
#define INI_VALUE       'V' /*VALUE LINE FALG*/
#define INI_COMM        'C' /*COMMENT LINE FLAG*/
#define INI_EMPTYLINE   'E' /*EMPYT LINE FLAG*/
#define INI_OTHER       'O' /*UNKNOWN LINE FLAG*/

/*
#define INI_W           2
#define INI_NEED_SECTION    1
#define INI_NONEED_SECTION  0
#define INI_EXISTS   0
#define INI_NO_SEC   1
#define INI_NO_NAME   2
*/
#ifndef _MSC_VER
#ifndef _STRICMP

static int stricmp(const char *cs, const char *ct)
{
    register signed char __res;

    while (1) {
        if ((__res = tolower(*cs) - tolower(*ct++)) != 0 || !*cs++)
            break;
    }

    return __res;
}

#endif
#endif

//string compare
//this function extends the strcmp that can ignore case
static int strcmpx(const char *s1, const char *s2, int ignorecase)
{
    if (!ignorecase)
        return strcmp(s1, s2);

    else
        return stricmp(s1, s2);
}

//check if the file exists
static int FileExists(char *FullFileName)
{
    struct stat buf;
    int FileSize = 0;

    if (stat(FullFileName, &buf) == 0) {
        FileSize = buf.st_size;
        return FileSize;

    } else {
        if (errno == ENOENT)
            return -1;
    }

    return -2;
}

static char *trimleft(char *inistr, char ch)
{
    while ((*inistr == ch) || (*inistr == '\t'))
        inistr++;

    //while(*inistr == ch) inistr++;
    return inistr;
}

static char *trimright(char *inistr, char ch)
{
    int ilen = 0;
    static char *_tpstr = NULL;
    _tpstr = strdup(inistr);
    ilen = (int)strlen(_tpstr);

    while ((_tpstr[ ilen - 1 ] == ch) || (_tpstr[ ilen - 1 ] == '\t'))
        ilen--;

    //while(_tpstr[ilen-1] == ch) ilen--;
    _tpstr[ ilen ] = 0;
    return _tpstr;
}

static char *trim(char *inistr, char ch)
{
    return trimleft(trimright(inistr, ch), ch);
}

static int pos_char(char *str, char ch, int start_index)
{
    int k = start_index;
    int iFound = 0;

    while (str[ k ] != '\0') {
        if (str[ k ] == ch) {
            iFound = 1;
            break;
        }

        k++;
    }

    if (iFound)
        return k;

    else
        return -1;
}

//if the string is a valide configure value
static int isconfig(char *inistr)
{
    if (*inistr == '\n')
        return 0;

    if (*inistr == 0)
        return 0;

    if (strlen(trimleft(inistr, INI_SPACE)) == 0)
        return 0;

    return 1;
}

//if the string is a comment line
static int iscommline(char *inistr)
{
    if (isconfig(inistr)) {
        if (inistr[ 0 ] == INI_COMMENT)
            return 1;

        else
            return 0;
    }

    return 0;
}

//if the string  is a section
static int issection(char *inistr)
{
    char *tmp = trim(inistr, INI_SPACE);

    if ((tmp[ 0 ] == INI_SEC_BEGIN) && (tmp[ strlen(tmp) - 1 ] == INI_SEC_END))
        return 1;

    else
        return 0;
}

//if the string  is a section
static char *issection2(char *inistr)
{
    char *tmp = trim(inistr, INI_SPACE);

    if ((tmp[ 0 ] == INI_SEC_BEGIN) && (tmp[ strlen(tmp) - 1 ] == INI_SEC_END)) {
        tmp++;
        tmp[ strlen(tmp) - 1 ] = 0;
        return tmp;

    } else
        return NULL;
}

//if the string  is a value
static int isvalueline(char *inistr)
{
    char *tmp = trim(inistr, INI_SPACE);

    //if ((!iscommline(inistr))&&(!issection(inistr))&&(*inistr != INI_FS))
    if ((!iscommline(inistr)) && (!issection(inistr)) && (*tmp != INI_FS))
        return pos_char(inistr, INI_FS, 0);

    else
        return -1;
}

//delete the comment of the string line
//static char *delcomment(char *inistr)
//{
//    static char *_commstr = NULL;
//    _commstr = strdup(inistr);
//    int commpos = pos_char(_commstr, INI_COMMENT, 0);
//
//    if (commpos != -1) {
//        _commstr[ commpos ] = 0;
//    }
//
//    return _commstr;
//}

//parse the string and get the name and value
//static void _strparse2(char *inistr, char *ininame, char *value)
//{
//    char tmpname[ INI_BUFFER + 1 ];
//    int iFSPos = isvalueline(inistr);
//    char *_tmp = NULL;
//
//    if (iFSPos != -1) {
//        memset(tmpname, 0, sizeof(tmpname));
//        memcpy(tmpname, inistr, iFSPos);
//        strcpy(tmpname, trim(tmpname, INI_SPACE));
//        strcpy(ininame, tmpname);
//        inistr += (iFSPos + 1);
//        _tmp = delcomment(inistr);
//        strcpy(value, trim(_tmp, INI_SPACE));
//
//    } else {
//        ininame = (char *) NULL;
//        value = (char *) NULL;
//    }
//}

static void _strparse(char *inistr, char *ininame, char *value)
{
    char tmpname[ INI_BUFFER + 1 ];
    int iFSPos = isvalueline(inistr);

    if (iFSPos != -1) {
        memset(tmpname, 0, sizeof(tmpname));
        memcpy(tmpname, inistr, iFSPos);
        strcpy(tmpname, trim(tmpname, INI_SPACE));
        strcpy(ininame, tmpname);
        inistr += (iFSPos + 1);
        strcpy(value, trim(inistr, INI_SPACE));

    } else {
        ininame = (char *) NULL;
        value = (char *) NULL;
    }
}

//do initialize
void IniFile::IniInit()
{
    Head = NULL;
    Tail = NULL;
    Head = new(TSectionList);
    Head->Name = NULL;
    Head->ValueList = NULL;
    Head->ValueTail = NULL;
    Head->Next = NULL;
    Tail = Head;
    FIgnoreCaseValue = 1;
    FIgnoreCaseName = 1;
    FileWriteOK = 0;
    FileModified = 0;
    FAutoLoad = 0;
    FIReadOnly = 0;
    memset(FCFileName, 0, sizeof(FCFileName));
}

IniFile::IniFile(const char *AFileName, const int AReadOnly)
{
    IniInit();
    memcpy(FCFileName, AFileName, FILENAME_LEN);
    FIReadOnly = AReadOnly;
    LoadFromFile();
}

IniFile::~IniFile()
{
    //SaveToFile();
    DeleteAll();
}

void IniFile::SetIgnoreCaseValue(int AValue)       //Ignore case?
{
    if (FIgnoreCaseValue != AValue)
        FIgnoreCaseValue = AValue;
}

void IniFile::SetIgnoreCaseName(int AValue)       //Ignore case?
{
    if (FIgnoreCaseName != AValue)
        FIgnoreCaseName = AValue;
}

TSectionList *IniFile::AddSection(const char *ASectionName, const char AFlag)
{
    if ((!FAutoLoad) && (FIReadOnly))
        return NULL;

    int iLen = (int)strlen(ASectionName);
    TSectionList *newp = new(TSectionList);
    newp->Flag = AFlag;
    newp->Name = new char[ iLen + 1 ];
    memset(newp->Name, 0, iLen + 1);
    memcpy(newp->Name, ASectionName, iLen);
    newp->ValueList = NULL;
    newp->ValueTail = NULL;
    newp->Next = NULL;
    Tail->Next = newp;
    Tail = newp;

    if (!FAutoLoad)
        FileModified = 1;

    return Tail;
}

TSectionList *IniFile::GetSection(const char *ASectionName)
{
    TSectionList *_temp = Head->Next;
    int iFound = 0;

    while (_temp != NULL) {
        if ((_temp->Flag == INI_SECTION) && (strcmpx(ASectionName, _temp->Name, FIgnoreCaseName) == 0)) {
            iFound = 1;
            break;
        }

        _temp = _temp->Next;
    }

    if (iFound)
        return _temp;

    else
        return NULL;
}

TValueList *IniFile::ValueExists(TSectionList *ASection, const char *AName)
{
    TValueList *_Value = ASection->ValueList;
    int iFound = 0;

    while (_Value != NULL) {
        if ((_Value->Flag == INI_VALUE) && (strcmpx(AName, _Value->Name, FIgnoreCaseValue) == 0)) {
            iFound = 1;
            break;
        }

        _Value = _Value->Next;
    }

    if (iFound)
        return _Value;

    else
        return NULL;
}

void IniFile::AddValue(const char *ASectionName, const char *AName, const char *AValue, const char AFlag)
{
    if ((!FAutoLoad) && (FIReadOnly))
        return ;

    TValueList *_Value = NULL;
    TSectionList *_Section = GetSection(ASectionName);

    if (_Section == NULL) {
        _Section = AddSection(ASectionName, INI_SECTION);
    }

    _Value = ValueExists(_Section, AName);

    if (_Value == NULL) {
        if (!FAutoLoad)
            FileModified = 1;

        _Value = new(TValueList);
        _Value->Next = NULL;
        _Value->Name = NULL;
        _Value->Value = NULL;
        _Value->Flag = AFlag;
        int iNameLen = (int)strlen(AName);

        if (iNameLen > 0) {
            _Value->Name = new char[ iNameLen + 1 ];
            memset(_Value->Name, 0, iNameLen + 1);
            memcpy(_Value->Name, AName, iNameLen);
        }

        int iValueLen = (int)strlen(AValue);

        if (iValueLen > 0) {
            _Value->Value = new char[ iValueLen + 1 ];
            memset(_Value->Value, 0, iValueLen + 1);
            memcpy(_Value->Value, AValue, iValueLen);
        }

        if (_Section->ValueList == NULL) {
            _Section->ValueList = _Value;
            _Section->ValueTail = _Value;

        } else {
            _Section->ValueTail->Next = _Value;
            _Section->ValueTail = _Value;
        }

    } else {
        int iValueLen = (int)strlen(AValue);

        if (_Value->Value != NULL) {
            //if (strcmpx(AValue,_Value->Value,FIgnoreCase) == 0) return;
            if (strcmpx(AValue, _Value->Value, 0) == 0)
                return ; //Must ignore the case of value

            delete[] _Value->Value;
            _Value->Value = NULL;
        }

        if (!FAutoLoad)
            FileModified = 1;

        if (iValueLen > 0) {
            _Value->Value = new char[ iValueLen + 1 ];
            memset(_Value->Value, 0, iValueLen + 1);
            memcpy(_Value->Value, AValue, iValueLen);
        }
    }
}

void IniFile::DeleteSection(TSectionList *ASection)
{
    if (ASection == NULL)
        return ;

    if (ASection->Name != NULL) {
        delete[] ASection->Name;
        ASection->Name = NULL;
    }

    //ASection->ValueList = NULL;
    //ASection->ValueTail = NULL;
    //ASection->Next = NULL;
    delete ASection;
    ASection = NULL;
}

void IniFile::DeleteValue(TValueList *AValue)
{
    if (AValue == NULL)
        return ;

    if (AValue->Name != NULL) {
        delete[] AValue->Name;
        AValue->Name = NULL;
    }

    if (AValue->Value != NULL) {
        delete[] AValue->Value;
        AValue->Value = NULL;
    }

    delete AValue;
    AValue = NULL;
}

void IniFile::DeleteSectionValues(TSectionList *ASection)
{
    TValueList *_temp = ASection->ValueList;
    //if (_temp == NULL) return;
    //TValueList* p = _temp->Next;
    int I = 0;

    while (_temp != NULL) {
        TValueList *p = _temp;
        ASection->ValueList = _temp->Next;
        DeleteValue(p);
        _temp = ASection->ValueList;
        I++;
    }

    ASection->ValueList = NULL;
    ASection->ValueTail = NULL;
}

void IniFile::DeleteAll()
{
    TSectionList *_temp = Head->Next;

    while (_temp != NULL) {
        TSectionList *p = _temp;

        if (p->ValueList == NULL) {
            Head->Next = p->Next;
            DeleteSection(p);

        } else {
            DeleteSectionValues(p);
            Head->Next = p->Next;
            DeleteSection(p);
        }

        _temp = Head->Next;
    }

    delete Head;
}

//parse the string:called by LoadFromFile
void IniFile::IniParse(char *ABuffer, char *AFlag, char *AName, char *AValue)
{
    int iLen = (int)strlen(ABuffer);

    if (iscommline(ABuffer)) {
        AFlag[ 0 ] = INI_COMM;
        memcpy(AName, ABuffer, (iLen > INI_BUFFER ? INI_BUFFER : iLen));
        AValue = (char *) NULL;
        return ;
    }

    char *tmp = issection2(ABuffer);

    if (tmp != NULL) {
        AFlag[ 0 ] = INI_SECTION;
        memcpy(AName, tmp, (iLen > INI_BUFFER ? INI_BUFFER : iLen));
        AValue = (char *) NULL;
        return ;
    }

    if (isvalueline(ABuffer) != -1) {
        AFlag[ 0 ] = INI_VALUE;
        _strparse(ABuffer, AName, AValue);
        return ;
    }

    if (isconfig(ABuffer)) {
        AFlag[ 0 ] = INI_OTHER;

        if (iLen > 0)
            memcpy(AName, ABuffer, (iLen > INI_BUFFER ? INI_BUFFER : iLen));

        else
            AName = (char *) NULL;

        AValue = (char *) NULL;
        return ;
    }

    AFlag[ 0 ] = INI_EMPTYLINE;

    if (iLen > 0)
        memcpy(AName, ABuffer, (iLen > INI_BUFFER ? INI_BUFFER : iLen));

    else
        AName = (char *) NULL;

    AValue = (char *) NULL;
    return ;
}

void IniFile::LoadFromFile()     //Load INI file into memory
{
    FILE *inifp;
    char buffer[ INI_BUFFER + 1 ];
    char tName[ INI_BUFFER + 1 ];
    char tValue[ INI_BUFFER + 1 ];
    char tCurSection[ INI_BUFFER + 1 ];
    char tFlag[ 2 ];
    char cFlag;
    int iTmp = 0;

    if (FileExists(FCFileName) == -1) {
        inifp = fopen(FCFileName, "w");    //Create the file when it is not exists!

        if (inifp == NULL) {
            // plog->addlog( "Error Open File To Write!\n" );
            perror("Error Open File To Write!\n");
            return ;
        }

        fclose(inifp);
    }

    inifp = fopen(FCFileName, "r");

    if (inifp == NULL) {
        // plog->addlog( "Error Open File!\n" );
        perror("Error Open File!\n");
        return ;
    }

    int iSection = 0;
    FAutoLoad = 1;

    while (fgets(buffer, INI_BUFFER, inifp) != NULL) {
        iTmp = (int)strlen(buffer);

        if (buffer[ iTmp - 1 ] == '\n')
            buffer[ iTmp - 1 ] = 0;

        memset(tName, 0, INI_BUFFER + 1);
        memset(tValue, 0, INI_BUFFER + 1);
        memset(tFlag, 0, 2);
        IniParse(buffer, tFlag, tName, tValue);
        cFlag = tFlag[ 0 ];

        if ((FIReadOnly) && ((cFlag != INI_SECTION) && (cFlag != INI_VALUE)))
            continue;

        if (cFlag == INI_SECTION) {
            memset(tCurSection, 0, sizeof(tCurSection));
            memcpy(tCurSection, tName, strlen(tName));
            iSection = 1;

        } else {
            if (!iSection) {
                AddSection(buffer, cFlag);

            } else {
                AddValue(tCurSection, tName, tValue, cFlag);
            }
        }
    }

    FAutoLoad = 0;
    fclose(inifp);
}

char *IniFile::GetValue(const char *ASectionName, const char *AName)
{
    TSectionList *_Section = GetSection(ASectionName);

    if (_Section == NULL) {
        //printf("[%s] not exist\n", ASectionName);
        return NULL;
    }

    TValueList *_Value = ValueExists(_Section, AName);

    //printf("Value=[%s]\n", _Value->Value);
    if (_Value != NULL)
        return _Value->Value;

    else
        return NULL;
}

char *IniFile::ReadValue(const char *ASection, const char *AName, const char *AValue)
{
    char *_tmpValue = NULL;
    _tmpValue = GetValue(ASection, AName);

    if (_tmpValue != NULL)
        return _tmpValue;

    else
        return (char *) AValue;
}

int IniFile::ReadValue(const char *ASection, const char *AName, const int AValue)
{
    char tValue[ INI_BUFFER + 1 ];
    char *pValue = NULL;
    int iValue;
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%d", AValue);
    pValue = ReadValue(ASection, AName, tValue);
    iValue = atoi(pValue);
    return iValue;
}

double IniFile::ReadValue(const char *ASection, const char *AName, const double AValue)
{
    char tValue[ INI_BUFFER + 1 ];
    char *pValue = NULL;
    double iValue;
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%.2f", AValue);
    pValue = ReadValue(ASection, AName, tValue);
    iValue = atof(pValue);
    return iValue;
}

char IniFile::ReadValue(const char *ASection, const char *AName, const char AValue)
{
    char tValue[ INI_BUFFER + 1 ];
    char *pValue = NULL;
    char iValue;
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%c", AValue);
    pValue = ReadValue(ASection, AName, tValue);
    iValue = pValue[ 0 ];
    return iValue;
}

char *IniFile::ReadString(const char *ASection, const char *AName, const char *AValue)
{
    return ReadValue(ASection, AName, AValue);
}

int IniFile::ReadInteger(const char *ASection, const char *AName, const int AValue)
{
    return ReadValue(ASection, AName, AValue);
}

double IniFile::ReadDouble(const char *ASection, const char *AName, const double AValue)
{
    //double tValue = AValue;
    return ReadValue(ASection, AName, AValue);
}

char IniFile::ReadChar(const char *ASection, const char *AName, const char AValue)
{
    return ReadValue(ASection, AName, AValue);
}

void IniFile::WriteValue(const char *ASection, const char *AName, const char *AValue)
{
    AddValue(ASection, AName, AValue, INI_VALUE);
}

void IniFile::WriteValue(const char *ASection, const char *AName, const int AValue)
{
    char tValue[ 24 + 1 ];
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%d", AValue);
    WriteValue(ASection, AName, tValue);
}

void IniFile::WriteValue(const char *ASection, const char *AName, const double AValue)
{
    char tValue[ 24 + 1 ];
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%.2f", AValue);
    WriteValue(ASection, AName, tValue);
}

void IniFile::WriteValue(const char *ASection, const char *AName, const char AValue)
{
    char tValue[ 1 + 1 ];
    memset(tValue, 0, sizeof(tValue));
    sprintf(tValue, "%c", AValue);
    WriteValue(ASection, AName, tValue);
}

void IniFile::WriteString(const char *ASection, const char *AName, const char *AValue)
{
    WriteValue(ASection, AName, AValue);
}

void IniFile::WriteInteger(const char *ASection, const char *AName, const int AValue)
{
    WriteValue(ASection, AName, AValue);
}

void IniFile::WriteDouble(const char *ASection, const char *AName, const double AValue)
{
    //double tValue = AValue;
    WriteValue(ASection, AName, AValue);
}

void IniFile::WriteChar(const char *ASection, const char *AName, const char AValue)
{
    WriteValue(ASection, AName, AValue);
}


void IniFile::ShowSection(TSectionList *ASection)
{
    if (ASection == NULL)
        return ;

    if (ASection->Name != NULL) {
        if (ASection->Flag == INI_SECTION)
            // plog->addlog( "[%s]\n", ASection->Name );
            printf("[%s]\n", ASection->Name);

        else
            // plog->addlog( "%s\n", ASection->Name );
            printf("%s\n", ASection->Name);
    }
}

int IniFile::ShowSectionValues(TSectionList *ASection)
{
    TValueList *_Value = ASection->ValueList;
    int iEmptyLine = 0;

    while (_Value != NULL) {
        if (_Value->Flag == INI_VALUE) {
            if (_Value->Name != NULL)
                // plog->addlog( "%s=", _Value->Name );
                printf("%s=", _Value->Name);

            if (_Value->Value != NULL)
                // plog->addlog( "%s\n", _Value->Value );
                printf("%s\n", _Value->Value);

            else
                printf("\n");

        } else {
            if (_Value->Name != NULL)
                // plog->addlog( "%s\n", _Value->Name );
                printf("%s\n", _Value->Name);

            else
                // plog->addlog( "\n" );
                printf("\n");
        }

        if (_Value->Flag == INI_EMPTYLINE)
            iEmptyLine = 1;

        else
            iEmptyLine = 0;

        _Value = _Value->Next;
    }

    return iEmptyLine;
}

void IniFile::ShowAll()
{
    TSectionList *_temp = Head->Next;
    int iEmptyLine = 0;

    while (_temp != NULL) {
        iEmptyLine = 0;
        TSectionList *p = _temp;
        ShowSection(p);

        if (p->ValueList != NULL) {
            iEmptyLine = ShowSectionValues(p);
        }

        if (p->Flag == INI_SECTION)
            if (!iEmptyLine)
                // plog->addlog( "\n" );
                printf("\n");

        _temp = _temp->Next;
    }
}

void IniFile::SaveSection(TSectionList *ASection, FILE *fp)
{
    if (ASection == NULL)
        return ;

    if (ASection->Name != NULL) {
        if (ASection->Flag == INI_SECTION)
            fprintf(fp, "[%s]\n", ASection->Name);

        else
            fprintf(fp, "%s\n", ASection->Name);
    }
}

int IniFile::SaveSectionValues(TSectionList *ASection, FILE *fp)
{
    TValueList *_Value = ASection->ValueList;
    int iEmptyLine = 0;

    while (_Value != NULL) {
        if (_Value->Flag == INI_VALUE) {
            if (_Value->Name != NULL)
                fprintf(fp, "%s=", _Value->Name);

            if (_Value->Value != NULL)
                fprintf(fp, "%s\n", _Value->Value);

            else
                fprintf(fp, "\n");

        } else {
            if (_Value->Name != NULL)
                fprintf(fp, "%s\n", _Value->Name);

            else
                fprintf(fp, "\n");
        }

        if (_Value->Flag == INI_EMPTYLINE)
            iEmptyLine = 1;

        else
            iEmptyLine = 0;

        _Value = _Value->Next;
    }

    return iEmptyLine;
}

void IniFile::SaveToFile(const char *AFileName)
{
    FILE *inifp = fopen(AFileName, "w");

    if (inifp == NULL) {
        // plog->addlog( "Write File Failed!\n" );
        printf("Write File Failed!\n");
        return ;
    }

    FileWriteOK = 0;
    TSectionList *_temp = Head->Next;
    int iEmptyLine = 0;

    while (_temp != NULL) {
        iEmptyLine = 0;
        TSectionList *p = _temp;
        SaveSection(p, inifp);

        if (p->ValueList != NULL) {
            iEmptyLine = SaveSectionValues(p, inifp);
        }

        if ((p->Flag == INI_SECTION) && (p != Tail))
            if (!iEmptyLine)
                fprintf(inifp, "\n");

        _temp = _temp->Next;
    }

    fclose(inifp);
    FileWriteOK = 1;

    if (strcmp(FCFileName, AFileName) == 0)
        FileModified = 0;

    return ;
}

void IniFile::SaveToFile()
{
    char tFileName[ FILENAME_LEN + 10 ];
    int BackupOK = 1;

    if (!FileModified)
        return ;

    memset(tFileName, 0, sizeof(tFileName));
    strcpy(tFileName, FCFileName);
    strcat(tFileName, ".TMP~~");
    unlink(tFileName);

    if (rename(FCFileName, tFileName) != 0) {
        // plog->addlog( "Backup File Failed!\n" );
        printf("Backup File Failed!\n");
        BackupOK = 0;
    }

    SaveToFile(FCFileName);

    if (!FileWriteOK) {
        if (BackupOK)
            rename(tFileName, FCFileName);

        else
            // plog->addlog( "Restore File Failed!\n" );
            printf("Restore File Failed!\n");

    } else {
        unlink(tFileName);
    }
}

void IniFile::UpdateFile()
{
    SaveToFile();
}

void IniFile::EraseSection(const char *ASection)
{
    TSectionList *_temp = Head;
    TSectionList *p = _temp->Next;
    int iFound = 0;

    while (p != NULL) {
        if ((p->Flag == INI_SECTION) && (strcmpx(ASection, p->Name, FIgnoreCaseName) == 0)) {
            iFound = 1;
            break;
        }

        _temp = p;
        p = _temp->Next;
    }

    if (iFound) {
        _temp->Next = p->Next;
        DeleteSectionValues(p);
        DeleteSection(p);
        FileModified = 1;
    }
}

void IniFile::DeleteKey(const char *ASection, const char *AName)
{
    TSectionList *_Section = GetSection(ASection);

    if (_Section == NULL)
        return ;

    TValueList *_Value = _Section->ValueList;

    if (_Value == NULL)
        return ;

    TValueList *p = NULL;
    p = _Value->Next;

    if ((_Value->Flag == INI_VALUE) && (strcmpx(AName, _Value->Name, FIgnoreCaseValue) == 0)) {
        _Section->ValueList = _Value->Next;
        DeleteValue(_Value);
        FileModified = 1;
        return ;
    }

    int iFound = 0;

    while (p != NULL) {
        if ((p->Flag == INI_VALUE) && (strcmpx(AName, p->Name, FIgnoreCaseValue) == 0)) {
            iFound = 1;
            break;
        }

        _Value = p;
        p = _Value->Next;
    }

    if (iFound) {
        _Value->Next = p->Next;
        DeleteValue(p);
        FileModified = 1;
    }
}

