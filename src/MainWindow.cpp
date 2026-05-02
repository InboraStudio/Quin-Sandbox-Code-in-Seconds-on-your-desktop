#include "MainWindow.h"
#include "CodeEditor.h"
#include "OutputPanel.h"
#include "Runner.h"
#include "TitleBar.h"
#include "SyntaxHighlighter.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QKeyEvent>
#include <QLabel>
#include <QFrame>
#include <QShortcut>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    resize(1280, 800);
    setMinimumSize(900, 560);

    m_runner      = new Runner(this);
    m_currentLang = LanguageConfig::byName("C++");

    setupUi();
    applyTheme();

    connect(m_runner, &Runner::output,      m_output, &OutputPanel::appendOutput);
    connect(m_runner, &Runner::errorOutput, m_output, &OutputPanel::appendError);
    connect(m_runner, &Runner::started, this, [this] {
        setRunning(true);
        m_output->showRunning();
        m_output->setInputEnabled(true);
    });
    connect(m_runner, &Runner::finished, this, [this](int ms) {
        setRunning(false);
        m_output->showDone(ms);
        m_output->setInputEnabled(false);
        m_statusLabel->setText(QString("Done %1 ms").arg(ms));
    });
    connect(m_output, &OutputPanel::inputSubmitted, m_runner, &Runner::sendInput);

    auto *sc = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(sc, &QShortcut::activated, this, &MainWindow::onRun);

    onLanguageChanged("C++");
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // title bar
    m_titleBar = new TitleBar(this);
    connect(m_titleBar, &TitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &TitleBar::maximizeClicked, this, [this] {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(m_titleBar, &TitleBar::closeClicked, this, &QMainWindow::close);
    root->addWidget(m_titleBar);

    // thin separator
    auto makeSep = [this]() -> QWidget* {
        auto *s = new QWidget(this);
        s->setFixedHeight(1);
        s->setStyleSheet("background:#1a1a1a;");
        return s;
    };
    root->addWidget(makeSep());

    // toolbar
    auto *tb = new QWidget(this);
    tb->setFixedHeight(52);
    tb->setObjectName("Toolbar");
    auto *tbL = new QHBoxLayout(tb);
    tbL->setContentsMargins(16, 0, 16, 0);
    tbL->setSpacing(10);

    // helper: small dim label
    auto label = [&](const QString &t) -> QLabel* {
        auto *l = new QLabel(t, tb);
        l->setStyleSheet("color:#383838; font-size:10px; letter-spacing:1px;");
        l->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        return l;
    };

    m_langBox = new QComboBox(tb);
    m_langBox->setObjectName("LangBox");
    m_langBox->setFixedHeight(34);
    m_langBox->setMinimumWidth(140);
    for (const auto &l : LanguageConfig::all())
        m_langBox->addItem(l.name);

    m_runBtn = new QPushButton("\u25b6   Run", tb);
    m_runBtn->setObjectName("RunBtn");
    m_runBtn->setFixedSize(96, 34);
    m_runBtn->setCursor(Qt::PointingHandCursor);

    m_stopBtn = new QPushButton("\u25a0   Stop", tb);
    m_stopBtn->setObjectName("StopBtn");
    m_stopBtn->setFixedSize(80, 34);
    m_stopBtn->setEnabled(false);
    m_stopBtn->setCursor(Qt::PointingHandCursor);

    m_clearBtn = new QPushButton("Clear", tb);
    m_clearBtn->setObjectName("ClearBtn");
    m_clearBtn->setFixedSize(64, 34);
    m_clearBtn->setCursor(Qt::PointingHandCursor);

    m_statusLabel = new QLabel("Ready", tb);
    m_statusLabel->setObjectName("StatusLabel");
    m_statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    // no fixed width - let it size naturally

    tbL->addWidget(label("LANGUAGE"));
    tbL->addWidget(m_langBox);
    tbL->addSpacing(6);
    tbL->addWidget(m_runBtn);
    tbL->addWidget(m_stopBtn);
    tbL->addSpacing(2);
    tbL->addWidget(m_clearBtn);
    tbL->addStretch();
    tbL->addWidget(m_statusLabel);
    root->addWidget(tb);
    root->addWidget(makeSep());

    // panel headers
    auto makeHeader = [this](const QString &t) -> QWidget* {
        auto *w = new QWidget(this);
        w->setFixedHeight(30);
        w->setStyleSheet("background:#0f0f0f;");
        auto *lay = new QHBoxLayout(w);
        lay->setContentsMargins(14, 0, 14, 0);
        auto *lbl = new QLabel(t, w);
        lbl->setStyleSheet("color:#383838; font-size:10px; letter-spacing:2px;");
        lbl->setAlignment(Qt::AlignVCenter);
        lay->addWidget(lbl);
        return w;
    };

    // editor container
    auto *editorContainer = new QWidget(this);
    auto *edLay = new QVBoxLayout(editorContainer);
    edLay->setContentsMargins(0, 0, 0, 0);
    edLay->setSpacing(0);
    edLay->addWidget(makeHeader("EDITOR"));
    edLay->addWidget(makeSep());
    m_editor = new CodeEditor(editorContainer);
    m_editor->setObjectName("CodeEditor");
    edLay->addWidget(m_editor);

    // output container
    auto *outContainer = new QWidget(this);
    auto *outLay = new QVBoxLayout(outContainer);
    outLay->setContentsMargins(0, 0, 0, 0);
    outLay->setSpacing(0);
    outLay->addWidget(makeHeader("OUTPUT"));
    outLay->addWidget(makeSep());
    m_output = new OutputPanel(outContainer);
    outLay->addWidget(m_output);

    // splitter
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle { background:#1a1a1a; }");
    splitter->addWidget(editorContainer);
    splitter->addWidget(outContainer);
    splitter->setSizes({640, 640});
    root->addWidget(splitter);

    connect(m_runBtn,   &QPushButton::clicked, this, &MainWindow::onRun);
    connect(m_stopBtn,  &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::onClearOutput);
    connect(m_langBox,  &QComboBox::currentTextChanged, this, &MainWindow::onLanguageChanged);
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet(R"(
        * { font-family: "Segoe UI", "Inter", sans-serif; }

        QMainWindow, QWidget { background:#111111; color:#c8c8c8; }

        #Toolbar { background:#0f0f0f; }

        #LangBox {
            background:#181818; color:#C792EA;
            border:1px solid #222; border-radius:5px;
            padding:0 10px; font-size:12px;
        }
        #LangBox QAbstractItemView {
            background:#181818; color:#c8c8c8;
            selection-background-color:#252525;
            border:1px solid #252525;
        }
        QComboBox::drop-down { border:none; width:24px; }
        QComboBox::down-arrow { image:none; }

        #RunBtn {
            background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                stop:0 #2d6e3e, stop:1 #235530);
            color:#c3e88d; border:none; border-radius:5px;
            font-size:12px; font-weight:600; letter-spacing:0.5px;
        }
        #RunBtn:hover  { background:#3a8050; }
        #RunBtn:pressed{ background:#1e4a28; }
        #RunBtn:disabled{ background:#182018; color:#2a3e2a; }

        #StopBtn {
            background:#1e1212; color:#f07178;
            border:1px solid #2e1515; border-radius:5px;
            font-size:12px; font-weight:600;
        }
        #StopBtn:enabled { background:#211414; }
        #StopBtn:hover   { background:#2e1818; border-color:#3e2020; }
        #StopBtn:disabled{ color:#2a1818; border-color:#1a1111; background:#141010; }

        #ClearBtn {
            background:#181818; color:#484848;
            border:1px solid #202020; border-radius:5px; font-size:12px;
        }
        #ClearBtn:hover { background:#202020; color:#888; }

        #TitleBar {
            background: #0a0a0a;
            border-bottom: 1px solid #1a1a1a;
        }

        #StatusLabel { color:#383838; font-size:11px; }

        QPlainTextEdit#CodeEditor {
            background:#111; color:#d4d4d4; border:none;
            selection-background-color:#264f78;
        }

        QSplitter::handle { background:#1a1a1a; }
    )");
}

