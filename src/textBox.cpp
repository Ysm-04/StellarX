// TextBox.cpp
#include "TextBox.h"
#include "SxLog.h"

TextBox::TextBox(int x, int y, int width, int height, std::string text, StellarX::TextBoxmode mode, StellarX::ControlShape shape)
	:Control(x, y, width, height), text(text), mode(mode), shape(shape)
{
	this->id = "TextBox";
}

void TextBox::draw()
{
	if (dirty && show)
	{
		saveStyle();
		setfillcolor(textBoxBkClor);
		setlinecolor(textBoxBorderClor);
		if (textStyle.nHeight > height)
			textStyle.nHeight = height;
		if (textStyle.nWidth > width)
			textStyle.nWidth = width;
		settextstyle(textStyle.nHeight, textStyle.nWidth, textStyle.lpszFace,
			textStyle.nEscapement, textStyle.nOrientation, textStyle.nWeight,
			textStyle.bItalic, textStyle.bUnderline, textStyle.bStrikeOut);

		settextcolor(textStyle.color);
		setbkmode(TRANSPARENT);
		
		int text_width = 0;
		int text_height = 0;
		std::string pwdText;
		std::string displayText;  // 用于显示的文本（可能被截断）
		bool isTextTruncated = false;  // 标记文本是否被截断
		
		if (StellarX::TextBoxmode::PASSWORD_MODE == mode)
		{
			for (size_t i = 0; i < text.size(); ++i)
				pwdText += '*';
			displayText = pwdText;
		}
		else
		{
			displayText = text;
		}

		// 计算可用宽度（留出左右边距）
		int availableWidth = width - 20;  // 左右各10像素边距
		
		// 截断文本以适应可用宽度
		int currentWidth = textwidth(LPCTSTR(displayText.c_str()));
		if (currentWidth > availableWidth && availableWidth > 0)
		{
			// 需要截断文本，预留空间放置省略号
			int ellipsisWidth = textwidth("...");
			int truncatedWidth = availableWidth - ellipsisWidth;
			
			std::string truncatedText = displayText;
			while (truncatedText.size() > 0 && textwidth(LPCTSTR(truncatedText.c_str())) > truncatedWidth)
			{
				truncatedText.pop_back();
			}
			displayText = truncatedText + "...";
			isTextTruncated = true;
			currentWidth = textwidth(LPCTSTR(displayText.c_str()));
		}
		
		text_width = currentWidth;
		text_height = textheight(LPCTSTR(displayText.c_str()));

		if ((saveBkX != this->x) || (saveBkY != this->y) || (!hasSnap) || (saveWidth != this->width) || (saveHeight != this->height) || !saveBkImage)
			saveBackground(this->x, this->y, this->width, this->height);
		// 恢复背景（清除旧内容）
		restBackground();
		//根据形状绘制
		switch (shape)
		{
		case StellarX::ControlShape::RECTANGLE:
			fillrectangle(x, y, x + width, y + height);//有边框填充矩形
			outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(displayText.c_str()));
			break;
		case StellarX::ControlShape::B_RECTANGLE:
			solidrectangle(x, y, x + width, y + height);//无边框填充矩形
			outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(displayText.c_str()));
			break;
		case StellarX::ControlShape::ROUND_RECTANGLE:
			fillroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//有边框填充圆角矩形
			outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(displayText.c_str()));
			break;
		case StellarX::ControlShape::B_ROUND_RECTANGLE:
			solidroundrect(x, y, x + width, y + height, rouRectangleSize.ROUND_RECTANGLEwidth, rouRectangleSize.ROUND_RECTANGLEheight);//无边框填充圆角矩形
			outtextxy(x + 10, (y + (height - text_height) / 2), LPCTSTR(displayText.c_str()));
			break;
		}
		restoreStyle();
		dirty = false;     //标记不需要重绘
	}
}

