#include "SyntaxHighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    m_keywordFmt.setForeground(QColor("#C792EA"));
    m_keywordFmt.setFontWeight(QFont::Bold);

    m_stringFmt.setForeground(QColor("#C3E88D"));

    m_commentFmt.setForeground(QColor("#546E7A"));
    m_commentFmt.setFontItalic(true);

    m_numberFmt.setForeground(QColor("#F78C6C"));

    m_preprocessorFmt.setForeground(QColor("#89DDFF"));
}

void SyntaxHighlighter::setLanguage(const Language &lang)
{
    m_rules.clear();

    // keywords
    for (const QString &kw : lang.keywords) {
        Rule r;
        r.pattern = QRegularExpression("\\b" + QRegularExpression::escape(kw) + "\\b");
        r.format  = m_keywordFmt;
        m_rules.append(r);
    }

    // strings (double and single quoted)
    {
        Rule r;
        r.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"|'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
        r.format  = m_stringFmt;
        m_rules.append(r);
    }

    // numbers
    {
        Rule r;
        r.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b");
        r.format  = m_numberFmt;
        m_rules.append(r);
    }

    // preprocessor (#include, #define, etc.)
    {
        Rule r;
        r.pattern = QRegularExpression("^\\s*#[a-zA-Z]+");
        r.format  = m_preprocessorFmt;
        m_rules.append(r);
    }

    // single-line comment //
    {
        Rule r;
        r.pattern = QRegularExpression("//[^\n]*");
        r.format  = m_commentFmt;
        m_rules.append(r);
    }

    // hash comment (Python, etc.)
    {
        Rule r;
        r.pattern = QRegularExpression("#[^\n]*");
        r.format  = m_commentFmt;
        m_rules.append(r);
    }

    rehighlight();
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const Rule &r : m_rules) {
        QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }
}
