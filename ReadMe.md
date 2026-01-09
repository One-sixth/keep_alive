# 保活程序

目标，非常小，占用内存也非常小

用于在任务计划程序崩溃时，自动启动

目前的 exe 使用 keep_alive.cpp 生成
因为 keep_alive.py 会占用过多的内存

首次启动时，会检查目标是否能成功启动，如果不能启动，则不进行保活。

使用示例
keep_alive notepad.exe
keep_alive notepad.exe 123.txt
