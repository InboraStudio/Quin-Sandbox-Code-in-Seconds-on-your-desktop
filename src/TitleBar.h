#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>

// frameless custom title bar
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    void setTitle(const QString &t);

signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QLabel      *m_title;
    QPushButton *m_close;
    QPushButton *m_maximize;
    QPushButton *m_minimize;
    QPoint       m_dragPos;
    bool         m_dragging = false;
};
