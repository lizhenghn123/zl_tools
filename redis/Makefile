all:RedisClient_Key_test RedisClient_String_test RedisClient_List_test RedisClient_Hash_test

RedisClient_Key_test:RedisClient_Key_test.cpp RedisClient.cpp
	g++ -o RedisClient_Key_test -std=c++0x  RedisClient_Key_test.cpp RedisClient.cpp -I/usr/local/include -L/usr/local/lib -lhiredis

RedisClient_String_test:RedisClient_String_test.cpp RedisClient.cpp
	g++ -o RedisClient_String_test -std=c++0x  RedisClient_String_test.cpp RedisClient.cpp -I/usr/local/include -L/usr/local/lib -lhiredis

RedisClient_List_test:RedisClient_List_test.cpp RedisClient.cpp
	g++ -o RedisClient_List_test -std=c++0x  RedisClient_List_test.cpp RedisClient.cpp -I/usr/local/include -L/usr/local/lib -lhiredis

RedisClient_Hash_test:RedisClient_Hash_test.cpp RedisClient.cpp
	g++ -o RedisClient_Hash_test -std=c++0x RedisClient_Hash_test.cpp RedisClient.cpp -I/usr/local/include -L/usr/local/lib -lhiredis

clean:
	-rm RedisClient_Key_test RedisClient_String_test RedisClient_List_test RedisClient_Hash_test -f
