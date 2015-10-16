Rewrite PostgreSQL souce code about memoryContext and make it a library to alloc memory. Memory Context can avoid memory leak and I add more statistical data about memory alloc and free. According to this data, we can change some parameter to optimize.
more detials is in www.zhangxiaojian.name.

Here is an example :

        MinContextSize: 1024    MaxBlockSize: 8092
        NextBlockSize: 2048     FreeListSize: 11
        AllocChunkLimit: 1024   MinChunkSize: 8
        Total 1 times alloc; 0 times is chunk; 1 times is block; 0 flailed !
        Total 1 times free; 0 times is chunk; 1 times is block;
        1024 total in 1 blocks; 992 free (0 chunks); 32 used
