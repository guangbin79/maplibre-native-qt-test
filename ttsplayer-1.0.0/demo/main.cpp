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
    explicit DemoRunner(const QString &modelPath, const QString &text, QObject *parent = nullptr)
        : QObject(parent), m_modelPath(modelPath), m_text(text)
    {
        m_player = new TTSPlayer(this);
        connect(m_player, &TTSPlayer::readyChanged, this, &DemoRunner::onReadyChanged);
        connect(m_player, &TTSPlayer::playbackStarted, this, &DemoRunner::onPlaybackStarted);
        connect(m_player, &TTSPlayer::finished, this, &DemoRunner::onFinished);
        connect(m_player, &TTSPlayer::stopped, this, &DemoRunner::onStopped);
        connect(m_player, &TTSPlayer::playingChanged, this, &DemoRunner::onPlayingChanged);
        connect(m_player, &TTSPlayer::errorOccurred, this, &DemoRunner::onErrorOccurred);
    }

    void startDemo()
    {
        qDebug() << "\n--- TTSPlayer Demo starting ---";
        qDebug() << "Waiting for TTS engine initialization...";
        m_player->initialize(m_modelPath);
    }

private slots:
    void onReadyChanged(bool ready) {
        qDebug() << "[SIGNAL] TTS Engine ready:" << ready;
        if (ready) {
            QTimer::singleShot(100, this, [=]() {
                qDebug() << "Playing text:" << m_text;
                m_player->play(m_text);
            });
        }
    }

    void onPlaybackStarted() { qDebug() << "[SIGNAL] Playback STARTED"; }
    void onFinished() {
        qDebug() << "[SIGNAL] Playback FINISHED NATURALLY";
        QTimer::singleShot(100, this, []() { QCoreApplication::quit(); });
    }
    void onStopped() { qDebug() << "[SIGNAL] Playback STOPPED/INTERRUPTED"; }
    void onPlayingChanged(bool playing) { qDebug() << "[SIGNAL] Playing status:" << playing; }
    void onErrorOccurred(const QString &error) {
        qCritical() << "[SIGNAL] ERROR:" << error;
        QTimer::singleShot(100, this, []() { QCoreApplication::exit(1); });
    }

private:
    TTSPlayer *m_player;
    QString m_modelPath;
    QString m_text;
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

    parser.addOption(modelOption);
    parser.addOption(textOption);
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

    QString modelPath = parser.value(modelOption);
    QString text = parser.value(textOption);

    QFileInfo modelDir(modelPath);
    if (!modelDir.isDir()) {
        fprintf(stderr, "Error: model path is not a valid directory: %s\n", qPrintable(modelPath));
        return 1;
    }

    DemoRunner demo(modelPath, text);
    QTimer::singleShot(0, &demo, &DemoRunner::startDemo);
    return app.exec();
}

#include "main.moc"