bool TextBox::handleEvent(const ExMessage& msg)
{
	if (!show) return false;

	bool hover = false;
	bool oldClick = click;
	bool consume = false;

	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
	case StellarX::ControlShape::B_RECTANGLE:
	case StellarX::ControlShape::ROUND_RECTANGLE:
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		hover = (msg.x > x && msg.x < (x + width) && msg.y > y && msg.y < (y + height));
		break;
	default:
		break;
	}

	if (hover && msg.message == WM_LBUTTONUP)
	{
		click = true;

		const size_t oldLen = text.size();
		SX_LOGI("TextBox") << SX_T("激活：id=","activate: id=") << id << " mode=" << (int)mode << " oldLen=" << oldLen;

		if (StellarX::TextBoxmode::INPUT_MODE == mode)
		{
			char* temp = new char[maxCharLen + 1];
			dirty = InputBox(temp, (int)maxCharLen + 1, "输入框", NULL, text.c_str(), NULL, NULL, false);
			if (dirty) text = temp;
			delete[] temp;
			consume = true;
		}
		else if (StellarX::TextBoxmode::READONLY_MODE == mode)
		{
			dirty = false;
			InputBox(NULL, (int)maxCharLen, "输出框（输入无效！）", NULL, text.c_str(), NULL, NULL, false);
			consume = true;
		}
		else if (StellarX::TextBoxmode::PASSWORD_MODE == mode)
		{
			char* temp = new char[maxCharLen + 1];
			// 不记录明文，只记录长度变化
			dirty = InputBox(temp, (int)maxCharLen + 1, "输入框\n不可见输入，覆盖即可", NULL, NULL, NULL, NULL, false);
			if (dirty) text = temp;
			delete[] temp;
			consume = true;
		}

		if (dirty)
		{
			SX_LOGI("TextBox") << SX_T("文本已更改: id=","text changed: id=") << id
				<< " oldLen=" << oldLen << " newLen=" << text.size();
		}
		else
		{
			SX_LOGD("TextBox") << SX_T("文本无变化：id=","no change: id=") << id;
		}

		flushmessage(EX_MOUSE | EX_KEY);
	}

	if (dirty)
		requestRepaint(parent);

	if (click)
		click = false;

	return consume;
}


void TextBox::setMode(StellarX::TextBoxmode mode)
{
	this->mode = mode;
	this->dirty = true;
}

void TextBox::setMaxCharLen(size_t len)
{
	if (len > 0)
		maxCharLen = len;
	this->dirty = true;
}

void TextBox::setTextBoxshape(StellarX::ControlShape shape)
{
	switch (shape)
	{
	case StellarX::ControlShape::RECTANGLE:
	case StellarX::ControlShape::B_RECTANGLE:
	case StellarX::ControlShape::ROUND_RECTANGLE:
	case StellarX::ControlShape::B_ROUND_RECTANGLE:
		this->shape = shape;
		this->dirty = true;
		break;
	case StellarX::ControlShape::CIRCLE:
	case StellarX::ControlShape::B_CIRCLE:
	case StellarX::ControlShape::ELLIPSE:
	case StellarX::ControlShape::B_ELLIPSE:
		this->shape = StellarX::ControlShape::RECTANGLE;
		this->dirty = true;
		break;
	}
}

void TextBox::setTextBoxBorder(COLORREF color)
{
	textBoxBorderClor = color;
	this->dirty = true;
}

void TextBox::setTextBoxBk(COLORREF color)
{
	textBoxBkClor = color;
	this->dirty = true;
}

void TextBox::setText(std::string text)
{
	if(text == this->text)
		return; // 文本未改变，无需更新和重绘
	if (text.size() > maxCharLen)
		text = text.substr(0, maxCharLen);
	this->text = text;
	this->dirty = true; // 标记需要重绘，不论是否窗口图形上下文是否已初始化，等第一次绘制时由窗口真正调用 draw() 来重绘显示文本
	
	//有父控件时请求父控件重绘，无父控件时直接重绘，确保文本更新后界面正确刷新显示
	if (nullptr != parent)
	{
		//通过hasSnap是否持有有效快照,判断控件是否已经绘制过，避免在控件未绘制前/窗口图形上下文未初始化调用draw()导致的错误
		if (hasSnap)
			requestRepaint(parent);
	}
	else
		if (hasSnap)
			draw();
	
}

std::string TextBox::getText() const
{
	return this->text;
}