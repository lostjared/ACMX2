#ifndef __SYNTAX__H_
#define __SYNTAX__H_

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QVector>
#include <QStringList>
#include <QTextCharFormat>

class GlslSyntaxHighlighter : public QSyntaxHighlighter { 
    Q_OBJECT
public:
    explicit GlslSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void initHighlightingRules();

    QVector<HighlightingRule> m_highlightingRules;
    QRegularExpression m_commentStartPattern;
    QRegularExpression m_commentEndPattern;
    QTextCharFormat m_multiLineCommentFormat;
};

#endif 