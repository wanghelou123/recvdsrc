#if !defined(AFX_LOG_H__B87F71E3_FFAE_4CFA_A528_3F4F2FF7D69E__INCLUDED_)  
#define AFX_LOG_H__B87F71E3_FFAE_4CFA_A528_3F4F2FF7D69E__INCLUDED_  

#include "log4cplus/loglevel.h"  
#include "log4cplus/ndc.h"   
#include "log4cplus/logger.h"  
#include "log4cplus/configurator.h"  
#include "iomanip"  
#include "log4cplus/fileappender.h"  
#include "log4cplus/consoleappender.h"
#include "log4cplus/layout.h"  

//#include "const.h"  
//#include "common.h"  
//#include "Main_config.h"  

using namespace log4cplus;  
using namespace log4cplus::helpers;  

#define  PATH_SIZE	100
//��־��װ  
#define TRACE(p) LOG4CPLUS_TRACE(Log::_logger, p)  
#define DEBUG(p) LOG4CPLUS_DEBUG(Log::_logger, p)  
#define NOTICE(p) LOG4CPLUS_INFO(Log::_logger, p)  
#define WARNING(p) LOG4CPLUS_WARN(Log::_logger, p)  
#define FATAL(p) LOG4CPLUS_ERROR(Log::_logger, p)  

// ��־�����࣬ȫ�ֹ���һ����־  
class Log  
{  
	public:  
		// ����־  
		bool open_log();  

		// �����־ʵ��  
		static Log& instance();  

		static Logger _logger;  

	private:  
		Log();  

		virtual ~Log();  

		//log�ļ�·��������  
		char _log_path[PATH_SIZE];  
		char _log_name[PATH_SIZE];  
};  
#endif // !defined(AFX_LOG_H__B87F71E3_FFAE_4CFA_A528_3F4F2FF7D69E__INCLUDED_)  

