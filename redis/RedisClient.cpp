#include "RedisClient.h"
#include <sys/time.h>
#include <strings.h>
#include <stdlib.h>

namespace zl { namespace redis {


#define SET_ERROR(code)   do                                      \
                    {                                             \
                        errorCode_ = code;                        \
                        errorMsg_ = RedisClient::kErrorDesc[code];\
                    }while(0)

#define JUST_RETUAN_IF_ERROR(expr, ret, msg)  do                  \
                    {                                             \
                        SET_ERROR(REDIS_NO_ERROR);                \
                        if((expr))                                \
                        {                                         \
                            errorCode_ = REDIS_ERROR;             \
                            errorMsg_ = msg;                      \
                            return ret;                           \
                        }                                         \
                    }while(0)

// 错误描述
const char* RedisClient::kErrorDesc[REDIS_BOTTOM] =
{
    "No Error",
    "NULL Pointer",
    "No Connection To The Redis Server",
    "Inser Error, Pivot Not Found.",
    "Key Not Found",
    "Hash Field Not Found",
    "Redis Error Index",
    "Redis Error Nil",
    "Redis Error"
};

RedisClient::RedisClient()
    : rdsContext_(NULL)
    , errorCode_(REDIS_NO_ERROR)
    , errorMsg_("")
    , host_("")
    , port_(0)
    , password_("")
    , timeoutMs_(5000)
    , connected_(false)
{

}

RedisClient::RedisClient(const std::string& ip, short port, int timeoutMs/* = 5000*/)
    : rdsContext_(NULL)
    , errorCode_(REDIS_NO_ERROR)
    , errorMsg_("")
    , host_(ip)
    , port_(port)
    , password_("")
    , timeoutMs_(timeoutMs)
    , connected_(false)
{
     connected_ = connect(ip, port, timeoutMs);
}

RedisClient::~RedisClient()
{
    disConnect();
}

bool RedisClient::getError(redisContext* context)
{
    SET_ERROR(REDIS_NO_ERROR);
    if (context == NULL)
    {
        errorCode_ = REDIS_REPLY_NULL;
        errorMsg_ = kErrorDesc[REDIS_REPLY_NULL];
        SET_ERROR(REDIS_REPLY_NULL);
        return true;
    }
    if (context->err != 0)
    {
        errorCode_ = REDIS_ERROR;
        errorMsg_ = context->errstr;
        return true;
    }

    return false;
}

bool RedisClient::getError(redisReply* reply)
{
    SET_ERROR(REDIS_NO_ERROR);
    if (reply == NULL)
    {
        SET_ERROR(REDIS_REPLY_NULL);
        return true;
    }

    switch(reply->type)
    {
    case REDIS_REPLY_STRING:
        return false;
    case REDIS_REPLY_ARRAY:
        return false;
    case REDIS_REPLY_INTEGER:
        return false;
    case REDIS_REPLY_NIL:
        //printf("==%s==\n", reply->str);
        SET_ERROR(REDIS_NIL);
        return true;
    case REDIS_REPLY_STATUS:
        return false;
        //printf("REDIS_REPLY_STATUS %s\n", reply->str);
        //return (strcasecmp(reply->str,"OK") == 0) ? false : true;
    case REDIS_REPLY_ERROR:
        errorCode_ = REDIS_ERROR;
        errorMsg_ = reply->str;
        return true;
    default:
        return false;
    }
}

bool RedisClient::connect(const std::string& ip, short port, int timeoutMs/* = 5000*/)
{
    if (connected_)
    {
        disConnect();
    }

    struct timeval time;
    time.tv_sec = timeoutMs / 1000;
    time.tv_usec = (timeoutMs % 1000) * 1000;

    redisContext* context = redisConnectWithTimeout(ip.c_str( ), port, time);
    if (getError(context))
    {
        if (context)
        {
            redisFree(context);
            context = NULL;
        }
        return false;
    }

    rdsContext_ = context;
    connected_ = true;
    return true;
}

bool RedisClient::reconnect()
{
    return (connect(host_, port_, timeoutMs_));
}

bool RedisClient::disConnect()
{
    connected_ = false;
    if(rdsContext_)
    {
        redisFree(rdsContext_);
        rdsContext_ = NULL;
    }
    return true;
}

bool RedisClient::auth(const std::string& password)
{
    JUST_RETUAN_IF_ERROR(!connected_, false, kErrorDesc[REDIS_NO_CONNECT]);

    bool ret = false;
    password_ = password;

    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "AUTH %s", password.c_str()));
    ret = !getError(reply);
    if (reply)
    {
        freeReplyObject(reply);
    }

