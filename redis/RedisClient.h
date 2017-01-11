#ifndef ZL_REDISCLIENT_H
#define ZL_REDISCLIENT_H
#include <string>
#include <vector>
#include <map>
#include "hiredis/hiredis.h"

// 临时写的一个redis client小工具，依赖hiredis库
// 待以后找到好用的C++ client库后就会废弃该实现
namespace zl { namespace redis {

/// redis error number
enum REDIS_ERROR_CODE
{
    REDIS_NO_ERROR = 0,
    REDIS_REPLY_NULL,
    REDIS_NO_CONNECT,
    REDIS_NO_PIVOT,
    REDIS_NO_KEY,
    REDIS_NO_FIELD,
    REDIS_INDEX,
    REDIS_NIL,
    REDIS_ERROR,
    REDIS_BOTTOM
};

enum REDIS_KEY_TYPE
{
    REDIS_KEY_NONE,
    REDIS_KEY_STRING,
    REDIS_KEY_LIST,
    REDIS_KEY_SET,
    REDIS_KEY_ZSET,
    REDIS_KEY_HASH,
};

class RedisClient
{
public:
    RedisClient();
    RedisClient(const std::string& ip, short port, int timeoutMs = 5000);
    ~RedisClient();
private:
    RedisClient(const RedisClient&);
    RedisClient& operator=(const RedisClient&);

public:
    bool connected() const { return connected_; }

    const char* reason() const { return errorMsg_.c_str(); }

    REDIS_ERROR_CODE reasonCode() const { return errorCode_; }

    redisContext* context() const { return rdsContext_; }

    bool connect(const std::string& ip, short port, int timeoutMs = 5000);

    bool reconnect();

    bool disConnect();

    bool auth(const std::string& password);

    bool ping();

    int32_t ttl(const char* key);

public:
    /// 直接发送命令给 redis 服务器
    /// return NULL(失败)，成功返回redisReply指针(注意要自行释放)
    redisReply* redisCmd(const char *format, ...);

    bool execCmdBool(const char *format, ...);
    bool execCmdString(std::string* retvalue, const char *format, ...);
    bool execCmdInteger(int64_t* retvalue, const char *format, ...);

    /// ============            key             ============
    /// @brief 检查给定 key 是否存在
    /// @return true(存在)，false(不存在或者查询失败)
    bool exist(const char* key);

    /// @brief 删除已存在的键。不存在的 key 会被忽略
    /// @param [in] key 
    /// @param [out] count 被删除 key 的数量
    /// @return true(成功)，false(失败)
    bool del(const char* key, int64_t* count = NULL);

    /// @brief 序列化给定 key ，并返回被序列化的值
    /// @param [in] key
    /// @param [out] value 序列化后的值
    /// @return true(成功)，false(失败)
    bool dump(const char* key, std::string& value);

    /// @brief 修改 key 的名称
    /// @param [in] key
    /// @param [in] newkey 新的key名称
    /// @return true(成功)，false(失败)
    bool rename(const char* key, const char* newkey);

    /// @brief 仅当 newkey 不存在时，将 key 改名为 newkey
    /// @param [in] key
    /// @param [in] newkey 新的key名称
    /// @return true(成功)，false(失败)
    bool renamenx(const char* key, const char* newkey);

    /// @brief 为给定 key 设置过期时间
    /// @param [in] key
    /// @param [in] seconds 时间，单位秒
    /// @return true(成功)，false(失败)
    bool expire(const char* key, int seconds);

    /// @brief 和 expire 类似，不同在于 expireat 命令接受的时间参数是 UNIX 时间戳(unix timestamp)
    bool expireat(const char* key, int unixtimestamp);

    /// @brief 移除 key 的过期时间，key 将持久保持
    /// @return true(成功)，false(失败, 或key 不存在或 key 没有设置过期时间)
    bool persist(const char* key);

    /// @brief 设置 key 的过期时间,以毫秒计
    /// @param [in] key
    /// @param [in] milliseconds 时间，单位毫秒
    /// @return true(成功)，false(失败)
    bool pexpire(const char* key, int milliseconds);

    /// @brief 设置 key 过期时间的时间戳(unix timestamp) 以毫秒计
    bool pexpireat(const char* key, int millisecondstimestamp);

    /// @brief 以秒为单位，返回给定 key 的剩余生存时间(TTL, time to live)
    /// @param [in] key
    /// @param [out] seconds 剩余过期时间，单位秒，注意： 在Redis 2.6和之前版本，如果key不存在或者key存在但是已经过期将返回-1，
    /// 但从 Redis 2.8开始，如果key不存在返回-2，如果key存在但是已经过期返回-1，否则以秒为单位返回 key 的剩余生存时间
    /// @return true(成功)，false(失败)
    bool ttl(const char* key, int64_t& seconds);

