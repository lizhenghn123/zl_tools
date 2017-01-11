#ifndef ZL_CPP_MYSQL_H
#define ZL_CPP_MYSQL_H
#include "mysql/mysql.h"
#include <string>

// see /usr/include/mysql/mysql.h
// typedef struct st_mysql MYSQL
// typedef struct st_mysql_res MYSQL_RES
// typedef struct st_mysql_field MYSQL_FIELD
// typedef char **MYSQL_ROW;

class CppMysqlClient;

class CppMysqlResultSet
{
    friend class CppMysqlClient;
public:
    CppMysqlResultSet();

    ~CppMysqlResultSet();

public:
    /// @brief 返回结果集中有多少行
    unsigned long rows() const
    {
        return rows_;
    }

    /// @brief 返回结果集中有多少列
    int fields() const
    {
        return fields_;
    }

    /// @brief 释放数据集
    void freeRes();

    /// @brief 是否还有记录
    bool eof();

    /// @brief 指向下一数据记录行
    void nextRow();

    /// @brief 跳过指定偏移量(类似fseek)
    unsigned long seekRow(unsigned long offerset);

    /// @brief 根据指定的列名返回该列在结果集中的列索引
    /// @param [in] colName : 列名
    /// @return 该列在结果集中位于第几列, 有效范围[0, fields_), 返回-1表示该列名不存在于结果集中
    int fieldIndex(const char* colName);

    /// @brief 根据列索引返回该列的列名
    /// @param [in] col : 列索引, 有效范围[0, fields_)
    /// @return 列名, 返回NULL表示找不到
    const char* fieldName(int col);

    /// @brief 根据列索引获取该列的值(整数值), 无法获取时返回默认值nullValue
    /// @param [in] col : 列索引
    /// @param [in] nullValue : 查找失败时返回的默认值
    int getIntField(int col, int nullValue = 0);

    /// @brief 根据列名获取该列的值(整数值), 无法获取时返回默认值nullValue
    /// @param [in] colName : 列名
    /// @param [in] nullValue : 查找失败时返回的默认值
    int getIntField(const char* colName, int nullValue = 0);

    /// @brief 根据列索引获取该列的值(浮点值), 无法获取时返回默认值nullValue
    /// @param [in] col : 列索引
    /// @param [in] nullValue : 查找失败时返回的默认值
    double getFloatField(int col, double nullValue = 0.0);

    /// @brief 根据列名获取该列的值(浮点值), 无法获取时返回默认值nullValue
    /// @param [in] colName : 列名
    /// @param [in] nullValue : 查找失败时返回的默认值
    double getFloatField(const char* colName, double nullValue = 0.0);

    /// @brief 根据列索引获取该列的值(字符串), 无法获取时返回默认值nullValue
    /// @param [in] col : 列索引
    /// @param [in] nullValue : 查找失败时返回的默认值
    const char* getStringField(int col, const char* nullValue = "");

    /// @brief 根据列索引获取该列的值(字符串), 无法获取时返回默认值nullValue
    /// @param [in] colName : 列名
    /// @param [in] nullValue : 查找失败时返回的默认值
    const char* getStringField(const char* colName, const char* nullValue = "");

    const unsigned char* getBlobField(int nField, int& nLen);
    const unsigned char* getBlobField(const char* colName, int& nLen);

    bool fieldIsNull(int nField);
    bool fieldIsNull(const char* colName);

private:
    CppMysqlResultSet(const CppMysqlResultSet& rQuery);
    CppMysqlResultSet& operator=(const CppMysqlResultSet& rQuery);

private:
    MYSQL_RES*          mysqlRes_;
    MYSQL_FIELD*        mysqlField_;
    MYSQL_ROW           mysqlRow_;
    unsigned long       rows_;
    unsigned int        fields_;
};

class CppMysqlClient
{
public:
    CppMysqlClient();
    ~CppMysqlClient();

public:
    int errorCode() const
    {
        return errorCode_;
    }

    const char* errorMsg() const
    {
        return errorMsg_.c_str();
    }

public:
    /// ========  数据库连接、关闭、创建、删除操作 ======== ///

    /// @brief 创建数据库连接
    /// @return true(ok) or false(failure)
    bool connect(const char* host,
                 unsigned int port,
                 const char* user,
                 const char* passwd,
                 const char* db,
                 const char* charset = 0,   //GBK or UTF8 or 0
                 unsigned long client_flag = 0);

    /// 关闭数据库连接
    void close();

    /// @brief 关闭mysql服务器(慎用,直接关闭了mysql服务)
    /// @return true(ok) or false(failure)
    bool shutdown();

    /// @brief 服务器是否存活
    /// @return true(ok) or false(failure)
    bool ping();

    /// @brief 返回数据库连接(通常不应该使用)
    MYSQL* mysql()
    {
        return mysql_;
    }

    /// @brief 设置字符集
    /// @param [in] charset : "GBK" | "gbk" | "UTF8" | "utf8"
    /// @return true(ok) or false(failure)
    bool setCharset(const char* charset);

    /// @brief 获取当前连接使用的字符集
    const char* getCharset();

    /// @brief 切换到某一数据库
    bool selectDB(const char* dbname);

    /// @brief 创建新的数据库
    bool createDB(const char* dbname);

    /// @brief 删除数据库
    bool dropDB(const char* dbname);


    /// ========  数据库CURD(增删改查)操作 ======== ///
    /// @brief 执行非返回结果SQL(增加、删除、更改等)
    /// @param [in] sql 待执行的sql查询语句
    /// @return 返回影响的行数, 返回-1表示失败
    int execSQL(const char* sql);

    /// @brief 执行查询SQL
    /// @param [in] sql 待执行的sql查询语句
    /// @param [in] data 输入的语音数据
    /// @param [in] len 语音长度
    /// @param [out] resultSet 返回的查询集
    /// @return 返回影响的行数, 返回-1表示失败
    int querySQL(const char *sql, CppMysqlResultSet& resultSet);


    /// ========  事务相关操作 ======== ///
    /// @brief 开始事务
    bool startTransaction();

    /// @brief 提交事务
    bool commit();

    /// @brief 回滚事务
    bool rollback();

    /// ========  其他辅助操作 ======== ///
    /// @brief 获取客户端信息
    const char* getClientInfo();

    /// @brief 获取远程服务器主机信息
    const char* getHostInfo();

private:
    CppMysqlClient(const CppMysqlClient& client);
    CppMysqlClient& operator=(const CppMysqlClient& client);

    MYSQL* mysql_;
    int errorCode_;
    std::string errorMsg_;
};

#endif  /// ZL_CPP_MYSQL_H