    return ret;
}

bool RedisClient::ping()
{
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);

    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "PING"));
    bool ret = !getError(reply);
    if (reply)
    {
        freeReplyObject(reply);
    }

    return ret;
}


redisReply* RedisClient::redisCmd(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    redisReply* r = static_cast<redisReply *>(redisvCommand(rdsContext_, format, ap));
    va_end(ap);
    return r;
}

bool RedisClient::execCmdBool(const char *format, ...)
{
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    va_list ap;
    va_start(ap, format);
    redisReply* r = static_cast<redisReply *>(redisvCommand(rdsContext_, format, ap));
    va_end(ap);

    bool ret = false;
    if(!getError(r))
    {
        if(r->type == REDIS_REPLY_STATUS)
        {
            ret = true;
        }
        else
        {
            ret = (r->integer == 1);
        }
    }
    freeReplyObject(r);
    return ret;
}

bool RedisClient::execCmdInteger(int64_t* retvalue, const char *format, ...)
{
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    va_list ap;
    va_start(ap, format);
    redisReply* r = static_cast<redisReply *>(redisvCommand(rdsContext_, format, ap));
    va_end(ap);

    bool ret = false;
    if(!getError(r))
    {
        ret = true;
        if(retvalue)
            *retvalue = r->integer;
    }
    freeReplyObject(r);
    return ret;
}

bool RedisClient::execCmdString(std::string* retvalue, const char *format, ...)
{
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    va_list ap;
    va_start(ap, format);
    redisReply* r = static_cast<redisReply *>(redisvCommand(rdsContext_, format, ap));
    va_end(ap);

    bool ret = false;
    if(!getError(r))
    {
        ret = true;
        //std::string v;
        //v.append(r->str, r->len);
        //printf("======%s==%d==%d===%s,\n", v.c_str(), v.size(), r->len, r->str);
        if(retvalue)
            retvalue->assign(r->str, r->len);
    }

    freeReplyObject(r);
    return ret;
}

/// ============            key             ============
bool RedisClient::exist(const char* key)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdBool("EXISTS %s", key);
}

bool RedisClient::del(const char* key, int64_t* count/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdInteger(count, "DEL %s", key);
}

bool RedisClient::dump(const char* key, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdString(&value, "DUMP %s", key);
}

bool RedisClient::rename(const char* key, const char* newkey)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !newkey, false, "key or newkey is null");
    return execCmdBool("RENAME %s %s", key, newkey);
}

bool RedisClient::renamenx(const char* key, const char* newkey)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !newkey, false, "key or newkey is null");
    return execCmdBool("RENAMENX %s %s", key, newkey);
}

bool RedisClient::expire(const char* key, int seconds)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdBool("EXPIRE %s %d", key, seconds);
}

bool RedisClient::expireat(const char* key, int unixtimestamp)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key or newkey is null");
    return execCmdBool("EXPIREAT %s %d", unixtimestamp);
}

bool RedisClient::persist(const char* key)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdBool(" PERSIST %s", key);
}

bool RedisClient::pexpire(const char* key, int milliseconds)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdBool("PEXPIRE %s %d", key, milliseconds);
}

bool RedisClient::pexpireat(const char* key, int millisecondstimestamp)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key or newkey is null");
    return execCmdBool("PEXPIREAT %s %d", millisecondstimestamp);
}

bool RedisClient::ttl(const char* key, int64_t& seconds)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key or newkey is null");
    return execCmdInteger(&seconds, "TTL %s", key);
}

bool RedisClient::pttl(const char* key, int64_t& milliseconds)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key or newkey is null");
    return execCmdInteger(&milliseconds, "PTTL %s", key);
}

bool RedisClient::type(const char* key, REDIS_KEY_TYPE& type)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key or newkey is null");
    std::string result;
    bool ret = execCmdString(&result, "TYPE %s", key);
    if(!ret)
    {
        return ret;
    }

    if(result == "string")
        type = REDIS_KEY_STRING;
    else if(result == "list")
        type = REDIS_KEY_LIST;
    else if(result == "set")
        type = REDIS_KEY_SET;
    else if(result == "zset")
        type = REDIS_KEY_ZSET;
    else if(result == "hash")
        type = REDIS_KEY_HASH;

    return true;
}

