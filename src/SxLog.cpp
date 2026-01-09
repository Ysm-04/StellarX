#include "SxLog.h"
#include <cstdlib>
#include <clocale>

/********************************************************************************
 * @文件: SxLog.cpp
 * @摘要: StellarX 日志系统实现（过滤/格式化/输出/文件滚动/RAII提交/作用域计时）
 * @描述:
 *     该实现文件主要包含 4 个关键点：
 *     1) FileSink: 文件打开、写入、flush 与按阈值滚动
 *     2) SxLogger: shouldLog 过滤、formatPrefix 前缀拼接、logLine 统一输出出口
 *     3) SxLogLine: 析构提交（RAII）确保“一条语句输出一整行”
 *     4) SxLogScope: 按需启用计时，析构输出耗时
 *
 * @实现难点提示:
 *     - shouldLog 必须“零副作用”，否则宏短路会带来不可预测行为
 *     - logLine 是统一出口，必须保证行级一致性，且避免在持锁状态下递归打日志
 *     - 文件滚动要处理文件名安全性与跨平台 rename 行为差异
 *     - 时间戳生成需要兼容 Windows 与 POSIX（localtime_s/localtime_r）
 ********************************************************************************/

namespace StellarX
{
    // -------- FileSink --------

    // 打开文件输出
    // 难点:
    // - 需要支持追加与清空两种模式
    // - open 前先 close，避免重复打开导致句柄泄漏
    bool FileSink::open(const std::string& path, bool append)
    {
        close();
        filePath = path;
        appendMode = append;

        std::ios::openmode mode = std::ios::out;
        mode |= (append ? std::ios::app : std::ios::trunc);

        ofs.open(path.c_str(), mode);
        return ofs.is_open();
    }

    // 关闭文件输出（可重复调用）
    void FileSink::close()
    {
        if (ofs.is_open()) ofs.close();
    }

    // 查询是否已打开
    bool FileSink::isOpen() const
    {
        return ofs.is_open();
    }

    // 写入一整行
    // 难点:
    // - 写入后若启用 rotateBytes，需要及时检测文件大小是否到阈值
    void FileSink::writeLine(const std::string& line)
    {
        if (!ofs.is_open()) return;
        ofs << line;
        if (rotateBytes > 0) rotateIfNeeded();
    }

    // flush 文件缓冲
    void FileSink::flush()
    {
        if (ofs.is_open()) ofs.flush();
    }

    // 滚动文件
    // 难点:
    // 1) tellp() 返回的是当前写指针位置，通常可近似视为文件大小
    // 2) 时间戳用于文件名时需要做字符清洗，避免出现不友好字符
    // 3) rename 行为与权限/占用有关，失败时需要保证不崩溃（此处选择“尽力而为”）
    bool FileSink::rotateIfNeeded()
    {
        if (!ofs.is_open() || rotateBytes == 0) return false;

        const std::streampos pos = ofs.tellp();
        if (pos < 0) return false;

        const std::size_t size = static_cast<std::size_t>(pos);
        if (size < rotateBytes) return false;

        ofs.flush();
        ofs.close();

        // xxx.log -> xxx.log.YYYYmmdd_HHMMSS
        // 说明:
        // - makeTimestampLocal 形如 "2026-01-09 12:34:56"
        // - 文件名中把 '-' ' ' ':' 替换为 '_'，只保留数字与 '_'，降低环境差异
        const std::string ts = SxLogger::makeTimestampLocal();
        std::string safeTs;
        safeTs.reserve(ts.size());
        for (char ch : ts)
        {
            if (ch >= '0' && ch <= '9') safeTs.push_back(ch);
            else if (ch == '-' || ch == ' ' || ch == ':') safeTs.push_back('_');
        }
        if (safeTs.empty()) safeTs = "rotated";

        const std::string rotated = filePath + "." + safeTs;
        std::rename(filePath.c_str(), rotated.c_str());

        // 重新打开新文件
        // 注意: 这里用 append=false，确保新文件从空开始
        return open(filePath, false);
    }

    // -------- SxLogger --------



    // 设置 Windows 控制台 codepage（只执行一次）
    // 难点:
    // - 只影响终端解释输出字节的方式，不影响源码文件编码
    // - 使用 once_flag 避免重复 system 调用造成噪声与性能浪费
    //
    // 注意:
    // - 下面原注释写“切到 UTF-8”，但实际命令是 chcp 936（GBK）
    // - 为避免改动你原注释，这里补充说明事实，保持行为不变
    void SxLogger::setGBK()
    {
#ifdef _WIN32
        static std::once_flag once;
        std::call_once(once, []() {
            // 切到 UTF-8，避免中文日志在 CP936 控制台下乱码
            // 说明：这不是 WinAPI；是执行系统命令
            std::system("chcp 936 >nul");

            // 补充说明：
            // - chcp 936 实际是设置为 CP936（GBK）
            // - 如果你的终端本身是 UTF-8 环境，调用它可能反而改变显示行为
            // - 该函数建议只在“明确需要 GBK 控制台输出”的场景调用

            // 尝试让 C/C++ 运行库按 UTF-8 工作（对部分流输出有帮助）
           // std::setlocale(LC_ALL, ".UTF8");
            });
#endif
    }

