#include "OutputPanel.h"
#include <QScrollBar>
#include <QHBoxLayout>
#include <QLabel>

OutputPanel::OutputPanel(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_view = new QTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setLineWrapMode(QTextEdit::NoWrap);
    m_view->setStyleSheet(
        "QTextEdit {"
        "  background:#0d0d0d; color:#d4d4d4;"
        "  border:none; padding:12px;"
        "  font-family:'JetBrains Mono','Consolas',monospace; font-size:12px;"
        "}"
        "QScrollBar:vertical   { background:#111; width:8px; border:none; }"
        "QScrollBar::handle:vertical { background:#2a2a2a; border-radius:4px; min-height:20px; }"
        "QScrollBar:horizontal { background:#111; height:8px; border:none; }"
        "QScrollBar::handle:horizontal { background:#2a2a2a; border-radius:4px; }"
    );
    root->addWidget(m_view);

    auto *sep = new QWidget(this);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background:#1e1e1e;");
    root->addWidget(sep);

    auto *bar = new QWidget(this);
    bar->setFixedHeight(38);
    bar->setStyleSheet("background:#111;");
    auto *hlay = new QHBoxLayout(bar);
    hlay->setContentsMargins(8, 4, 8, 4);
    hlay->setSpacing(6);

    auto *prompt = new QLabel(">", bar);
    prompt->setStyleSheet("color:#C792EA; font-family:monospace; font-size:13px;");
    prompt->setFixedWidth(14);

    m_input = new QLineEdit(bar);
    m_input->setPlaceholderText("stdin input...");
    m_input->setStyleSheet(
        "QLineEdit {"
        "  background:#181818; color:#d4d4d4; border:1px solid #222;"
        "  border-radius:4px; padding:2px 8px;"
        "  font-family:'JetBrains Mono','Consolas',monospace; font-size:12px;"
        "}"
        "QLineEdit:focus { border-color:#C792EA; }"
        "QLineEdit:disabled { color:#333; border-color:#1a1a1a; background:#111; }"
    );

    m_sendBtn = new QPushButton("Send", bar);
    m_sendBtn->setFixedWidth(54);
    m_sendBtn->setFixedHeight(26);
    m_sendBtn->setStyleSheet(
        "QPushButton { background:#1e3a28; color:#c3e88d; border:none; border-radius:4px; font-size:11px; }"
        "QPushButton:hover { background:#2a5535; }"
        "QPushButton:disabled { background:#151515; color:#333; }"
    );

    hlay->addWidget(prompt);
    hlay->addWidget(m_input);
    hlay->addWidget(m_sendBtn);
    root->addWidget(bar);

    connect(m_sendBtn, &QPushButton::clicked, this, &OutputPanel::onSend);
    connect(m_input, &QLineEdit::returnPressed, this, &OutputPanel::onSend);

    // render at ~60fps so we never show a partial frame
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);
    m_renderTimer->setSingleShot(false);
    connect(m_renderTimer, &QTimer::timeout, this, &OutputPanel::renderToView);
    m_renderTimer->start();

    setInputEnabled(false);
}

void OutputPanel::setInputEnabled(bool on)
{
    m_input->setEnabled(on);
    m_sendBtn->setEnabled(on);
    if (on) m_input->setFocus();
}

void OutputPanel::onSend()
{
    QString txt = m_input->text();
    if (txt.isEmpty()) return;
    writeText(txt + "\n");
    m_dirty = true;
    emit inputSubmitted(txt);
    m_input->clear();
}

// write a single char into the screen buffer at the current cursor
void OutputPanel::writeChar(QChar c)
{
    while (m_lines.size() <= m_curRow)
        m_lines.append(QString());

    QString &line = m_lines[m_curRow];
    while (line.size() <= m_curCol)
        line += ' ';

    line[m_curCol] = c;
    m_curCol++;
}

// write plain text into the buffer (no escape processing)
void OutputPanel::writeText(const QString &text)
{
    for (QChar c : text) {
        if (c == '\n') {
            m_curRow++;
            m_curCol = 0;
        } else if (c == '\r') {
            m_curCol = 0;
        } else {
            writeChar(c);
        }
    }
}