/// ============           string           ============
bool RedisClient::set(const char* key, const char* value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");
    return execCmdBool("SET %s %s", key, value);
}

bool RedisClient::setnx(const char* key, const char* value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");
    return execCmdBool("SETNX %s %s", key, value);
}

bool RedisClient::mset(const std::vector<std::pair<std::string, std::string> >& datas)
{
    SET_ERROR(REDIS_NO_ERROR);
    std::string cmd("MSET");
    for(std::vector<std::pair<std::string, std::string> >::const_iterator it = datas.begin(); it != datas.end(); ++it)
    {
        cmd += " ";
        cmd += (*it).first;
        cmd += " ";
        cmd += (*it).second;
    }
    execCmdBool(cmd.c_str());
    return true;
}

bool RedisClient::msetnx(const std::vector<std::pair<std::string, std::string> >& datas)
{
    SET_ERROR(REDIS_NO_ERROR);
    std::string cmd("MSETNX");
    for(std::vector<std::pair<std::string, std::string> >::const_iterator it = datas.begin(); it != datas.end(); ++it)
    {
        cmd += " ";
        cmd += (*it).first;
        cmd += " ";
        cmd += (*it).second;
    }
    return execCmdBool(cmd.c_str());
}

bool RedisClient::setbit(const char* key, uint32_t offset, int32_t value, int32_t* oldvalue)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !(value == 0 || value == 1), false, "key is null or value is not 0/1");
    int64_t v = 0;
    bool ret = execCmdInteger(&v, "SETBIT %s %d %d", key, offset, value);
    if(value)
        *oldvalue = static_cast<int32_t>(v);
    return ret;
}

bool RedisClient::setrange(const char* key, uint32_t offset, const char* value, int64_t* length/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");
    return execCmdInteger(length, "SETRANGE %s %d %s", key, offset, value);
}

bool RedisClient::setex(const char* key, const char* value, uint32_t timeout)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !value|| timeout == 0, false, "key/value is null or timeout is 0");
    return execCmdBool("SETEX %s %d %s", key, timeout, value);
}

bool RedisClient::psetex(const char* key, const char* value, uint32_t timeoutMs)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key || !value|| timeoutMs == 0, false, "key/value is null or timeoutMs is 0");
    return execCmdBool("PSETEX %s %d %s", key, timeoutMs, value);
}

bool RedisClient::get(const char* key, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdString(&value, "GET %s", key);
}

bool RedisClient::mget(const std::vector<std::string>& keys, std::vector<std::string>& values)
{
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);

    std::string command("MGET");
    for(std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
    {
        command += " ";
        command += *it;
    }

    bool ret = false;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, command.c_str()));
    if (getError(reply))
    {
    }
    else
    {
        if(reply->type == REDIS_REPLY_ARRAY)
        {
            ret = true;
            for(std::size_t i = 0; i < reply->elements; i ++)
            {
                values.push_back(reply->element[i]->str ? reply->element[i]->str : "nil");
            }
        }
        else
        {
            errorCode_ = REDIS_NIL;
            errorMsg_ =  reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::getbit(const char* key, uint32_t offset, int32_t* value)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    int64_t v = 0;
    bool ret = execCmdInteger(&v, "GETBIT %s %d", key, offset);
    if(value)
        *value = static_cast<int32_t>(v);
    return ret;
}

bool RedisClient::getrange(const char* key, int64_t start, int64_t end, std::string& value)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdString(&value, "GETRANGE %s %d %d", key, start, end);
}

bool RedisClient::getset(const char* key, const char* newvalue, std::string& oldvalue)
{
    {
        JUST_RETUAN_IF_ERROR(!key, false, "key is null");
        return execCmdString(&oldvalue, "GETSET %s %s", key, newvalue);
    }
}

bool RedisClient::append(const char* key, const char* value, int64_t* length/* = NULL*/)
{
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");
    return execCmdInteger(length, "APPEND %s %s", key, value);
}

int64_t RedisClient::strlen(const char* key)
{
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    int64_t len = 0;
    execCmdInteger(&len, "STRLEN %s", key);
    return len;
}

bool RedisClient::incr(const char* key, int64_t* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdInteger(value, "INCR %s", key);
}

bool RedisClient::incrby(const char* key, int64_t increment, int64_t* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdInteger(value, "INCRBY %s %lld", key, increment);
}