    /// @brief 以毫秒为单位返回 key 的剩余的过期时间
    /// @param [in] key
    /// @param [out] seconds 剩余过期时间，具体说明同ttl命令
    /// @return true(成功)，false(失败)
    bool pttl(const char* key, int64_t& milliseconds);

    /// @brief 获取 key 所储存的值的类型
    /// @param [in] key
    /// @param [out] type 相应的类型
    /// @return true(成功)，false(失败)
    bool type(const char* key, REDIS_KEY_TYPE& type);

    /// ============           string           ============
    /// @brief 设置给定 key 的值。如果 key 已经存储其他值， SET 就覆写旧值，且无视类型
    /// @param [in] key 字符串key
    /// @param [in] value 对应的值
    /// @return true(成功)，false(失败)
    bool set(const char* key, const char* value);

    /// @brief 设置给定 key 的值，但仅在指定的 key 不存在时，才进行设置
    /// @param [in] key 字符串key
    /// @param [in] value 对应的值
    /// @return true(成功)，false(失败)
    bool setnx(const char* key, const char* value);

    /// @brief 同时设置一个或多个 key-value 对
    /// @param [in] datas key-value 对
    /// @return true(永远返回成功，除非连接失败)
    bool mset(const std::vector<std::pair<std::string, std::string> >& datas);

    /// @brief 当所有给定 key 都不存在时，同时设置一个或多个 key-value 对
    /// @param [in] datas key-value 对
    /// @return true(所有key-value都设置成功)， false(所有给定 key 都设置失败，比如至少有一个 key 已经存在)
    bool msetnx(const std::vector<std::pair<std::string, std::string> >& datas);

    /// @brief 对 key 所储存的字符串值，设置或清除指定偏移量上的位(bit)
    /// @param [in] key 字符串key
    /// @param [in] offset 字符串第offset个位置
    /// @param [in] value 新值，只能是0或者1
    /// @param [out] oldvalue 旧值
    /// @return true(成功)，false(失败)
    bool setbit(const char* key, uint32_t offset, int32_t value, int32_t* oldvalue);

    /// @brief 用指定的字符串覆盖给定 key 所储存的字符串值，覆盖的位置从偏移量 offset 开始
    /// @param [in] key 字符串key
    /// @param [in] offset 字符串偏移量
    /// @param [in] value 新字串
    /// @param [out] length 追加指定值之后， key 中字符串的长度
    /// @return true(成功)，false(失败)
    bool setrange(const char* key, uint32_t offset, const char* value, int64_t* length = NULL);

    /// @brief 为指定的 key 设置值及其过期时间。如果 key 已经存在， SETEX 命令将会替换旧的值。
    /// @param [in] key 字符串key
    /// @param [in] value 对应的值
    /// @param [in] timeout 过期时间，单位是秒，必须大于0
    /// @return true(成功)，false(失败)
    bool setex(const char* key, const char* value, uint32_t timeout);

    /// @brief 为指定的 key 设置值及其过期时间。如果 key 已经存在， SETEX 命令将会替换旧的值。
    /// @param [in] key 字符串key
    /// @param [in] value 对应的值
    /// @param [in] timeout 过期时间，单位是毫秒，必须大于0
    /// @return true(成功)，false(失败)
    bool psetex(const char* key, const char* value, uint32_t timeoutMs);

    /// @brief 获取字符串key的值，如果key不是字符串类型，会产生一个错误
    /// @param [in] key 字符串key
    /// @param [out] value 对应的值
    /// @return true(成功)，false(失败)
    bool get(const char* key, std::string& value);

    /// @brief 返回所有给定 key 的值。 如果给定的 key 里面有某个 key 不存在，那么这个 key 返回特殊值 nil 
    /// @param [in] keys 多个字符串key
    /// @param [out] values 各字符串对应的值，如果某个字符串不存在，则其相应位置的value为nil(FIXME : 所以不要设置value为nil啊)
    /// @return true(成功)，false(失败)
    bool mget(const std::vector<std::string>& keys, std::vector<std::string>& values);

    /// @brief 字符串指定偏移量上的位(bit), 当偏移量 OFFSET 比字符串值的长度大，或者 key 不存在时，返回 0
    /// @param [in] key 字符串key
    /// @param [in] offset 字符串第offset个位置
    /// @param [out] value offset位置上的值
    /// @return true(成功)，false(失败)
    bool getbit(const char* key, uint32_t offset, int32_t* value);

