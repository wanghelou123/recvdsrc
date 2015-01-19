
#ifndef _MSC_VER

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include "log.h"

#define LOCKFILE "/var/run/recv-data-platform.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)


void daemonize(const char*cmd)
{
		int 				i,fd0,fd1,fd2;
		pid_t				pid;
		struct rlimit 		r1;
		struct sigaction	sa;

		/*
		 *clear file creation mask;
		 */
		umask(0);
		/*
		 * Get maxinum number of file descriptors.
		 */
		if(getrlimit(RLIMIT_NOFILE,&r1)<0)
				log_output("%s: can't get file limit\n", cmd);

		/*
		 *Become a sssion leader to lose controlling TTY
		 */
		if((pid=fork())<0)
				log_output("%s:,can't fork\n",cmd);
		else if(pid!=0)/*parent*/
				exit(0);
		setsid();
		/*
		 *Ensure future opens won't allocate controlling TTYs
		 */
		sa.sa_handler=SIG_IGN;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		if(sigaction(SIGHUP,&sa,NULL) < 0)
				log_output("%s: can't ignore SIGHUP\n");
		if((pid = fork())<0)
				log_output("%s: can't fork\n",cmd);
		else if(pid != 0)
				exit(0);

		/*
		 *Change the current working directory to the root so
		 *we won't prevent file system from being unmounted.
		 */
		if(chdir("/")<0)
				log_output("%s: can't change directory to / \n");

		/*
		 *Clsoe all open file descriptors.
		 */
		if(r1.rlim_max==RLIM_INFINITY)
				r1.rlim_max = 1024;
		for(i=0; i < r1.rlim_max; i++)
				close(i);

		/*
		 *Attach file descriptors 0, 1, and 2 to /dev/null.
		 */
		fd0 = open("/dev/null", O_RDWR);
		fd1 = dup(0);
		fd2 = dup(0);


}
int lockfile(int fd)
{
		struct flock fl;

		fl.l_type = F_WRLCK;
		fl.l_start = 0;
		fl.l_whence = SEEK_SET;
		fl.l_len = 0;
		return(fcntl(fd, F_SETLK, &fl));
}
		int
already_running(void)
{
		int		fd;
		char	buf[16];

		fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
		if (fd < 0) {
				log_output("can't open %s: %s\n", LOCKFILE, strerror(errno));
				exit(1);
		}
		if (lockfile(fd) < 0) {
				if (errno == EACCES || errno == EAGAIN) {
						close(fd);
						return(1);
				}
				log_output("can't lock %s: %s\n", LOCKFILE, strerror(errno));
				exit(1);
		}
		ftruncate(fd, 0);
		sprintf(buf, "%ld", (long)getpid());
		write(fd, buf, strlen(buf)+1);
		return(0);
}

int InitDaemo(char * str)
{
		char 				*cmd;
		if( (cmd = strrchr(str,'/')) == NULL)
				cmd = str;
		else
				cmd++;

		/*
		 *Become a daemon.
		 */
		daemonize(cmd);


		/*
		 *Make sure only one copy of the daemon is running.
		 */
			if(already_running()){
			log_output("daemon already running\n");
			exit(1);
			}
		 


/*
		while(1)
		{
				system("echo \"hello\">> /home/whl/log.txt");
				printf("hello\n");
						sleep(2);
		}
*/
		return 0;
}

#endif
