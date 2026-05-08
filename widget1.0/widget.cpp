#include "widget.h"
#include <QPainter>      // Qt的绘图库，用于在窗口上绘制图形
#include <QMouseEvent>   // 处理鼠标事件的库
#include <QtMath>        // 数学函数库，提供各种数学计算功能
#include <QDateTime>     // 日期时间库，用于获取当前时间和计时

// ======================================
// 构造函数：初始化窗口、游戏状态和资源
// ======================================
Widget::Widget(QWidget *parent)
    : QWidget(parent)  // 调用父类QWidget的构造函数，传入父窗口指针
{
    // 设置窗口固定大小为900x600像素
    // setFixedSize是QWidget的方法，用于固定窗口大小，防止用户调整
    setFixedSize(900, 600);
    
    // 设置窗口背景颜色为深紫色
    // setStyleSheet是QWidget的方法，用于设置窗口的样式
    // 这里使用CSS语法设置背景颜色
    setStyleSheet("background-color: #0a0510;");
    
    // 设置窗口标题
    // setWindowTitle是QWidget的方法，用于设置窗口标题栏的文本
    setWindowTitle("CYBER SNAKE // HARP GRID");

    // 初始化鼠标位置为屏幕外的一个点，表示鼠标未在窗口内
    // QPointF是Qt的浮点坐标点类，用于表示2D坐标
    mousePos = QPointF(-9999, -9999);
    
    // 设置初始游戏状态为规则界面
    gameState = STATE_RULES;
    
    // 音频系统初始化为未激活状态
    audioActivated = false;
    
    // 鼠标移动状态初始化为未移动
    mouseMoved = false;
    
    // 记录最后一次鼠标移动的时间
    lastMoveTime = 0;
    
    // 使用当前时间作为随机数生成器的种子
    // QRandomGenerator是Qt的随机数生成器类
    randGen.seed(QDateTime::currentMSecsSinceEpoch());
    
    // 音频状态初始化为空闲状态
    audioState = AUDIO_IDLE;
    
    // 鼠标触碰状态初始化为未触碰
    isTouching = false;

    // 初始化网格系统
    initGrid();
    
    // 初始化游戏状态
    initGame();

    // 创建动画定时器
    // QTimer是Qt的定时器类，用于定时触发事件
    animTimer = new QTimer(this);
    
    // 设置定时器间隔为20毫秒，即每秒更新50次
    animTimer->setInterval(UPDATE_INTERVAL);
    
    // 连接定时器的timeout信号到updateSimulation槽函数
    // Qt的信号槽机制，当定时器触发时，会调用updateSimulation函数
    connect(animTimer, &QTimer::timeout, this, &Widget::updateSimulation);
    
    // 启动定时器
    animTimer->start();

    // 初始化音频输出对象
    // QAudioOutput是Qt的音频输出类，用于控制音频播放
    audioOutput = new QAudioOutput(this);
    
    // 初始化音频播放器对象
    // QMediaPlayer是Qt的媒体播放器类，用于播放音频文件
    audioPlayer = new QMediaPlayer(this);
    
    // 将音频播放器连接到音频输出设备
    audioPlayer->setAudioOutput(audioOutput);
    
    // 设置初始音量为0.05（5%）
    audioOutput->setVolume(0.05);

    // 连接音频播放器的媒体状态变化信号到onAudioStatusChanged槽函数
    // 当音频播放状态变化时（如播放完成），会调用onAudioStatusChanged函数
    connect(audioPlayer, &QMediaPlayer::mediaStatusChanged, this, &Widget::onAudioStatusChanged);
}

// ======================================
// 初始化网格系统：创建所有网格节点
// ======================================
void Widget::initGrid()
{
    // 清空节点列表
    nodes.clear();
    
    // 计算每个网格单元格的宽度和高度
    // width()和height()是QWidget的方法，返回窗口的宽度和高度
    float cellW = width() / (float)GRID_COLS;
    float cellH = height() / (float)GRID_ROWS;

    // 循环创建所有网格节点
    for (int r = 0; r <= GRID_ROWS; r++) {
        for (int c = 0; c <= GRID_COLS; c++) {
            // 创建一个网格节点
            GridNode n;
            
            // 设置节点的基础位置（初始位置）
            n.basePos = QPointF(c * cellW, r * cellH);
            
            // 设置节点的当前位置
            n.currentPos = n.basePos;
            
            // 设置节点的速度为0
            n.velocity = QPointF(0, 0);
            
            // 将节点添加到节点列表
            nodes.append(n);
        }
    }
}