    /// @brief 获取存储在 key 中的子字符串。字符串的截取范围由 start 和 end 两个偏移量决定(包括start和end在内)
    /// @param [in] key 字符串key
    /// @param [in] start 偏移量开始位置
    /// @param [in] end 偏移量结束位置
    /// @param [out] value 待返回的子字符串
    /// @return true(成功)，false(失败)
    bool getrange(const char* key, int64_t start, int64_t end, std::string& value);

    /// @brief 将给定 key 的值设为 newvalue ，并返回 key 的旧值(old value)
    /// @param [in] key 字符串key
    /// @param [in] newvalue 新的要设置的值
    /// @param [out] value 旧值
    /// @return true(成功)，false(失败)
    bool getset(const char* key, const char* newvalue, std::string& oldvalue);

    /// @brief 如果 key 已经存在并且是一个字符串， APPEND 命令将 value 追加到 key 原来的值的末尾
    /// @param [in] key 字符串key
    /// @param [in] value 要追加的新字符串
    /// @param [out] length 追加指定值之后， key 中字符串的长度
    /// @return true(成功)，false(失败)
    bool append(const char* key, const char* value, int64_t* length = NULL);

    /// @brief 获取指定 key 所储存的字符串值的长度。当 key 储存的不是字符串值时，返回一个错误。
    /// @param [in] key 字符串key
    /// @return 字符串长度，如果字符串不存在则返回0
    int64_t strlen(const char* key);

    /// 将 key 中储存的数字值增一
    /// @param [in] key 字符串key
    /// @param [out] value 执行 INCR 命令之后 key 的值
    /// @return true(成功)，false(失败)
    bool incr(const char* key, int64_t* value = NULL);

    /// 将 key 中储存的值加上给定的增量值（increment）
    /// @param [in] key 字符串key
    /// @param [in] increment 增量
    /// @param [out] value 执行 INCR 命令之后 key 的值
    /// @return true(成功)，false(失败)
    bool incrby(const char* key, int64_t increment, int64_t* value = NULL);

    /// @brief 将 key 中储存的值加上给定的浮点数增量值
    /// @param [in] key 字符串key
    /// @param [in] increment 增量
    /// @param [out] value 执行 INCR 命令之后 key 的值
    /// @return true(成功)，false(失败)
    bool incrbyfloat(const char* key, float increment, float* value = NULL);

    /// 将 key 中储存的数字值减一
    /// @param [in] key 字符串key
    /// @param [out] value 执行 INCR 命令之后 key 的值
    /// @return true(成功)，false(失败)
    bool decr(const char* key, int64_t* value = NULL);

    /// 将 key 中储存的值加上给定的增量值（increment）
    /// @param [in] key 字符串key
    /// @param [in] increment 增量
    /// @param [out] value 执行 INCR 命令之后 key 的值
    /// @return true(成功)，false(失败)
    bool decrby(const char* key, int64_t increment, int64_t* value = NULL);


    /// ============            list            ============
    /// 获取list长度
    /// return -1 ： 执行失败；0 ： 长度是0；正数值 ：list长度
    int32_t  llen(const char* key);

    /// 从list左边插入一个元素，retval 返回插入成功后list长度
    /// return false(插入失败) or true(插入成功)
    bool lpush(const char* key, const char* value, uint64_t* retval = NULL);

    /// 从list左边插入一个元素(仅当参数中指定的Key存在时)，retval 返回插入成功后list长度
    /// return false(插入失败) or true(插入成功)
    bool lpushx(const char* key, const char* value, uint64_t* retval = NULL);

    /// 从list右边插入一个元素，retval 返回插入成功后list长度
    /// return false(插入失败) or true(插入成功)
    bool rpush(const char* key, const char* value, uint64_t* retval = NULL);

    /// 从list右边插入一个元素(仅当参数中指定的Key存在时)，retval 返回插入成功后list长度
    /// return false(插入失败) or true(插入成功)
    bool rpushx(const char* key, const char* value, uint64_t* retval = NULL);

    /// 从list左边弹出一个元素， value 返回弹出的元素值
    /// return false(弹出失败) or true(弹出成功)
    bool lpop(const char* key, std::string& value);

    /// 从list右边弹出一个元素， value 返回弹出的元素值
    /// return false(弹出失败) or true(弹出成功)
    bool rpop(const char* key, std::string& value);

