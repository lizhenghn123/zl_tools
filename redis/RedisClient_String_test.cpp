#include <stdio.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <assert.h>
#include <string.h>
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

void test_string();

int main(int argc, char *argv[])
{
    client.connect("127.0.0.1", 6379, 5000);
    printf("connectd [%s], reason [%s]\n", client.connected() ? "success" : "failure", client.reason());

    printf("ping server and return %d\n", client.ping());

    test_string();
    return 0;
}


void test_set_get()
{
    cout << "######   test_set_get   ######\n";
    const char* hellokey = "hello";
    bool ret;
    ret = client.set(hellokey, "bar");
    check(ret);

    std::string v;
    ret = client.get(hellokey, v);
    check(ret);
    cout << "key = " << hellokey << ", value = " << v << "\n";

    v.clear();
    ret = client.get("non-exist", v);
    check(ret);
    cout << "key = " << "non-exist" << ", value = " << v << "\n";

    client.hset("test_set_get", "field1", "1234");
    v.clear();
    ret = client.get("non-exist", v);
    check(ret);
    cout << "key = " << "test_set_get" << ", value = " << v << "\n";

    ret = client.set(hellokey, "foo");
    check(ret);
    v.clear();
    ret = client.get(hellokey, v);
    check(ret);
    cout << "key = " << hellokey << ", value = " << v << "\n";

    ret = client.setnx(hellokey, "newfoo");
    check(ret);
    v.clear();
    ret = client.get(hellokey, v);
    check(ret);
    assert(std::string("foo") == v);

    ret = client.append(hellokey, "123");
    check(ret);
    v.clear();
    ret = client.get(hellokey, v);
    check(ret);
    assert(std::string("foo123") == v);

    const char* newhellokey = "newhello";
    client.del(newhellokey);
    ret = client.append(newhellokey, "1111");
    check(ret);
    v.clear();
    ret = client.get(newhellokey, v);
    check(ret);
    assert(std::string("1111") == v);

    std::string oldvalue;
    ret = client.getset(newhellokey, "2222", oldvalue);
    check(ret);
    v.clear();
    ret = client.get(newhellokey, v);
    check(ret);
    assert(std::string("2222") == v);
    assert(std::string("1111") == oldvalue);

    oldvalue="";
    client.del("secondhello");
    ret = client.getset("secondhello", "1234", oldvalue);
    check(ret);
    v.clear();
    ret = client.get("secondhello", v);
    check(ret);
    assert(std::string("1234") == v);
    cout << oldvalue << "\n";
    assert(std::string("") == oldvalue);
}

void test_setex()
{
    cout << "######   test_setex   ######\n";
    bool ret;
    ret = client.setex("hello", "bar", 60);
    check(ret);

    ret = client.setex("hello2", "bar2", 0);
    check(ret);

    ret = client.psetex("hello2", "bar2", 6000);
    check(ret);
}

void test_bit_range()
{
    cout << "######   test_bit_range   ######\n";
    const char* key = "test_bit_range";

    bool ret;
    ret = client.del(key);
    check(ret);

    int32_t value;
    //client.setbit();
    ret = client.setbit(key, 4, 1, &value);
    check(ret);
    assert(value == 0);

    ret = client.getbit(key, 4, &value);
    check(ret);
    assert(value == 1);
    ret = client.getbit(key, 2, &value);
    check(ret);
    assert(value == 0);

    ret = client.setrange(key, 6, "redis");
    check(ret);
    std::string s;
    ret = client.getrange(key, 6, -1, s);
    check(ret);
    cout << s << "\n";

    s.clear();
    ret = client.get(key, s);
    check(ret);
    cout << s << "\n";
}

void test_mset_mget()
{
    cout << "######   test_mset_mget   ######\n";
    client.del("key1");
    client.del("key2");
    client.del("key3");

    vector<pair<string, string> > data;
    data.push_back(make_pair<string, string>("key1", "value1"));
    data.push_back(make_pair<string, string>("key2", "value2"));
    data.push_back(make_pair<string, string>("key3", "1234567"));

    bool ret;
    ret = client.mset(data);
    check(ret);

    data.clear();
    data.push_back(make_pair<string, string>("key3", "1234567"));
    data.push_back(make_pair<string, string>("key4", "1111111"));
    data.push_back(make_pair<string, string>("key5", "value5"));
    ret = client.msetnx(data);
    check(ret);

    std::vector<string> keys;
    keys.push_back("key1");
    keys.push_back("key2");
    keys.push_back("key3");

    std::vector<string> values;
    ret = client.mget(keys, values);
    check(ret);
    copy(values.begin(), values.end(), ostream_iterator<string>(cout, "\n"));
    cout << "\n";

    keys.push_back("key4");
    values.clear();
    ret = client.mget(keys, values);
    check(ret);
    copy(values.begin(), values.end(), ostream_iterator<string>(cout, "\n"));
    cout << "\n";
}

void test_string()
{
    test_set_get();
    test_setex();

    {    // strlen
        bool ret;
        ret = client.set("test_len", "foo+bar$123%");
        check(ret);
        int64_t len = client.strlen("test_len");
        assert(len == strlen("foo+bar$123%"));

        len = client.strlen("no345dg34534f");
        assert(len == 0);
    }
    {   // incr decr
        bool ret;
        ret = client.set("test_incr_decr", "12");
        check(ret);
        int64_t i = 0;
        ret = client.incr("test_incr_decr", &i);
        check(ret);
        assert(i == 12 + 1);
        ret = client.incrby("test_incr_decr", 1, &i);
        check(ret);
        assert(i == 12 + 1 + 1);
        ret = client.incrby("test_incr_decr", 100, NULL);
        check(ret);

        std::string v;
        ret = client.get("test_incr_decr", v);
        check(ret);
        assert(v == std::string("114"));

        ret = client.set("test_incr_decr2", "1.2");
        float f;
        client.incrbyfloat("test_incr_decr2", 3.4, &f);
        assert(f - 4.6 <= 0.0000001 || f - 4.6 >= -0.0000001);


        client.decr("test_incr_decr", &i);
        assert(i == 113);
        client.decrby("test_incr_decr", 13, &i);
        assert(i == 100);
    }

    test_mset_mget();
    test_bit_range();
}