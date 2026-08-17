#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <ttsplayer/TTSPlayer.h>

class DemoRunner : public QObject
{
    Q_OBJECT
public:
    enum Phase { Init, Playing1, Switching, Playing2 };

    explicit DemoRunner(const QString &modelPath, const QString &text,
                        const QString &switchModelPath, const QString &switchText,
                        bool switchDuringPlay, bool noInit,
                        QObject *parent = nullptr)
        : QObject(parent), m_modelPath(modelPath), m_text(text)
        , m_switchModelPath(switchModelPath), m_switchText(switchText)
        , m_switchDuringPlay(switchDuringPlay), m_noInit(noInit)
    {
        m_player = new TTSPlayer(this);
        connect(m_player, &TTSPlayer::readyChanged, this, &DemoRunner::onReadyChanged);
        connect(m_player, &TTSPlayer::playbackStarted, this, &DemoRunner::onPlaybackStarted);
        connect(m_player, &TTSPlayer::finished, this, &DemoRunner::onFinished);
        connect(m_player, &TTSPlayer::stopped, this, &DemoRunner::onStopped);
        connect(m_player, &TTSPlayer::playingChanged, this, &DemoRunner::onPlayingChanged);
        connect(m_player, &TTSPlayer::errorOccurred, this, &DemoRunner::onErrorOccurred);
        connect(m_player, &TTSPlayer::modelSwitched, this, &DemoRunner::onModelSwitched);
    }

    void startDemo()
    {
        qDebug() << "\n--- TTSPlayer Demo starting ---";
        if (m_noInit) {
            qDebug() << "Skipping initialize, switchModel acts as initialize";
            m_phase = Switching;
            QTimer::singleShot(100, this, [=]() {
                qDebug() << "Switching model to:" << m_switchModelPath;
                m_player->switchModel(m_switchModelPath);
            });
            return;
        }
        qDebug() << "Waiting for TTS engine initialization...";
        m_player->initialize(m_modelPath);
    }

private slots:
    void onReadyChanged(bool ready) {
        qDebug() << "[SIGNAL] TTS Engine ready:" << ready;
        // 相位守卫：仅常规初始化路径自动播放；--no-init 切换成功尾部发的
        // readyChanged(true) 不得触发播放（由 onModelSwitched 接管播 switchText）
        if (ready && m_phase == Init) {
            m_phase = Playing1;
            QTimer::singleShot(100, this, [=]() {
                qDebug() << "Playing text:" << m_text;
                m_player->play(m_text);
            });
        }
    }

