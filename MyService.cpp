#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "Log.h"

extern int Startup();

SERVICE_STATUS          ServiceStatus; 
SERVICE_STATUS_HANDLE   hStatus;
void  ServiceMain(int argc, char** argv); 
void  ControlHandler(DWORD request); 
int InitService();
void InstallService(const char * szServiceName);
bool UnInstallService(const char * szServiceName);

// Service initialization
int InitService() 
{ 
	// 打开日志  
	if (!Log::instance().open_log())  
	{   
		std::cout << "Log::open_log() failed" << std::endl;  
		exit(-1);
	}   
	NOTICE("准备启动收数软件服务");
	return 0;
}

// Control Handler
void ControlHandler(DWORD request) 
{ 
   switch(request) 
   { 
      case SERVICE_CONTROL_STOP: 
		 NOTICE("动收数软件服务已经停止.");

         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
 
      case SERVICE_CONTROL_SHUTDOWN: 
		 NOTICE("动收数软件服务已经停止.");
         ServiceStatus.dwWin32ExitCode = 0; 
         ServiceStatus.dwCurrentState = SERVICE_STOPPED; 
         SetServiceStatus (hStatus, &ServiceStatus);
         return; 
        
      default:
         break;
    } 
 
    // Report current status
    SetServiceStatus (hStatus, &ServiceStatus);
 
    return; 
}

void ServiceMain(int argc, char** argv) 
{ 
   int error; 
 
   ServiceStatus.dwServiceType = 
      SERVICE_WIN32; 
   ServiceStatus.dwCurrentState = 
      SERVICE_START_PENDING; 
   ServiceStatus.dwControlsAccepted   =  
      SERVICE_ACCEPT_STOP | 
      SERVICE_ACCEPT_SHUTDOWN;
   ServiceStatus.dwWin32ExitCode = 0; 
   ServiceStatus.dwServiceSpecificExitCode = 0; 
   ServiceStatus.dwCheckPoint = 0; 
   ServiceStatus.dwWaitHint = 0; 
 
   hStatus = RegisterServiceCtrlHandler(
      "recv-data-platform", 
      (LPHANDLER_FUNCTION)ControlHandler); 
   if (hStatus == (SERVICE_STATUS_HANDLE)0) 
   { 
      // Registering Control Handler failed
      return; 
   }  

   // Initialize Service 
   error = InitService(); 
   if (error) 
   {
      // Initialization failed
      ServiceStatus.dwCurrentState = 
         SERVICE_STOPPED; 
      ServiceStatus.dwWin32ExitCode = -1; 
      SetServiceStatus(hStatus, &ServiceStatus); 
      return; 
   } 

   // We report the running status to SCM. 
   ServiceStatus.dwCurrentState = 
      SERVICE_RUNNING; 
   SetServiceStatus (hStatus, &ServiceStatus);
 

	Startup();//执行服务循环，在此处跳到了收数平台执行程序	

   return; 
}


void InstallService(const char * szServiceName)
{
	SC_HANDLE handle = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	char szFilename[256];
	::GetModuleFileName(NULL, szFilename, 255);
	SC_HANDLE hService = ::CreateService(handle, szServiceName,
		szServiceName, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, szFilename, NULL,
		NULL, NULL, NULL, NULL);
	::CloseServiceHandle(hService);
	::CloseServiceHandle(handle);

	printf("install recv-data-platform Service  complete!\n");
}


bool UnInstallService(const char * szServiceName)
{
	BOOL bResult = FALSE;

	SC_HANDLE handle = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	char szFilename[256];
	::GetModuleFileName(NULL, szFilename, 255);

	if (handle != NULL)
    {
        SC_HANDLE hService = ::OpenService(handle, szServiceName, SERVICE_QUERY_CONFIG);
        if (hService != NULL)
        {
            bResult = TRUE;
            ::CloseServiceHandle(hService);
        }
        ::CloseServiceHandle(handle);
    }

	if(!bResult)//服务没有安装
	{
		printf("Service not install\n");
		return true;
	}


	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (hSCM == NULL)
    {
        MessageBox(NULL, _T("Couldn't open service manager"), szServiceName, MB_OK);
        return FALSE;
    }

    SC_HANDLE hService = ::OpenService(hSCM, szServiceName, SERVICE_STOP | DELETE);

    if (hService == NULL)
    {
        ::CloseServiceHandle(hSCM);
        MessageBox(NULL, _T("Couldn't open service"), szServiceName, MB_OK);
        return FALSE;
    }
    SERVICE_STATUS status;
    ::ControlService(hService, SERVICE_CONTROL_STOP, &status);

    BOOL bDelete = ::DeleteService(hService);
    ::CloseServiceHandle(hService);
    ::CloseServiceHandle(hSCM);

    if (bDelete)
        return TRUE;

    MessageBox(NULL, _T("Service could not be deleted"), szServiceName, MB_OK);
    return FALSE;
	
}

void main(int argc, char* argv[])
{ 

	if ((argc==2) && (::strcmp(argv[1], "install")==0))
	{

		printf("installing recv-data-platform...\n");
		InstallService("recv-data-platform");
		return ;

	}else if((argc==2) && (::strcmp(argv[1], "uninstall")==0))
	{

		printf("unistalling recv-data-platform...\n");	
		if(UnInstallService("recv-data-platform") ) {
			printf("uninstall recv-data-platform Service complete!\n ");
		}

	}else if((argc==2) && ((::strcmp(argv[1], "-d")==0)|| (::strcmp(argv[1], "-D")==0))){
		printf("DEBUG mode...\n");
		// 打开日志  
		if (!Log::instance().open_log())  
		{   
			std::cout << "Log::open_log() failed" << std::endl;  
			exit(-1);
		}   
		NOTICE("准备启动收数软件服务");
        Startup();
    }  

   SERVICE_TABLE_ENTRY ServiceTable[2];
   ServiceTable[0].lpServiceName = "recv-data-platform";
   ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)ServiceMain;

   ServiceTable[1].lpServiceName = NULL;
   ServiceTable[1].lpServiceProc = NULL;


   // Start the control dispatcher thread for our service
   StartServiceCtrlDispatcher(ServiceTable);

}
#endif
