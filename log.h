#ifndef LOG_H
#define LOG_H

#define CORRECTYEAR                     1900
#define CORRECTMONTH                    1
#define LOG_ERROR						2


#define  	    LOG_OPEN        "a"

extern char CONFIG_PATH[2048];
extern char LOG_SYS_PATH[2048];

int log_output(const char * fmt, ...);
void init_path();

#ifdef _MSC_VER
#define snprintf			_snprintf
#endif

#endif
