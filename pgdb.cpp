#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "pgdb.h"
#include "Log.h"
#include "IniFile.h"
GatewayDB::GatewayDB()
{
}
GatewayDB::~GatewayDB()
{		
}


/*****************************************postgres 数据库 start *******************************************/
#ifndef ODBC
//数据库连接
int GatewayDB::ConnectDB(const char *dbaddr, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd, int timeout)
{


	char constr[512]= {0};

	assert(NULL != dbaddr);
	assert(NULL != dbname);
	assert(NULL != dbuser);

	snprintf(constr, sizeof(constr) - 1, "hostaddr='%s' port='%d' dbname='%s' user='%s' password='%s' connect_timeout=%d",
			dbaddr, dbport, dbname, dbuser, dbpasswd, timeout);

	if ((g_pconn = PQconnectdb(constr)) == NULL){
		FATAL("Connect PGDB failed!: " << PQerrorMessage(g_pconn));
		return -1;
	}

	if (PQstatus(g_pconn) != CONNECTION_OK){
		PQfinish(g_pconn);
		FATAL("Connect PGDB failed!");
		return -1;
	}

	if (PQsetClientEncoding(g_pconn, "SQL_ASCII") < 0){
		PQfinish(g_pconn);
		FATAL("PQsetClientEncoding failed!");
		return -1;
	}

	return 0;
}

//断开数据库连接
void GatewayDB::DisConnectDB()
{
	if(NULL != g_pconn ){
		PQfinish(g_pconn);
		g_pconn = NULL;
	}
}


//更新数据
int GatewayDB::UpdateData(const char * sql){
	DBresult *res_update = NULL;
	res_update = PQexec(g_pconn, sql);
	if (PQresultStatus(res_update) != PGRES_COMMAND_OK){		
		FATAL("exec sql failed: "<< sql << "\n" << PQerrorMessage(g_pconn));
		DBclear(res_update);
		return -1;
	}

	if(strcmp(PQcmdStatus(res_update),"UPDATE 0")==0)return -1;

	DBclear(res_update);

	return 0;
}

int GatewayDB::DeleteData(const char * sql){

	DBresult *res_delete = NULL;
	res_delete = PQexec(g_pconn, sql);
	if (PQresultStatus(res_delete) != PGRES_COMMAND_OK){		
		FATAL("execute sql failed:"<< sql << "\n"<< PQerrorMessage(g_pconn));
		DBclear(res_delete);
		return -1;
	}

	DBclear(res_delete);

	return 0;
}

//插入数据
int GatewayDB::InsertData(const char * sql){
	DBresult *res_insert = NULL;
	res_insert = PQexec(g_pconn, sql);
	if (PQresultStatus(res_insert) != PGRES_COMMAND_OK){		
		FATAL("execute sql failed:"<< sql << "\n"<< PQerrorMessage(g_pconn));
		DBclear(res_insert);
		return -1;
	}
	DBclear(res_insert);

	return 0;
}

//查询数据
DBresult * GatewayDB::SelectData(const char * sql){
	DBresult *res_select = NULL;
	res_select = PQexec(g_pconn, sql);
	if (PQresultStatus(res_select) != PGRES_TUPLES_OK){		
		FATAL("execute sql failed:"<< sql << "\n"<< PQerrorMessage(g_pconn));
		DBclear(res_select);
		return NULL;
	}	
	//	printf("pgdb.cpp 117 DBntupleds(res) = %d\n", DBntuples(res_select));

	return res_select;
}

//取得记录集中某行某个字段的值
char* GatewayDB::DBgetvalue(DBresult *res, int row, int line)
{
	return PQgetvalue(res, row, line);
}

//取得记录的个数
int GatewayDB::DBntuples(DBresult *res)
{
	int n = PQntuples(res);
	return n;
}

//获得字段的个数
int GatewayDB::DBnfields(DBresult *res)
{
	return PQnfields(res);
}

