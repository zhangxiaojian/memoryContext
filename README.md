Rewrite PostgreSQL souce code about memoryContext and make it a library to alloc memory. Memory Context can avoid memory leak and I add more statistical data about memory alloc and free. According to this data, we can change some parameter to optimize.
more detials is in www.zhangxiaojian.name.

Here is an example :

myContext :
       +----------------+------+------+------+------+------+-------+-------+-------+-------+
       |   Chunk Size   |  8B  | 16B  | 32B  | 64B  | 128B | 256B  | 512B  | >512B | Total |
       +----------------+------+------+------+------+------+-------+-------+-------+-------+
       | Alloced Times  | 10   | 9    | 10   | 44   | 88   | 179   | 280   | 9381  | 10001 |
       | Wasted Space   | 48   | 24   | 82   | 634  | 2717 | 11547 | 36402 | 0     | 51454 |
       | Average Wasted | 4    | 2    | 8    | 14   | 30   | 64    | 130   | 0     | 5     |
       | Free Times     | 0    | 0    | 0    | 0    | 0    | 0     | 0     | 0     | 0     |
       | Hit Rate       | 0    | 0    | 0    | 0    | 0    | 0.01  | 0.02  | 0.93  | 0.06  |
       +----------------+------+------+------+------+------+-------+-------+-------+-------+
       41161804 total in 9657 blocks; 63016 free (514 chunks); 41098788 used

Usage:
See INSTALL first. 

Then the memory context lib is in ./mctx/lib/ . To use this lib, user's program need include file mctx.h which is installed in ./mctx/include/

My test code is in ./mctx/src/tools/, after installation, the executable file mctx is installed in ./mctx/
