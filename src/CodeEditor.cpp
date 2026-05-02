#include "CodeEditor.h"
#include "SyntaxHighlighter.h"
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineArea   = new LineNumberArea(this);
    m_highlighter = new SyntaxHighlighter(document());

    connect(this, &CodeEditor::blockCountChanged,   this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,       this, &CodeEditor::updateLineNumberArea);

    updateLineNumberAreaWidth(0);

    // font
    QFont f("JetBrains Mono", 12);
    f.setStyleHint(QFont::Monospace);
    if (!QFont("JetBrains Mono").exactMatch())
        f.setFamily("Consolas");
    setFont(f);

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max    = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 12 + fontMetrics().horizontalAdvance('9') * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineArea->scroll(0, dy);
    else
        m_lineArea->update(0, rect.y(), m_lineArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_lineArea->setGeometry(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height());
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    // auto-indent on Enter
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        QTextCursor c = textCursor();
        QString line  = c.block().text();
        QString indent;
        for (QChar ch : line) {
            if (ch == ' ' || ch == '\t') indent += ch;
            else break;
        }
        // extra indent after {
        if (line.trimmed().endsWith('{'))
            indent += "    ";
        QPlainTextEdit::keyPressEvent(e);
        insertPlainText(indent);
        return;
    }
    // Tab = 4 spaces
    if (e->key() == Qt::Key_Tab) {
        insertPlainText("    ");
        return;
    }
    QPlainTextEdit::keyPressEvent(e);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineArea);
    painter.fillRect(event->rect(), QColor("#1a1a1a"));

    QTextBlock block = firstVisibleBlock();
    int num = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(QColor("#444"));
            painter.setFont(font());
            painter.drawText(0, top, m_lineArea->width() - 6,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(num + 1));
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++num;
    }
}

SyntaxHighlighter* CodeEditor::highlighter() { return m_highlighter; }
