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
        "\\bif\\b", "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\bdo\\b",
        "\\bbreak\\b", "\\bcontinue\\b", "\\breturn\\b", "\\bdiscard\\b",
        "\\bswitch\\b", "\\bcase\\b", "\\bdefault\\b",
        "\\bstruct\\b", "\\bconst\\b",
        "\\bin\\b", "\\bout\\b", "\\binout\\b",
        "\\buniform\\b", "\\bvarying\\b", "\\battribute\\b",
        "\\bcentroid\\b", "\\bflat\\b", "\\bsmooth\\b", "\\bnoperspective\\b",
        "\\blayout\\b", "\\bprecision\\b", "\\bhighp\\b", "\\bmediump\\b", "\\blowp\\b",
        "\\binvariant\\b", "\\bprecise\\b"
    };

    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(86, 156, 214));
    keywordFormat.setFontWeight(QFont::Bold);

    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        m_highlightingRules.append(rule);
    }

    QStringList typePatterns = {
        "\\bvoid\\b", "\\bbool\\b", "\\bint\\b", "\\buint\\b", "\\bfloat\\b", "\\bdouble\\b",
        "\\bbvec2\\b", "\\bbvec3\\b", "\\bbvec4\\b",
        "\\bivec2\\b", "\\bivec3\\b", "\\bivec4\\b",
        "\\buvec2\\b", "\\buvec3\\b", "\\buvec4\\b",
        "\\bvec2\\b", "\\bvec3\\b", "\\bvec4\\b",
        "\\bdvec2\\b", "\\bdvec3\\b", "\\bdvec4\\b",
        "\\bmat2\\b", "\\bmat3\\b", "\\bmat4\\b",
        "\\bmat2x2\\b", "\\bmat2x3\\b", "\\bmat2x4\\b",
        "\\bmat3x2\\b", "\\bmat3x3\\b", "\\bmat3x4\\b",
        "\\bmat4x2\\b", "\\bmat4x3\\b", "\\bmat4x4\\b",
        "\\bdmat2\\b", "\\bdmat3\\b", "\\bdmat4\\b",
        "\\bsampler1D\\b", "\\bsampler2D\\b", "\\bsampler3D\\b",
        "\\bsamplerCube\\b", "\\bsampler1DShadow\\b", "\\bsampler2DShadow\\b",
        "\\bsampler1DArray\\b", "\\bsampler2DArray\\b",
        "\\bsampler2DRect\\b", "\\bsampler2DMS\\b",
        "\\bisampler1D\\b", "\\bisampler2D\\b", "\\bisampler3D\\b",
        "\\busampler1D\\b", "\\busampler2D\\b", "\\busampler3D\\b"
    };

    QTextCharFormat typeFormat;
    typeFormat.setForeground(QColor(78, 201, 176));
    typeFormat.setFontWeight(QFont::Bold);

    for (const QString &pattern : typePatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = typeFormat;
        m_highlightingRules.append(rule);
    }

    QStringList builtinPatterns = {
        "\\bradians\\b", "\\bdegrees\\b", "\\bsin\\b", "\\bcos\\b", "\\btan\\b",
        "\\basin\\b", "\\bacos\\b", "\\batan\\b", "\\bsinh\\b", "\\bcosh\\b", "\\btanh\\b",
        "\\basinh\\b", "\\bacosh\\b", "\\batanh\\b",
        "\\bpow\\b", "\\bexp\\b", "\\blog\\b", "\\bexp2\\b", "\\blog2\\b",
        "\\bsqrt\\b", "\\binversesqrt\\b",
        "\\babs\\b", "\\bsign\\b", "\\bfloor\\b", "\\btrunc\\b", "\\bround\\b",
        "\\broundEven\\b", "\\bceil\\b", "\\bfract\\b", "\\bmod\\b", "\\bmodf\\b",
        "\\bmin\\b", "\\bmax\\b", "\\bclamp\\b", "\\bmix\\b", "\\bstep\\b", "\\bsmoothstep\\b",
        "\\bisnan\\b", "\\bisinf\\b", "\\bfloatBitsToInt\\b", "\\bfloatBitsToUint\\b",
        "\\bintBitsToFloat\\b", "\\buintBitsToFloat\\b",
        "\\blength\\b", "\\bdistance\\b", "\\bdot\\b", "\\bcross\\b",
        "\\bnormalize\\b", "\\bfaceforward\\b", "\\breflect\\b", "\\brefract\\b",
        "\\bmatrixCompMult\\b", "\\bouterProduct\\b", "\\btranspose\\b",
        "\\bdeterminant\\b", "\\binverse\\b",
        "\\blessThan\\b", "\\blessThanEqual\\b", "\\bgreaterThan\\b", "\\bgreaterThanEqual\\b",
        "\\bequal\\b", "\\bnotEqual\\b", "\\bany\\b", "\\ball\\b", "\\bnot\\b",
        "\\btexture\\b", "\\btexture1D\\b", "\\btexture2D\\b", "\\btexture3D\\b",
        "\\btextureCube\\b", "\\btextureProj\\b", "\\btextureLod\\b",
        "\\btextureOffset\\b", "\\btexelFetch\\b", "\\btexelFetchOffset\\b",
        "\\btextureProjOffset\\b", "\\btextureLodOffset\\b", "\\btextureGrad\\b",
        "\\btextureGradOffset\\b", "\\btextureProjGrad\\b", "\\btextureProjGradOffset\\b",
        "\\btextureSize\\b", "\\btextureQueryLod\\b", "\\btextureQueryLevels\\b",
        "\\btextureSamples\\b", "\\btextureGather\\b", "\\btextureGatherOffset\\b",
        "\\btextureGatherOffsets\\b",
        "\\bdFdx\\b", "\\bdFdy\\b", "\\bdFdxFine\\b", "\\bdFdyFine\\b",
        "\\bdFdxCoarse\\b", "\\bdFdyCoarse\\b", "\\bfwidth\\b", "\\bfwidthFine\\b", "\\bfwidthCoarse\\b",
        "\\binterpolateAtCentroid\\b", "\\binterpolateAtSample\\b", "\\binterpolateAtOffset\\b",
        "\\bnoise1\\b", "\\bnoise2\\b", "\\bnoise3\\b", "\\bnoise4\\b",
        "\\bEmitVertex\\b", "\\bEndPrimitive\\b",
        "\\batomicCounterIncrement\\b", "\\batomicCounterDecrement\\b", "\\batomicCounter\\b",
        "\\batomicAdd\\b", "\\batomicMin\\b", "\\batomicMax\\b", "\\batomicAnd\\b",
        "\\batomicOr\\b", "\\batomicXor\\b", "\\batomicExchange\\b", "\\batomicCompSwap\\b",
        "\\bimageSize\\b", "\\bimageLoad\\b", "\\bimageStore\\b", "\\bimageAtomicAdd\\b",
        "\\bimageAtomicMin\\b", "\\bimageAtomicMax\\b", "\\bimageAtomicAnd\\b",
        "\\bimageAtomicOr\\b", "\\bimageAtomicXor\\b", "\\bimageAtomicExchange\\b",
        "\\bimageAtomicCompSwap\\b",
        "\\bpackUnorm2x16\\b", "\\bpackSnorm2x16\\b", "\\bpackUnorm4x8\\b", "\\bpackSnorm4x8\\b",
        "\\bunpackUnorm2x16\\b", "\\bunpackSnorm2x16\\b", "\\bunpackUnorm4x8\\b", "\\bunpackSnorm4x8\\b",
        "\\bpackHalf2x16\\b", "\\bunpackHalf2x16\\b", "\\bpackDouble2x32\\b", "\\bunpackDouble2x32\\b"
    };

    QTextCharFormat builtinFormat;
    builtinFormat.setForeground(QColor(220, 220, 170));

    for (const QString &pattern : builtinPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = builtinFormat;
        m_highlightingRules.append(rule);
    }

    QStringList constantPatterns = {
        "\\bgl_Position\\b", "\\bgl_PointSize\\b", "\\bgl_ClipDistance\\b",
        "\\bgl_VertexID\\b", "\\bgl_InstanceID\\b",
        "\\bgl_FragCoord\\b", "\\bgl_FrontFacing\\b", "\\bgl_PointCoord\\b",
        "\\bgl_FragColor\\b", "\\bgl_FragData\\b", "\\bgl_FragDepth\\b",
        "\\bgl_PrimitiveID\\b", "\\bgl_Layer\\b", "\\bgl_ViewportIndex\\b",
        "\\bgl_MaxVertexAttribs\\b", "\\bgl_MaxVertexUniformComponents\\b",
        "\\bgl_MaxVaryingFloats\\b", "\\bgl_MaxVaryingComponents\\b",
        "\\bgl_MaxVertexOutputComponents\\b", "\\bgl_MaxGeometryInputComponents\\b",
        "\\bgl_MaxGeometryOutputComponents\\b", "\\bgl_MaxFragmentInputComponents\\b",
        "\\bgl_MaxVertexTextureImageUnits\\b", "\\bgl_MaxCombinedTextureImageUnits\\b",
        "\\bgl_MaxTextureImageUnits\\b", "\\bgl_MaxFragmentUniformComponents\\b",
        "\\bgl_MaxDrawBuffers\\b", "\\bgl_MaxClipDistances\\b",
        "\\bgl_MaxGeometryTextureImageUnits\\b", "\\bgl_MaxGeometryOutputVertices\\b",
        "\\bgl_MaxGeometryTotalOutputComponents\\b", "\\bgl_MaxGeometryUniformComponents\\b",
        "\\bgl_MaxGeometryVaryingComponents\\b",
        "\\btrue\\b", "\\bfalse\\b"
    };

    QTextCharFormat constantFormat;
    constantFormat.setForeground(QColor(181, 206, 168));

    for (const QString &pattern : constantPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = constantFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(QColor(155, 155, 155));
    preprocessorFormat.setFontWeight(QFont::Bold);

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("^\\s*#\\s*(version|define|undef|if|ifdef|ifndef|else|elif|endif|error|pragma|extension|line)\\b");
        rule.format = preprocessorFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(181, 206, 168));

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\\b((0[xX][0-9a-fA-F]+)|(0[0-7]+)|([0-9]+\\.?[0-9]*([eE][+-]?[0-9]+)?))([fFuU]|[uU][lL]|[lL][uU])?\\b");
        rule.format = numberFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(206, 145, 120));

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("\"([^\"\\\\]|\\\\.)*\"");
        rule.format = stringFormat;
        m_highlightingRules.append(rule);
    }

    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(QColor(106, 153, 85));
    singleLineCommentFormat.setFontItalic(true);

    {
        HighlightingRule rule;
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = singleLineCommentFormat;
        m_highlightingRules.append(rule);
    }

    m_commentStartPattern = QRegularExpression("/\\*");
    m_commentEndPattern = QRegularExpression("\\*/");
    m_multiLineCommentFormat.setForeground(QColor(106, 153, 85));
    m_multiLineCommentFormat.setFontItalic(true);
}
