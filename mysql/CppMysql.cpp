#include "CppMysql.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


CppMysqlResultSet::CppMysqlResultSet()
{
    mysqlRes_ = NULL;
    mysqlField_ = NULL;
    mysqlRow_ = NULL;
    rows_ = 0;
    fields_ = 0;
}

CppMysqlResultSet::~CppMysqlResultSet()
{
    freeRes();
}

void CppMysqlResultSet::freeRes()
{
    if(mysqlRes_ != NULL)
    {
        mysql_free_result(mysqlRes_);
        mysqlRes_ = NULL;
    }
}

bool CppMysqlResultSet::eof()
{
    return mysqlRow_ == NULL;
}

void CppMysqlResultSet::nextRow()
{
    if (mysqlRes_ == NULL)
        return;

    mysqlRow_ = mysql_fetch_row(mysqlRes_);
}

/// @brief 跳过指定偏移量(类似fseek)
unsigned long CppMysqlResultSet::seekRow(unsigned long offerset)
{
    if (offerset < 0)
        offerset = 0;
    if (offerset >= rows_)
        offerset = rows_ -1;

    mysql_data_seek(mysqlRes_, offerset);
    mysqlRow_ = mysql_fetch_row(mysqlRes_);
    return offerset;
}

/// @brief 根据指定的列名返回该列在结果集中的列索引
int CppMysqlResultSet::fieldIndex(const char* colName)
{
    if (mysqlRes_ == NULL || colName == NULL)
        return -1;

    mysql_field_seek(mysqlRes_, 0);//定位到第0列
    unsigned int i = 0;
    while(i < fields_)
    {
        mysqlField_ = mysql_fetch_field( mysqlRes_ );
        if (mysqlField_ != NULL && strcmp(mysqlField_->name, colName) == 0) //找到
        {
            return i;
        }
        i++;
    }

    return -1;
}

/// @brief 根据列索引返回该列的列名
const char* CppMysqlResultSet::fieldName(int nCol)
{
    if (mysqlRes_)
        return NULL;

    mysql_field_seek(mysqlRes_, nCol);
    mysqlField_ = mysql_fetch_field(mysqlRes_);

    return mysqlField_ != NULL ? mysqlField_->name : NULL;
}

int CppMysqlResultSet::getIntField(int col, int nullValue/*=0*/)
{
    if (mysqlRes_ == NULL || mysqlRow_ == NULL)
        return nullValue;

    if (col + 1 > fields_)
        return nullValue;

    return atoi(mysqlRow_[col]);
}

int CppMysqlResultSet::getIntField(const char* colName, int nullValue/*=0*/)
{
    if (mysqlRes_ == NULL || mysqlRow_ == NULL || colName == NULL)
        return nullValue;

    const char* field = getStringField(colName);
    return field == NULL ? nullValue : atoi(field);
}

double CppMysqlResultSet::getFloatField(int col, double nullValue/*=0.0*/)
{
    const char* field = getStringField(col);
    return field == NULL ? nullValue : atol(field);
}

double CppMysqlResultSet::getFloatField(const char* colName, double nullValue/*=0.0*/)
{
    const char* field = getStringField(colName);
    return field == NULL ? nullValue : atol(field);
}

const char* CppMysqlResultSet::getStringField(int col, const char* nullValue/*=""*/)
{
    if (mysqlRes_ == NULL || mysqlRow_ == NULL)
        return nullValue;

    if (col + 1 > fields_)
        return nullValue;

    return mysqlRow_[col];
}

const char* CppMysqlResultSet::getStringField(const char* colName, const char* nullValue/*=""*/)
{
    if (mysqlRes_ == NULL || mysqlRow_ == NULL)
        return nullValue;

    int nField = fieldIndex(colName);
    if (nField == -1)
        return nullValue;

    return getStringField(nField);
}


/// ####################### CppMysqlClient ####################### ///
#define SET_ERROR_INFO_AND_RETURN_IF_FAIL(expr, valueIfFailure)  \
    if(!expr)                                \
    {                                        \
        errorCode_ = mysql_errno(mysql_);    \
        errorMsg_ = mysql_error(mysql_);     \
        return valueIfFailure;               \
    }                                        \
    else                                     \
    {                                        \
        errorCode_ = 0, errorMsg_ = "";      \
    }

#define SET_ERROR_INFO_AND_RETURN(expr, valueIfFailure, valueIfSuccess)  \
    if(!expr)                                \
    {                                        \
        errorCode_ = mysql_errno(mysql_);    \
        errorMsg_ = mysql_error(mysql_);     \
        return valueIfFailure;               \
    }                                        \
    else                                     \
    {                                        \
        errorCode_ = 0, errorMsg_ = "";      \
        return valueIfSuccess;               \
    }

CppMysqlClient::CppMysqlClient()
{
    mysql_ = NULL;
    errorCode_ = 0;
    errorMsg_ = "";
}

CppMysqlClient::~CppMysqlClient()
{
    close();
}

