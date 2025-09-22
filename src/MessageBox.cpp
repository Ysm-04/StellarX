#include "MessageBox.h"

namespace StellarX
{
	MessageBoxResult MessageBox::ShowModal(Window& wnd,const std::string& text,const std::string& caption,
		MessageBoxType type)
	{
		Dialog dlg(wnd, caption, text, type, true); // 模态
		dlg.Show();
		return dlg.GetResult();
	}

	void MessageBox::ShowAsync(Window& wnd,const std::string& text,const std::string& caption,MessageBoxType type,
		std::function<void(MessageBoxResult)> onResult)
	{
		//去重，如果窗口内已有相同的对话框被触发，则不再创建
		if (wnd.hasNonModalDialogWithCaption(caption))
		{
			std::cout << "\a" << std::endl; 
			return;
		}
		auto dlg = std::make_unique<Dialog>(wnd, caption, text, type, false); // 非模态
		Dialog* dlgPtr = dlg.get();

		// 设置回调
		if (onResult)
			dlgPtr->SetResultCallback(std::move(onResult));
		// 交给 Window 管理生命周期
		wnd.addDialog(std::move(dlg));
		dlgPtr->Show();
	}
}
