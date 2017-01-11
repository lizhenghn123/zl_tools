
基于[redis](https://github.com/antirez/redis) c api([hiredis](https://github.com/redis/hiredis))封装的C++版本，目的是简化使用。

使用该实现时需要自行编译hiredis，很简单，从此处[https://github.com/antirez/redis](https://github.com/antirez/redis)下载并make即可。

本实现目前已完成对redis服务及部分数据结构的访问：
	
- [x] redis连接
- [x] 键(Key)
- [x] 字符串(String)
- [x] 列表(List)
- [x] 哈希、字典(Hash)
- [] 集合(Set)
- [] 有序集合(Sorted Set)

本实现自带了多个测试程序，可自行进行验证。