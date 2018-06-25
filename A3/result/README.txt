************************************ Comparison paragraph ************************************
#interesting# is the program we wrote to distribute read and write in a very large range of pages,
thus our discussion is based on the observation and statistics extracted from the three programs
given because they are more comman in real world. No algorithms can save #interesting# overhead 
here. Generally speaking, all algorithms' hit rate would increase as memory size increase. 
Because locality ensure that most pages could stay in the frame space if frame space is large 
enough and we do not deliberately distribute pages in a wide range. *RANDOM* alogrithm behaves
decently when memory size is limited within 100 because there is a threshold when exploiting 
specific strategy to achieve efficiency. *FIFO* does not behave as good as the last three 
algorithms in #matmul#, because #matmul#'s implementation follows an ordered pattern such that
recently accessed memory locations are less likely to be accessed again. *LRU* and *CLOCK* 
behave quite similar to each other and a little bit better than *FIFO* in #matmul# because they
at least will either bring back a page that is going to be evicted if "rescued" in time. As for 
*OPT*, it is obviously the best algorithm compared to the rest because it's based on the future 
references.

************************************ LRU ************************************
Just like all other algorithms, *LRU* benefits from increasing memory size. Because it holds the
records of least recently used pages and would evict it once the frame is being take up, the
property of locality of most programs(processes) would help this algorithm to increase its hit
rates and thus achieve its effiency. Frequently accessed pages would stay in the frame, less
frequently accessed pages get higher chance to be kicked out and of course it is less referenced.
Besides the general property we inspected from table, we also notice that there is a giant leap 
of hit rate from 100 to 150 when doing #matmul#, the reason behind this we think is that a 
hundred and more pages are really frequently accessed and not accessed in a good order, making 
100 frames hard to maintain hit rate in a high position. Increasing 50 frames would ensure those 
long-term active pages remain in the frame for a long period. Thus we have that kind of data 
shown in our table. 