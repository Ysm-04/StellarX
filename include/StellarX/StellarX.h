/*******************************************************************************
 * @文件: StellarX.h
 * @摘要: 星垣(StellarX) GUI框架 - 主包含头文件
 * @版本: v2.0.0
 * @描述:
 *     一个为Windows平台打造的轻量级、模块化C++ GUI框架。
 *     基于EasyX图形库，提供简洁易用的API和丰富的控件。
 *
 *     通过包含此单一头文件，即可使用框架的所有功能。
 *     内部包含顺序经过精心设计，确保所有依赖关系正确解析。
 *
 * @作者: 我在人间做废物
 * @邮箱: [3150131407@qq.com] | [ysm3150131407@gmail.com]
 * @仓库: [https://github.com/Ysm-04/StellarX]
 *
 * @许可证: MIT License
 * @版权: Copyright (c) 2025 我在人间做废物
 *
 * @使用说明:
 *     只需包含此文件即可使用框架所有功能。
 *     示例: #include "StellarX.h"
 * @包含顺序:
 *     1. CoreTypes.h - 基础类型定义
 *     2. Control.h   - 控件基类
 *     3. 其他具体控件头文件
 *     4. Dialog：继承自 Canvas（Dialog 为可包含子控件的对话框容器）
 *     5.MessageBox：对话框工厂，提供便捷的模态/非模态调用方式
 ******************************************************************************/

#pragma once

#include "CoreTypes.h"
#include "Control.h"
#include"Button.h"
#include"Window.h"
#include"Label.h"
#include"TextBox.h"
#include"Canvas.h"
#include"Table.h"
#include"Dialog.h"
#include"MessageBox.h"
