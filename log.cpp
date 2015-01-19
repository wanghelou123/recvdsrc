#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <iostream>
#include "log.h"
#include "IniFile.h"

#include <boost/thread/mutex.hpp>	//boost基本工具
#include <boost/thread/condition_variable.hpp>	



#ifdef _MSC_VER		//windows平台 用于 GetModuleFileName(NULL, buffer, 500);
#include "windows.h"
#else				//linux平台	  用于 getcwd(buffer,   500);
#include <unistd.h>
#include <dirent.h>
#define READ_BUF_SIZE 200
long* find_pid_by_name( char* pidName);
#endif

using std::string;
extern int log_level;

char CONFIG_PATH[2048];
char LOG_SYS_PATH[2048];

boost::mutex log_mutex;
boost::condition_variable log_condition_variable;
int log_output(const char * fmt, ...)
{
	boost::mutex::scoped_lock lock(log_mutex);
    FILE *fp_log=NULL;
	fp_log = fopen(LOG_SYS_PATH, LOG_OPEN);
	if (fp_log == NULL){   
			fprintf(stderr, "fopen() %s", strerror(errno));	
			return -1;
	} 
		
    va_list ap;
    time_t timenow;
    struct tm *pst_time = NULL;

    if (LOG_ERROR > log_level)
    {
        va_end(ap);
        return -1;
    }

    va_start(ap, fmt);

    time(&timenow);
    pst_time = localtime(&timenow);

    fprintf(fp_log, "[%4d/%02d/%02d %02d:%02d:%02d] "
               , (CORRECTYEAR + pst_time->tm_year)
               , (CORRECTMONTH + pst_time->tm_mon)
               , pst_time->tm_mday
               , pst_time->tm_hour
               , pst_time->tm_min
               , pst_time->tm_sec);

    vfprintf(fp_log, fmt, ap);
    va_end(ap);
    fflush(fp_log);
    fclose(fp_log);

	lock.unlock();
	return 0;
    
}

void init_path()
{


//////////////////////////////////////////////////////////////
/*初始化配置文件路径*/
	int pos = 0;
	char buffer[2048]; 
	memset(buffer, '\0', sizeof(buffer));
	memset(CONFIG_PATH, '\0', sizeof(CONFIG_PATH));
	memset(LOG_SYS_PATH, '\0', sizeof(LOG_SYS_PATH));


#ifdef _MSC_VER		//windows平台

    GetModuleFileName(NULL, buffer, 500);
	string m_path = string(buffer);
	while(string::npos != m_path.find('\\'))
		m_path.replace(m_path.find('\\'),1, 1, '/');
	pos = m_path.find_last_of('/');
	m_path.erase(pos, m_path.length()-1);

#else 			//linux平台
	long *pid = find_pid_by_name("recv-data-platf");
	char proc[2048];
	char *p;
	sprintf(proc, "/proc/%d/exe", *pid);
	readlink(proc,buffer, 128); /*proc/pid/exe 是一个链接，用readlink读*/
	p = strchr(buffer,'('); /*读出的路径后面有可能会有 (deleted)字样，删去*/
	if(p!=NULL)
	{
			p--;
			*p = '\0';
	}
	p = strrchr(buffer,'/');
	if(p!=NULL)*p='\0';
	//puts(buffer);

	string m_path = string(buffer);

#endif


	m_path.append("/config/init.conf");
	strcpy(CONFIG_PATH, m_path.c_str());
	pos = m_path.find_last_of('/');
	m_path.erase(pos, m_path.length()-1);
	pos = m_path.find_last_of('/');
	m_path.erase(pos, m_path.length()-1);
	m_path.append("/log/error.log");
	strcpy(LOG_SYS_PATH, m_path.c_str());
   	IniFile config(CONFIG_PATH);
	log_level = config.ReadInteger("log", "LOG_LEVEL", 1); 

	log_output(" %s\n", buffer);
////////////////////////////////////////////////////////////////
}

#ifndef _MSC_VER		//linux
long* find_pid_by_name( char* pidName)
{
		DIR *dir;
		struct dirent *next;
		long* pidList=NULL;
		int i=0;

		///proc中包括当前的进程信息,读取该目录
		dir = opendir("/proc");
		if (!dir)
				perror("Cannot open /proc");
		//遍历
		while ((next = readdir(dir)) != NULL) {
				FILE *status;
				char filename[READ_BUF_SIZE];
				char buffer[READ_BUF_SIZE];
				char name[READ_BUF_SIZE];

				/* Must skip ".." since that is outside /proc */
				if (strcmp(next->d_name, "..") == 0)
						continue;

				/* If it isn't a number, we don't want it */
				if (!isdigit(*next->d_name))
						continue;
				//设置进程
				sprintf(filename, "/proc/%s/status", next->d_name);
				if (! (status = fopen(filename, "r")) ) {
						continue;
				}
				if (fgets(buffer, READ_BUF_SIZE-1, status) == NULL) {
						fclose(status);
						continue;
				}
				fclose(status);

				//得到进程id
				/* Buffer should contain a string like "Name:   binary_name" */
				sscanf(buffer, "%*s %s", name);

				if ((strncmp(name, pidName, 15)) == 0) {
						pidList=(long *)realloc( pidList, sizeof(long) * (i+2));
						pidList[i++]=strtol(next->d_name, NULL, 0);
				}
		}

		if (pidList) {
				pidList[i]=0;
				return pidList;
		}

		return NULL;
}

#endif