//清除记录集
int GatewayDB::DBclear(DBresult *res)
{
	PQclear(res);
	return 0;
}
/*****************************************postgres 数据库 end *******************************************/


#else

/*****************************************ODBC 数据库 start *********************************************/
wchar_t* ctow(const char *str)
{
	wchar_t* buffer;
	if(str)

	{

		size_t nu = strlen(str);
		size_t n =(size_t)MultiByteToWideChar(CP_ACP,0,(const char *)str,int(nu),NULL,0);
		buffer=0;

		buffer = new wchar_t[n+1];

		//if(n>=len) n=len-1;

		::MultiByteToWideChar(CP_ACP,0,(const char *)str,int(nu),buffer,int(n));    
		buffer[n]='\0';
	}

	return buffer;
}

//数据库连接,正确反回0，错误返回-1
int GatewayDB::ConnectDB(const char *dbaddr, int dbport, const char *dbname, const char *dbuser, const char *dbpasswd, int timeout)
{
	assert(NULL != dbaddr);
	assert(NULL != dbname);
	assert(NULL != dbuser);

	int retcode=0;
	//1.连接数据源   
	//1.环境句柄   
	retcode   =   SQLAllocHandle   (SQL_HANDLE_ENV,   NULL,   &henv);  
	//("SQLAllocHandle: %d\n", retcode);
	retcode   =   SQLSetEnvAttr(henv,   SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);    
	//log_output("SQLAllocHandle: %d\n", retcode);
	//2.连接句柄     
	retcode   =   SQLAllocHandle(SQL_HANDLE_DBC,   henv,   &hdbc1);    
	//log_output("SQLAllocHandle:%d\n", retcode);
	retcode = SQLConnect(hdbc1, (SQLCHAR *)dbaddr, SQL_NTS, (SQLCHAR *)dbuser, SQL_NTS, (SQLCHAR *)dbpasswd, SQL_NTS);


	//判断连接是否成功   
	if   (   (retcode   !=   SQL_SUCCESS)   &&   (retcode   !=   SQL_SUCCESS_WITH_INFO)   ){    
		FATAL("SQLConnect error, recode:"<< retcode);
	}

	return retcode;
}


//断开数据库连接
void GatewayDB::DisConnectDB()
{
	if(NULL != hdbc1 ){
		SQLDisconnect(hdbc1);       
		SQLFreeHandle(SQL_HANDLE_DBC, hdbc1);      
		SQLFreeHandle(SQL_HANDLE_ENV, henv);      
	}
}


//更新数据
int GatewayDB::UpdateData(const char * sql){
	int retcode = 0;
	//	wchar_t * w_sql = ctow(sql);
	retcode   =   SQLAllocHandle(SQL_HANDLE_STMT,   hdbc1,   &hstmt1); 
	retcode = SQLSetStmtAttr (hstmt1, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);
	retcode = SQLExecDirect (hstmt1, (SQLCHAR *)sql, SQL_NTS);
	if(retcode != SQL_SUCCESS &&  retcode != SQL_SUCCESS_WITH_INFO){
		FATAL( "SQL:"<<sql<<"\nSQLExecDirect error, recode:"<< retcode);
		return -1;	
	}	

	return retcode;
}

//删除数据
int GatewayDB::DeleteData(const char * sql){
	int retcode = 0;
	retcode   =   SQLAllocHandle(SQL_HANDLE_STMT,   hdbc1,   &hstmt1); 
	retcode = SQLSetStmtAttr (hstmt1, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);

	retcode = SQLExecDirect (hstmt1, (SQLCHAR *)sql, SQL_NTS);

	if(retcode != SQL_SUCCESS &&  retcode != SQL_SUCCESS_WITH_INFO){
		FATAL( "SQL:"<<sql<<"\nSQLExecDirect error, recode:"<< retcode);
		return -1;	
	}	
	SQLCancel(hstmt1);
	return retcode;
}


