/*************************************************************************
	File Name   : test_hmget.cpp
	Author      : LIZHENG
	Mail        : lizhenghn@gmail.com
 ************************************************************************/
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <hiredis/hiredis.h>
using namespace std;

bool getError(redisContext* context)
{
    if (context == NULL)
    {
        printf("context is null\n");
        return true;
    }
    if (context->err != 0)
    {
        printf("connect error: %s\n", context->errstr);
        return true;
    }

    return false;
}

int main()
{
    int timeout = 5000;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout * 1000;

    //redisContext* c = redisConnectWithTimeout("127.0.0.1", 6379, tv);
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (getError(c))
    {
        if (c)
        {
            redisFree(c);
            c = NULL;
        }
        return 0;
    }

    //if (c->err) {
    //    printf("connect error: %s\n", c->err);
    //    redisFree(c);
    //    return 0;
    //}

    const char* command1 = "set stest1 value1";
    redisReply* r = (redisReply*)redisCommand(c,command1);
    if (!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK") == 0)) {
        printf("Failed to execute command[%s].\n",command1);
        freeReplyObject(r);
        redisFree(c);
        return 0;
    }
    freeReplyObject(r);
    printf("Succeed to execute command[%s].\n",command1);

    //command1 = "HMSET test_hmset_hmget field1 foo field2 12345 field3 bar";
    command1 = "field1 foo field2 12345 field3 bar";
    r = (redisReply*)redisCommand(c, "HMSET test_hmset_hmget %s", command1);
    printf("hmset %s\n", r->str);
    if (!(r->type == REDIS_REPLY_STATUS && strcasecmp(r->str,"OK") == 0)) {
        printf("Failed to execute command[%s].\n",command1);
        freeReplyObject(r);
        redisFree(c);
        return 0;
    }
}
