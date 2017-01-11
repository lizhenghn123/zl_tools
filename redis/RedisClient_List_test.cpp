#include <stdio.h>
#include <iostream>
#include <iterator>
//#include <ostream_iterator>
#include <vector>
#include <string>
#include "RedisClient.h"
using namespace std;

zl::redis::RedisClient client;

void check(bool ret)
{
    if(!ret)
    {
        printf("Error : %s\n", client.reason());
    }
}

void test_list();

int main(int argc, char *argv[])
{
    client.connect("127.0.0.1", 6379, 5000);
    printf("connectd [%s], reason [%s]\n", client.connected() ? "success" : "failure", client.reason());

    printf("ping server and return %d\n", client.ping());

    test_list();
    return 0;
}


void test_llen(const char* key)
{
    printf("list(%s) len = %d\n", key, client.llen(key));
}

void test_pop(const char* key, bool popFromLeft)
{
    std::string val;
    //while(true)
    {
        bool ret = popFromLeft ? client.lpop(key, val) : client.rpop(key, val);
        if(ret)
        {
            printf("get key(%s), value = %s\n", key, val.c_str());
        }
        else
        {
            printf("get key(%s) error, reason : %s\n", key, client.reason());
            //break;
        }
    }
}

void test_lrem()
{
    cout << "######   test_lrem   ######\n";
    bool ret = true;
    ret = client.rpush("test_lrem", "hello");
    check(ret);
    ret = client.rpush("test_lrem", "world");
    check(ret);
    ret = client.rpush("test_lrem", "hello");
    check(ret);
    ret = client.rpush("test_lrem", "bar");
    check(ret);
    ret = client.rpush("test_lrem", "world");
    check(ret);

    ret = client.lrem("test_lrem", -2, "hello");
    check(ret);
    ret = client.lrem("test_lrem", 1, "world");
    check(ret);

    vector<string> lists;
    ret = client.lrange("test_lrem", 0, -1, lists);
    check(ret);

    std::copy(lists.begin(), lists.end(), std::ostream_iterator<string>(cout, " "));
    cout << "\n";

    ret = client.lrem("test_lremnoexist", -1, "china");
    check(ret == 0);
}

void test_lset()
{
    cout << "######   test_lset   ######\n";
    bool ret = false;
    ret = client.rpush("list1", "hello");
    check(ret);
    ret = client.rpush("list1", "world");
    check(ret);
    ret = client.rpush("list1", "foo");
    check(ret);
    ret = client.rpush("list1", "bar");
    check(ret);

    ret = client.lset("list1", 0, "none");
    check(ret);
    ret = client.lset("list1", -1, "china");
    check(ret);

    vector<string> lists;
    ret = client.lrange("list1", 0, -1, lists);
    check(ret);

    std::copy(lists.begin(), lists.end(), std::ostream_iterator<string>(cout, " "));
    cout << "\n";

    ret = client.lset("listnoexist", -1, "china");
    check(ret == 0);
}

void test_ltrim()
{
    cout << "######   test_ltrim   ######\n";
    bool ret = false;
    ret = client.rpush("test_ltrim", "hello");
    check(ret);
    ret = client.rpush("test_ltrim", "world");
    check(ret);
    ret = client.rpush("test_ltrim", "foo");
    check(ret);
    ret = client.rpush("test_ltrim", "bar");
    check(ret);

    ret = client.ltrim("test_ltrim", 1, -1);
    check(ret);

    vector<string> lists;
    ret = client.lrange("list1", 0, -1, lists);
    check(ret);

    std::copy(lists.begin(), lists.end(), std::ostream_iterator<string>(cout, " "));
    cout << "\n";

    ret = client.ltrim("test_ltrimnoexist", 1, -1);
    check(ret);
}

void test_bpop(const char* key, bool popFromLeft)
{
    bool ret;
    client.lpush(key, "hello");
    std::pair<std::string, std::string> pairs;
    ret = popFromLeft ? client.blpop(key, 3, pairs) : client.brpop(key, 3, pairs);
    check(ret);
    if(ret)
        cout << pairs.first << "\t" << pairs.second << "\n";
    cout << "-----\n\n";

    ret = popFromLeft ? client.blpop(key, 3, pairs) : client.brpop(key, 3, pairs);
    check(ret);
    if(ret)
        cout << pairs.first << "\t" << pairs.second << "\n";
    cout << "-----\n\n";

    ret = popFromLeft ? client.blpop("xxxxxxlist1", 3, pairs) : client.brpop("xxxxxxlist1", 3, pairs);
    check(ret);
    if(ret)
        cout << pairs.first << "\t" << pairs.second << "\n";
    cout << "-----\n\n";

    std::string value;
    client.lpush("xxxxxx", "fgh");
    ret = client.brpoplpush("xxxxxx", "x2", 3, value);
    check(ret);
    if(ret)
        cout << value << "\n";
}

void test_list()
{
    test_bpop("test_bpop", true);
    //return ;
    bool ret = false;
    ret = client.lpush("hello", "1");
    check(ret);
    ret = client.lpush("world", "wor");
    ret = client.lpush("world", "ld");
    check(ret);
    ret = client.lpushx("noexist", "ld");
    check(ret);
    ret = client.rpushx("noexist", "ld");
    check(ret);

    test_llen("hello");
    test_llen("world");
    test_llen("noexist");
    test_llen(NULL);

    test_pop("hello", true);
    test_pop("world", true);
    test_pop("noexist", true);
    test_pop(NULL, true);

    test_pop("hello", false);
    test_pop("world", false);
    test_pop("noexist", false);
    test_pop(NULL, false);

    test_lrem();

    test_lset();

    test_ltrim();

    test_bpop("test_bpop", true);
    test_bpop("test_bpop", false);
}