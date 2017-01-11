#ifndef ZL_REDISCLIENT_H
#define ZL_REDISCLIENT_H
#include <string>
#include <vector>
#include <map>
#include "hiredis/hiredis.h"

// ��ʱд��һ��redis clientС���ߣ�����hiredis��
// ���Ժ��ҵ����õ�C++ client���ͻ������ʵ��
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
    /// ֱ�ӷ�������� redis ������
    /// return NULL(ʧ��)���ɹ�����redisReplyָ��(ע��Ҫ�����ͷ�)
    redisReply* redisCmd(const char *format, ...);

    bool execCmdBool(const char *format, ...);
    bool execCmdString(std::string* retvalue, const char *format, ...);
    bool execCmdInteger(int64_t* retvalue, const char *format, ...);

    /// ============            key             ============
    /// @brief ������ key �Ƿ����
    /// @return true(����)��false(�����ڻ��߲�ѯʧ��)
    bool exist(const char* key);

    /// @brief ɾ���Ѵ��ڵļ��������ڵ� key �ᱻ����
    /// @param [in] key 
    /// @param [out] count ��ɾ�� key ������
    /// @return true(�ɹ�)��false(ʧ��)
    bool del(const char* key, int64_t* count = NULL);

    /// @brief ���л����� key �������ر����л���ֵ
    /// @param [in] key
    /// @param [out] value ���л����ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool dump(const char* key, std::string& value);

    /// @brief �޸� key ������
    /// @param [in] key
    /// @param [in] newkey �µ�key����
    /// @return true(�ɹ�)��false(ʧ��)
    bool rename(const char* key, const char* newkey);

    /// @brief ���� newkey ������ʱ���� key ����Ϊ newkey
    /// @param [in] key
    /// @param [in] newkey �µ�key����
    /// @return true(�ɹ�)��false(ʧ��)
    bool renamenx(const char* key, const char* newkey);

    /// @brief Ϊ���� key ���ù���ʱ��
    /// @param [in] key
    /// @param [in] seconds ʱ�䣬��λ��
    /// @return true(�ɹ�)��false(ʧ��)
    bool expire(const char* key, int seconds);

    /// @brief �� expire ���ƣ���ͬ���� expireat ������ܵ�ʱ������� UNIX ʱ���(unix timestamp)
    bool expireat(const char* key, int unixtimestamp);

    /// @brief �Ƴ� key �Ĺ���ʱ�䣬key ���־ñ���
    /// @return true(�ɹ�)��false(ʧ��, ��key �����ڻ� key û�����ù���ʱ��)
    bool persist(const char* key);

    /// @brief ���� key �Ĺ���ʱ��,�Ժ����
    /// @param [in] key
    /// @param [in] milliseconds ʱ�䣬��λ����
    /// @return true(�ɹ�)��false(ʧ��)
    bool pexpire(const char* key, int milliseconds);

    /// @brief ���� key ����ʱ���ʱ���(unix timestamp) �Ժ����
    bool pexpireat(const char* key, int millisecondstimestamp);

    /// @brief ����Ϊ��λ�����ظ��� key ��ʣ������ʱ��(TTL, time to live)
    /// @param [in] key
    /// @param [out] seconds ʣ�����ʱ�䣬��λ�룬ע�⣺ ��Redis 2.6��֮ǰ�汾�����key�����ڻ���key���ڵ����Ѿ����ڽ�����-1��
    /// ���� Redis 2.8��ʼ�����key�����ڷ���-2�����key���ڵ����Ѿ����ڷ���-1����������Ϊ��λ���� key ��ʣ������ʱ��
    /// @return true(�ɹ�)��false(ʧ��)
    bool ttl(const char* key, int64_t& seconds);

    /// @brief �Ժ���Ϊ��λ���� key ��ʣ��Ĺ���ʱ��
    /// @param [in] key
    /// @param [out] seconds ʣ�����ʱ�䣬����˵��ͬttl����
    /// @return true(�ɹ�)��false(ʧ��)
    bool pttl(const char* key, int64_t& milliseconds);

    /// @brief ��ȡ key �������ֵ������
    /// @param [in] key
    /// @param [out] type ��Ӧ������
    /// @return true(�ɹ�)��false(ʧ��)
    bool type(const char* key, REDIS_KEY_TYPE& type);

    /// ============           string           ============
    /// @brief ���ø��� key ��ֵ����� key �Ѿ��洢����ֵ�� SET �͸�д��ֵ������������
    /// @param [in] key �ַ���key
    /// @param [in] value ��Ӧ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool set(const char* key, const char* value);

    /// @brief ���ø��� key ��ֵ��������ָ���� key ������ʱ���Ž�������
    /// @param [in] key �ַ���key
    /// @param [in] value ��Ӧ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool setnx(const char* key, const char* value);

    /// @brief ͬʱ����һ������ key-value ��
    /// @param [in] datas key-value ��
    /// @return true(��Զ���سɹ�����������ʧ��)
    bool mset(const std::vector<std::pair<std::string, std::string> >& datas);

    /// @brief �����и��� key ��������ʱ��ͬʱ����һ������ key-value ��
    /// @param [in] datas key-value ��
    /// @return true(����key-value�����óɹ�)�� false(���и��� key ������ʧ�ܣ�����������һ�� key �Ѿ�����)
    bool msetnx(const std::vector<std::pair<std::string, std::string> >& datas);

    /// @brief �� key ��������ַ���ֵ�����û����ָ��ƫ�����ϵ�λ(bit)
    /// @param [in] key �ַ���key
    /// @param [in] offset �ַ�����offset��λ��
    /// @param [in] value ��ֵ��ֻ����0����1
    /// @param [out] oldvalue ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool setbit(const char* key, uint32_t offset, int32_t value, int32_t* oldvalue);

    /// @brief ��ָ�����ַ������Ǹ��� key ��������ַ���ֵ�����ǵ�λ�ô�ƫ���� offset ��ʼ
    /// @param [in] key �ַ���key
    /// @param [in] offset �ַ���ƫ����
    /// @param [in] value ���ִ�
    /// @param [out] length ׷��ָ��ֵ֮�� key ���ַ����ĳ���
    /// @return true(�ɹ�)��false(ʧ��)
    bool setrange(const char* key, uint32_t offset, const char* value, int64_t* length = NULL);

    /// @brief Ϊָ���� key ����ֵ�������ʱ�䡣��� key �Ѿ����ڣ� SETEX ������滻�ɵ�ֵ��
    /// @param [in] key �ַ���key
    /// @param [in] value ��Ӧ��ֵ
    /// @param [in] timeout ����ʱ�䣬��λ���룬�������0
    /// @return true(�ɹ�)��false(ʧ��)
    bool setex(const char* key, const char* value, uint32_t timeout);

    /// @brief Ϊָ���� key ����ֵ�������ʱ�䡣��� key �Ѿ����ڣ� SETEX ������滻�ɵ�ֵ��
    /// @param [in] key �ַ���key
    /// @param [in] value ��Ӧ��ֵ
    /// @param [in] timeout ����ʱ�䣬��λ�Ǻ��룬�������0
    /// @return true(�ɹ�)��false(ʧ��)
    bool psetex(const char* key, const char* value, uint32_t timeoutMs);

    /// @brief ��ȡ�ַ���key��ֵ�����key�����ַ������ͣ������һ������
    /// @param [in] key �ַ���key
    /// @param [out] value ��Ӧ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool get(const char* key, std::string& value);

    /// @brief �������и��� key ��ֵ�� ��������� key ������ĳ�� key �����ڣ���ô��� key ��������ֵ nil 
    /// @param [in] keys ����ַ���key
    /// @param [out] values ���ַ�����Ӧ��ֵ�����ĳ���ַ��������ڣ�������Ӧλ�õ�valueΪnil(FIXME : ���Բ�Ҫ����valueΪnil��)
    /// @return true(�ɹ�)��false(ʧ��)
    bool mget(const std::vector<std::string>& keys, std::vector<std::string>& values);

    /// @brief �ַ���ָ��ƫ�����ϵ�λ(bit), ��ƫ���� OFFSET ���ַ���ֵ�ĳ��ȴ󣬻��� key ������ʱ������ 0
    /// @param [in] key �ַ���key
    /// @param [in] offset �ַ�����offset��λ��
    /// @param [out] value offsetλ���ϵ�ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool getbit(const char* key, uint32_t offset, int32_t* value);

    /// @brief ��ȡ�洢�� key �е����ַ������ַ����Ľ�ȡ��Χ�� start �� end ����ƫ��������(����start��end����)
    /// @param [in] key �ַ���key
    /// @param [in] start ƫ������ʼλ��
    /// @param [in] end ƫ��������λ��
    /// @param [out] value �����ص����ַ���
    /// @return true(�ɹ�)��false(ʧ��)
    bool getrange(const char* key, int64_t start, int64_t end, std::string& value);

    /// @brief ������ key ��ֵ��Ϊ newvalue �������� key �ľ�ֵ(old value)
    /// @param [in] key �ַ���key
    /// @param [in] newvalue �µ�Ҫ���õ�ֵ
    /// @param [out] value ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool getset(const char* key, const char* newvalue, std::string& oldvalue);

    /// @brief ��� key �Ѿ����ڲ�����һ���ַ����� APPEND ��� value ׷�ӵ� key ԭ����ֵ��ĩβ
    /// @param [in] key �ַ���key
    /// @param [in] value Ҫ׷�ӵ����ַ���
    /// @param [out] length ׷��ָ��ֵ֮�� key ���ַ����ĳ���
    /// @return true(�ɹ�)��false(ʧ��)
    bool append(const char* key, const char* value, int64_t* length = NULL);

    /// @brief ��ȡָ�� key ��������ַ���ֵ�ĳ��ȡ��� key ����Ĳ����ַ���ֵʱ������һ������
    /// @param [in] key �ַ���key
    /// @return �ַ������ȣ�����ַ����������򷵻�0
    int64_t strlen(const char* key);

    /// �� key �д��������ֵ��һ
    /// @param [in] key �ַ���key
    /// @param [out] value ִ�� INCR ����֮�� key ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool incr(const char* key, int64_t* value = NULL);

    /// �� key �д����ֵ���ϸ���������ֵ��increment��
    /// @param [in] key �ַ���key
    /// @param [in] increment ����
    /// @param [out] value ִ�� INCR ����֮�� key ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool incrby(const char* key, int64_t increment, int64_t* value = NULL);

    /// @brief �� key �д����ֵ���ϸ����ĸ���������ֵ
    /// @param [in] key �ַ���key
    /// @param [in] increment ����
    /// @param [out] value ִ�� INCR ����֮�� key ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool incrbyfloat(const char* key, float increment, float* value = NULL);

    /// �� key �д��������ֵ��һ
    /// @param [in] key �ַ���key
    /// @param [out] value ִ�� INCR ����֮�� key ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool decr(const char* key, int64_t* value = NULL);

    /// �� key �д����ֵ���ϸ���������ֵ��increment��
    /// @param [in] key �ַ���key
    /// @param [in] increment ����
    /// @param [out] value ִ�� INCR ����֮�� key ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool decrby(const char* key, int64_t increment, int64_t* value = NULL);


    /// ============            list            ============
    /// ��ȡlist����
    /// return -1 �� ִ��ʧ�ܣ�0 �� ������0������ֵ ��list����
    int32_t  llen(const char* key);

    /// ��list��߲���һ��Ԫ�أ�retval ���ز���ɹ���list����
    /// return false(����ʧ��) or true(����ɹ�)
    bool lpush(const char* key, const char* value, uint64_t* retval = NULL);

    /// ��list��߲���һ��Ԫ��(����������ָ����Key����ʱ)��retval ���ز���ɹ���list����
    /// return false(����ʧ��) or true(����ɹ�)
    bool lpushx(const char* key, const char* value, uint64_t* retval = NULL);

    /// ��list�ұ߲���һ��Ԫ�أ�retval ���ز���ɹ���list����
    /// return false(����ʧ��) or true(����ɹ�)
    bool rpush(const char* key, const char* value, uint64_t* retval = NULL);

    /// ��list�ұ߲���һ��Ԫ��(����������ָ����Key����ʱ)��retval ���ز���ɹ���list����
    /// return false(����ʧ��) or true(����ɹ�)
    bool rpushx(const char* key, const char* value, uint64_t* retval = NULL);

    /// ��list��ߵ���һ��Ԫ�أ� value ���ص�����Ԫ��ֵ
    /// return false(����ʧ��) or true(�����ɹ�)
    bool lpop(const char* key, std::string& value);

    /// ��list�ұߵ���һ��Ԫ�أ� value ���ص�����Ԫ��ֵ
    /// return false(����ʧ��) or true(�����ɹ�)
    bool rpop(const char* key, std::string& value);

    /// ��list��ߵ���һ��Ԫ�أ� ����б�û��Ԫ�ػ������б�ֱ���ȴ���ʱ������Ԫ��Ϊֹ��
    /// ������ָ��ʱ����û���κ�Ԫ�ر��������򷵻�һ�� nil �͵ȴ�ʱ����
    /// ��֮������һ����������Ԫ�ص��б���һ��Ԫ���Ǳ�����Ԫ�������� key ���ڶ���Ԫ���Ǳ�����Ԫ�ص�ֵ��
    /// ���ֵ����pairs��
    bool blpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// ͬ�ϣ�ֻ�����Ƕ��list��ֻҪ�κ�һ��list��Ԫ�ط��ؼ���
    bool blpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// ��list�ұߵ���һ��Ԫ�أ� ����б�û��Ԫ�ػ������б�ֱ���ȴ���ʱ������Ԫ��Ϊֹ��
    /// ������ָ��ʱ����û���κ�Ԫ�ر��������򷵻�һ�� nil �͵ȴ�ʱ����
    /// ��֮������һ����������Ԫ�ص��б���һ��Ԫ���Ǳ�����Ԫ�������� key ���ڶ���Ԫ���Ǳ�����Ԫ�ص�ֵ��
    /// ���ֵ����pairs��
    bool brpop(const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// ͬ�ϣ�ֻ�����Ƕ��list��ֻҪ�κ�һ��list��Ԫ�ط��ؼ���
    bool brpop(const std::vector<std::string>& keys, uint32_t timeout, std::pair<std::string, std::string>& pairs);

    /// ��һ��list�е���һ��ֵ������������Ԫ�ز��뵽����һ��list�в�������
    /// ���listû��Ԫ�ػ������б�ֱ���ȴ���ʱ���ֿɵ���Ԫ��Ϊֹ
    /// ������ָ��ʱ����û���κ�Ԫ�ر��������򷵻�һ�� nil �͵ȴ�ʱ����
    /// ��֮���ر��ƶ��ĸ�Ԫ��
    bool brpoplpush(const char* key, const char* another_key, uint32_t timeout, std::string& value);

    /// ��ȡlist��Ԫ���±�Ϊindex��ֵ
    /// return false(ʧ��) or true(�ɹ�)
    bool lindex(const char* key, int32_t index, std::string& value);

    /// �Ӵ��� key ���б����Ƴ�ǰ count �γ��ֵ�ֵΪ value ��Ԫ�ء� ��� count ����ͨ�����漸�ַ�ʽӰ�����������
    /// count > 0: ��ͷ��β�Ƴ�ֵΪ value ��Ԫ��
    /// count < 0: ��β��ͷ�Ƴ�ֵΪ value ��Ԫ��
    /// count = 0: �Ƴ�����ֵΪ value ��Ԫ��
    /// return ���Ƴ���Ԫ�ظ���, �б�����ʱ���� 0 
    int32_t  lrem(const char* key, int count, const char* value);

    /// ͨ������������Ԫ�ص�ֵ�� ����������������Χ��������һ�����б�ʱ������һ������
    bool lset(const char* key, int index, const char* value);

    /// ��һ���б�����޼�(trim)�����б�ֻ����ָ�������ڵ�Ԫ�أ�����ָ������֮�ڵ�Ԫ�ض�����ɾ����
    /// �±� 0 ��ʾ�б�ĵ�һ��Ԫ�أ� 1 ��ʾ�б�ĵڶ���Ԫ�أ��Դ����ơ� 
    /// Ҳ����ʹ�ø����±꣬�� -1 ��ʾ�б�����һ��Ԫ�أ� -2 ��ʾ�б�ĵ����ڶ���Ԫ�أ��Դ����ơ�
    /// return false(ʧ��) or true(�ɹ�)
    bool ltrim(const char* key, int start, int end);

    /// ��ȡlistָ�������ڵ�Ԫ��, start/end �ֱ��ʾ���俪ʼ/�����±꣬�ɹ�ʱ���ؽ��������valueList
    /// return false(ʧ��) or true(�ɹ�)
    bool lrange(const char* key, uint32_t start, int32_t end, std::vector<std::string>& valueList);

    /// ��list�е�pivotԪ��֮ǰ����һ��Ԫ�أ� �ɹ�ʱ retval ����list����
    /// return false(����ʧ��) or true(����ɹ�)
    bool linsertBefroe(const char* key, const char* pivot, const char* value, int64_t* retval = NULL);

    /// ��list�е�pivotԪ��֮ǰ���һ��Ԫ�أ� retval ���ص�����Ԫ��ֵ
    /// return false(����ʧ��) or true(����ɹ�)
    bool linsertAfter(const char* key, const char* pivot, const char* value, int64_t* retval = NULL);


    /// ============            hash            ============
    /// @brief ���ù�ϣ������key��field����Ӧ��valueֵ
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [in] field ���ֶ���
    /// @param [in] value ��������������Ӧ��ֵ
    /// @param [out] retval 0:field�Ѵ����Ҹ�����value�� 1:field�����ڣ��½�field�ҳɹ�������value
    /// @return true(�ɹ�)��false(ʧ��)
    bool hset(const char* key, const char* field, const char* value, uint32_t* retval = NULL);

    /// @brief ֻ�ڶ��󲻴���ָ�����ֶ�ʱ�������ֶε�ֵ
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [in] field ���ֶ�
    /// @param [in] value ֵ
    /// @param [out] retval 1:���óɹ�, 0:��������ֶ��Ѿ�������û�в�����ִ��
    /// @return true(�ɹ�)��false(ʧ��)
    bool hsetnx(const char* key, const char* field, const char* value, uint32_t* retval = NULL);

    /// @brief �ӹ�ϣ����ȡ����key��field����Ӧ��valueֵ
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [in] field ���ֶ���
    /// @param [out] value �ǻ�ȡ��ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool hget(const char* key, const char* field, std::string& value);

    /// @brief ɾ����ϣ����key����Ӧ��field���ԣ�������Բ����ڽ�������
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [in] field ���ֶ���
    /// @param [out] retval��ɾ����field����, 0:�޴�field��1:ɾ���˸�field
    /// @return true(�ɹ�)��false(ʧ��)
    ///@warning ɾ��ʧ�� retvalΪ0���ɹ�Ϊ1
    bool hdel(const char* key, const char* field, uint32_t* retval = NULL);

    /// @brief ȡ�ù�ϣ����key����Ӧ�������������ֵ
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [out] valueMap ��ȡ�ļ�ֵ��
    /// @return true(�ɹ�)��false(ʧ��)
    bool hgetall(const char* key, std::map<std::string , std::string>& valueMap);

    /// @brief �鿴key����Ķ����Ƿ���ڸ�field������
    /// @param [in] key �Ǽ������൱�ڱ���
    /// @param [in] field �ֶ���
    /// @return true(����)��false(������)
    bool hexists(const char* key, const char* field);

    /// @brief ��ȡ��������������ֶε�����
    /// @param [in] key �Ƕ�����
    /// @return �ֶε���������key������ʱ������0
    uint32_t hlen(const char* key);

    /// @brief ��ȡ����ָ��field��value���ַ�������
    /// @param [in] key �Ƕ�����
    /// @param [in] field �ֶ���
    /// @return �ַ������ȣ����ö������field������ʱ������0
    uint32_t hstrlen(const char* key, const char* field);

    /// @brief ��ȡ��������������ֶ���
    /// @param [in] key �����ֵ
    /// @param [out] keys �ö�������������ֶ���
    /// @return true(�ɹ�)��false(ʧ��)
    bool hkeys(const char* key, std::vector<std::string>& keys);

    /// @brief ��ȡ�������������ֵ
    /// @param [in] key �����ֵ
    /// @param [out] keys �ö������������ֵ
    /// @return true(�ɹ�)��false(ʧ��)
    bool hvals(const char* key, std::vector<std::string>& values);

    /// @brief ���ö�����ָ�����ֵ����ָ��������ֵ��ԭ������������ע�⣺ֻ����integer������ֵ����ʹ��
    /// @param [in] key �����ֵ
    /// @param [in] field �ֶ���
    /// @param [in] increment ����
    /// @param [out] value ִ�к���ֶε�ֵ
    /// @return true(ִ�гɹ�), false(ʧ��)
    /// ע�⣺���key�����ڣ�һ���µĹ�ϣ��������ִ��HINCRBY������field��������ô��ִ������ǰ�ֶε�ֵ����ʼ��Ϊ0
    bool hincrby(const char* key, const char* field, int64_t increment, int64_t* value = NULL);

    /// @brief ���ö�����ָ�����ֵ����ָ���ĸ���������ֵ
    /// @param [in] key �����ֵ
    /// @param [in] field �ֶ���
    /// @param [in] increment ����
    /// @param [out] value ִ�к���ֶε�ֵ
    /// @return true(ִ�гɹ�), false(ʧ��)
    /// ע�⣺���key�����ڣ�һ���µĹ�ϣ��������ִ��������field��������ô��ִ������ǰ�ֶε�ֵ����ʼ��Ϊ0
    bool hincrbyfloat(const char* key, const char* field, float increment, float* value = NULL);

    /// @brief ͬʱ���ù�ϣ����һ�������ֶε�ֵ
    /// @param [in] key ����
    /// @param [in] kv ���ֶκ����Ӧֵ�ļ�ֵ��
    /// @return true(�ɹ�)��false(ʧ��)
    bool hmset(const char* key, const std::map<std::string, std::string>& kv);

    /// @brief ͬʱ��ȡ��ϣ����һ�������ֶε�ֵ
    /// @param [in] key ����
    /// @param [in] keys һ�������ֶ�
    /// @param [out] vals ���ص�һ�������ֶεĶ�Ӧֵ�����ÿ��key�����ڣ����Ӧ��valֵ��Ϊ�մ�
    /// @return true(�ɹ�)��false(ʧ��)
    bool hmget(const char* key, const std::vector<std::string>& keys, std::vector<std::string>& vals);

    /// ============            set            ============

    /// ============       ordered set         ============
private:
    /// list ��������
    bool push(const char* type, const char* xtype, const char* key, const char* value, uint64_t* retval = NULL);
    bool pop(const char* type, const char* key, std::string& value);
    bool bpop(const char* type, const char* key, uint32_t timeout, std::pair<std::string, std::string>& pairs);
    bool linsert(const char* key, const char* insertType, const char* pivot, const char* value, int64_t* retval = NULL);

    /// hash ��������
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
