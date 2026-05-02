#pragma once
#include <QObject>
#include <QProcess>
#include <QElapsedTimer>
#include <QProcessEnvironment>
#include <functional>
#include "LanguageConfig.h"

class Runner : public QObject {
    Q_OBJECT
public:
    explicit Runner(QObject *parent = nullptr);
    ~Runner() override;

    void run(const Language &lang, const QString &code);
    void stop();
    bool isRunning() const;

public slots:
    void sendInput(const QString &text);

signals:
    void output(const QString &text);
    void errorOutput(const QString &text);
    void finished(int ms);
    void started();

private slots:
    void onStdOut();
    void onStdErr();
    void onFinished(int code, QProcess::ExitStatus s);

private:
    QProcess     *m_process = nullptr;
    QElapsedTimer m_timer; 
    QString       m_tmpSrc;
    QString       m_tmpOut;
    QString       m_tmpObj;

    void cleanup();
    QString makeTmp(const QString &ext);
    void launchProcess(const QString &prog, const QStringList &args);
    void compileStep(const QString &prog, const QStringList &args,
                     std::function<void(int)> onDone);

    void runCompiled(const Language &lang);
    void runInterpreted(const Language &lang);
    void runAsm(const Language &lang);
    void runCSharp(const Language &lang);

    static QProcessEnvironment buildEnv();
};
