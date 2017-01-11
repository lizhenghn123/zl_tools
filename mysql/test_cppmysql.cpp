#include <stdio.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <unistd.h>
#include "CppMysql.h"
using namespace std;

void printQuerySet(const char* sql, CppMysqlResultSet& query)
{
    printf("query(%s) : row = %d, fields = %d\n", sql, query.rows(), query.fields());
    for (int i = 0; i < query.rows(); i++)
    {
        std::cout << "row #" << i << " : ";
        for (int j = 0; j < query.fields(); j++)
        {
            std::cout << query.getStringField(j) << ", \t";
        }
        std::cout << "\n";
        query.nextRow();
    }
}

int main()
{
    CppMysqlClient mysql;
    int ret = mysql.connect("192.168.14.202", 3306, "root", "TBNR$0987", "wuxi" );
    printf("ret(open) = %d\n", ret);

    assert(mysql.mysql());

    ret = mysql.setCharset("UTF8");
    printf("ret(open) = %d\n", ret);

    std::string sql;
    CppMysqlResultSet query;
    {
        sql = "select count(*) from wxlog";
        ret = mysql.querySQL(sql.c_str(), query);
        printf("ret(query) = %d\n", ret);
        printQuerySet(sql.c_str(), query);
    }

    {
        sql = "select * from wxlog";
        ret = mysql.querySQL(sql.c_str(), query);
        printf("ret(query) = %d\n", ret);
        printQuerySet(sql.c_str(), query);
    }

    {
        sql = "update wxlog set log_username = 'lizheng' where log_id = 776";
        ret = mysql.execSQL(sql.c_str());
        printf("ret(execSQL) = %d\n", ret);
        sql = "select log_id,log_datetime,log_username from wxlog where log_id = 776";
        ret = mysql.querySQL(sql.c_str(), query);
        printf("ret(query) = %d\n", ret);
        printQuerySet(sql.c_str(), query);
    }

    printf("getCharset = %s\n", mysql.getCharset());
    printf("getClientInfo = %s\n", mysql.getClientInfo());
    printf("getHostInfo = %s\n", mysql.getHostInfo());
    printf("game over\n");
}