bool RedisClient::incrbyfloat(const char* key, float increment, float* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    //return execCmdInteger(value, "INCRBYFLOAT %s %f", key, increment);

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "INCRBYFLOAT %s %f", key, increment));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if(reply->str && value)
            *value = atof(reply->str);
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::decr(const char* key, int64_t* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdInteger(value, "DECR %s", key);
}

bool RedisClient::decrby(const char* key, int64_t increment, int64_t* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    return execCmdInteger(value, "DECRBY %s %lld", key, increment);
}

/// ============            list            ============
int32_t  RedisClient::llen(const char* key)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, -1, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, -1, "key is null");

    int32_t len = 0;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LLEN %s", key));
    if (getError(reply))
    {
        len = -1;
    }
    else
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            len = reply->integer;
        }
    }

    freeReplyObject(reply);
    return len;
}

bool RedisClient::push(const char* type, const char* xtype, const char* key, const char* value, uint64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");

    bool ret = false;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "%s%s %s %s", type, xtype, key, value));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        ret = true;
        if(retval)
        {
            *retval = reply->integer;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::lpush(const char* key, const char* value, uint64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    return push("LPUSH", "", key, value, retval);
}

bool RedisClient::lpushx(const char* key, const char* value, uint64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    uint64_t num = 0;
    bool ret = push("LPUSH", "X", key, value, &num);
    if(retval)
    {
        *retval = num;
    }
    if(!ret || num == 0)
    {
        return false;
    }

    return ret;
}

bool RedisClient::rpush(const char* key, const char* value, uint64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    return push("RPUSH", "", key, value, retval);
}

bool RedisClient::rpushx(const char* key, const char* value, uint64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    uint64_t num = 0;
    bool ret = push("RPUSH", "X", key, value, &num);
    if(retval)
    {
        *retval = num;
    }
    if(!ret || num == 0)
    {
        return false;
    }

    return ret;
}

bool RedisClient::pop(const char* type, const char* key, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "%s %s", type, key));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->str == NULL)
        {
            errorCode_ = REDIS_NO_KEY;
            errorMsg_ = kErrorDesc[REDIS_NO_KEY];
            ret = false;
        }
        else
        {
            value = reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::lpop(const char* key, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    return pop("LPOP", key, value);
}

bool RedisClient::rpop(const char* key, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    return pop("RPOP", key, value);
}

bool RedisClient::bpop(const char* type, const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs)
{ 
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    std::string value;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "%s %s %d", type, key, timeout));
    if (getError(reply)/* || reply->elements != 2*/)
    {
        ret = false;
        //pairs.first = "NIL";
    }
    else if(reply->elements != 2)
    {
        ret = false;
    }
    else
    {
        pairs.first = reply->element[0]->str;
        pairs.second = reply->element[1]->str;
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::blpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs)
{
    SET_ERROR(REDIS_NO_ERROR);
    return bpop("BLPOP", key, timeout, pairs);
}

bool RedisClient::blpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs)
{
    SET_ERROR(REDIS_NO_ERROR);
    std::string allkeys;
    for(size_t i = 0; i < keys.size() - 1; ++i)
    {
        allkeys += keys[i];
        allkeys += " ";
    }
    allkeys += keys[keys.size()-1];

    return bpop("BLPOP", allkeys.c_str(), timeout, pairs);
}

bool RedisClient::brpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs)
{
    SET_ERROR(REDIS_NO_ERROR);
    return bpop("BRPOP", key, timeout, pairs);
}

bool RedisClient::brpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs)
{
    SET_ERROR(REDIS_NO_ERROR);
    std::string allkeys;
    for(size_t i = 0; i < keys.size() - 1; ++i)
    {
        allkeys += keys[i];
        allkeys += " ";
    }
    allkeys += keys[keys.size()-1];

    return bpop("BRPOP", allkeys.c_str(), timeout, pairs);
}