//插入数据
int GatewayDB::InsertData(const char * sql){

	int retcode=0;
	//wchar_t * w_sql = ctow(sql);
	retcode   =   SQLAllocHandle(SQL_HANDLE_STMT,   hdbc1,   &hstmt1); 
	//printf("GatewayDB::InsertData retcode1 = %d\n", retcode);

	retcode = SQLSetStmtAttr (hstmt1, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);
	//printf("GatewayDB::InsertData retcode2 = %d\n", retcode);
	//printf("sql=%s\n",sql);
	retcode = SQLExecDirect (hstmt1, (SQLCHAR *)sql, SQL_NTS);

	//log_output("sql=%s\n",sql);
	//printf("retcode=%d\n", retcode);
	//printf("GatewayDB::InsertData retcode3 = %d\n", retcode);

	//if(w_sql != NULL)free(w_sql);
	if(retcode != SQL_SUCCESS &&  retcode != SQL_SUCCESS_WITH_INFO){
		FATAL( "SQL:"<<sql<<"\nSQLExecDirect error, recode:"<< retcode);
	}	
	SQLCancel(hstmt1);  
	return retcode;
}


//查询数据
DBresult * GatewayDB::SelectData(const char * sql){
	int retcode = 0;
	//	wchar_t * w_sql = ctow(sql);
	retcode   =   SQLAllocHandle(SQL_HANDLE_STMT,   hdbc1,   &hstmt1); 
	retcode = SQLSetStmtAttr (hstmt1, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN, 0);
	retcode = SQLExecDirect (hstmt1, (SQLCHAR *)sql, SQL_NTS);
	//	printf("SelectData retcode = %d\n", retcode);
	//	if(w_sql != NULL)free(w_sql);
	//	printf("sql: %s\n", sql);
	if(retcode != SQL_SUCCESS &&  retcode != SQL_SUCCESS_WITH_INFO){
		FATAL( "SQL:"<<sql<<"\nSQLExecDirect error, recode:"<< retcode);
	}	
	return &hstmt1;
}		


//取得记录集中某行某个字段的值
char* GatewayDB::DBgetvalue(DBresult *res, int row, int line)
{
	SQLCHAR	value[50];
	static char myval[50];
	int retcode=0;
	retcode=SQLFetchScroll(*res,SQL_FETCH_ABSOLUTE , row+1 );//将记录集的光标移动一绝对位置行
	//printf("SQLFetchScroll(): retcode = %d\n", retcode);
	SQLINTEGER  cb_value;
	retcode=SQLGetData(hstmt1, line+1, SQL_C_CHAR, &value, 50, &cb_value);
	//	printf("SQLGetData(): retcode = %d\n", retcode);
	memcpy(myval,value,cb_value+1);

	return myval;
}


//取得记录的个数
int GatewayDB::DBntuples(DBresult *res)
{
	//获得记录集中的行数
	SQLINTEGER  rows = 0;
	while(SQL_NO_DATA != SQLFetch(*res)) //移动光标，一直到集合末尾
	{
		rows++;
	}
	SQLFetchScroll(*res,SQL_FETCH_FIRST, 0 );//将光标移动到记录集的第一行

	return rows;	
}

//获得字段的个数
int GatewayDB::DBnfields(DBresult *res)
{
	SQLSMALLINT cols;
	int retCode = SQLNumResultCols (*res, &cols);
	if((retCode != SQL_SUCCESS) && (retCode != SQL_SUCCESS_WITH_INFO))  
	{  
		FATAL("errro ColCount, retCode: "<<retCode);
		return -1;  
	}  

	return cols;
}


//清除记录集
int GatewayDB::DBclear(DBresult *res)
{
	SQLFreeHandle(SQL_HANDLE_STMT, *res);

	return 0;
}

/*****************************************ODBC 数据库 end ***********************************************/
#endif
