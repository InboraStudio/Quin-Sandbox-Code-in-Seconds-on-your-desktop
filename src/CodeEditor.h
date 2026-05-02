#pragma once
#include <QPlainTextEdit>
#include <QWidget>

class LineNumberArea;
class SyntaxHighlighter;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int  lineNumberAreaWidth() const;
    SyntaxHighlighter *highlighter();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget          *m_lineArea;
    SyntaxHighlighter *m_highlighter;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return { m_editor->lineNumberAreaWidth(), 0 }; }
protected:
    void paintEvent(QPaintEvent *e) override { m_editor->lineNumberAreaPaintEvent(e); }
private:
    CodeEditor *m_editor;
};
