#pragma once

/********************************************************************************
 * @文件: SxLog.h
 * @摘要: StellarX 日志系统对外接口定义（控制台/文件输出 + 级别过滤 + Tag过滤 + 中英文选择）
 * @描述:
 *     该日志系统采用“宏 + RAII(析构提交)”的方式实现：
 *       - 调用端通过 SX_LOGD/SX_LOGI... 写日志
 *       - 宏内部先 shouldLog 短路过滤，未命中时不构造对象、不拼接字符串
 *       - 命中时构造 SxLogLine，使用 operator<< 拼接内容
 *       - 语句结束时 SxLogLine 析构，统一提交到 SxLogger::logLine 输出
 *
 *     输出通道（Sink）目前提供：
 *       - ConsoleSink: 写入 std::cout（不走 WinAPI 调试输出通道）
 *       - FileSink: 写入文件，支持按字节阈值滚动
 *
 * @特性:
 *     - 日志级别：Trace/Debug/Info/Warn/Error/Fatal/Off
 *     - Tag 过滤：None/Whitelist/Blacklist
 *     - 可选前缀：时间戳/级别/Tag/线程ID/源码位置
 *     - 中英文选择：SX_T(zh, en) / setLanguage
 *     - 文件滚动：rotateBytes > 0 时按阈值滚动
 *
 * @使用场景:
 *     - 排查重绘链路、脏标记传播、Tab 切换、Table 数据刷新等时序问题
 *     - 输出可复现日志，配合回归验证
 *
 * @注意:
 *     - SX_T 仅做“字符串选择”，不做编码转换
 *     - 控制台显示是否乱码由“终端 codepage/字体/环境”决定
 *     - 该头文件只声明接口，实现位于 SxLog.cpp
 *
 * @所属框架: 星垣(StellarX) GUI框架
 * @作者: 我在人间做废物
 ********************************************************************************/

 // SxLog.h - header-only interface (implementation in SxLog.cpp)
 // Pure standard library: std::cout and optional file sink.

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef SX_LOG_ENABLE
#define SX_LOG_ENABLE 1
#endif

namespace StellarX
{
    /* ========================= 日志级别 ========================= */
    // 说明：
    // - minLevel 表示最低输出级别，小于 minLevel 的日志会被 shouldLog 直接过滤
    // - Off 表示全局关闭
    enum class SxLogLevel : int
    {
        Trace = 0,   // 最细粒度：高频路径追踪（谨慎开启）
        Debug = 1,   // 调试信息：状态变化/关键分支
        Info = 2,   // 业务信息：关键流程节点
        Warn = 3,   // 警告：非致命但异常的情况
        Error = 4,   // 错误：功能失败、需要关注
        Fatal = 5,   // 致命：通常意味着无法继续运行
        Off = 6    // 关闭全部日志
    };

    /* ========================= 语言选择 ========================= */
    // 说明：仅用于 SX_T 选择输出哪一段文本，不做编码转换
    enum class SxLogLanguage : int
    {
        ZhCN = 0,    // 中文
        EnUS = 1     // 英文
    };

    /* ========================= Tag 过滤模式 ========================= */
    // None      : 不过滤，全部输出
    // Whitelist : 只输出 tagList 中包含的 tag
    // Blacklist : 输出除 tagList 以外的 tag
    enum class SxTagFilterMode : int
    {
        None = 0,
        Whitelist = 1,
        Blacklist = 2
    };

    /* ========================= 日志配置 ========================= */
    // 说明：SxLogger 内部持有该配置，shouldLog 与 logLine 都依赖它
    struct SxLogConfig
    {
        SxLogLevel minLevel = SxLogLevel::Info; // 最低输出级别

        bool showTimestamp = true;   // 是否输出时间戳前缀
        bool showLevel = true;       // 是否输出级别前缀
        bool showTag = true;         // 是否输出 tag 前缀
        bool showThreadId = false;   // 是否输出线程ID（排查并发时开启）
        bool showSource = false;     // 是否输出源码位置（file:line func）
        bool autoFlush = true;       // 每行写完是否 flush（排查问题更稳，性能略差）