void MainWindow::onRun()
{
    if (m_runner->isRunning()) return;
    m_statusLabel->setText("Running...");
    m_runner->run(m_currentLang, m_editor->toPlainText());
}

void MainWindow::onStop()
{
    m_runner->stop();
    setRunning(false);
    m_output->setInputEnabled(false);
    m_statusLabel->setText("Stopped");
}

void MainWindow::onLanguageChanged(const QString &name)
{
    m_currentLang = LanguageConfig::byName(name);
    m_editor->highlighter()->setLanguage(m_currentLang);

    QString cur = m_editor->toPlainText().trimmed();
    bool isSnippet = cur.isEmpty();
    if (!isSnippet) {
        for (const auto &l : LanguageConfig::all())
            if (cur == l.helloWorld.trimmed()) { isSnippet = true; break; }
    }
    if (isSnippet)
        m_editor->setPlainText(m_currentLang.helloWorld);

    m_titleBar->setTitle(QString("Quin Sandbox  —  %1").arg(name));
    m_statusLabel->setText("Ready");
}

void MainWindow::onClearOutput()
{
    m_output->clear();
}

void MainWindow::setRunning(bool r)
{
    m_runBtn->setEnabled(!r);
    m_stopBtn->setEnabled(r);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape && isMaximized()) showNormal();
    QMainWindow::keyPressEvent(e);
}
