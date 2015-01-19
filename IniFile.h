#ifndef INIFILE_H__
#define INIFILE_H__
//#include "Util.h"


#define INI_BUFFER          4096  /*max bytes of each line*/
#define FILENAME_LEN        256   /*max bytes of file name*/

#define INI_R               0
#define INI_RW              1

#define INI_IGCASEVALUE     1
#define INI_NOIGCASEVALUE   0

#define INI_IGCASENAME      1
#define INI_NOIGCASENAME    0

typedef struct _TValueList {
    char Flag;
    char *Name;
    char *Value;
    struct _TValueList *Next;
}
TValueList;

typedef struct _TSectionList {
    char Flag;
    char *Name;
    TValueList *ValueList;
    TValueList *ValueTail;
    struct _TSectionList *Next;
}
TSectionList;

class IniFile
{
private:
    TSectionList *Head;
    TSectionList *Tail;

    int FIgnoreCaseValue;
    int FIgnoreCaseName;
    int FileWriteOK;
    int FileModified;
    int FAutoLoad;
    int FIReadOnly;
    char FCFileName[ FILENAME_LEN + 1 ];
private:
    void IniInit();
    void LoadFromFile();
    TSectionList *GetSection(const char *ASectionName);
    TSectionList *AddSection(const char *ASectionName, const char AFlag);

    TValueList *ValueExists(TSectionList *ASection, const char *AName);
    void IniParse(char *ABuffer, char *AFlag, char *AName, char *AValue);
    void AddValue(const char *ASectionName, const char *AName, const char *AValue, const char AFlag);

    void DeleteSection(TSectionList *ASection);
    void DeleteValue(TValueList *AValue);
    void DeleteSectionValues(TSectionList *ASection);
    void DeleteAll();

    char *GetValue(const char *ASection, const char *AName);

    void ShowSection(TSectionList *ASection);
    int ShowSectionValues(TSectionList *ASection);
    void ShowAll();

    void SaveSection(TSectionList *ASection, FILE *fp);
    int SaveSectionValues(TSectionList *ASection, FILE *fp);
    void SaveToFile(const char *AFileName);
    void SaveToFile();

    char *ReadValue(const char *ASection, const char *AName, const char *AValue);
    int ReadValue(const char *ASection, const char *AName, const int AValue);
    double ReadValue(const char *ASection, const char *AName, const double AValue);
    char ReadValue(const char *ASection, const char *AName, const char AValue);

    void WriteValue(const char *ASection, const char *AName, const char *AValue);
    void WriteValue(const char *ASection, const char *AName, const int AValue);
    void WriteValue(const char *ASection, const char *AName, const double AValue);
    void WriteValue(const char *ASection, const char *AName, const char AValue);

public:
    IniFile(const char *AFileName, const int AReadOnly = 0);
    ~IniFile();

    void SetIgnoreCaseValue(int AValue);
    void SetIgnoreCaseName(int AValue);

    char *ReadString(const char *ASection, const char *AName, const char *AValue);
    int ReadInteger(const char *ASection, const char *AName, const int AValue);
    double ReadDouble(const char *ASection, const char *AName, const double AValue);
    char ReadChar(const char *ASection, const char *AName, const char AValue);

    void WriteString(const char *ASection, const char *AName, const char *AValue);
    void WriteInteger(const char *ASection, const char *AName, const int AValue);
    void WriteDouble(const char *ASection, const char *AName, const double AValue);
    void WriteChar(const char *ASection, const char *AName, const char AValue);

    void DeleteKey(const char *ASection, const char *AName);
    void EraseSection(const char *ASection);
    void UpdateFile();
};



#endif