    /// 从list左边弹出一个元素， 如果列表没有元素会阻塞列表直到等待超时或有了元素为止。
    /// 假如在指定时间内没有任何元素被弹出，则返回一个 nil 和等待时长。
    /// 反之，返回一个含有两个元素的列表，第一个元素是被弹出元素所属的 key ，第二个元素是被弹出元素的值。
    /// 输出值存在pairs中
    bool blpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// 同上，只不过是多个list中只要任何一个list有元素返回即可
    bool blpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// 从list右边弹出一个元素， 如果列表没有元素会阻塞列表直到等待超时或有了元素为止。
    /// 假如在指定时间内没有任何元素被弹出，则返回一个 nil 和等待时长。
    /// 反之，返回一个含有两个元素的列表，第一个元素是被弹出元素所属的 key ，第二个元素是被弹出元素的值。
    /// 输出值存在pairs中
    bool brpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// 同上，只不过是多个list中只要任何一个list有元素返回即可
    bool brpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// 从一个list中弹出一个值，并将弹出的元素插入到另外一个list中并返回它
    /// 如果list没有元素会阻塞列表直到等待超时或发现可弹出元素为止
    /// 假如在指定时间内没有任何元素被弹出，则返回一个 nil 和等待时长。
    /// 反之返回被移动的该元素
    bool brpoplpush(const char* key, const char* another_key, uint32_t timeout, std::string& value);

    /// 获取list中元素下标为index的值
    /// return false(失败) or true(成功)
    bool lindex(const char* key, int32_t index, std::string& value);

    /// 从存于 key 的列表里移除前 count 次出现的值为 value 的元素。 这个 count 参数通过下面几种方式影响这个操作：
    /// count > 0: 从头往尾移除值为 value 的元素
    /// count < 0: 从尾往头移除值为 value 的元素
    /// count = 0: 移除所有值为 value 的元素
    /// return 被移除的元素个数, 列表不存在时返回 0 
    int32_t  lrem(const char* key, int count, const char* value);

    /// 通过索引来设置元素的值。 当索引参数超出范围，或这是一个空列表时，返回一个错误
    bool lset(const char* key, int index, const char* value);

    /// 对一个列表进行修剪(trim)，让列表只保留指定区间内的元素，不在指定区间之内的元素都将被删除。
    /// 下标 0 表示列表的第一个元素， 1 表示列表的第二个元素，以此类推。 
    /// 也可以使用负数下标，以 -1 表示列表的最后一个元素， -2 表示列表的倒数第二个元素，以此类推。
    /// return false(失败) or true(成功)
    bool ltrim(const char* key, int start, int end);

    /// 获取list指定区间内的元素, start/end 分别表示区间开始/结束下标，成功时返回结果保存在valueList
    /// return false(失败) or true(成功)
    bool lrange(const char* key, uint32_t start, int32_t end, std::vector<std::string>& valueList);

    /// 在list中的pivot元素之前插入一个元素， 成功时 retval 返回list长度
    /// return false(插入失败) or true(插入成功)
    bool linsertBefroe(const char* key, const char* pivot, const char* value, int64_t* retval = NULL);

    /// 在list中的pivot元素之前插后一个元素， retval 返回弹出的元素值
    /// return false(插入失败) or true(插入成功)
    bool linsertAfter(const char* key, const char* pivot, const char* value, int64_t* retval = NULL);


    /// ============            hash            ============
    /// @brief 设置哈希表中以key和field所对应的value值
    /// @param [in] key 是键名，相当于表名
    /// @param [in] field 是字段名
    /// @param [in] value 是以上两参数对应的值
    /// @param [out] retval 0:field已存在且覆盖了value； 1:field不存在，新建field且成功设置了value
    /// @return true(成功)，false(失败)
    bool hset(const char* key, const char* field, const char* value, uint32_t* retval = NULL);

    /// @brief 只在对象不存在指定的字段时才设置字段的值
    /// @param [in] key 是键名，相当于表名
    /// @param [in] field 是字段
    /// @param [in] value 值
    /// @param [out] retval 1:设置成功, 0:如果给定字段已经存在且没有操作被执行
    /// @return true(成功)，false(失败)
    bool hsetnx(const char* key, const char* field, const char* value, uint32_t* retval = NULL);

    /// @brief 从哈希表中取出以key和field所对应的value值
    /// @param [in] key 是键名，相当于表名
    /// @param [in] field 是字段名
    /// @param [out] value 是获取的值
    /// @return true(成功)，false(失败)
    bool hget(const char* key, const char* field, std::string& value);

    /// @brief 删除哈希表中key所对应的field属性，如果属性不存在将被忽略
    /// @param [in] key 是键名，相当于表名
    /// @param [in] field 是字段名
    /// @param [out] retval：删除的field个数, 0:无此field，1:删掉了该field
    /// @return true(成功)，false(失败)
    ///@warning 删除失败 retval为0，成功为1
    bool hdel(const char* key, const char* field, uint32_t* retval = NULL);