// ======================================
// 初始化游戏状态：随机生成竖琴线和陷阱线
// ======================================
void Widget::initGame()
{
    // 设置初始倒计时为60秒
    timeLeft = GAME_TIME;
    
    // 清空鼠标轨迹
    trail.clear();
    
    // 清空竖琴线列表
    harpLines.clear();

    // 随机生成3条不重复的竖琴线
    QVector<int> usedCols;  // 用于记录已使用的列，避免重复
    for (int i = 0; i < 3; i++) {
        int col;
        // 生成一个未使用过的列
        do {
            col = randGen.bounded(5, GRID_COLS - 5);
        } while (usedCols.contains(col));
        usedCols.append(col);
        
        // 创建竖琴线对象，初始进度为0，未显示
        harpLines.append({col, 0, false});
    }

    // 随机生成陷阱线（不与竖琴线重复）
    int trapCol;
    do {
        trapCol = randGen.bounded(5, GRID_COLS - 5);
    } while (usedCols.contains(trapCol));
    trapLine = {trapCol, 0};
}

// ======================================
// 鼠标移动事件：处理拨弦逻辑和轨迹记录
// ======================================
void Widget::mouseMoveEvent(QMouseEvent *event)
{
    // 获取鼠标当前位置
    // event->pos()返回鼠标在窗口内的坐标
    mousePos = event->pos();
    
    // 标记鼠标已移动
    mouseMoved = true;
    
    // 记录当前时间作为最后一次鼠标移动的时间
    lastMoveTime = QDateTime::currentMSecsSinceEpoch();

    // 如果游戏正在进行中
    if (gameState == STATE_PLAYING) {
        // 将当前鼠标位置添加到轨迹列表
        trail.append(mousePos);
        
        // 如果轨迹列表超过500个点，移除最早的点
        if (trail.size() > 500) trail.removeFirst();
        
        // 检查竖琴线和陷阱线的触发状态
        checkLines();
    }
    
    // 触发窗口重绘
    // update()是QWidget的方法，会触发paintEvent事件
    update();
}

// ======================================
// 鼠标点击事件：处理游戏开始和重新开始
// ======================================
void Widget::mousePressEvent(QMouseEvent *event)
{
    // 忽略未使用的参数
    Q_UNUSED(event);
    
    // 如果当前是规则界面
    if (gameState == STATE_RULES) {
        // 激活音频系统
        audioActivated = true;
        
        // 切换到游戏进行状态
        gameState = STATE_PLAYING;
        
        // 初始化游戏状态
        initGame();
    } else if (gameState == STATE_WIN || gameState == STATE_LOSE) {
        // 如果游戏已结束，切换回规则界面
        gameState = STATE_RULES;
    }
}

// ======================================
// 鼠标离开事件：重置鼠标位置
// ======================================
void Widget::leaveEvent(QEvent *event)
{
    // 忽略未使用的参数
    Q_UNUSED(event);
    
    // 将鼠标位置设置为屏幕外的一个点
    mousePos = QPointF(-9999, -9999);
}

// ======================================
// 检查竖琴线和陷阱线的触发状态
// ======================================
void Widget::checkLines()
{
    // 计算每个网格单元格的宽度
    float cellW = width() / (float)GRID_COLS;

    // 检查竖琴线触发
    for (auto &line : harpLines) {
        // 计算竖琴线的x坐标
        float lineX = line.col * cellW;
        
        // 如果鼠标在竖琴线附近，并且竖琴线未显示
        if (qAbs(mousePos.x() - lineX) < cellW * 1.5 && !line.revealed) {
            // 增加竖琴线的进度
            line.progress++;
            
            // 如果进度达到阈值，显示竖琴线
            if (line.progress >= TRIGGER_THRESHOLD * 10) {
                line.revealed = true;
            }
        }
    }

    // 检查陷阱线触发
    float trapX = trapLine.col * cellW;
    if (qAbs(mousePos.x() - trapX) < cellW * 1.5) {
        // 增加陷阱线的触发次数
        trapLine.count++;
        
        // 如果触发次数达到阈值，游戏失败
        if (trapLine.count >= TRIGGER_THRESHOLD * 10) {
            gameState = STATE_LOSE;
        }
    }

    // 检查游戏胜利条件
    int revealed = 0;
    for (auto &line : harpLines) {
        if (line.revealed) revealed++;
    }
    if (revealed == 3) {
        // 如果所有竖琴线都已显示，游戏胜利
        gameState = STATE_WIN;
    }
}

