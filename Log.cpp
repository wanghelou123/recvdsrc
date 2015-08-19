// Log.cpp: implementation of the Log class.  
//  
//////////////////////////////////////////////////////////////////////  

#include "Log.h"  
#include "IniFile.h"
#include <string>
#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#endif

char CONFIG_PATH[2048];

using namespace std;



//////////////////////////////////////////////////////////////////////  
// Construction/Destruction  
//////////////////////////////////////////////////////////////////////  

/*定义一个全局的记录器对象*/
Logger Log::_logger = log4cplus::Logger::getInstance("main_log");  

Log::Log()  
{  
#ifdef WIN32
	int pos = 0;
	char buffer[2048]; 
	memset(buffer, '\0', sizeof(buffer));
	GetModuleFileName(NULL, buffer, 500);
	string m_path = string(buffer);
	while(string::npos != m_path.find('\\'))
		m_path.replace(m_path.find('\\'),1, 1, '/');
	pos = m_path.find_last_of('/');
	m_path.erase(pos, m_path.length()-1);

	snprintf(_log_path, sizeof(_log_path), "%s", m_path.c_str());
	//cout << "_log_path:"<<_log_path<<endl;
	snprintf(_log_name, sizeof(_log_name), "%s/log/%s.%s", _log_path, "error", "log");
	snprintf(_conf_name, sizeof(_log_name), "%s/config/%s.%s", _log_path, "init", "conf");
#else
	char proc[2048];
	char *p;
	sprintf(proc, "/proc/%d/exe", getpid());
	readlink(proc, _log_path, 128); /*proc/pid/exe 是一个链接，用readlink读*/
	p = strchr(_log_path,'('); /*读出的路径后面有可能会有 (deleted)字样，删去*/
	if(p!=NULL) *(--p)='\0';
	p = strrchr(_log_path,'/');
	if(p!=NULL) *p='\0';
	snprintf(_log_name, sizeof(_log_name), "%s/%s/%s.%s", _log_path,"log", "error", "log");
	snprintf(_conf_name, sizeof(_conf_name), "%s/%s/%s.%s", _log_path, "config","init", "conf");

#endif
	strcpy(CONFIG_PATH, _conf_name);

	//cout << "CONFIG_PATH:"<< CONFIG_PATH<<endl;
	//cout << "_log_name: " << _log_name<<endl;
	//cout << "_conf_name: " << _conf_name<<endl;
	
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

	/* step 1: Instantiate an appender object,实例化一个挂接器对象 */  
	//SharedAppenderPtr _append(new FileAppender(_log_name, LOG4CPLUS_FSTREAM_NAMESPACE::ios::app,true)); //文件挂接器
	SharedAppenderPtr _append(new RollingFileAppender(_log_name,1024*1024, 5, true)); //文件挂接器,每个文件1MB,6个文件
	//SharedAppenderPtr _append(new ConsoleAppender());//控制台挂接器
	_append->setName("file log test");  

	/* step 2: Instantiate a layout object，实例化一个布局器对象，控制输出信息的格式*/  
	//std::string pattern = "[%d{%m/%d/%y %H:%M:%S}] [%p] [%t] - %m %l%n";
	std::string pattern = "[%D{%m/%d/%y %H:%M:%S}] [%p] [%t] - %m %l%n";
	std::auto_ptr<Layout> _layout(new PatternLayout(pattern));  

	/* step 3: Attach the layout object to the appender，将布局器绑定到挂接器上 */  
	_append->setLayout(_layout);  

	/* step 4: Instantiate a logger object，实例化一个记录器对象 */  

	/* step 5: Attach the appender object to the logger，将挂接器添加到记录器中  */  
	Log::_logger.addAppender(_append);  

	/* step 6: Set a priority for the logger，设置记录器的优先级  */  
	/* TRACE_LOG_LEVEL   = 0
	 * DEBUG_LOG_LEVEL   = 10000
	 * INFO_LOG_LEVEL    = 20000
	 * WARN_LOG_LEVEL    = 30000
	 * ERROR_LOG_LEVEL   = 40000
	 * FATAL_LOG_LEVEL   = 50000;
	 * OFF_LOG_LEVEL     = 60000;
	 */
	Log::_logger.setLogLevel(Log_level*10000);  

	return true;  
} 
