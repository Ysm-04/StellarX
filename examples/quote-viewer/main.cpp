#include "StellarX.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>

using namespace std;
using namespace StellarX;

/*
 * Demo：名言浏览器
 * 展示内容：使用固定数据集合、前后翻页按钮和 Label 动态更新文本。
 * 来源：熊猫老师反馈并提供。
 */
namespace
{
struct Quote
{
    string text;
    string source;
};

// 使用公版古典名句，避免示例程序引入现代文本的版权和错误归属问题。
const array<Quote, 6> quotes = {
    Quote{"学而不思则罔，思而不学则殆。", "《论语·为政》"},
    Quote{"千里之行，始于足下。", "《道德经》第六十四章"},
    Quote{"锲而不舍，金石可镂。", "《荀子·劝学》"},
    Quote{"路漫漫其修远兮，吾将上下而求索。", "屈原《离骚》"},
    Quote{"会当凌绝顶，一览众山小。", "杜甫《望岳》"},
    Quote{"纸上得来终觉浅，绝知此事要躬行。", "陆游《冬夜读书示子聿》"}
};
}

int main()
{
    const COLORREF backgroundColor = RGB(135, 206, 235);
    Window win(760, 430, 0, backgroundColor, "名言浏览器");

    // quote/source/progress 三个 Label 分别显示正文、出处和当前位置。
    auto title = make_unique<Label>(255, 35, "名言浏览器", RGB(255, 255, 255), backgroundColor);
    title->textStyle.nHeight = 40;
    auto quote = make_unique<Label>(70, 125, quotes[0].text, RGB(255, 255, 255), backgroundColor);
    quote->textStyle.nHeight = 32;
    quote->textStyle.lpszFace = "华文楷体";
    auto source = make_unique<Label>(450, 205, quotes[0].source, RGB(30, 30, 30), backgroundColor);
    source->textStyle.nHeight = 24;
    source->textStyle.lpszFace = "华文楷体";
    auto progress = make_unique<Label>(350, 270, "1 / 6", RGB(70, 70, 70), backgroundColor);
    progress->textStyle.nHeight = 20;

    auto previous = make_unique<Button>(235, 340, 120, 36, "上一条", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto next = make_unique<Button>(405, 340, 120, 36, "下一条", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);

    Label* quotePtr = quote.get();
    Label* sourcePtr = source.get();
    Label* progressPtr = progress.get();
    size_t index = 0;

    // 集中刷新所有动态 Label，确保翻页后正文、出处和页码保持同步。
    auto refresh = [&index, quotePtr, sourcePtr, progressPtr]()
        {
            quotePtr->setText(quotes[index].text);
            sourcePtr->setText(quotes[index].source);
            progressPtr->setText(to_string(index + 1) + " / " + to_string(quotes.size()));
            quotePtr->draw();
            sourcePtr->draw();
            progressPtr->draw();
        };

    // 模运算让第一条向前翻时跳到末尾，最后一条向后翻时回到开头。
    previous->setOnClickListener([&index, &refresh]()
        {
            index = (index + quotes.size() - 1) % quotes.size();
            refresh();
        });
    next->setOnClickListener([&index, &refresh]()
        {
            index = (index + 1) % quotes.size();
            refresh();
        });

    // 所有权转移给 Window 后进入 GUI 事件循环。
    win.addControl(move(title));
    win.addControl(move(quote));
    win.addControl(move(source));
    win.addControl(move(progress));
    win.addControl(move(previous));
    win.addControl(move(next));

    win.draw();
    return win.runEventLoop();
}