// ======================================
// 音频状态回调：处理音频播放完成后的逻辑
// ======================================
void Widget::onAudioStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 如果音频播放完成
    if (status == QMediaPlayer::EndOfMedia) {
        // 如果鼠标仍在触碰
        if (isTouching) {
            // 播放下一段音频
            playNextAudio();
        } else {
            // 否则设置音频状态为空闲
            audioState = AUDIO_IDLE;
        }
    }
}

// ======================================
// 播放下一段音频：循环切换harp1和harp2
// ======================================
void Widget::playNextAudio()
{
    // 如果音频状态为空闲或刚播放完harp2
    if (audioState == AUDIO_IDLE || audioState == AUDIO_PLAYING_2) {
        // 播放harp1
        audioPlayer->setSource(QUrl("qrc:///sound/harp1.mp3"));
        audioState = AUDIO_PLAYING_1;
    } else {
        // 否则播放harp2
        audioPlayer->setSource(QUrl("qrc:///sound/harp2.mp3"));
        audioState = AUDIO_PLAYING_2;
    }
    // 开始播放音频
    audioPlayer->play();
}

// ======================================
// 更新音频状态：根据鼠标位置调整音量和播放逻辑
// ======================================
void Widget::updateAudio()
{
    // 获取当前时间
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    // 记录之前的触碰状态
    bool wasTouching = isTouching;
    
    // 判断当前是否在触碰（鼠标在500毫秒内移动过）
    isTouching = (mouseMoved && mousePos.x() > 0 && (now - lastMoveTime <= 500));

    // 如果音频系统已激活
    if (audioActivated) {
        // 如果正在触碰
        if (isTouching) {
            // 根据鼠标y坐标调整音量（y坐标越大，音量越大）
            float volume = qMin(1.0, 0.1 + (mousePos.y() / height()) * 0.9);
            audioOutput->setVolume(volume);

            // 如果音频状态为空闲或未在播放
            if (audioState == AUDIO_IDLE || !audioPlayer->isPlaying()) {
                // 播放下一段音频
                playNextAudio();
            }
        } else {
            // 否则设置音量为0.05
            audioOutput->setVolume(0.05);
            
            // 如果音频正在播放
            if (audioPlayer->isPlaying()) {
                // 停止播放
                audioPlayer->stop();
                
                // 设置音频状态为空闲
                audioState = AUDIO_IDLE;
            }
        }
    }
}

// ======================================
// 主循环更新：物理模拟、倒计时、音频更新
// ======================================
void Widget::updateSimulation()
{
    // 物理模拟：网格节点的引力和弹性
    for (auto &n : nodes) {
        // 计算鼠标到节点的距离
        float dx = n.basePos.x() - mousePos.x();
        float dy = n.basePos.y() - mousePos.y();
        float dist = sqrt(dx*dx + dy*dy);

        // 如果鼠标在引力范围内
        if (dist < GRAVITY_RANGE && dist > 0) {
            // 计算引力大小
            float force = (GRAVITY_RANGE - dist) / GRAVITY_RANGE * 6.0f;
            
            // 应用引力到节点速度
            n.velocity += QPointF(dx/dist * force, dy/dist * force);
        }

        // 计算弹性力（使节点回到基础位置）
        QPointF spring = (n.basePos - n.currentPos) * ELASTICITY;
        
        // 应用弹性力到节点速度
        n.velocity += spring;
        
        // 应用阻尼（使速度逐渐减小）
        n.velocity *= DAMPING;
        
        // 更新节点位置
        n.currentPos += n.velocity;
    }

    // 倒计时更新
    if (gameState == STATE_PLAYING) {
        static int frameCount = 0;
        frameCount++;
        
        // 如果达到1秒（50帧）
        if (frameCount >= 1000 / UPDATE_INTERVAL) {
            frameCount = 0;
            
            // 倒计时减1
            timeLeft--;
            
            // 如果倒计时结束，游戏失败
            if (timeLeft <= 0) {
                gameState = STATE_LOSE;
            }
        }
    }

    // 更新音频状态
    updateAudio();
    
    // 触发窗口重绘
    update();
}