        SxTagFilterMode tagFilterMode = SxTagFilterMode::None; // Tag 过滤模式
        std::vector<std::string> tagList;                      // Tag 列表（白名单/黑名单）

        bool fileEnabled = false;     // 文件输出是否启用（enableFile 成功才为 true）
        std::string filePath;         // 文件路径
        bool fileAppend = true;       // 是否追加写入
        std::size_t rotateBytes = 0;  // 滚动阈值（0 表示不滚动）
    };

    /* ========================= Sink 接口 ========================= */
    // 说明：
    // - Sink 负责“把完整的一行日志写到某个地方”
    // - SxLogger 负责过滤/格式化/分发
    class ILogSink
    {
    public:
        virtual ~ILogSink() = default;

        // 返回 Sink 名称，用于调试识别（例如 "console"/"file"）
        virtual const char* name() const = 0;

        // 写入一整行（调用方保证 line 已包含换行或按约定追加换行）
        virtual void writeLine(const std::string& line) = 0;

        // 刷新缓冲（可选实现）
        virtual void flush() {}
    };

    /* ========================= 控制台输出 Sink ========================= */
    // 作用：把日志写入指定输出流（默认用 std::cout）
    class ConsoleSink : public ILogSink
    {
    public:
        // 绑定一个输出流引用（常见用法：std::cout）
        explicit ConsoleSink(std::ostream& os) : out(os) {}

        const char* name() const override { return "console"; }

        // 写入一行（不自动追加换行，换行由上层统一拼接）
        void writeLine(const std::string& line) override { out << line; }

        // 立即 flush（当 autoFlush=true 时由 SxLogger 调用）
        void flush() override { out.flush(); }

    private:
        std::ostream& out; // 输出流引用（不负责生命周期）
    };

    /* ========================= 文件输出 Sink ========================= */
    // 作用：把日志写入文件，支持按字节阈值滚动
    class FileSink : public ILogSink
    {
    public:
        FileSink() = default;

        const char* name() const override { return "file"; }

        // 打开文件
        // path   : 文件路径
        // append : true 追加写；false 清空重写
        bool open(const std::string& path, bool append);

        // 关闭文件（安全可重复调用）
        void close();

        // 查询文件是否处于打开状态
        bool isOpen() const;

        // 设置滚动阈值（字节）
        // bytes = 0 表示不滚动
        void setRotateBytes(std::size_t bytes) { rotateBytes = bytes; }

        // 写入一行，并在需要时触发滚动
        void writeLine(const std::string& line) override;

        // flush 文件缓冲
        void flush() override;

    private:
        // 检查并执行滚动
        // 返回值：是否发生滚动（或是否重新打开）
        bool rotateIfNeeded();

        std::ofstream ofs;       // 文件输出流
        std::string filePath;    // 当前文件路径
        bool appendMode = true;  // 是否追加模式（用于 reopen）
        std::size_t rotateBytes = 0; // 滚动阈值
    };

    /* ========================= 日志中心 SxLogger ========================= */
    // 作用：
    // - 保存配置（SxLogConfig）
    // - 过滤（level/tag/sink enabled）
    // - 格式化前缀（时间/级别/tag/线程/源码位置）
    // - 分发到 console/file 等 sink
    class SxLogger
    {
    public:
        // 仅用于 Windows 控制台：把 codepage 切到 GBK，解决中文乱码。
        // 不使用 WinAPI：内部通过 system("chcp 936") 实现
        // 注意：这只影响终端解释输出字节的方式，不影响源码文件编码
        static void setGBK();

        // 获取全局单例
        // 说明：函数内静态对象，C++11 起保证线程安全初始化
        static SxLogger& Get();

        // 设置最低输出级别
        void setMinLevel(SxLogLevel level);

        // 获取最低输出级别
        SxLogLevel getMinLevel() const;

        // 设置语言（用于 SX_T 选择）
        void setLanguage(SxLogLanguage lang);