bool RedisClient::brpoplpush(const char* key, const char* another_key, uint32_t timeout, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !another_key, false, "key or another_keyis null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "BRPOPLPUSH %s %s %d", key, another_key, timeout));
    //printf("=================== %d, %d\n", reply->elements, reply->type);
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        // FIXME 并没有返回两个元素啊，只有reply->str返回的是弹出的元素
        value = reply->str;
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::lindex(const char* key, int32_t index, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LINDEX %s %d", key, index));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type == REDIS_REPLY_NIL)
        {
            errorMsg_ = std::string( kErrorDesc[REDIS_NO_KEY] ) + " or " + kErrorDesc[REDIS_INDEX];
            ret = false;
        }
        else
        {
            value = reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

int32_t  RedisClient::lrem(const char* key, int count, const char* value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");

    int32_t ret = 0;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LREM %s %d %s", key, count, value));
    if (getError(reply))
    {
        ret = 0;
    }
    else
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            ret = reply->integer;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::lset(const char* key, int index, const char* value)
{ 
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !value, false, "key or value is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LSET %s %d %s", key, index, value));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (!(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0))
        {
            errorMsg_ =  reply->str;
            ret = false;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::ltrim(const char* key, int start, int end)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LTRIM %s %d %d", key, start, end));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (!(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0))
        {
            errorMsg_ =  reply->str;
            ret = false;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::lrange(const char* key, uint32_t start, int32_t end, std::vector<std::string>& valueList)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LRANGE %s %d %d", key, start, end));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 0) //<  key是list类型但 start > end
        {
            errorMsg_ = std::string( kErrorDesc[REDIS_INDEX] ) + " or " + kErrorDesc[REDIS_NO_KEY];
            ret = false;
        }
        else
        {
            for(std::size_t i = 0; i < reply->elements; i++)
            {
                valueList.push_back(reply->element[i]->str);
            }
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::linsert(const char* key, const char* insertType, const char* pivot, const char* value, int64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");
    JUST_RETUAN_IF_ERROR(!pivot, false, "pivot is null");
    JUST_RETUAN_IF_ERROR(!value, false, "value is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "LINSERT %s %s %s %s", key, insertType, pivot, value));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            if (reply->integer == -1)
            {
                errorMsg_ = kErrorDesc[REDIS_NO_PIVOT];
                ret = false;
            }
            else if (reply->integer == 0)
            {
                errorMsg_ = kErrorDesc[REDIS_NO_KEY];
                ret = false;
            }
            else
            {
                if(retval)
                {
                    *retval = reply->integer;
                }
            }
        }
        else
        {
            ret = false;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::linsertBefroe(const char* key, const char* pivot, const char* value, int64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    return linsert(key, "BEFORE", pivot, value, retval);
}

bool RedisClient::linsertAfter(const char* key, const char* pivot, const char* value, int64_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    return linsert(key, "AFTER", pivot, value, retval);
}


/// ============            hash            ============
bool RedisClient::hset(const char* key, const char* field, const char* value, uint32_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field || !value, false, "key or field or value is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HSET %s %s %s", key, field, value));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if(retval)
        {
            *retval = reply->integer;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hsetnx(const char* key, const char* field, const char* value, uint32_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field || !value, false, "key or field or value is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HSETNX %s %s %s", key, field, value));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if(retval)
        {
            *retval = reply->integer;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hget(const char* key, const char* field, std::string& value)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field, false, "key or field is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HGET %s %s", key, field));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type == REDIS_REPLY_NIL)
        {
            errorMsg_ = std::string( kErrorDesc[REDIS_NO_KEY] ) + " or " + kErrorDesc[REDIS_NO_FIELD];
            ret = false;
        }
        else
        {
            value = reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hdel(const char* key, const char* field, uint32_t* retval/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field, false, "key or field is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HDEL %s %s", key, field));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 0)
        {
            errorMsg_ = std::string( kErrorDesc[REDIS_NO_KEY] ) + " or " +  kErrorDesc[REDIS_NO_FIELD];
        }
        else
        {
            if(retval)
            {
                *retval = reply->integer;
            }
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hgetall(const char* key, std::map<std::string , std::string>& valueMap)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key, false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HGETALL %s", key));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {

        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 0)
        {
            errorMsg_ = kErrorDesc[REDIS_NO_KEY];
            ret = false;
        }
        else
        {
            for(std::size_t i = 0; i < reply->elements; i += 2)
            {
                valueMap.insert(std::make_pair(reply->element[i]->str, reply->element[i + 1]->str));
            }
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hexists(const char* key, const char* field)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field, false, "key or field is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HEXISTS %s %s", key, field));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type != REDIS_REPLY_INTEGER)
        {
            errorMsg_ = std::string( kErrorDesc[REDIS_NO_KEY] ) + " or " + kErrorDesc[REDIS_NO_FIELD];
            ret = false;
        }
        else
        {
            ret = (reply->integer == 1);
        }

    }
    freeReplyObject(reply);
    return ret;
}

