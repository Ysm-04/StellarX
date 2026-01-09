#include"StellarX.h"//包含SXLog头文件

#if 1
// 业务场景只需用日志系统时的最简用法示例
/*
 //设置编码 按需开启
 StellarX::SxLogger::setGBK();

 //启用日志文件和控制台输出 按需开启
 StellarX::SxLogger::Get().enableFile("stellarx.log", false, 1024);
 StellarX::SxLogger::Get().enableConsole(true);

 //设置最低日志级别和语言  按需设置
 StellarX::SxLogger::Get().setMinLevel(StellarX::SxLogLevel::Debug);   // Info/Debug/Trace 自己切
 StellarX::SxLogger::Get().setLanguage(StellarX::SxLogLanguage::ZhCN); // ZhCN / EnUS
 */
using namespace StellarX;

int main()
{

	// 1) 可选：把 Windows 控制台切到 GBK，避免中文乱码（内部用 system("chcp 936")）
	SxLogger::setGBK();

	// 2) 获取全局 logger 并开启控制台输出
	SxLogger& log = SxLogger::Get();
	log.enableConsole(true);

	// 3) 最低输出级别（Debug 及以上都会输出）
	log.setMinLevel(SxLogLevel::Debug);

	// 4) 语言选择（影响 SX_T 的文本选择，不做转码）
	log.setLanguage(SxLogLanguage::ZhCN);

	// 5) 打几条日志（流式拼接）
	SX_LOGI("Init") << SX_T("日志系统已启用", "logging enabled");
	SX_LOGD("Init") << "minLevel=" << (int)log.getMinLevel();

	// 6) 作用域耗时统计（只有 Trace 级别打开且 shouldLog 命中才会输出）
	{
		SX_TRACE_SCOPE("Perf", "demo-scope");
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// 7) 切换语言演示
	log.setLanguage(SxLogLanguage::EnUS);
	SX_LOGI("Init") << SX_T("这条不会显示中文", "this line is English");

	// 8) Tag 过滤演示（只允许 Init）
	log.setTagFilter(SxTagFilterMode::Whitelist, { "Init" });
	SX_LOGI("Init") << "allowed";
	SX_LOGI("Table") << "blocked (should not appear)";


	return 0;
}

#endif