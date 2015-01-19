// Log.cpp: implementation of the Log class.  
//  
//////////////////////////////////////////////////////////////////////  

#include "Log.h"  
#include <string>
#include <stdio.h>
using namespace std;

//////////////////////////////////////////////////////////////////////  
// Construction/Destruction  
//////////////////////////////////////////////////////////////////////  

/*����һ��ȫ�ֵļ�¼������*/
Logger Log::_logger = log4cplus::Logger::getInstance("main_log");  

Log::Log()  
{  
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

	int Log_level = 0;    

	/* step 1: Instantiate an appender object,ʵ����һ���ҽ������� */  
	//SharedAppenderPtr _append(new FileAppender(_log_name)); //�ļ��ҽ���
	SharedAppenderPtr _append(new ConsoleAppender());//����̨�ҽ���
	_append->setName("file log test");  

	/* step 2: Instantiate a layout object��ʵ����һ�����������󣬿��������Ϣ�ĸ�ʽ*/  
	//std::string pattern = "[%d{%m/%d/%y %H:%M:%S}] [%p] [%t] - %m %l%n";
	std::string pattern = "[%d{%m/%d/%y %H:%M:%S}] [%p] - %m %l%n";
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