uint32_t RedisClient::hlen(const char* key)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key , false, "key is null");

    uint32_t ret = 0;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HLEN %s", key));
    if (getError(reply))
    {
        ret = 0;
    }
    else
    {
        if(reply->type != REDIS_REPLY_INTEGER)
        {
            errorMsg_ = kErrorDesc[REDIS_NO_KEY];
        }
        else
        {
            ret = reply->integer;
        }
    }

    freeReplyObject(reply);
    return ret;
}

uint32_t RedisClient::hstrlen(const char* key, const char* field)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key , false, "key is null");

    uint32_t ret = 0;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HSTRLEN %s %s", key, field));
    if (getError(reply))
    {
        ret = 0;
    }
    else
    {
        if(reply->type != REDIS_REPLY_INTEGER)
        {
            errorMsg_ = kErrorDesc[REDIS_NO_KEY];
        }
        else
        {
            ret = reply->integer;
        }
    }
    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hkeysOrvals(const char* type, const char* key, std::vector<std::string>& lists)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key , false, "key is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "%s %s", type, key));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if (reply->type != REDIS_REPLY_ARRAY || reply->elements == 0)
        {
            errorMsg_ = std::string(kErrorDesc[REDIS_NO_KEY]) + " or " + "hash is empty";
            ret = false;
        }
        else
        {
            for(std::size_t i = 0; i < reply->elements; i++)
            {
                lists.push_back(reply->element[i]->str);
            }
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hkeys(const char* key, std::vector<std::string>& keys)
{
    SET_ERROR(REDIS_NO_ERROR);
    return hkeysOrvals("HKEYS", key, keys);
}

bool RedisClient::hvals(const char* key, std::vector<std::string>& values)
{
    SET_ERROR(REDIS_NO_ERROR);
    return hkeysOrvals("HVALS", key, values);
}

bool RedisClient::hincrby(const char* key, const char* field, int64_t increment, int64_t* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field, false, "key or field is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HINCRBY %s %s %lld", key, field, increment));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if(value)
            *value = reply->integer;
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hincrbyfloat(const char* key, const char* field, float increment, float* value/* = NULL*/)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || !field, false, "key or field is null");

    bool ret = true;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, "HINCRBYFLOAT %s %s %f", key, field, increment));
    if (getError(reply))
    {
        ret = false;
    }
    else
    {
        if(reply->str && value)
            *value = atof(reply->str);
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hmset(const char* key, const std::map<std::string, std::string>& kv)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || kv.empty(), false, "key or field is null");

    std::string command(std::string("HMSET ") + key);
    for(std::map<std::string, std::string>::const_iterator it = kv.begin(); it != kv.end(); ++it)
    {
        command += " ";
        command += it->first;
        command += " ";
        command += it->second;
    }
    bool ret = false;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, command.c_str()));
    if (getError(reply))
    {
    }
    else
    {
        if(reply->type == REDIS_REPLY_STATUS && strcasecmp(reply->str,"OK") == 0)
        {
            ret = true;
        }
        else
        {
            errorMsg_ =  reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

bool RedisClient::hmget(const char* key, const std::vector<std::string>& keys, std::vector<std::string>& vals)
{
    SET_ERROR(REDIS_NO_ERROR);
    JUST_RETUAN_IF_ERROR(!connected_ || !rdsContext_, false, kErrorDesc[REDIS_NO_CONNECT]);
    JUST_RETUAN_IF_ERROR(!key || keys.empty(), false, "key or keys is null");

    std::string command(std::string("HMGET ") + key);
    for(std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
    {
        command += " ";
        command += *it;
    }

    bool ret = false;
    redisReply* reply = static_cast<redisReply *>(redisCommand(rdsContext_, command.c_str()));
    if (getError(reply))
    {
    }
    else
    {
        if(reply->type == REDIS_REPLY_ARRAY)
        {
            ret = true;
            for(std::size_t i = 0; i < reply->elements; i ++)
            {
                vals.push_back(reply->element[i]->str ? reply->element[i]->str : "");
            }
        }
        else
        {
            errorMsg_ =  reply->str;
        }
    }

    freeReplyObject(reply);
    return ret;
}

} } // namespace zl { namespace redis {
