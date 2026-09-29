#include "StellarX.h"

#include <memory>
#include <string>

using namespace std;
using namespace StellarX;

/*
 * Demo：篮球比赛计分牌
 * 展示内容：Window、Label、TextBox、Button 的组合使用，以及按钮回调更新界面状态。
 * 来源：熊猫老师反馈并提供。
 */
namespace
{
// 分数不足两位时补零，让计分牌在 0~9 分时保持统一的视觉宽度。
string formatScore(int score)
{
    return score < 10 ? "0" + to_string(score) : to_string(score);
}

// 将“加几分”的公共行为绑定到按钮，避免为六个按钮重复编写相同逻辑。
// score 和 scoreLabel 指向 main 事件循环期间一直有效的状态与控件。
void bindScoreButton(Button* button, int points, int* score, Label* scoreLabel)
{
    button->setOnClickListener([points, score, scoreLabel]()
        {
            *score += points;
            scoreLabel->setText(formatScore(*score));
        });
}
}

int main()
{
    // 创建主窗口：最后一个数值 0 表示使用普通 EasyX 窗口模式。
    Window win(640, 420, 0, RGB(135, 206, 235), "篮球比赛计分牌");

    // 标题和队名输入框。队名输入框本身即可保存并显示用户输入。
    auto title = make_unique<Label>(205, 25, "篮球比赛计分牌", RGB(255, 255, 255), RGB(135, 206, 235));
    title->textStyle.nHeight = 36;

    auto teamA = make_unique<TextBox>(100, 75, 160, 30, "红队", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);
    auto teamB = make_unique<TextBox>(380, 75, 160, 30, "蓝队", TextBoxmode::INPUT_MODE, ControlShape::ROUND_RECTANGLE);

    // 两个大号 Label 用于显示双方实时比分。
    auto scoreAView = make_unique<Label>(130, 115, "00", RGB(255, 255, 255), RGB(220, 60, 60));
    auto scoreBView = make_unique<Label>(410, 115, "00", RGB(255, 255, 255), RGB(65, 105, 225));
    auto versus = make_unique<Label>(295, 135, "VS", RGB(30, 30, 30), RGB(135, 206, 235));
    scoreAView->textStyle.nHeight = 100;
    scoreBView->textStyle.nHeight = 100;
    versus->textStyle.nHeight = 38;

    // 每队分别提供 1、2、3 分按钮，并提供统一的比分重置按钮。
    auto addA1 = make_unique<Button>(70, 255, 70, 36, "+1", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto addA2 = make_unique<Button>(155, 255, 70, 36, "+2", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto addA3 = make_unique<Button>(240, 255, 70, 36, "+3", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto addB1 = make_unique<Button>(330, 255, 70, 36, "+1", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto addB2 = make_unique<Button>(415, 255, 70, 36, "+2", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto addB3 = make_unique<Button>(500, 255, 70, 36, "+3", ButtonMode::NORMAL, ControlShape::ROUND_RECTANGLE);
    auto reset = make_unique<Button>(260, 335, 120, 38, "重置比分", ButtonMode::NORMAL, ControlShape::B_ROUND_RECTANGLE);

    // addControl 会转移 unique_ptr 所有权，因此在 move 前保存需要在回调中访问的控件地址。
    // 控件随后由 Window 持有，直至事件循环结束，所以这些指针在回调期间有效。
    Label* scoreAPtr = scoreAView.get();
    Label* scoreBPtr = scoreBView.get();
    int scoreA = 0;
    int scoreB = 0;

    bindScoreButton(addA1.get(), 1, &scoreA, scoreAPtr);
    bindScoreButton(addA2.get(), 2, &scoreA, scoreAPtr);
    bindScoreButton(addA3.get(), 3, &scoreA, scoreAPtr);
    bindScoreButton(addB1.get(), 1, &scoreB, scoreBPtr);
    bindScoreButton(addB2.get(), 2, &scoreB, scoreBPtr);
    bindScoreButton(addB3.get(), 3, &scoreB, scoreBPtr);

    // 重置时既要清空业务变量，也要同步刷新两个比分 Label。
    reset->setOnClickListener([&scoreA, &scoreB, scoreAPtr, scoreBPtr]()
        {
            scoreA = 0;
            scoreB = 0;
            scoreAPtr->setText("00");
            scoreBPtr->setText("00");
        });

    // 将控件所有权交给窗口。move 后不能再通过原 unique_ptr 访问控件。
    win.addControl(move(title));
    win.addControl(move(teamA));
    win.addControl(move(teamB));
    win.addControl(move(versus));
    win.addControl(move(scoreAView));
    win.addControl(move(scoreBView));
    win.addControl(move(addA1));
    win.addControl(move(addA2));
    win.addControl(move(addA3));
    win.addControl(move(addB1));
    win.addControl(move(addB2));
    win.addControl(move(addB3));
    win.addControl(move(reset));

    // 首次绘制完成后进入消息循环，窗口会持续响应鼠标和键盘事件。
    win.draw();
    return win.runEventLoop();
}