    /// @brief 取得哈希表中key所对应的所有属性域和值
    /// @param [in] key 是键名，相当于表名
    /// @param [out] valueMap 获取的键值对
    /// @return true(成功)，false(失败)
    bool hgetall(const char* key, std::map<std::string , std::string>& valueMap);

    /// @brief 查看key代表的对象是否存在该field属性域
    /// @param [in] key 是键名，相当于表名
    /// @param [in] field 字段名
    /// @return true(存在)，false(不存在)
    bool hexists(const char* key, const char* field);

    /// @brief 获取对象的所有属性字段的总数
    /// @param [in] key 是对象名
    /// @return 字段的数量，当key不存在时，返回0
    uint32_t hlen(const char* key);

    /// @brief 获取对象指定field的value的字符串长度
    /// @param [in] key 是对象名
    /// @param [in] field 字段名
    /// @return 字符串长度，当该对象或者field不存在时，返回0
    uint32_t hstrlen(const char* key, const char* field);

    /// @brief 获取对象的所有属性字段名
    /// @param [in] key 对象键值
    /// @param [out] keys 该对象的所有属性字段名
    /// @return true(成功)，false(失败)
    bool hkeys(const char* key, std::vector<std::string>& keys);

    /// @brief 获取对象的所有属性值
    /// @param [in] key 对象键值
    /// @param [out] keys 该对象的所有属性值
    /// @return true(成功)，false(失败)
    bool hvals(const char* key, std::vector<std::string>& values);

    /// @brief 将该对象中指定域的值加上指定的增量值，原子自增操作，注意：只能是integer的属性值可以使用
    /// @param [in] key 对象键值
    /// @param [in] field 字段名
    /// @param [in] increment 增量
    /// @param [out] value 执行后该字段的值
    /// @return true(执行成功), false(失败)
    /// 注意：如果key不存在，一个新的哈希表被创建并执行HINCRBY命令，如果field不存在那么在执行命令前字段的值被初始化为0
    bool hincrby(const char* key, const char* field, int64_t increment, int64_t* value = NULL);

    /// @brief 将该对象中指定域的值加上指定的浮点数增量值
    /// @param [in] key 对象键值
    /// @param [in] field 字段名
    /// @param [in] increment 增量
    /// @param [out] value 执行后该字段的值
    /// @return true(执行成功), false(失败)
    /// 注意：如果key不存在，一个新的哈希表被创建并执行命令，如果field不存在那么在执行命令前字段的值被初始化为0
    bool hincrbyfloat(const char* key, const char* field, float increment, float* value = NULL);

    /// @brief 同时设置哈希表中一个或多个字段的值
    /// @param [in] key 键名
    /// @param [in] kv 是字段和其对应值的键值对
    /// @return true(成功)，false(失败)
    bool hmset(const char* key, const std::map<std::string, std::string>& kv);

    /// @brief 同时获取哈希表中一个或多个字段的值
    /// @param [in] key 键名
    /// @param [in] keys 一个或多个字段
    /// @param [out] vals 返回的一个或多个字段的对应值，如果每个key不存在，其对应的val值设为空串
    /// @return true(成功)，false(失败)
    bool hmget(const char* key, const std::vector<std::string>& keys, std::vector<std::string>& vals);

    /// ============            set            ============

    /// ============       ordered set         ============
private:
    /// list 辅助函数
    bool push(const char* type, const char* xtype, const char* key, const char* value, uint64_t* retval = NULL);
    bool pop(const char* type, const char* key, std::string& value);
    bool bpop(const char* type, const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);
    bool linsert(const char* key, const char* insertType, const char* pivot, const char* value, int64_t* retval = NULL);

    /// hash 辅助函数
    bool hkeysOrvals(const char* type, const char* key, std::vector<std::string>& lists);

private:
    // return true if error, otherwise return false if ok
    bool getError(redisContext* context);
    bool getError(redisReply* reply);

private:
    redisContext* rdsContext_;      /// redis context

    REDIS_ERROR_CODE errorCode_;    /// error code
    std::string errorMsg_;          /// error message

    std::string host_;              /// redis sever host
    short port_;                    /// redis sever port
    std::string password_;          /// redis server password
    int timeoutMs_;                 /// connect timeout(mill second)
    bool connected_;                ///< if connected

    static const char* kErrorDesc[REDIS_BOTTOM];	///< describe error
};

} } // namespace zl { namespace redis {
#endif  // ZL_REDISCLIENT_H
