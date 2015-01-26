// Log.cpp: implementation of the Log class.  
//  
//////////////////////////////////////////////////////////////////////  

#include "Log.h"  
#include "IniFile.h"
#include <string>
#include <string.h>
#include <stdio.h>

#ifdef _MSC_VER		//windowsƽ̨ ���� GetModuleFileName(NULL, buffer, 500)
#include "windows.h"
#else				//linuxƽ̨	  ���� getcwd(buffer,   500);
#include <unistd.h>
#include <dirent.h>
#endif

char CONFIG_PATH[2048];

using namespace std;



//////////////////////////////////////////////////////////////////////  
// Construction/Destruction  
//////////////////////////////////////////////////////////////////////  

/*����һ��ȫ�ֵļ�¼������*/
Logger Log::_logger = log4cplus::Logger::getInstance("main_log");  

Log::Log()  
{  
#ifdef _MSC_VER
	snprintf(_log_path, sizeof(_log_path), "%s", "log");  
	snprintf(_log_name, sizeof(_log_name), "%s/%s.%s", _log_path, "error", "log");
	snprintf(_log_path, sizeof(_log_path), "%s", "config");  
	snprintf(_conf_name, sizeof(_log_name), "%s/%s.%s", _log_path, "init", "conf");
#else
	char proc[2048];
	char *p;
	sprintf(proc, "/proc/%d/exe", getpid());
	readlink(proc, _log_path, 128); /*proc/pid/exe ��һ�����ӣ���readlink��*/
	p = strchr(_log_path,'('); /*������·�������п��ܻ��� (deleted)������ɾȥ*/
	if(p!=NULL) *(--p)='\0';
	p = strrchr(_log_path,'/');
	if(p!=NULL) *p='\0';
#endif
	snprintf(_log_name, sizeof(_log_name), "%s/%s/%s.%s", _log_path,"log", "error", "log");
	snprintf(_conf_name, sizeof(_conf_name), "%s/%s/%s.%s", _log_path, "config","init", "conf");
	strcpy(CONFIG_PATH, _conf_name);
	cout << "_log_name: " << _log_name<<endl;
	cout << "_conf_name: " << _conf_name<<endl;
	
}  

Log::~Log()  
{  
}  

Log& Log::instance()  
{  
	static Log log;  
	return log;  
}  

bool Log::open_log()  
{  

	IniFile config(_conf_name);
	int Log_level = config.ReadInteger("log", "LOG_LEVEL", 1);

	/* step 1: Instantiate an appender object,ʵ����һ���ҽ������� */  
	SharedAppenderPtr _append(new FileAppender(_log_name, LOG4CPLUS_FSTREAM_NAMESPACE::ios::app,true)); //�ļ��ҽ���
	//SharedAppenderPtr _append(new ConsoleAppender());//����̨�ҽ���
	_append->setName("file log test");  

	/* step 2: Instantiate a layout object��ʵ����һ�����������󣬿��������Ϣ�ĸ�ʽ*/  
	//std::string pattern = "[%d{%m/%d/%y %H:%M:%S}] [%p] [%t] - %m %l%n";
	std::string pattern = "[%D{%m/%d/%y %H:%M:%S}] [%p] - %m %l%n";
	std::auto_ptr<Layout> _layout(new PatternLayout(pattern));  

	/* step 3: Attach the layout object to the appender�����������󶨵��ҽ����� */  
	_append->setLayout(_layout);  

	/* step 4: Instantiate a logger object��ʵ����һ����¼������ */  

	/* step 5: Attach the appender object to the logger�����ҽ�����ӵ���¼����  */  
	Log::_logger.addAppender(_append);  

	/* step 6: Set a priority for the logger�����ü�¼�������ȼ�  */  
	Log::_logger.setLogLevel(Log_level);  

	return true;  
} 