        // 获取当前语言
        SxLogLanguage getLanguage() const;

        // 设置 Tag 过滤
        // mode: None/Whitelist/Blacklist
        // tags: 过滤列表（精确匹配）
        void setTagFilter(SxTagFilterMode mode, const std::vector<std::string>& tags);

        // 清空 Tag 过滤（恢复 None）
        void clearTagFilter();

        // 开关控制台输出
        void enableConsole(bool enable);

        // 开启文件输出
        // path       : 文件路径
        // append     : 追加写/清空写
        // rotateBytes: 滚动阈值（0 不滚动）
        // 返回值：是否打开成功
        bool enableFile(const std::string& path, bool append = true, std::size_t rotateBytes = 0);

        // 关闭文件输出（不影响控制台输出）
        void disableFile();

        // 快速判定是否需要输出（宏层面的短路依赖它）
        // 说明：
        // - shouldLog 一定要“副作用为 0”
        // - 若返回 false，调用端不会创建 SxLogLine，也不会拼接字符串
        bool shouldLog(SxLogLevel level, const char* tag) const;

        // 输出一条完整日志
        // 说明：这是统一出口，SxLogLine 析构最终会走到这里
        void logLine(
            SxLogLevel level,
            const char* tag,
            const char* file,
            int line,
            const char* func,
            const std::string& msg);

        // 获取配置副本（避免外部直接改内部 cfg）
        SxLogConfig getConfigCopy() const;

        // 批量设置配置（整体替换）
        void setConfig(const SxLogConfig& cfg);

        // 工具：把级别转为字符串（用于前缀）
        static const char* levelToString(SxLogLevel level);

        // 工具：生成本地时间戳字符串（用于前缀与文件滚动名）
        static std::string makeTimestampLocal();

    private:
        SxLogger();

        // 判断 tag 是否允许输出（根据 Tag 过滤模式与 tagList）
        static bool tagAllowed(const SxLogConfig& cfg, const char* tag);

        // 生成前缀（调用方需已持有锁）
        std::string formatPrefixUnlocked(
            const SxLogConfig& cfg,
            SxLogLevel level,
            const char* tag,
            const char* file,
            int line,
            const char* func) const;

        mutable std::mutex mtx;           // 保护 cfg 与 sink 写入，确保多线程行级一致性
        SxLogConfig cfg;                  // 当前配置
        std::atomic<SxLogLanguage> lang;  // 语言开关（仅影响 SX_T 选择）

        std::unique_ptr<ConsoleSink> consoleSink; // 控制台 sink（enableConsole 控制）
        std::unique_ptr<FileSink> fileSink;       // 文件 sink（enableFile 控制）
    };

    /* ========================= 双语选择辅助 ========================= */
    // 说明：
    // - 只做“选择 zhCN 或 enUS”，不做编码转换
    // - 输出显示是否正常由终端环境决定
    inline const char* SxT(const char* zhCN, const char* enUS)
    {
        return (SxLogger::Get().getLanguage() == SxLogLanguage::ZhCN) ? zhCN : enUS;
    }

#if defined(__cpp_char8_t) && (__cpp_char8_t >= 201811L)
    // 说明：
    // - C++20 的 u8"xxx" 是 char8_t*，为了兼容调用端，这里提供重载
    // - reinterpret_cast 只是改指针类型，不做 UTF-8 -> GBK 转码
    inline const char* SxT(const char8_t* zhCN, const char* enUS)
    {
        return (SxLogger::Get().getLanguage() == SxLogLanguage::ZhCN)
            ? reinterpret_cast<const char*>(zhCN)
            : enUS;
    }
#endif

    /* ========================= RAII 日志行对象 ========================= */
    // 作用：
    // - 构造时记录 level/tag/源码位置
    // - operator<< 拼接内容
    // - 析构时统一提交给 SxLogger::logLine 输出
    //
    // 设计意义：
    // - 避免调用端忘记写换行
    // - 保证一行日志作为整体写出
    class SxLogLine
    {
    public:
        // 构造：记录元信息（不输出）
        SxLogLine(SxLogLevel level, const char* tag, const char* file, int line, const char* func);

