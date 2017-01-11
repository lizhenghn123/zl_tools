#include <stdio.h>
#include <iostream>
#include <iterator>
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

void test_hash();



int main(int argc, char *argv[])
{
    client.connect("127.0.0.1", 6379, 5000);
    printf("connectd [%s], reason [%s]\n", client.connected() ? "success" : "failure", client.reason());
    printf("ping server and return %d\n", client.ping());

    test_hash();
    return 0;
}

void printMap(const map<string, string>& m)
{
    cout << "{\n";
    for(map<string, string>::const_iterator it = m.begin(); it != m.end(); ++it)
    {
        cout << "  " << it->first << " : \t" << it->second << "\n";
    }
    cout << "}\n";
}

namespace detail
{
    void test_hsetnx(const char* key, const char* field, const char* value)
    {
        cout << "######   test_hsetnx   ######\n";
        bool ret = false;
        uint32_t val = 0;
        ret = client.hsetnx(key, field, value, &val);
        check(ret);
        cout << ((val == 0) ? "no op, because field already exist" : "set success") << "\n";

        map<string, string> infos;
        ret = client.hgetall(key, infos);
        check(ret);
        printMap(infos);
    }

    void test_hset(const char* key, const char* field, const char* value)
    {
        cout << "######   test_hset   ######\n";
        bool ret = false;
        ret = client.hset(key, field, value);
        check(ret);

        map<string, string> infos;
        ret = client.hgetall(key, infos);
        check(ret);
        printMap(infos);
    }

    void test_hget(const char* key, const char* field)
    {
        cout << "######   test_hget   ######\n";
        string value;
        bool ret = false;
        ret = client.hget(key, field, value);
        check(ret);
        cout << "hget : (" << key << ", " << field << ", " << value << ")\n";
    }

    void test_hdel(const char* key, const char* field)
    {
        cout << "######   test_hdel   ######\n";
        string value;
        bool ret = false;
        uint32_t u = 0;
        ret = client.hdel(key, field, &u);
        check(ret);
        if(ret)
        {
            cout << "hdel success : (" << key << ", " << field << ", " << u << ")\n";
        }
        map<string, string> infos;
        ret = client.hgetall(key, infos);
        check(ret);
        printMap(infos);
    }

    void test_hexists(const char* key, const char* field)
    {
        bool ret = client.hexists(key, field);
        cout << "(" << key << "," << field << ") " << (ret ? "exist" : "non-exist") << "\n";
        check(ret);
    }

    void test_hkeys_hvals(const char* key)
    {
        bool ret;
        vector<string> keys;
        ret = client.hkeys(key, keys);
        check(ret);
        cout << "hash(" << key << ") keys:\n  ";
        std::copy(keys.begin(), keys.end(), std::ostream_iterator<string>(cout, "\n  "));
        cout << "\n";

        vector<string> values;
        ret = client.hvals(key, values);
        check(ret);
        cout << "hash(" << key << ") values:\n  ";
        std::copy(values.begin(), values.end(), std::ostream_iterator<string>(cout, "\n  "));
        cout << "\n";
    }

}

void test_hset_hget_hdel()
{
    using namespace detail;
    cout << "######   test_hset_hget_hdel   ######\n";
    test_hset("user_lz", "name", "lizheng");
    test_hset("user_lz", "age", "28");
    test_hset("user_lz", "email", "lizhenghn@gmail.com");

    test_hsetnx("user_lz", "name", "zhengli");
    test_hsetnx("user_lz", "address", "BeiJing");

    test_hget("user_lz", "name");
    test_hget("user_lz", "age");
    test_hget("user_lz", "nothing");

    test_hdel("user_lz", "name");
    test_hdel("user_lz", "nothing");
    test_hdel("nonexist", "name");
}

