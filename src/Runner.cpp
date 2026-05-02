#include "Runner.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <memory>

static QString sub(const QString &arg, const QString &src,
                   const QString &out, const QString &obj)
{
    QString a = arg;
    a.replace("%SRC%", src);
    a.replace("%OUT%", out);
    a.replace("%OBJ%", obj);
    return a;
}

static QStringList subList(const QStringList &tmpl, const QString &src,
                           const QString &out, const QString &obj)
{
    QStringList r;
    for (const QString &a : tmpl) r << sub(a, src, out, obj);
    return r;
}

QProcessEnvironment Runner::buildEnv()
{
    auto env = QProcessEnvironment::systemEnvironment();
    QString mingw = LanguageConfig::findMinGWBin();
    if (!mingw.isEmpty()) {
        QString p = QDir::toNativeSeparators(mingw) + ";" + env.value("PATH");
        env.insert("PATH", p);
    }
    return env;
}

Runner::Runner(QObject *parent) : QObject(parent) {}
Runner::~Runner() { stop(); cleanup(); }

bool Runner::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void Runner::stop()
{
    if (isRunning()) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

void Runner::cleanup()
{
    if (!m_tmpSrc.isEmpty()) { QFile::remove(m_tmpSrc); m_tmpSrc.clear(); }
    if (!m_tmpOut.isEmpty()) { QFile::remove(m_tmpOut); m_tmpOut.clear(); }
    if (!m_tmpObj.isEmpty()) { QFile::remove(m_tmpObj); m_tmpObj.clear(); }
}

void Runner::sendInput(const QString &text)
{
    if (isRunning())
        m_process->write((text + "\n").toLocal8Bit());
}

QString Runner::makeTmp(const QString &ext)
{
    return QDir::tempPath() + "/qs_" +
           QUuid::createUuid().toString(QUuid::Id128).left(8) + "." + ext;
}

void Runner::run(const Language &lang, const QString &code)
{
    stop();
    cleanup();

    m_tmpSrc = (lang.name == "Java")
        ? QDir::tempPath() + "/Main.java"
        : makeTmp(lang.extension);

#ifdef Q_OS_WIN
    m_tmpOut = makeTmp("exe");
#else
    m_tmpOut = makeTmp("out");
#endif
    m_tmpObj = makeTmp("o");

    QFile f(m_tmpSrc);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOutput("Error: could not create temp source file.\n");
        return;
    }
    QTextStream ts(&f);
    ts << code;
    f.close();

    emit started();
    m_timer.restart();

    if      (lang.name == "ASM") runAsm(lang);
    else if (lang.name == "C#")  runCSharp(lang);
    else if (!lang.compiler.isEmpty()) runCompiled(lang);
    else runInterpreted(lang);
}

void Runner::launchProcess(const QString &prog, const QStringList &args)
{
    if (m_process) {
        m_process->disconnect();
        delete m_process;
    }
    m_process = new QProcess(this);
    m_process->setProcessEnvironment(buildEnv());
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &Runner::onStdOut);
    connect(m_process, &QProcess::readyReadStandardError,  this, &Runner::onStdErr);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Runner::onFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, [this, prog](QProcess::ProcessError err) {
                if (err == QProcess::FailedToStart)
                    emit errorOutput(
                        QString("Error: '%1' not found.\n"
                                "Install it and make sure it is in your system PATH.\n")
                        .arg(prog));
            });

    m_process->start(prog, args);
}

// single compile step with proper deduplication (errorOccurred + finished can both fire)
void Runner::compileStep(const QString &prog, const QStringList &args,
                         std::function<void(int)> onDone)
{
    auto *proc   = new QProcess(this);
    auto  fired  = std::make_shared<bool>(false);

    proc->setProcessEnvironment(buildEnv());

    // errorOccurred fires when program is not found (FailedToStart)
    connect(proc, &QProcess::errorOccurred,
            this, [this, prog, proc, onDone, fired](QProcess::ProcessError err) {
                if (*fired) return;
                if (err == QProcess::FailedToStart) {
                    *fired = true;
                    emit errorOutput(
                        QString("Error: '%1' not found.\n"
                                "Install it and make sure it is in your system PATH.\n")
                        .arg(prog));
                    proc->deleteLater();
                    emit finished((int)m_timer.elapsed());
                }
            });

    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, onDone, fired](int code, QProcess::ExitStatus) {
                if (*fired) return; // already handled by errorOccurred
                *fired = true;

                // collect any compiler output (errors/warnings go to stderr, messages to stdout)
                QString err = QString::fromLocal8Bit(proc->readAllStandardError());
                QString out = QString::fromLocal8Bit(proc->readAllStandardOutput());
                proc->deleteLater();

                if (!err.isEmpty()) emit errorOutput(err);
                if (!out.isEmpty()) emit errorOutput(out); // csc writes errors to stdout
                onDone(code);
            });

    proc->start(prog, args);

    // if start fails synchronously (very rare), check immediately
    if (proc->state() == QProcess::NotRunning && !*fired) {
        *fired = true;
        emit errorOutput(QString("Error: could not start '%1'.\n").arg(prog));
        proc->deleteLater();
        emit finished((int)m_timer.elapsed());
    }
}