// process raw output which may contain ANSI escape sequences
void OutputPanel::processAnsi(const QString &raw)
{
    m_rawBuf += raw;

    int i = 0;
    while (i < m_rawBuf.size()) {
        QChar c = m_rawBuf[i];

        if (c == '\x1b') {
            if (i + 1 >= m_rawBuf.size()) break; // wait for more data

            if (m_rawBuf[i + 1] == '[') {
                // find the terminating letter
                int j = i + 2;
                while (j < m_rawBuf.size() && !m_rawBuf[j].isLetter()) j++;
                if (j >= m_rawBuf.size()) break; // incomplete, wait

                QChar cmd = m_rawBuf[j];
                QString param = m_rawBuf.mid(i + 2, j - (i + 2));

                if (cmd == 'A') {
                    // cursor up
                    int n = param.isEmpty() ? 1 : param.toInt();
                    m_curRow = qMax(0, m_curRow - n);
                    m_curCol = 0;
                } else if (cmd == 'B') {
                    // cursor down
                    int n = param.isEmpty() ? 1 : param.toInt();
                    m_curRow += n;
                } else if (cmd == 'C') {
                    // cursor right
                    int n = param.isEmpty() ? 1 : param.toInt();
                    m_curCol += n;
                } else if (cmd == 'D') {
                    // cursor left
                    int n = param.isEmpty() ? 1 : param.toInt();
                    m_curCol = qMax(0, m_curCol - n);
                } else if (cmd == 'H' || cmd == 'f') {
                    // cursor position ESC[row;colH (1-based)
                    QStringList parts = param.split(';');
                    m_curRow = (parts.size() > 0 && !parts[0].isEmpty()) ? qMax(0, parts[0].toInt() - 1) : 0;
                    m_curCol = (parts.size() > 1 && !parts[1].isEmpty()) ? qMax(0, parts[1].toInt() - 1) : 0;
                } else if (cmd == 'J') {
                    // erase display
                    int n = param.isEmpty() ? 0 : param.toInt();
                    if (n == 2 || n == 3) {
                        m_lines.clear();
                        m_curRow = 0;
                        m_curCol = 0;
                    }
                } else if (cmd == 'K') {
                    // erase line - clear from cursor to end of line
                    if (m_curRow < m_lines.size()) {
                        QString &line = m_lines[m_curRow];
                        if (m_curCol < line.size())
                            line = line.left(m_curCol);
                    }
                }
                // SGR (color codes 'm') and others are silently ignored
                i = j + 1;
            } else {
                i++; // unknown escape type, skip
            }
        } else if (c == '\r') {
            m_curCol = 0;
            i++;
        } else if (c == '\n') {
            m_curRow++;
            m_curCol = 0;
            i++;
        } else {
            writeChar(c);
            i++;
        }
    }

    m_rawBuf = m_rawBuf.mid(i);
    m_dirty = true;
}

// called by timer at ~60fps - only repaints when something changed
void OutputPanel::renderToView()
{
    if (!m_dirty) return;
    m_dirty = false;

    QString text;
    text.reserve(m_lines.size() * 80);
    for (int r = 0; r < m_lines.size(); r++) {
        if (r > 0) text += '\n';
        text += m_lines[r];
    }

    // block signals to avoid triggering scroll callbacks during update
    m_view->blockSignals(true);
    int scrollPos = m_view->verticalScrollBar()->value();
    bool atBottom = (scrollPos == m_view->verticalScrollBar()->maximum());

    m_view->setPlainText(text);

    if (atBottom)
        m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
    else
        m_view->verticalScrollBar()->setValue(scrollPos);

    m_view->blockSignals(false);
}

void OutputPanel::appendOutput(const QString &text)
{
    processAnsi(text);
}

void OutputPanel::appendError(const QString &text)
{
    writeText(text);
    m_dirty = true;
}

void OutputPanel::clear()
{
    m_lines.clear();
    m_curRow = 0;
    m_curCol = 0;
    m_rawBuf.clear();
    m_dirty = true;
}

void OutputPanel::showRunning()
{
    clear();
    writeText("Running...\n\n");
    m_dirty = true;
}

void OutputPanel::showDone(int ms)
{
    // move to end of buffer then append done line
    m_curRow = m_lines.size();
    m_curCol = 0;
    writeText(QString("\n\n--- done in %1 ms ---").arg(ms));
    m_dirty = true;
}