void test_hlen_hstrlen()
{
    bool ret;
    ret = client.hset("myhash", "field1", "foo");
    check(ret);
    ret = client.hset("myhash", "field2", "barbar");
    check(ret);
    ret = client.hset("myhash", "field3", "1234");
    check(ret);

    map<string, string> infos;
    ret = client.hgetall("myhash", infos);
    check(ret);
    printMap(infos);

    uint32_t len = client.hlen("myhash");
    cout << "hash(" << "myhash" << ") len = " << len << "\n";
    len = client.hlen("nonhash");      // key不存在
    cout << "hash(" << "nonhash" << ") len = " << len << "\n";

    len = client.hstrlen("myhash", "field1");
    cout << "hash(" << "myhash, field1" << ") len = " << len << "\n";

    len = client.hstrlen("myhash", "field3");
    cout << "hash(" << "myhash, field3" << ") len = " << len << "\n";

    len = client.hstrlen("myhash", "nonfield");  // key存在，但field不存在
    cout << "hash(" << "myhash, nonfield" << ") len = " << len << "\n";

    len = client.hstrlen("nonhash", "nonfield");  // key不存在
    cout << "hash(" << "nonhash, nonfield" << ") len = " << len << "\n";
}

void test_hincrby()
{
    bool ret;
    std::string v;

    client.hset("test_hincrby", "field1", "2");
    int64_t value = -1;
    ret = client.hincrby("test_hincrby", "field1", 23, &value);
    check(ret);
    cout << "after hincrby, value = " << value << "\n";

    ret = client.hget("test_hincrby", "field1", v);
    check(ret);
    cout << "hget value = " << v << "\n";

    v.clear();
    client.hdel("test_hincrby", "field2");
    ret = client.hget("test_hincrby", "field2", v);
    check(ret);
    cout << "hget value = " << v << "\n";


    client.hset("test_hincrby", "field2", "2.3");
    float f = 0;
    ret = client.hincrbyfloat("test_hincrby", "field2", 4.6, &f);
    check(ret);
    cout << "after hincrbyfloat, value = " << f << "\n";

    v.clear();
    ret = client.hget("test_hincrby", "field2", v);
    check(ret);
    cout << "hget value = " << v << "\n";
}

void test_hmset_hmget()
{
    bool ret;
    std::string v;

    std::map<std::string, std::string> kv;
    kv["field1"] = "foo";
    kv["field2"] = "12345";
    kv["field3"] = "bar";

    ret = client.hmset("test_hmset_hmget", kv);
    check(ret);

    {
        vector<string> keys{"field1", "field3", "field2"};
        vector<string> vals;
        ret = client.hmget("test_hmset_hmget", keys, vals);
        check(ret);
        cout << "hash(" << "test_hmset_hmget" << ") vals:\n  ";
        std::copy(vals.begin(), vals.end(), std::ostream_iterator<string>(cout, "\n  "));
        cout << "\n";
    }
    {
        vector<string> keys{"field1", "field4", "field5", "field3"};
        vector<string> vals;
        ret = client.hmget("test_hmset_hmget", keys, vals);
        check(ret);
        cout << "hash(" << "test_hmset_hmget" << ") vals:\n  ";
        std::copy(vals.begin(), vals.end(), std::ostream_iterator<string>(cout, "\n  "));
        cout << "\n";
    }
    {
        vector<string> keys{"field4", "field5"};
        vector<string> vals;
        ret = client.hmget("test_hmset_hmget", keys, vals);
        check(ret);
        cout << "hash(" << "test_hmset_hmget" << ") vals:\n  ";
        std::copy(vals.begin(), vals.end(), std::ostream_iterator<string>(cout, "\n  "));
        cout << "\n";
    }
}

void test_hash()
{
    using namespace detail;

    //test_hset_hget_hdel();

    //test_hexists("set1", "name");
    //test_hexists("set1", "nothing");
    //test_hexists("noexist", "name");

    //test_hlen_hstrlen();

    //test_hkeys_hvals("myhash");
    //test_hkeys_hvals("non-hash");

    //test_hincrby();

    test_hmset_hmget();

    return;
}