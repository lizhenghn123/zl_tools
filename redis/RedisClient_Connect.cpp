#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "RedisClient.h"

void* test_connect(void *)
{
    int loop = 100;
    uint32_t count = 0;
    
    while (loop > 0)
    {
        //loop --;
        redisContext *ctx = redisConnect("127.0.0.1", 6379);
        if (ctx)
        {                                   
                            
        }

        count ++;
        printf("thread(%d) connected %ld times\n", pthread_self(), count);

        redisFree(ctx);                
    }
}

int main()
{
    const static int thread_num = 20;
    pthread_t ids[thread_num];
    for(int i = 0; i < thread_num; ++i)
    {
        pthread_t id;
        int ret = pthread_create(&ids[i], NULL, test_connect, NULL);
        if(ret != 0)
        {
            printf("pthread_create error\n");
            exit(0);
        }
    }

    for(int i = 0; i < thread_num; ++i)
    {
        pthread_join(ids[i], NULL);
    }
}
