/**
 * Demo：SXLog 日志系统
 * 展示内容：日志级别、双语文本、流式拼接、作用域计时和 Tag 过滤。
 * 来源：项目作者提供。
 */
#include "StellarX.h"

#include <chrono>
#include <iostream>
#include <thread>

using namespace StellarX;

int main()
{
    // 源码保存为 UTF-8，构建时将窄字符串转换为 CP936；控制台同步切换到 GBK。
    SxLogger::setGBK();

    SxLogger& log = SxLogger::Get();
    log.enableConsole(true);
    log.setLanguage(SxLogLanguage::ZhCN);

    // Trace 是最低级别；设置后 Debug、Info、Warn 等更高级别也会输出。
    log.setMinLevel(SxLogLevel::Trace);
    SX_LOGI("Init") << SX_T("日志系统已启用", "logging enabled");
    SX_LOGD("Init") << "minLevel=" << static_cast<int>(log.getMinLevel());

    // RAII 计时对象在离开作用域时自动输出耗时。
    {
        SX_TRACE_SCOPE("Perf", "demo-scope");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    log.setLanguage(SxLogLanguage::EnUS);
    SX_LOGI("Init") << SX_T("这条不会显示中文", "this line is English");

    // 白名单启用后，仅 Init 标签可以继续输出。
    log.setTagFilter(SxTagFilterMode::Whitelist, { "Init" });
    SX_LOGI("Init") << "allowed";
    SX_LOGI("Table") << "blocked (should not appear)";

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    return 0;
}
