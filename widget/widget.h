#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPoint>
#include <QVector>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QRandomGenerator>

#define GRID_ROWS 30
#define GRID_COLS 45
#define ELASTICITY 0.12
#define DAMPING 0.85
#define GRAVITY_RANGE 60
#define GAME_TIME 60
#define TRIGGER_THRESHOLD 3
#define UPDATE_INTERVAL 20

enum GameState { STATE_RULES, STATE_PLAYING, STATE_WIN, STATE_LOSE };

enum AudioState { AUDIO_IDLE, AUDIO_PLAYING_1, AUDIO_PLAYING_2 };

struct GridNode {
    QPointF basePos;
    QPointF currentPos;
    QPointF velocity;
};

struct HarpLine {
    int col;
    int progress;
    bool revealed;
};

struct TrapLine {
    int col;
    int count;
};

class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private slots:
    void updateSimulation();
    void onAudioStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void initGrid();
    void initGame();
    void drawRules(QPainter &p);
    void drawGame(QPainter &p);
    void drawResult(QPainter &p);
    void checkLines();
    void updateAudio();
    void playNextAudio();

    QVector<GridNode> nodes;
    QVector<HarpLine> harpLines;
    TrapLine trapLine;
    QVector<QPointF> trail;
    QPointF mousePos;
    QTimer *animTimer;
    
    // 音频系统
    QMediaPlayer *audioPlayer;
    QAudioOutput *audioOutput;
    AudioState audioState;
    bool isTouching;
    
    GameState gameState;
    int timeLeft;
    bool audioActivated;
    bool mouseMoved;
    qint64 lastMoveTime;
    QRandomGenerator randGen;
};

#endif
