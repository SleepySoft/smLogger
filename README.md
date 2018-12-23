I've worked for 4 companies. And 5 logging ways I've used. 4 different ways in different company and the other one is printf.  

I'm considering, what's the perfect way for our logging, I think it should be:  

1. Speedy, high efficiency - Of cause, it's the most important thing.
2. Easy to access - We do not need a c tool to access the log. We can just do it on the terminal.
3. Persistance - We need to keep the log in file or somewhere, when error occours, we can analysis the log offline.

And as greed as me, I'm wondering these advanced features:  

A. Mark/Trigger supports - When error occurs we want to make a "mark" in log. It's the best way to locate the log we're care about.  
B. Scope control - In most cases, log amount is huge. Filter in viewer is insufficient. It's better to filter dynamically in the upstream, to reduce the log count and increase its quality.  
C. Easy for interactive - For point A and B. We can just use a simple way to do them, like, just use some internal commands on termal. We don't want to use a special tool for them.  
D. Remote access/Output integration - It's similar to point 2. We can access the log over ethernet or serial port. And we can also access and integrate the log in our test system.  
E. Cross platform - It's better, but at most case, we jsut keep the upper-level logging interface and choose different low-layer implemention for different platform.  

There're also some amazing features and important choices, but they should/can be implemented in appliction layer, so I didn't put them into this topic, such as:  

a. Auto timestamp/function/lines logging.  
b. Different debug level supporting.  
c. Crash dumpping.  
d. printf style of cout style selection.  

What's my imagination:  

I. It'd better like a character device, we can just cat or tail it for the log. We can access it from terminal or other program easily. But the flaw is, it can just keep the last line and we cannot copy this file out of file system.  
II. If the file can keep it's size and dropping old data at front, it's the best. But all programer knows it's just a fantasy. A ring buffer is a second choice.  
III. Simple and Rapid. We don't like a heavy library or a complex solution. A heavy implementation always costs a lot and kind of slow.  

So whats' our solution?  

One day we found mmap() and inter process mutex.  

https://www.cnblogs.com/huxiao-tee/p/4660352.html  
https://www.ibm.com/developerworks/cn/linux/l-ipc/part5/index1.html  
https://segmentfault.com/a/1190000000630435  

We realized that, this can be the way for our log. It's memory operation which is the most speedy. If the process is terminated, the mapping memory can write to disk automatically. If I use 'tail', we can see the last update, though there's some flaw.  

The last article provides a advanced solution for multiple access of shared memory. But for us, it's not that complex:  

0x01. We just need a master who in charge of creating the mapping file and a lot of slave who just simply read and write.  
0x02. And an increasing write pos is shared across all participant.  
0x03. Everyone manages its read pos. Because the write pos is monotonically increasing, so the read pos could auto adjust and never be overhead.  
0x04. A inter-process mutex to protect the shared memory access.  

You can compile this demo with eclipse or g++. Just remember to add -pthread and -std=c++11 options.  

Then you can run the binary and have a try:  

"./smLogger" : Run as slave  
"./smLogger m" : Run as master, manual input mode  
"./smLogger m r" : Run as master, random mode  
"./smLogger m l" : Run as master, test log format with random data  

Once you run a master and at least a slave. You can see the statistics data on the master console and the message that got from master on the slave console.  

So let's review the requirement we metioned at the begining:

For 1: It's memory operation. It's really speedy (except log formatting, is there's any way to optmise it?).  
For 2: We can use tail to view the log. But it's a pity that there's some messy code at the begining and because it's a ring buffer so the tail command will not works well.(Maybe we can use another mapping file for the extra data?)  
For 3: When the application crashed. The log will keep in file. But when the application restartd, the file will be overwritten (For inprovement, we can use another shared memory to transfer the control data and the master can specify a new log file).  

For A & D: Every involver can write the memory. So the trigger feature and inter porcess access is very easy.  
For B: We can reserve some space as "swap" space, which can implement the filter or callback feature.  

Flaws:  

0. Some additional data attached on the front of log file, which will cause messy code.  
00. It's not friendly for tail. Because the file is pre-allocated and ring-writed. So the -f parameter may not work.  
000. Currently only support C++11, Linux. It's not a cross platform lib. But we can implement another mechanism and keep the interface.  

I think it could have some improvement.  

----------------------------------------------------------------------------------------------------------  

Code design:  

smlogger.h: Main code.  
master.cpp: Test code of master  
slave.cpp: Test code of slave  
main.cpp: main()  

In smlogger.h:  

InterProcessMemory: Shared memory. Provides the pointer of shared memory and the inter-process lock.  
RingBufferShell: An implemention of ring buffer. The buffer and the read/write pointer are injected.  
InterProcessDebugBuffer: The input/output of logger.  
smLogger: The logger access point (singleton) and formater.
	
The log procedure is: smLogger::log() -> InterProcessDebugBuffer::write() -> RingBufferShell::put() -> directlly memory access.  

The call Hierarchy is very short. So this log library can be very fast and high efficiency.  

----------------------------------------------------------------------------------

Update on 2018/12/23:  
Add build option "libsmLogger". Use this build option to build dynamic lib (libsmLogger/libsmLogger.so).  
smlogger.py is a demo to show using smLogger in python as a slave. You can run a master first then run smlogger.py directly and you can see the outputs and triggers on python console.  











