#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVector>
#include <QTimer>

class OutputPanel : public QWidget {
    Q_OBJECT
public:
    explicit OutputPanel(QWidget *parent = nullptr);
    void setInputEnabled(bool on);

public slots:
    void appendOutput(const QString &text);
    void appendError(const QString &text);
    void clear();
    void showRunning();
    void showDone(int ms);

signals:
    void inputSubmitted(const QString &text);

private slots:
    void onSend();
    void renderToView();

private:
    QTextEdit   *m_view;
    QLineEdit   *m_input;
    QPushButton *m_sendBtn;

    // screen buffer
    QVector<QString> m_lines;
    int m_curRow = 0;
    int m_curCol = 0;
    QString m_rawBuf;
    bool m_dirty = false;
    QTimer *m_renderTimer = nullptr;

    void writeChar(QChar c);
    void writeText(const QString &text);
    void processAnsi(const QString &raw);
};