// ======================================
// 绘制游戏规则界面
// ======================================
void Widget::drawRules(QPainter &p)
{
    // 启用反锯齿，使绘制的图形更平滑
    p.setRenderHint(QPainter::Antialiasing);

    // 填充背景色
    p.fillRect(rect(), QColor(10, 5, 16));

    // 设置字体为Courier New，大小28，粗体
    QFont titleFont("Courier New", 28, QFont::Bold);
    p.setFont(titleFont);
    
    // 设置画笔颜色为粉红色
    p.setPen(QColor(255, 80, 120));
    
    // 绘制标题文本，居中显示
    p.drawText(rect(), Qt::AlignCenter, "CYBER SNAKE // HARP GRID");

    // 设置字体为Courier New，大小12
    QFont infoFont("Courier New", 12);
    p.setFont(infoFont);
    
    // 设置画笔颜色为浅蓝色
    p.setPen(QColor(180, 180, 220));

    // 准备规则文本
    QString rules = QString(
        "游戏规则速览\n\n"
        "目标：在 %1 秒内找出并拨动隐藏的 3 条竖琴线\n"
        "操作：鼠标滑动拨弦，首次点击激活音频\n"
        "陷阱：小心带电陷阱线，触发 %2 次会失败！\n\n"
        "点击任意位置开始游戏"
    ).arg(GAME_TIME).arg(TRIGGER_THRESHOLD);

    // 绘制规则文本，居中显示
    QRect infoRect(150, 200, 600, 300);
    p.drawText(infoRect, Qt::AlignCenter, rules);
}

// ======================================
// 绘制游戏主界面
// ======================================
void Widget::drawGame(QPainter &p)
{
    // 启用反锯齿
    p.setRenderHint(QPainter::Antialiasing);
    
    // 创建渐变背景
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(20, 5, 10));
    gradient.setColorAt(0.5, QColor(30, 10, 15));
    gradient.setColorAt(1, QColor(20, 5, 10));
    p.fillRect(rect(), gradient);

    // 计算网格单元格的宽度和高度
    float cellW = width() / (float)GRID_COLS;
    float cellH = height() / (float)GRID_ROWS;

    // 设置画笔颜色为粉红色，透明度150
    p.setPen(QPen(QColor(255, 50, 80, 150), 1.0));
    
    // 绘制水平网格线
    for (int r = 0; r <= GRID_ROWS; r++) {
        QPointF prev = nodes[r * (GRID_COLS + 1)].currentPos;
        for (int c = 1; c <= GRID_COLS; c++) {
            QPointF curr = nodes[r * (GRID_COLS + 1) + c].currentPos;
            p.drawLine(prev, curr);
            prev = curr;
        }
    }
    
    // 绘制垂直网格线
    for (int c = 0; c <= GRID_COLS; c++) {
        QPointF prev = nodes[c].currentPos;
        for (int r = 1; r <= GRID_ROWS; r++) {
            QPointF curr = nodes[r * (GRID_COLS + 1) + c].currentPos;
            p.drawLine(prev, curr);
            prev = curr;
        }
    }

    // 绘制竖琴线
    for (auto &line : harpLines) {
        float x = line.col * cellW;
        if (line.revealed) {
            // 如果竖琴线已显示，使用绿色画笔
            p.setPen(QPen(QColor(0, 255, 200), 4));
            p.drawLine(QPointF(x, 0), QPointF(x, height()));
        } else {
            // 否则根据进度调整透明度
            int alpha = line.progress * 10;
            p.setPen(QPen(QColor(0, 220, 180, alpha), 2));
            p.drawLine(QPointF(x, 0), QPointF(x, height()));
        }
    }

    // 绘制陷阱线
    float trapX = trapLine.col * cellW;
    int trapAlpha = trapLine.count * 10;
    p.setPen(QPen(QColor(255, 100, 100, trapAlpha), 3));
    p.drawLine(QPointF(trapX, 0), QPointF(trapX, height()));

    // 设置字体为Courier New，大小16，粗体
    QFont titleFont("Courier New", 16, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor(255, 80, 120));
    
    // 绘制标题文本
    p.drawText(20, 35, "CYBER SNAKE // HARP GRID");
    
    // 绘制倒计时文本
    p.drawText(width() - 180, 35, QString("%1s TIME ATTACK").arg(timeLeft));

    // 设置字体为Courier New，大小10
    QFont statusFont("Courier New", 10);
    p.setFont(statusFont);
    p.setPen(QColor(200, 150, 180));
    
    // 准备状态文本
    QString status = "竖琴线: ";
    for (int i = 0; i < harpLines.size(); i++) {
        status += QString("[%1%2] ").arg(harpLines[i].progress/10).arg(harpLines[i].revealed ? "✓" : "");
    }
    status += QString(" 陷阱: [%1]").arg(trapLine.count/10);
    
    // 绘制状态文本
    p.drawText(20, height() - 20, status);
}