/// @brief 创建数据库连接
bool CppMysqlClient::connect(const char* host,
                             unsigned int port,
                             const char* user,
                             const char* passwd,
                             const char* db,
                             const char* charset/* = NULL*/,
                             unsigned long client_flag/*= 0*/)
{
    mysql_ = mysql_init(NULL);
    SET_ERROR_INFO_AND_RETURN_IF_FAIL(mysql_, false);

    char value = 1;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, (char *)&value);

    MYSQL* mysql = mysql_real_connect(mysql_, host, user, passwd, db, port, NULL, client_flag);
    SET_ERROR_INFO_AND_RETURN_IF_FAIL(mysql, false);

    if(charset && mysql_set_character_set(mysql_, charset) != 0)
    {
        int ret = mysql_set_character_set(mysql_, charset);
        SET_ERROR_INFO_AND_RETURN_IF_FAIL(ret != 0, false);
    }

    return true;
}

/// @brief 关闭数据库连接
void CppMysqlClient::close()
{
    if (mysql_ != NULL)
    {
        mysql_close(mysql_);
        mysql_ = NULL;
    }
}

/// @brief 服务器是否存活
bool CppMysqlClient::ping()
{
    int ret = mysql_ping(mysql_);
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);  // 如果ret == 0, return true
}

/// @brief 关闭mysql服务器
bool CppMysqlClient::shutdown()
{
    //return mysql_shutdown(mysql_, SHUTDOWN_DEFAULT) == 0;
    int ret = mysql_shutdown(mysql_, SHUTDOWN_DEFAULT);
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);
}

/// @brief 切换到某一数据库
bool CppMysqlClient::selectDB(const char* dbname)
{
    if (!mysql_)
    {
        return false;
    }
    int ret = mysql_select_db(mysql_, dbname);
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);
}

/// @brief 创建新的数据库
bool CppMysqlClient::createDB(const char* dbname)
{
    char sql[1024] = { 0 };
    sprintf(sql, "CREATE DATABASE %s", dbname);
    int ret = mysql_real_query(mysql_, sql, strlen(sql));
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);
}

/// @brief 删除数据库
bool CppMysqlClient::dropDB(const char* dbname)
{
    char sql[1024] = { 0 };
    sprintf(sql, "DROP DATABASE %s", dbname);
    int ret = mysql_real_query(mysql_, sql, strlen(sql));
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);
}

/// @brief 设置字符集
bool CppMysqlClient::setCharset(const char* charset)
{
    int ret = mysql_set_character_set(mysql_, charset);
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);
}

/// @brief 获取当前连接使用的字符集
const char* CppMysqlClient::getCharset()
{
    return mysql_character_set_name(mysql_);
}


/// @brief 执行非返回结果SQL
int CppMysqlClient::execSQL(const char* sql)
{
    try
    {
        int ret = mysql_real_query(mysql_, sql, strlen(sql));
        SET_ERROR_INFO_AND_RETURN(ret == 0, -1, (int)mysql_affected_rows(mysql_));  // 返回受影响的行数
    }
    catch (...)
    {
        SET_ERROR_INFO_AND_RETURN(false, -1, -1);
    }
}

/// @brief 执行查询语句
int CppMysqlClient::querySQL(const char *sql, CppMysqlResultSet& resultSet)
{
    try
    {
        if (!mysql_real_query(mysql_, sql, strlen(sql)))
        {
            resultSet.mysqlRes_ = mysql_store_result( mysql_ );
            resultSet.mysqlRow_ =  mysql_fetch_row( resultSet.mysqlRes_ );
            resultSet.rows_ = mysql_num_rows( resultSet.mysqlRes_ );
            //得到字段数量
            resultSet.fields_ = mysql_num_fields( resultSet.mysqlRes_ );

            return resultSet.rows_;
        }
        else
        {
            SET_ERROR_INFO_AND_RETURN(false, -1, -1);
        }
    }
    catch(...)
    {
        return -1;
    }
}


/// @brief 开始事务
bool CppMysqlClient::startTransaction()
{
    int ret = mysql_real_query(mysql_, "START TRANSACTION", strlen("START TRANSACTION"));
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);  // 成功返回true
}

/// @brief 提交事务
bool CppMysqlClient::commit()
{
    int ret = mysql_real_query(mysql_, "COMMIT", strlen("COMMIT"));
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);  // 成功返回true
}

/// @brief 回滚事务
bool CppMysqlClient::rollback()
{
    int ret = mysql_real_query(mysql_, "ROLLBACK", strlen("ROLLBACK"));
    SET_ERROR_INFO_AND_RETURN(ret == 0, false, true);  // 成功返回true
}


/// @brief 获取客户端信息
const char* CppMysqlClient::getClientInfo()
{
    return mysql_get_client_info();
}

/// @brief 获取远程服务器主机信息
const char* CppMysqlClient::getHostInfo()
{
    return mysql_get_host_info(mysql_);
}
