#include "TitleBar.h"
#include <QHBoxLayout>
#include <QMouseEvent>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(40);
    setObjectName("TitleBar");

    auto *lay = new QHBoxLayout(this);
    // left 12px, right 0 so close button touches the window edge
    lay->setContentsMargins(12, 0, 0, 0);
    lay->setSpacing(0);

    auto *logo = new QLabel("⬡", this);
    logo->setStyleSheet("color:#C792EA; font-size:16px; font-weight:bold;");
    logo->setContentsMargins(0, 0, 8, 0);

    m_title = new QLabel("Quin Sandbox", this);
    m_title->setStyleSheet("color:#555; font-size:11px; letter-spacing:1px;");

    lay->addWidget(logo);
    lay->addWidget(m_title);
    lay->addStretch();

    // window control buttons — fixed 46x40 to match native feel
    auto makeBtn = [this](const QString &sym, const QString &hoverBg, const QString &hoverFg) -> QPushButton* {
        auto *b = new QPushButton(sym, this);
        b->setFixedSize(46, 40);
        b->setFlat(true);
        b->setCursor(Qt::ArrowCursor);
        b->setStyleSheet(
            "QPushButton { color:#505050; background:transparent; border:none; font-size:13px; }"
            "QPushButton:hover { color:" + hoverFg + "; background:" + hoverBg + "; }"
        );
        return b;
    };

    m_minimize = makeBtn("\u2212", "#2a2a2a", "#c8c8c8"); // minus sign
    m_maximize = makeBtn("\u25a1", "#2a2a2a", "#c8c8c8"); // white square
    m_close    = makeBtn("\u00d7", "#c0392b", "#ffffff"); // times sign, red bg

    lay->addWidget(m_minimize);
    lay->addWidget(m_maximize);
    lay->addWidget(m_close);

    connect(m_minimize, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(m_maximize, &QPushButton::clicked, this, &TitleBar::maximizeClicked);
    connect(m_close,    &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::setTitle(const QString &t) { m_title->setText(t); }

void TitleBar::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - window()->frameGeometry().topLeft();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging)
        window()->move(e->globalPosition().toPoint() - m_dragPos);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *)
{
    m_dragging = false;
}
