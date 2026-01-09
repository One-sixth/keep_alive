#if !_DEBUG
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"wmainCRTStartup\"" ) // 设置入口地址
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <format>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <memory>

using namespace std;


wstring get_error_text(DWORD errCode)
{
    wstring err;
    LPWSTR lpBuffer = NULL;
    if (0 == FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, //标志位，决定如何说明lpSource参数，dwFlags的低位指定如何处理换行功能在输出缓冲区，也决定最大宽度的格式化输出行,可选参数。
        NULL,//根据dwFlags标志而定。
        errCode,//请求的消息的标识符。当dwFlags标志为FORMAT_MESSAGE_FROM_STRING时会被忽略。
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),//请求的消息的语言标识符。
        (LPWSTR)&lpBuffer,//接收错误信息描述的缓冲区指针。
        0,//如果FORMAT_MESSAGE_ALLOCATE_BUFFER标志没有被指定，这个参数必须指定为输出缓冲区的大小，如果指定值为0，这个参数指定为分配给输出缓冲区的最小数。
        NULL//保存格式化信息中的插入值的一个数组。
    )){
        //失败
        err = format(L"未定义错误描述()", errCode);
    }
    else {
        //成功
        err = wstring(lpBuffer);
        LocalFree(lpBuffer);
    }
    return err;
}


class ProcessKeeper {
private:
    wstring processPath;
    HANDLE hProcess;
    DWORD processId;

public:
    ProcessKeeper(const wstring& path) : processPath(path), hProcess(NULL), processId(0) {}

    ~ProcessKeeper() {
        if (hProcess) {
            CloseHandle(hProcess);
        }
    }

    bool isProcessAlive() {
        if (hProcess == NULL) {
            return false;
        }

        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            // 如果进程仍在运行，退出代码是 STILL_ACTIVE
            return (exitCode == STILL_ACTIVE);
        }

        // 如果无法获取退出代码，假设进程已终止
        return false;
    }

    bool startProcess() {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        // 创建可修改的命令行字符串
        wstring cmdLine = processPath + L"\0";

        BOOL success = CreateProcessW(
            nullptr,                    // 应用程序名（使用命令行）
            cmdLine.data(),            // 命令行
            nullptr,                    // 进程安全属性
            nullptr,                    // 线程安全属性
            FALSE,                      // 不继承句柄
            0,                          // 创建标志
            nullptr,                    // 环境块
            nullptr,                    // 当前目录
            &si,                        // 启动信息
            &pi                         // 进程信息
        );

        if (success) {
            // 保存进程句柄和ID
            if (hProcess) {
                CloseHandle(hProcess);
            }
            hProcess = pi.hProcess;
            processId = pi.dwProcessId;

            // 不需要线程句柄，关闭它
            CloseHandle(pi.hThread);

            wcout << L"成功启动进程" << endl;
            wcout << L"进程ID: " << processId << endl;
            return true;
        }
        else {
            wcout << L"启动进程失败，错误信息: " << get_error_text(GetLastError()) << endl;
            return false;
        }
    }

    void keepAlive() {
        wcout << L"开始保活进程" << endl;
        wcout << L"按 Ctrl+C 退出保活程序\n" << endl;

        // 首次启动进程
        if (!startProcess()) {
            wcout << L"首次启动失败，程序退出" << endl;
            return;
        }

        while (true) {
            if (!isProcessAlive()) {
                wcout << L"进程已终止，正在重新启动..." << endl;
                if (!startProcess()) {
                    wcout << L"启动失败，5秒后重试..." << endl;
                    this_thread::sleep_for(chrono::seconds(5));
                }
            }

            // 每1秒检查一次
            this_thread::sleep_for(chrono::seconds(5));
        }
    }
};


void print_usage(wstring_view program_name) {
    wcout << L"用法: " << program_name << L" <命令行>..." << endl;
    wcout << L"注意，命令行参数至少有1个" << endl;
    wcout << L"示例: " << program_name << L" C:\\Windows\\notepad.exe" << endl;
    wcout << L"示例: " << program_name << L" myapp.exe \"C:\\Program Files\\MyApp\\myapp.exe\" -arg1 -arg2" << endl;
}

int wmain(int argc, wchar_t* argv[]) {

    // 必须设定语言区域，不然 wcout 看不到输出
    wcout.imbue(locale("zh_CN"));

    // 检查参数
    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    wstring command_line;

    for (int i = 1; i < argc; ++i)
    {
        wstring p = wstring(argv[i]);

        // 如果参数有空格，就用双引号括起来
        if (p.find(L" ") != -1)
            p = L"\"" + p + L"\"";

        if (i > 1)
            p = L" " + p;

        command_line += p;
    }
    wcout << L"命令行：\t" << command_line << endl;

    ProcessKeeper keeper(command_line);
    keeper.keepAlive();

    return 0;
}
