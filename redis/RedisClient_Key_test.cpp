#include <stdio.h>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>
#include <assert.h>
#include "RedisClient.h"
using namespace std;

zl::redis::RedisClient client;

void check(bool ret, const char* file, int line)
{
    if(!ret)
    {
        printf("Error : %s at %s:%d\n", client.reason(), file, line);
    }
}

#define CHECK(ret)    check(ret, __FILE__, __LINE__)

void test_key();

int main(int argc, char *argv[])
{
    client.connect("127.0.0.1", 6379, 5000);
    printf("connectd [%s], reason [%s]\n", client.connected() ? "success" : "failure", client.reason());
    printf("ping server and return %d\n", client.ping());

    test_key();
    return 0;
}

void test_expire()
{
    const char* key = "hello";
    bool ret;
    ret = client.del(key, NULL);

    ret = client.set(key, "world");
    CHECK(ret);

    ret = client.expire(key, 10);
    CHECK(ret);

    int64_t v;
    ret = client.ttl(key, v);
    CHECK(ret);
    cout << "[" << key << "] expire time " << v << " seconds\n";
    ret = client.pttl(key, v);
    CHECK(ret);
    cout << "[" << key << "] expire time " << v << " milliseconds\n";

    client.del(key, NULL);
    ret = client.ttl(key, v);
    CHECK(ret);
    cout << "after delete, [" << key << "] expire time " << v << " seconds\n";

    key = "non-expire-key";
    client.set(key, "foo");
    ret = client.ttl(key, v);
    CHECK(ret);
    cout << "[" << key << "] expire time " << v << " seconds\n";

    ret = client.persist(key);
    CHECK(ret);
}

void test_key()
{
    {
        client.set("test_type", "1234");
        zl::redis::REDIS_KEY_TYPE type;
        bool ret = client.type("test_type", type);
        CHECK(ret);
        cout << "====" << type << "\n";
    }
    return;
    {
        const char* key = "hello";
        bool ret;
        int64_t count = 0;
        ret = client.del(key, &count);
        CHECK(ret);
        CHECK(count == 0);

        ret = client.exist(key);
        assert(!ret);

        client.set(key, "12345");
        client.del("newhello");
        ret = client.rename(key, "newhello");
        CHECK(ret);

        ret = client.exist(key);
        assert(!ret);

        ret = client.exist("newhello");
        CHECK(ret);
        assert(ret);

        // 如果原key不存在，则返回错误
        ret = client.rename("non-exist-key", "newkey");
        CHECK(ret);
        assert(!ret);

        client.set("foo", "bar");
        // newkey 已存在时， RENAME 会覆盖旧 newkey
        ret = client.rename("foo", "newhello");
        CHECK(ret);

        std::string v;
        ret = client.get("newhello", v);
        CHECK(ret);
        assert(v == "bar");

        client.set("fish", "five");
        // newkey 已存在时， RENAMENX修改失败
        ret = client.renamenx("fish", "newhello");
        assert(!ret);

        assert(client.exist("fish"));

        v.clear();
        ret = client.get("newhello", v);
        CHECK(ret);
        assert(v == "bar");

        // newkey 不存在时， RENAMENX修改成功
        client.del("food");
        ret = client.renamenx("fish", "food");
        CHECK(ret);
        v.clear();
        ret = client.get("food", v);
        CHECK(ret);
        assert(v == "five");

        v.clear();
        ret = client.dump("food", v);
        CHECK(ret);
        cout << v << "\n";
    }

    test_expire();

    return;
}