#ifndef PGDB_H
#define PGDB_H

#include <vector>
using namespace std; 

#ifdef _MSC_VER
#define ODBC
#endif

#include <vector>
using namespace std; 

#ifdef ODBC 

/************ODBC STRT************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>      
#include <sql.h>      
#include <sqlext.h>      
#include <sqltypes.h>      
#include <odbcss.h>
#define WIN32_LEAN_AND_MEAN
//SQLHENV henv = SQL_NULL_HENV;   
//SQLHDBC hdbc1 = SQL_NULL_HDBC; 
//SQLHSTMT hstmt1 = SQL_NULL_HSTMT;  
#define DBresult SQLHSTMT  //记录集类型
#define DBconn   SQLHDBC    //数据库连接句柄

#else

/*************postgres start***************/
#include <libpq-fe.h>
#include <postgres_ext.h>
#define DBresult PGresult  //记录集类型
#define DBconn   PGconn    //数据库连接句柄
/*************postgres end*****************/
#endif

extern  int log_level;
extern  char dbaddr[50];
extern  int dbport ; 
extern  char dbname[50];
extern  char dbuser[50];
extern  char dbpwd[50];

class GatewayDB{
public:
		GatewayDB();
		~GatewayDB();		
		int ConnectDB(const char *pch_dbaddr, int ndbport, const char *pch_dbname, const char *pch_dbuser, const char *pch_dbpasswd, int timeout);
		int MyconnectDB();
		void DisConnectDB();
		int UpdateData(const char *);
		int DeleteData(const char * sql);
		int InsertData(const char *);
		DBresult * SelectData(const char *);
		char* DBgetvalue(DBresult *res, int row, int line);
		int DBntuples(DBresult *res);
		int DBnfields(DBresult *res);
		int DBclear(DBresult *res);						

public:
		#ifdef ODBC 
			SQLHENV henv;   
			SQLHDBC hdbc1; 
			SQLHSTMT hstmt1;
		#else
			DBconn *g_pconn;
		#endif
};
#endif /* PGDB_H */