    void onPlaybackStarted() {
        qDebug() << "[SIGNAL] Playback STARTED";
        // --switch-during-play：第一段播放中被切断，覆盖 doSwitchModel 的 stop 分支
        if (m_switchDuringPlay && m_phase == Playing1 && !m_switchModelPath.isEmpty()) {
            m_phase = Switching;
            qDebug() << "Switching model during playback to:" << m_switchModelPath;
            m_player->switchModel(m_switchModelPath);
        }
    }
    void onFinished() {
        qDebug() << "[SIGNAL] Playback FINISHED NATURALLY";
        if (m_phase == Playing1 && !m_switchModelPath.isEmpty()) {
            m_phase = Switching;
            QTimer::singleShot(100, this, [=]() {
                qDebug() << "Switching model to:" << m_switchModelPath;
                m_player->switchModel(m_switchModelPath);
            });
            return;
        }
        QTimer::singleShot(100, this, []() { QCoreApplication::quit(); });
    }
    void onModelSwitched(const QString &modelPath) {
        qDebug() << "[SIGNAL] modelSwitched:" << modelPath;
        m_phase = Playing2;
        QTimer::singleShot(100, this, [=]() {
            qDebug() << "Playing text:" << m_switchText;
            m_player->play(m_switchText);
        });
    }
    void onStopped() { qDebug() << "[SIGNAL] Playback STOPPED/INTERRUPTED"; }
    void onPlayingChanged(bool playing) { qDebug() << "[SIGNAL] Playing status:" << playing; }
    void onErrorOccurred(const QString &error) {
        qCritical() << "[SIGNAL] ERROR:" << error;
        if (m_phase == Switching) {
            m_phase = Playing2;
            QTimer::singleShot(100, this, [=]() {
                qDebug() << "Switch failed, playing text on old engine:" << m_switchText;
                m_player->play(m_switchText);
            });
            return;
        }
        QTimer::singleShot(100, this, []() { QCoreApplication::exit(1); });
    }

private:
    TTSPlayer *m_player;
    QString m_modelPath;
    QString m_text;
    QString m_switchModelPath;
    QString m_switchText;
    bool m_switchDuringPlay = false;
    bool m_noInit = false;
    Phase m_phase = Init;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("ttsplayer_demo");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("TTSPlayer Demo - Text-to-Speech player");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption modelOption(
        QStringList() << "m" << "model",
        "Directory path containing TTS model files (required)",
        "directory"
    );
    QCommandLineOption textOption(
        QStringList() << "t" << "text",
        "Text to synthesize and play (required)",
        "text"
    );
    QCommandLineOption switchModelOption(
        QStringList() << "switch-model",
        "Directory path of TTS model to switch to at runtime (optional)",
        "directory"
    );
    QCommandLineOption switchTextOption(
        QStringList() << "switch-text",
        "Text to play after model switch, or after a failed switch (required with --switch-model)",
        "text"
    );
    QCommandLineOption switchDuringPlayOption(
        QStringList() << "switch-during-play",
        "Switch model immediately after playback starts, interrupting the first segment (requires --switch-model/--switch-text)"
    );
    QCommandLineOption noInitOption(
        QStringList() << "no-init",
        "Skip initialize and call switchModel directly, so switchModel acts as initialization (requires --switch-model/--switch-text)"
    );

    parser.addOption(modelOption);
    parser.addOption(textOption);
    parser.addOption(switchModelOption);
    parser.addOption(switchTextOption);
    parser.addOption(switchDuringPlayOption);
    parser.addOption(noInitOption);
    parser.process(app);

    if (!parser.isSet(modelOption)) {
        fprintf(stderr, "Error: --model is required\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    if (!parser.isSet(textOption)) {
        fprintf(stderr, "Error: --text is required\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    if (parser.isSet(switchModelOption) != parser.isSet(switchTextOption)) {
        fprintf(stderr, "Error: --switch-model and --switch-text must be used together\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    if (parser.isSet(switchModelOption) && parser.value(switchModelOption).isEmpty()) {
        fprintf(stderr, "Error: --switch-model value must not be empty\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    if (parser.isSet(switchTextOption) && parser.value(switchTextOption).isEmpty()) {
        fprintf(stderr, "Error: --switch-text value must not be empty\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    bool switchDuringPlay = parser.isSet(switchDuringPlayOption);
    bool noInit = parser.isSet(noInitOption);
    if (switchDuringPlay && noInit) {
        fprintf(stderr, "Error: --switch-during-play and --no-init are mutually exclusive\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }
    if ((switchDuringPlay || noInit) && !parser.isSet(switchModelOption)) {
        fprintf(stderr, "Error: --switch-during-play/--no-init require --switch-model and --switch-text\n\n");
        fprintf(stderr, "%s\n", qPrintable(parser.helpText()));
        return 1;
    }

    QString modelPath = parser.value(modelOption);
    QString text = parser.value(textOption);
    QString switchModelPath = parser.value(switchModelOption);
    QString switchText = parser.value(switchTextOption);

    QFileInfo modelDir(modelPath);
    if (!modelDir.isDir()) {
        fprintf(stderr, "Error: model path is not a valid directory: %s\n", qPrintable(modelPath));
        return 1;
    }

    DemoRunner demo(modelPath, text, switchModelPath, switchText, switchDuringPlay, noInit);
    QTimer::singleShot(0, &demo, &DemoRunner::startDemo);
    return app.exec();
}

#include "main.moc"