        // 析构：提交输出（真正写出发生在这里）
        ~SxLogLine();

        // 拼接内容（流式写法）
        template<typename T>
        SxLogLine& operator<<(const T& v)
        {
            ss << v;
            return *this;
        }

    private:
        SxLogLevel lvl;          // 日志级别
        const char* tg;          // Tag（不拥有内存）
        const char* srcFile;     // 源文件名（来自 __FILE__）
        int srcLine;             // 行号（来自 __LINE__）
        const char* srcFunc;     // 函数名（来自 __func__）
        std::ostringstream ss;   // 内容拼接缓冲
    };

    /* ========================= RAII 作用域计时对象 ========================= */
    // 作用：
    // - 仅在 shouldLog(Trace, tag) 为 true 时启用计时
    // - 析构时输出耗时（微秒）
    //
    // 使用建议：
    // - 只在需要定位性能瓶颈时开启 Trace
    // - name 建议传入常量字符串，便于检索
    class SxLogScope
    {
    public:
        // 构造：根据 shouldLog 决定是否启用计时
        SxLogScope(SxLogLevel level, const char* tag, const char* file, int line, const char* func, const char* name);

        // 析构：若启用则输出耗时
        ~SxLogScope();

    private:
        bool enabled = false;    // 是否启用（未启用则析构无输出）
        SxLogLevel lvl = SxLogLevel::Trace;  // 级别（通常用 Trace）
        const char* tg = nullptr;            // Tag
        const char* srcFile = nullptr;       // 源文件
        int srcLine = 0;                     // 行号
        const char* srcFunc = nullptr;       // 函数
        const char* scopeName = nullptr;     // 作用域名
        std::chrono::steady_clock::time_point t0; // 起始时间点
    };

} // namespace StellarX

#if SX_LOG_ENABLE

// SX_T：双语选择宏，调用 SxT 根据当前语言选择输出
#define SX_T(zh, en) ::StellarX::SxT(zh, en)

// 日志宏说明：
// 1) 先 shouldLog 短路过滤，未命中则不会构造 SxLogLine，也不会执行 else 分支的表达式
// 2) 命中则构造临时 SxLogLine，并允许继续使用 operator<< 拼接
// 3) 语句结束时临时对象析构，触发真正输出
#define SX_LOG_TRACE(tag) if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Trace, tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Trace, tag, __FILE__, __LINE__, __func__)
#define SX_LOGD(tag)      if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Debug, tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Debug, tag, __FILE__, __LINE__, __func__)
#define SX_LOGI(tag)      if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Info,  tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Info,  tag, __FILE__, __LINE__, __func__)
#define SX_LOGW(tag)      if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Warn,  tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Warn,  tag, __FILE__, __LINE__, __func__)
#define SX_LOGE(tag)      if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Error, tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Error, tag, __FILE__, __LINE__, __func__)
#define SX_LOGF(tag)      if(!::StellarX::SxLogger::Get().shouldLog(::StellarX::SxLogLevel::Fatal, tag)) ; else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Fatal, tag, __FILE__, __LINE__, __func__)

// 作用域耗时统计宏：默认用 Trace 级别
#define SX_TRACE_SCOPE(tag, nameLiteral) ::StellarX::SxLogScope sx_scope_##__LINE__(::StellarX::SxLogLevel::Trace, tag, __FILE__, __LINE__, __func__, nameLiteral)

#else

// 关闭日志时的兼容宏：保证调用端代码不需要改动
#define SX_T(zh, en) (en)
#define SX_LOG_TRACE(tag) if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_LOGD(tag)      if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_LOGI(tag)      if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_LOGW(tag)      if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_LOGE(tag)      if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_LOGF(tag)      if(true) {} else ::StellarX::SxLogLine(::StellarX::SxLogLevel::Off, tag, "", 0, "")
#define SX_TRACE_SCOPE(tag, nameLiteral) do {} while(0)

#endif
