#include "syntax.hpp"

GlslSyntaxHighlighter::GlslSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    initHighlightingRules();
}

void GlslSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : m_highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStartPattern);

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = m_commentEndPattern.match(text, startIndex);
        int endIndex = endMatch.capturedStart();
        int commentLength = 0;

        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartPattern, startIndex + commentLength);
    }
}

void GlslSyntaxHighlighter::initHighlightingRules()
{
    QStringList keywordPatterns = {
        "\\bif\\b", "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\bdo\\b", "\\bbreak\\b",
        "\\bcontinue\\b", "\\breturn\\b", "\\bdiscard\\b", "\\bvoid\\b", "\\bbool\\b",
        "\\bint\\b", "\\bfloat\\b", "\\bvec2\\b", "\\bvec3\\b", "\\bvec4\\b",
        "\\bmat2\\b", "\\bmat3\\b", "\\bmat4\\b", "\\bsampler2D\\b", "\\bsamplerCube\\b",
        "\\bin\\b", "\\bout\\b", "\\binout\\b", "\\buniform\\b",
        "\\#version", "\\#define", "\\#if", "\\#ifdef", "\\#ifndef", "\\#endif",
        "\\#else", "\\#elif", "\\#extension"
    };

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(Qt::blue);
    keywordFormat.setFontWeight(QFont::Bold);

    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(Qt::darkGreen);

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = singleLineCommentFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat numberFormat;
    numberFormat.setForeground(Qt::darkCyan);

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
        rule.format = numberFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat stringFormat;
    stringFormat.setForeground(Qt::darkRed);

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\".*\"");
        rule.format = stringFormat;
        m_highlightingRules.append(rule);
    }

    m_commentStartPattern = QRegularExpression("/\\*");
    m_commentEndPattern = QRegularExpression("\\*/");
    m_multiLineCommentFormat.setForeground(Qt::darkGreen);
}