// ======================================
// 绘制游戏结束界面
// ======================================
void Widget::drawResult(QPainter &p)
{
    // 启用反锯齿
    p.setRenderHint(QPainter::Antialiasing);
    
    // 绘制渐变背景
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(20, 5, 10));
    gradient.setColorAt(0.5, QColor(30, 10, 15));
    gradient.setColorAt(1, QColor(20, 5, 10));
    p.fillRect(rect(), gradient);

    // 绘制结果卡片
    QRect cardRect(width()/2 - 250, height()/2 - 200, 500, 400);
    
    // 设置画笔颜色为浅蓝色，宽度2
    p.setPen(QPen(QColor(200, 200, 220), 2));
    
    // 设置画刷颜色为深灰色
    p.setBrush(QColor(40, 40, 50));
    
    // 绘制圆角矩形卡片
    p.drawRoundedRect(cardRect, 10, 10);

    // 绘制轨迹画布
    QRect canvasRect = cardRect.adjusted(20, 20, -20, -80);
    p.fillRect(canvasRect, QColor(20, 20, 30));
    
    // 绘制小网格背景
    p.setPen(QPen(QColor(30, 30, 40), 1));
    for (int x = canvasRect.left(); x <= canvasRect.right(); x += 20) {
        p.drawLine(x, canvasRect.top(), x, canvasRect.bottom());
    }
    for (int y = canvasRect.top(); y <= canvasRect.bottom(); y += 20) {
        p.drawLine(canvasRect.left(), y, canvasRect.right(), y);
    }

    // 绘制鼠标轨迹
    if (trail.size() > 1) {
        // 计算缩放比例
        float scaleX = canvasRect.width() / (float)width();
        float scaleY = canvasRect.height() / (float)height();
        float scale = qMin(scaleX, scaleY);
        
        // 计算偏移量，使轨迹居中显示
        QPointF offset(
            canvasRect.left() + (canvasRect.width() - width() * scale) / 2,
            canvasRect.top() + (canvasRect.height() - height() * scale) / 2
        );

        // 设置画笔颜色为绿色，宽度3
        p.setPen(QPen(QColor(80, 255, 120), 3));
        
        // 绘制轨迹线
        for (int i = 1; i < trail.size(); i++) {
            QPointF p1 = trail[i-1] * scale + offset;
            QPointF p2 = trail[i] * scale + offset;
            p.drawLine(p1, p2);
        }
    }

    // 设置字体为Courier New，大小24，粗体
    QFont titleFont("Courier New", 24, QFont::Bold);
    p.setFont(titleFont);

    // 根据游戏状态绘制不同的结果文本
    if (gameState == STATE_WIN) {
        p.setPen(QColor(0, 255, 200));
        p.drawText(cardRect.adjusted(0, 0, 0, -30), Qt::AlignBottom | Qt::AlignCenter, "MISSION COMPLETE!");
    } else {
        p.setPen(QColor(255, 100, 100));
        p.drawText(cardRect.adjusted(0, 0, 0, -30), Qt::AlignBottom | Qt::AlignCenter, "SYSTEM FAILURE");
    }

    // 设置字体为Courier New，大小10
    QFont infoFont("Courier New", 10);
    p.setFont(infoFont);
    p.setPen(QColor(150, 150, 180));
    
    // 绘制重新开始提示
    p.drawText(rect().adjusted(0, cardRect.bottom() + 20, 0, 0), Qt::AlignTop | Qt::AlignCenter, "点击任意位置重新开始");
}

// ======================================
// 主绘制函数：根据游戏状态调用不同的绘制函数
// ======================================
void Widget::paintEvent(QPaintEvent *event)
{
    // 忽略未使用的参数
    Q_UNUSED(event);
    
    // 创建QPainter对象，用于绘制
    QPainter p(this);

    // 根据游戏状态调用不同的绘制函数
    switch (gameState) {
        case STATE_RULES:
            drawRules(p);
            break;
        case STATE_PLAYING:
            drawGame(p);
            break;
        case STATE_WIN:
        case STATE_LOSE:
            drawResult(p);
            break;
    }
}