void Runner::runCompiled(const Language &lang)
{
    QString prog = lang.compiler;
    QStringList args = subList(lang.compileArgs, m_tmpSrc, m_tmpOut, m_tmpObj);

    if (lang.name == "Java") {
        // need JDK javac
        QString javac = LanguageConfig::findJavac();
        if (javac.isEmpty()) {
            emit errorOutput(
                "Error: Java JDK (javac) not found.\n"
                "You have JRE installed but need the full JDK.\n"
                "Download from: https://adoptium.net\n");
            emit finished(0);
            return;
        }
        QString java = LanguageConfig::findJava();
        compileStep(javac, { m_tmpSrc }, [this, java](int code) {
            if (code != 0) { emit finished((int)m_timer.elapsed()); return; }
            launchProcess(java, { "-cp", QDir::tempPath(), "Main" });
        });
        return;
    }

    compileStep(prog, args, [this](int code) {
        if (code != 0) { emit finished((int)m_timer.elapsed()); return; }
        launchProcess(m_tmpOut, {});
    });
}

void Runner::runInterpreted(const Language &lang)
{
    // for Go, check if it's installed first
    if (lang.name == "Go") {
        QString go = LanguageConfig::findGo();
        if (go.isEmpty()) {
            emit errorOutput(
                "Error: Go is not installed.\n"
                "Download from: https://go.dev/dl\n");
            emit finished(0);
            return;
        }
        launchProcess(go, { "run", m_tmpSrc });
        return;
    }

    launchProcess(lang.interpreter, subList(lang.runArgs, m_tmpSrc, m_tmpOut, m_tmpObj));
}

void Runner::runAsm(const Language &lang)
{
    QStringList nasmArgs = subList(lang.compileArgs, m_tmpSrc, m_tmpOut, m_tmpObj);
    compileStep(lang.compiler, nasmArgs, [this, lang](int code) {
        if (code != 0) { emit finished((int)m_timer.elapsed()); return; }
        QStringList linkArgs = subList(lang.linkArgs, m_tmpSrc, m_tmpOut, m_tmpObj);
        compileStep(lang.linker, linkArgs, [this](int code2) {
            if (code2 != 0) { emit finished((int)m_timer.elapsed()); return; }
            launchProcess(m_tmpOut, {});
        });
    });
}

void Runner::runCSharp(const Language &lang)
{
    // find csc.exe — .NET Framework ships it with Windows
    QString csc;
    QStringList candidates = {
        "C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe",
        "C:/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe",
        "C:/Windows/Microsoft.NET/Framework64/v3.5/csc.exe",
    };
    for (const QString &p : candidates)
        if (QFile::exists(p)) { csc = p; break; }

    if (csc.isEmpty()) {
        emit errorOutput(
            "Error: C# compiler (csc.exe) not found.\n"
            "Install .NET SDK from: https://dotnet.microsoft.com\n");
        emit finished(0);
        return;
    }

    // build args — subList handles /out:%OUT% → /out:actualpath
    QStringList args = subList(lang.compileArgs, m_tmpSrc, m_tmpOut, m_tmpObj);

    compileStep(csc, args, [this](int code) {
        if (code != 0) { emit finished((int)m_timer.elapsed()); return; }
        launchProcess(m_tmpOut, {});
    });
}

void Runner::onStdOut()
{
    emit output(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
}

void Runner::onStdErr()
{
    emit errorOutput(QString::fromLocal8Bit(m_process->readAllStandardError()));
}

void Runner::onFinished(int, QProcess::ExitStatus)
{
    // flush any output that arrived just before the process exited
    if (m_process) {
        QByteArray rem = m_process->readAllStandardOutput();
        if (!rem.isEmpty()) emit output(QString::fromLocal8Bit(rem));
        rem = m_process->readAllStandardError();
        if (!rem.isEmpty()) emit errorOutput(QString::fromLocal8Bit(rem));
    }
    emit finished((int)m_timer.elapsed());
    cleanup();
}