    // 获取单例
    // 难点:
    // - 作为全局入口，初始化必须线程安全
    // - C++11 起函数内静态对象初始化由标准保证线程安全
    SxLogger& SxLogger::Get()
    {
        static SxLogger inst;
        return inst;
    }

    // 构造：设置默认语言
    SxLogger::SxLogger()
        : lang(SxLogLanguage::ZhCN)
    {
    }

    // 设置最低输出级别
    void SxLogger::setMinLevel(SxLogLevel level)
    {
        std::lock_guard<std::mutex> lock(mtx);
        cfg.minLevel = level;
    }

    // 获取最低输出级别
    SxLogLevel SxLogger::getMinLevel() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return cfg.minLevel;
    }

    // 设置语言
    // 难点:
    // - 语言只影响 SX_T 的字符串选择
    // - 这里用 atomic relaxed，避免频繁加锁
    void SxLogger::setLanguage(SxLogLanguage l)
    {
        lang.store(l, std::memory_order_relaxed);
    }

    // 获取语言
    SxLogLanguage SxLogger::getLanguage() const
    {
        return lang.load(std::memory_order_relaxed);
    }

    // 设置 Tag 过滤
    // 难点:
    // - 当前实现是 vector<string> 线性匹配，适合 tag 数量不大
    // - 若未来 tag 很多，可考虑 unordered_set 优化（但会增加依赖与复杂度）
    void SxLogger::setTagFilter(SxTagFilterMode mode, const std::vector<std::string>& tags)
    {
        std::lock_guard<std::mutex> lock(mtx);
        cfg.tagFilterMode = mode;
        cfg.tagList = tags;
    }

    // 清空 Tag 过滤
    void SxLogger::clearTagFilter()
    {
        std::lock_guard<std::mutex> lock(mtx);
        cfg.tagFilterMode = SxTagFilterMode::None;
        cfg.tagList.clear();
    }

    // 开关控制台输出
    // 难点:
    // - ConsoleSink 持有 ostream 引用，不管理其生命周期
    void SxLogger::enableConsole(bool enable)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (enable)
        {
            if (!consoleSink) consoleSink.reset(new ConsoleSink(std::cout));
        }
        else
        {
            consoleSink.reset();
        }
    }

    // 开启文件输出
    // 难点:
    // - enableFile 成功与否决定 cfg.fileEnabled
    // - 需要把 rotateBytes 同步到 FileSink
    bool SxLogger::enableFile(const std::string& path, bool append, std::size_t rotateBytes_)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (!fileSink) fileSink.reset(new FileSink());
        fileSink->setRotateBytes(rotateBytes_);

        const bool ok = fileSink->open(path, append);
        cfg.fileEnabled = ok;
        cfg.filePath = path;
        cfg.fileAppend = append;
        cfg.rotateBytes = rotateBytes_;
        return ok;
    }

    // 关闭文件输出
    void SxLogger::disableFile()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (fileSink) fileSink->close();
        cfg.fileEnabled = false;
    }

    // 获取配置副本
    // 难点:
    // - 返回副本避免外部拿到内部引用后绕过锁修改
    SxLogConfig SxLogger::getConfigCopy() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return cfg;
    }

    // 设置配置（整体替换）
    void SxLogger::setConfig(const SxLogConfig& c)
    {
        std::lock_guard<std::mutex> lock(mtx);
        cfg = c;
    }

    // 级别转字符串
    const char* SxLogger::levelToString(SxLogLevel level)
    {
        switch (level)
        {
        case SxLogLevel::Trace: return "TRACE";
        case SxLogLevel::Debug: return "DEBUG";
        case SxLogLevel::Info:  return "INFO ";
        case SxLogLevel::Warn:  return "WARN ";
        case SxLogLevel::Error: return "ERROR";
        case SxLogLevel::Fatal: return "FATAL";
        default:                return "OFF  ";
        }
    }

    // 判断 tag 是否允许输出
    // 难点:
    // - 精确匹配 tag 字符串
    // - tag==nullptr 时默认允许，避免“无 tag 日志被误杀”
    bool SxLogger::tagAllowed(const SxLogConfig& c, const char* tag)
    {
        if (c.tagFilterMode == SxTagFilterMode::None) return true;
        if (!tag) return true;

        bool found = false;
        for (const auto& t : c.tagList)
        {
            if (t == tag) { found = true; break; }
        }

        if (c.tagFilterMode == SxTagFilterMode::Whitelist) return found;
        if (c.tagFilterMode == SxTagFilterMode::Blacklist) return !found;
        return true;
    }

    // 快速判定是否需要输出（宏短路依赖）
    // 难点:
    // 1) 必须无副作用：返回 false 时调用端不会构造对象也不会拼接
    // 2) 过滤维度要完整：级别、tag、sink 是否启用
    // 3) 当前实现加锁保证 cfg 与 sink 状态一致；代价是高频路径会有锁开销
    bool SxLogger::shouldLog(SxLogLevel level, const char* tag) const
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (cfg.minLevel == SxLogLevel::Off) return false;
        if (level < cfg.minLevel) return false;
        if (!tagAllowed(cfg, tag)) return false;
        if (!consoleSink && !cfg.fileEnabled) return false;
        return true;
    }

    // 生成本地时间戳字符串
    // 难点:
    // - Windows 与 POSIX 的线程安全 localtime API 不同
    std::string SxLogger::makeTimestampLocal()
    {
        using namespace std::chrono;
        const auto now = system_clock::now();
        const std::time_t t = system_clock::to_time_t(now);

        std::tm tmv{};
#if defined(_WIN32)
        localtime_s(&tmv, &t);
#else
        localtime_r(&t, &tmv);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmv, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    // 拼接日志前缀（调用方已持锁）
    // 难点:
    // - 前缀拼接必须与配置项严格对应，且尽量避免多余开销
    // - showSource 会输出 (file:line func)，对定位时序问题很有价值
    std::string SxLogger::formatPrefixUnlocked(
        const SxLogConfig& c,
        SxLogLevel level,
        const char* tag,
        const char* file,
        int line,
        const char* func) const
    {
        std::ostringstream oss;

        if (c.showTimestamp) oss << "[" << makeTimestampLocal() << "] ";
        if (c.showLevel)     oss << "[" << levelToString(level) << "] ";
        if (c.showTag && tag) oss << "[" << tag << "] ";

        if (c.showThreadId)
        {
            oss << "[T:" << std::this_thread::get_id() << "] ";
        }

        if (c.showSource && file && func)
        {
            oss << "(" << file << ":" << line << " " << func << ") ";
        }

        return oss.str();
    }

    // 统一输出出口
    // 难点:
    // 1) 行级一致性：必须把 prefix + msg + "\n" 当作整体写入
    // 2) 线程安全：持锁写入可避免不同线程日志互相穿插
    // 3) 避免重入：在持锁期间不要再调用 SX_LOG...（会导致死锁）
    void SxLogger::logLine(
        SxLogLevel level,
        const char* tag,
        const char* file,
        int line,
        const char* func,
        const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (cfg.minLevel == SxLogLevel::Off) return;
        if (level < cfg.minLevel) return;
        if (!tagAllowed(cfg, tag)) return;

        const std::string prefix = formatPrefixUnlocked(cfg, level, tag, file, line, func);
        const std::string lineText = prefix + msg + "\n";

        if (consoleSink) consoleSink->writeLine(lineText);

        if (cfg.fileEnabled && fileSink && fileSink->isOpen())
        {
            fileSink->writeLine(lineText);
        }

        if (cfg.autoFlush)
        {
            if (consoleSink) consoleSink->flush();
            if (cfg.fileEnabled && fileSink) fileSink->flush();
        }
    }

    // -------- SxLogLine --------

    // 构造：只记录元信息
    SxLogLine::SxLogLine(SxLogLevel level, const char* tag, const char* file, int line, const char* func)
        : lvl(level), tg(tag), srcFile(file), srcLine(line), srcFunc(func)
    {
    }

    // 析构：提交输出
    // 难点:
    // - 这是 RAII 设计的核心：保证语句结束时日志自动落地
    // - 也要求调用端不要把临时对象跨语句保存（宏用法本身也不支持那样做）
    SxLogLine::~SxLogLine()
    {
        SxLogger::Get().logLine(lvl, tg, srcFile, srcLine, srcFunc, ss.str());
    }

    // -------- SxLogScope --------

    // 构造：按需启用计时
    // 难点:
    // - 只有 shouldLog 为 true 才记录起点，避免在未输出场景做无意义计时
    SxLogScope::SxLogScope(SxLogLevel level, const char* tag, const char* file, int line, const char* func, const char* name)
        : lvl(level), tg(tag), srcFile(file), srcLine(line), srcFunc(func), scopeName(name)
    {
        enabled = SxLogger::Get().shouldLog(lvl, tg);
        if (enabled) t0 = std::chrono::steady_clock::now();
    }

    // 析构：输出耗时
    // 难点:
    // - steady_clock 用于衡量耗时，避免系统时间调整造成跳变
    SxLogScope::~SxLogScope()
    {
        if (!enabled) return;
        const auto t1 = std::chrono::steady_clock::now();
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        std::ostringstream oss;
        oss << "SCOPE " << (scopeName ? scopeName : "") << " cost=" << us << "us";
        SxLogger::Get().logLine(lvl, tg, srcFile, srcLine, srcFunc, oss.str());
    }

} // namespace StellarX
