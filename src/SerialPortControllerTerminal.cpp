#include "SerialPortController.h"

#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QStringList>
#include <QVector>

#include <utility>

namespace {
constexpr qsizetype kTerminalBufferMaxChars = 65536;
constexpr int kTerminalDefaultColor = 256;
constexpr quint32 kTerminalColorMask = 0x1FFU;
constexpr quint32 kTerminalBoldBit = 1U << 18;
constexpr quint32 kTerminalInverseBit = 1U << 19;

quint32 terminalStyle(int fg, int bg, bool bold, bool inverse)
{
    return (static_cast<quint32>(fg) & kTerminalColorMask) |
           ((static_cast<quint32>(bg) & kTerminalColorMask) << 9) |
           (bold ? kTerminalBoldBit : 0U) |
           (inverse ? kTerminalInverseBit : 0U);
}

int terminalFg(quint32 style)
{
    return static_cast<int>(style & kTerminalColorMask);
}

int terminalBg(quint32 style)
{
    return static_cast<int>((style >> 9) & kTerminalColorMask);
}

bool terminalBold(quint32 style)
{
    return (style & kTerminalBoldBit) != 0U;
}

bool terminalInverse(quint32 style)
{
    return (style & kTerminalInverseBit) != 0U;
}

quint32 withTerminalFg(quint32 style, int fg)
{
    return terminalStyle(fg, terminalBg(style), terminalBold(style), terminalInverse(style));
}

quint32 withTerminalBg(quint32 style, int bg)
{
    return terminalStyle(terminalFg(style), bg, terminalBold(style), terminalInverse(style));
}

quint32 withTerminalBold(quint32 style, bool bold)
{
    return terminalStyle(terminalFg(style), terminalBg(style), bold, terminalInverse(style));
}

quint32 withTerminalInverse(quint32 style, bool inverse)
{
    return terminalStyle(terminalFg(style), terminalBg(style), terminalBold(style), inverse);
}

QString terminalColorCss(int color, bool bold)
{
    if (color == kTerminalDefaultColor) {
        return {};
    }

    static constexpr const char *normalColors[] = {
        "#000000", "#cd3131", "#0dbc79", "#e5e510",
        "#2472c8", "#bc3fbc", "#11a8cd", "#e5e5e5",
    };
    static constexpr const char *brightColors[] = {
        "#666666", "#f14c4c", "#23d18b", "#f5f543",
        "#3b8eea", "#d670d6", "#29b8db", "#ffffff",
    };

    if (color >= 0 && color < 8) {
        return QString::fromLatin1((bold ? brightColors : normalColors)[color]);
    }
    if (color >= 8 && color < 16) {
        return QString::fromLatin1(brightColors[color - 8]);
    }
    if (color >= 16 && color <= 231) {
        const int n = color - 16;
        const int r = n / 36;
        const int g = (n / 6) % 6;
        const int b = n % 6;
        const auto component = [](int value) { return value == 0 ? 0 : 55 + value * 40; };
        return QStringLiteral("#%1%2%3")
            .arg(component(r), 2, 16, QLatin1Char('0'))
            .arg(component(g), 2, 16, QLatin1Char('0'))
            .arg(component(b), 2, 16, QLatin1Char('0'));
    }
    if (color >= 232 && color <= 255) {
        const int gray = 8 + (color - 232) * 10;
        return QStringLiteral("#%1%1%1").arg(gray, 2, 16, QLatin1Char('0'));
    }

    return {};
}

QString terminalCharHtml(QChar ch)
{
    if (ch == QChar(' ')) {
        return QStringLiteral("&nbsp;");
    }
    if (ch == QChar('\t')) {
        return QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
    }
    return QString(ch).toHtmlEscaped();
}

QString terminalSpanHtml(const QString &content, quint32 style, bool cursor)
{
    int fg = terminalFg(style);
    int bg = terminalBg(style);
    const bool bold = terminalBold(style);
    const bool inverse = terminalInverse(style) ^ cursor;

    if (inverse) {
        if (fg == kTerminalDefaultColor) {
            fg = 7;
        }
        if (bg == kTerminalDefaultColor) {
            bg = 0;
        }
        std::swap(fg, bg);
    }

    QStringList css;
    const QString fgCss = terminalColorCss(fg, bold);
    const QString bgCss = terminalColorCss(bg, false);
    if (!fgCss.isEmpty()) {
        css.append(QStringLiteral("color:%1").arg(fgCss));
    }
    if (!bgCss.isEmpty()) {
        css.append(QStringLiteral("background-color:%1").arg(bgCss));
    }
    if (bold) {
        css.append(QStringLiteral("font-weight:bold"));
    }

    if (css.isEmpty()) {
        return content;
    }
    return QStringLiteral("<span style=\"%1\">%2</span>").arg(css.join(QLatin1Char(';')), content);
}
}

bool SerialPortController::activateTerminal()
{
    if (!m_serialPort.isOpen() && !openPort()) {
        return false;
    }

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_ACTIVATE_TERMINAL),
                            nullptr,
                            0U,
                            QStringLiteral("终端激活"))) {
        return false;
    }

    setTerminalMode(true);
    clearTerminalBuffer();
    appendTerminalLine(QStringLiteral("[mcu_terminal] serial=%1 baud=%2 activate_len=6")
                           .arg(m_serialPort.portName())
                           .arg(m_baudRate));
    appendTerminalLine(QStringLiteral("[mcu_terminal] start passthrough, use UI button to exit"));
    setStatusMessage(QStringLiteral("已进入终端透传模式"));
    return true;
}

bool SerialPortController::sendTerminalText(const QString &text)
{
    if (!m_terminalMode || !m_serialPort.isOpen()) {
        setLastError(QStringLiteral("请先进入终端模式，再发送终端输入。"));
        return false;
    }

    const QByteArray bytes = text.toUtf8();
    if (bytes.isEmpty()) {
        return true;
    }

    const qint64 queued = m_serialPort.write(bytes);
    if (queued != bytes.size()) {
        setLastError(QStringLiteral("终端输入写串口失败：%1").arg(m_serialPort.errorString()));
        return false;
    }

    m_txBytes += static_cast<quint64>(queued);
    emit statsChanged();
    setStatusMessage(QStringLiteral("终端已发送 %1 字节").arg(queued));
    return true;
}

bool SerialPortController::sendClipboardText()
{
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        setLastError(QStringLiteral("无法读取剪贴板。"));
        return false;
    }

    const QString text = clipboard->text();
    if (text.isEmpty()) {
        setLastError(QStringLiteral("剪贴板为空。"));
        return false;
    }

    return sendTerminalText(text);
}

void SerialPortController::copyTextToClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr) {
        setLastError(QStringLiteral("无法写入剪贴板。"));
        return;
    }

    clipboard->setText(text);
    setStatusMessage(QStringLiteral("已复制终端选中文本"));
}

QString SerialPortController::terminalHtml() const
{
    QString html;
    html.reserve(m_terminalBuffer.size() * 8 + 256);
    html += QStringLiteral("<html><body style=\"margin:0;background-color:#0c0c0c;color:#cccccc;"
                           "font-family:monospace;font-size:13px;white-space:nowrap;\">");

    const auto styleAt = [this](qsizetype index) {
        return (index >= 0 && index < m_terminalStyles.size()) ? m_terminalStyles.at(index) : kTerminalDefaultStyle;
    };

    qsizetype index = 0;
    while (index < m_terminalBuffer.size()) {
        if (m_terminalBuffer.at(index) == QChar('\n')) {
            html += QStringLiteral("<br/>");
            ++index;
            continue;
        }

        const quint32 style = styleAt(index);
        QString run;
        while (index < m_terminalBuffer.size() &&
               m_terminalBuffer.at(index) != QChar('\n') &&
               styleAt(index) == style) {
            run += terminalCharHtml(m_terminalBuffer.at(index));
            ++index;
        }
        html += terminalSpanHtml(run, style, false);
    }

    html += QStringLiteral("</body></html>");
    return html;
}

void SerialPortController::clearTerminalBuffer()
{
    resetTerminalRenderer();
    emit terminalBufferChanged();
}

void SerialPortController::appendTerminalBytes(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return;
    }

    renderTerminalBytes(bytes);
    emit terminalBufferChanged();
}

void SerialPortController::appendTerminalLine(const QString &line)
{
    appendTerminalBytes(line.toUtf8() + '\n');
}

void SerialPortController::setTerminalMode(bool enabled)
{
    if (m_terminalMode == enabled) {
        return;
    }

    m_terminalMode = enabled;
    m_terminalOutputPrevWasCr = false;
    if (!enabled) {
        m_terminalEscapeBuffer.clear();
        m_terminalParserState = TerminalParserState::Ground;
    }
    emit terminalModeChanged();
    emit terminalBufferChanged();
}

void SerialPortController::resetTerminalRenderer()
{
    m_terminalBuffer.clear();
    m_terminalCursor = 0;
    m_terminalSavedCursor = 0;
    m_terminalOutputPrevWasCr = false;
    m_terminalStyles.clear();
    m_terminalCurrentStyle = kTerminalDefaultStyle;
    m_terminalEscapeBuffer.clear();
    m_terminalParserState = TerminalParserState::Ground;
}

void SerialPortController::renderTerminalBytes(const QByteArray &bytes)
{
    QByteArray printable;

    const auto flushPrintable = [this, &printable]() {
        if (printable.isEmpty()) {
            return;
        }
        putTerminalText(QString::fromUtf8(printable));
        printable.clear();
    };

    for (const unsigned char raw : bytes) {
        const char ch = static_cast<char>(raw);

        switch (m_terminalParserState) {
        case TerminalParserState::Ground:
            if (raw == 0x1B) {
                flushPrintable();
                m_terminalParserState = TerminalParserState::Escape;
                m_terminalEscapeBuffer.clear();
            } else if (raw < 0x20 || raw == 0x7F) {
                flushPrintable();
                handleTerminalControl(ch);
            } else {
                printable.append(ch);
            }
            break;
        case TerminalParserState::Escape:
            if (ch == '[') {
                m_terminalParserState = TerminalParserState::Csi;
                m_terminalEscapeBuffer.clear();
            } else {
                if (ch == 'c') {
                    resetTerminalRenderer();
                } else if (ch == '7') {
                    m_terminalSavedCursor = m_terminalCursor;
                } else if (ch == '8') {
                    m_terminalCursor = qBound<qsizetype>(0, m_terminalSavedCursor, m_terminalBuffer.size());
                }
                m_terminalParserState = TerminalParserState::Ground;
            }
            break;
        case TerminalParserState::Csi:
            if (raw >= 0x40 && raw <= 0x7E) {
                finishTerminalCsi(ch);
                m_terminalParserState = TerminalParserState::Ground;
                m_terminalEscapeBuffer.clear();
            } else {
                m_terminalEscapeBuffer.append(ch);
                if (m_terminalEscapeBuffer.size() > 64) {
                    m_terminalParserState = TerminalParserState::Ground;
                    m_terminalEscapeBuffer.clear();
                }
            }
            break;
        }
    }

    flushPrintable();
    trimTerminalBuffer();
}

void SerialPortController::putTerminalText(const QString &text)
{
    for (const QChar ch : text) {
        if (ch == QChar::ReplacementCharacter || ch.isNull()) {
            continue;
        }
        putTerminalChar(ch);
    }
}

void SerialPortController::putTerminalChar(QChar ch)
{
    if (m_terminalCursor < 0) {
        m_terminalCursor = 0;
    }
    if (m_terminalCursor > m_terminalBuffer.size()) {
        insertTerminalText(m_terminalBuffer.size(),
                           QString(m_terminalCursor - m_terminalBuffer.size(), QChar(' ')),
                           m_terminalCurrentStyle);
    }

    if (m_terminalCursor == m_terminalBuffer.size() || m_terminalBuffer.at(m_terminalCursor) == QChar('\n')) {
        insertTerminalText(m_terminalCursor, QString(ch), m_terminalCurrentStyle);
    } else {
        m_terminalBuffer[m_terminalCursor] = ch;
        if (m_terminalStyles.size() < m_terminalBuffer.size()) {
            m_terminalStyles.resize(m_terminalBuffer.size());
        }
        m_terminalStyles[m_terminalCursor] = m_terminalCurrentStyle;
    }
    ++m_terminalCursor;
}

void SerialPortController::handleTerminalControl(char ch)
{
    switch (ch) {
    case '\r':
        m_terminalCursor = terminalLineStart(m_terminalCursor);
        m_terminalOutputPrevWasCr = true;
        break;
    case '\n': {
        const qsizetype lineEnd = terminalLineEnd(m_terminalCursor);
        if (lineEnd == m_terminalBuffer.size()) {
            insertTerminalText(m_terminalBuffer.size(), QString(QChar('\n')), m_terminalCurrentStyle);
            m_terminalCursor = m_terminalBuffer.size();
        } else {
            m_terminalCursor = lineEnd + 1;
        }
        m_terminalOutputPrevWasCr = false;
        break;
    }
    case '\b':
        if (m_terminalCursor > terminalLineStart(m_terminalCursor)) {
            --m_terminalCursor;
        }
        m_terminalOutputPrevWasCr = false;
        break;
    case 0x7F:
        m_terminalOutputPrevWasCr = false;
        break;
    case '\t': {
        const qsizetype column = m_terminalCursor - terminalLineStart(m_terminalCursor);
        const int spaces = 8 - static_cast<int>(column % 8);
        for (int i = 0; i < spaces; ++i) {
            putTerminalChar(QChar(' '));
        }
        m_terminalOutputPrevWasCr = false;
        break;
    }
    case '\a':
    case 0:
        break;
    default:
        m_terminalOutputPrevWasCr = false;
        break;
    }
}

void SerialPortController::finishTerminalCsi(char finalByte)
{
    QByteArray paramsBytes = m_terminalEscapeBuffer;
    while (!paramsBytes.isEmpty() && (paramsBytes.front() == '?' || paramsBytes.front() == '>' || paramsBytes.front() == '!')) {
        paramsBytes.remove(0, 1);
    }

    const QStringList parts = QString::fromLatin1(paramsBytes).split(QLatin1Char(';'));
    QVector<int> params;
    params.reserve(parts.size());
    for (const QString &part : parts) {
        bool ok = false;
        const int value = part.toInt(&ok);
        params.append(ok ? value : 0);
    }
    if (params.isEmpty()) {
        params.append(0);
    }

    const auto paramOrDefault = [&params](int index, int defaultValue) {
        if (index >= params.size() || params.at(index) == 0) {
            return defaultValue;
        }
        return params.at(index);
    };

    switch (finalByte) {
    case 'A':
        moveTerminalCursorVertical(-paramOrDefault(0, 1));
        break;
    case 'B':
        moveTerminalCursorVertical(paramOrDefault(0, 1));
        break;
    case 'C':
        m_terminalCursor = qMin(m_terminalCursor + paramOrDefault(0, 1), terminalLineEnd(m_terminalCursor));
        break;
    case 'D':
        m_terminalCursor = qMax(terminalLineStart(m_terminalCursor), m_terminalCursor - paramOrDefault(0, 1));
        break;
    case 'E':
        moveTerminalCursorVertical(paramOrDefault(0, 1));
        m_terminalCursor = terminalLineStart(m_terminalCursor);
        break;
    case 'F':
        moveTerminalCursorVertical(-paramOrDefault(0, 1));
        m_terminalCursor = terminalLineStart(m_terminalCursor);
        break;
    case 'G': {
        const qsizetype lineStart = terminalLineStart(m_terminalCursor);
        const qsizetype target = lineStart + paramOrDefault(0, 1) - 1;
        while (terminalLineEnd(m_terminalCursor) < target) {
            insertTerminalText(terminalLineEnd(m_terminalCursor), QString(QChar(' ')), m_terminalCurrentStyle);
        }
        m_terminalCursor = target;
        break;
    }
    case 'H':
    case 'f':
        m_terminalCursor = terminalCursorForRowColumn(paramOrDefault(0, 1), paramOrDefault(1, 1));
        break;
    case '@': {
        const qsizetype lineEnd = terminalLineEnd(m_terminalCursor);
        const int count = paramOrDefault(0, 1);
        for (int i = 0; i < count; ++i) {
            insertTerminalText(qMin(m_terminalCursor + i, lineEnd + i), QString(QChar(' ')), m_terminalCurrentStyle);
        }
        break;
    }
    case 'J':
        if (params.at(0) == 2 || params.at(0) == 3) {
            m_terminalBuffer.clear();
            m_terminalStyles.clear();
            m_terminalCursor = 0;
        } else if (params.at(0) == 1) {
            removeTerminalRange(0, m_terminalCursor);
            m_terminalCursor = 0;
        } else {
            truncateTerminalBuffer(m_terminalCursor);
        }
        break;
    case 'K': {
        const qsizetype lineStart = terminalLineStart(m_terminalCursor);
        const qsizetype lineEnd = terminalLineEnd(m_terminalCursor);
        if (params.at(0) == 2) {
            removeTerminalRange(lineStart, lineEnd - lineStart);
            m_terminalCursor = lineStart;
        } else if (params.at(0) == 1) {
            const qsizetype removed = m_terminalCursor - lineStart;
            removeTerminalRange(lineStart, removed);
            m_terminalCursor = lineStart;
        } else {
            removeTerminalRange(m_terminalCursor, lineEnd - m_terminalCursor);
        }
        break;
    }
    case 'P':
        removeTerminalRange(m_terminalCursor, qMin<qsizetype>(paramOrDefault(0, 1), terminalLineEnd(m_terminalCursor) - m_terminalCursor));
        break;
    case 'X': {
        const qsizetype count = qMin<qsizetype>(paramOrDefault(0, 1), terminalLineEnd(m_terminalCursor) - m_terminalCursor);
        for (qsizetype i = 0; i < count; ++i) {
            m_terminalBuffer[m_terminalCursor + i] = QChar(' ');
            if (m_terminalStyles.size() <= m_terminalCursor + i) {
                m_terminalStyles.resize(m_terminalCursor + i + 1);
            }
            m_terminalStyles[m_terminalCursor + i] = m_terminalCurrentStyle;
        }
        break;
    }
    case 's':
        m_terminalSavedCursor = m_terminalCursor;
        break;
    case 'u':
        m_terminalCursor = qBound<qsizetype>(0, m_terminalSavedCursor, m_terminalBuffer.size());
        break;
    case 'm':
        applyTerminalSgr(params);
        break;
    case 'h':
    case 'l':
        break;
    default:
        break;
    }
}

void SerialPortController::applyTerminalSgr(const QVector<int> &params)
{
    if (params.isEmpty()) {
        m_terminalCurrentStyle = kTerminalDefaultStyle;
        return;
    }

    const auto setIndexedColor = [this](int color, bool background) {
        const int bounded = qBound(0, color, 255);
        m_terminalCurrentStyle = background ? withTerminalBg(m_terminalCurrentStyle, bounded)
                                            : withTerminalFg(m_terminalCurrentStyle, bounded);
    };
    const auto rgbToXterm = [](int red, int green, int blue) {
        const auto toCube = [](int value) {
            value = qBound(0, value, 255);
            if (value < 48) {
                return 0;
            }
            if (value < 115) {
                return 1;
            }
            return qBound(0, (value - 35) / 40, 5);
        };
        return 16 + 36 * toCube(red) + 6 * toCube(green) + toCube(blue);
    };

    for (int index = 0; index < params.size(); ++index) {
        const int param = params.at(index);
        if (param == 0) {
            m_terminalCurrentStyle = kTerminalDefaultStyle;
        } else if (param == 1) {
            m_terminalCurrentStyle = withTerminalBold(m_terminalCurrentStyle, true);
        } else if (param == 22) {
            m_terminalCurrentStyle = withTerminalBold(m_terminalCurrentStyle, false);
        } else if (param == 7) {
            m_terminalCurrentStyle = withTerminalInverse(m_terminalCurrentStyle, true);
        } else if (param == 27) {
            m_terminalCurrentStyle = withTerminalInverse(m_terminalCurrentStyle, false);
        } else if (param >= 30 && param <= 37) {
            m_terminalCurrentStyle = withTerminalFg(m_terminalCurrentStyle, param - 30);
        } else if (param == 39) {
            m_terminalCurrentStyle = withTerminalFg(m_terminalCurrentStyle, kTerminalDefaultColor);
        } else if (param >= 40 && param <= 47) {
            m_terminalCurrentStyle = withTerminalBg(m_terminalCurrentStyle, param - 40);
        } else if (param == 49) {
            m_terminalCurrentStyle = withTerminalBg(m_terminalCurrentStyle, kTerminalDefaultColor);
        } else if (param >= 90 && param <= 97) {
            m_terminalCurrentStyle = withTerminalFg(m_terminalCurrentStyle, 8 + param - 90);
        } else if (param >= 100 && param <= 107) {
            m_terminalCurrentStyle = withTerminalBg(m_terminalCurrentStyle, 8 + param - 100);
        } else if ((param == 38 || param == 48) && index + 2 < params.size() && params.at(index + 1) == 5) {
            setIndexedColor(params.at(index + 2), param == 48);
            index += 2;
        } else if ((param == 38 || param == 48) && index + 4 < params.size() && params.at(index + 1) == 2) {
            setIndexedColor(rgbToXterm(params.at(index + 2), params.at(index + 3), params.at(index + 4)), param == 48);
            index += 4;
        }
    }
}

void SerialPortController::insertTerminalText(qsizetype position, const QString &text, quint32 style)
{
    if (text.isEmpty()) {
        return;
    }

    position = qBound<qsizetype>(0, position, m_terminalBuffer.size());
    m_terminalBuffer.insert(position, text);
    if (m_terminalStyles.size() < m_terminalBuffer.size() - text.size()) {
        m_terminalStyles.resize(m_terminalBuffer.size() - text.size());
    }
    for (qsizetype index = 0; index < text.size(); ++index) {
        m_terminalStyles.insert(position + index, style);
    }
}

void SerialPortController::removeTerminalRange(qsizetype position, qsizetype count)
{
    position = qBound<qsizetype>(0, position, m_terminalBuffer.size());
    count = qBound<qsizetype>(0, count, m_terminalBuffer.size() - position);
    if (count == 0) {
        return;
    }

    m_terminalBuffer.remove(position, count);
    if (m_terminalStyles.size() > position) {
        m_terminalStyles.remove(position, qMin(count, m_terminalStyles.size() - position));
    }
}

void SerialPortController::truncateTerminalBuffer(qsizetype size)
{
    size = qBound<qsizetype>(0, size, m_terminalBuffer.size());
    m_terminalBuffer.truncate(size);
    if (m_terminalStyles.size() > size) {
        m_terminalStyles.resize(size);
    }
}

qsizetype SerialPortController::terminalLineStart(qsizetype cursor) const
{
    cursor = qBound<qsizetype>(0, cursor, m_terminalBuffer.size());
    if (cursor == 0) {
        return 0;
    }

    const qsizetype start = m_terminalBuffer.lastIndexOf(QChar('\n'), cursor - 1);
    return start < 0 ? 0 : start + 1;
}

qsizetype SerialPortController::terminalLineEnd(qsizetype cursor) const
{
    cursor = qBound<qsizetype>(0, cursor, m_terminalBuffer.size());
    const qsizetype end = m_terminalBuffer.indexOf(QChar('\n'), cursor);
    return end < 0 ? m_terminalBuffer.size() : end;
}

qsizetype SerialPortController::terminalCursorForRowColumn(int row, int column)
{
    row = qMax(1, row);
    column = qMax(1, column);

    qsizetype cursor = 0;
    for (int currentRow = 1; currentRow < row; ++currentRow) {
        const qsizetype nextLine = m_terminalBuffer.indexOf(QChar('\n'), cursor);
        if (nextLine < 0) {
            insertTerminalText(m_terminalBuffer.size(), QString(QChar('\n')), m_terminalCurrentStyle);
            cursor = m_terminalBuffer.size();
        } else {
            cursor = nextLine + 1;
        }
    }

    const qsizetype target = cursor + column - 1;
    while (terminalLineEnd(cursor) < target) {
        insertTerminalText(terminalLineEnd(cursor), QString(QChar(' ')), m_terminalCurrentStyle);
    }
    return qBound<qsizetype>(cursor, target, terminalLineEnd(cursor));
}

void SerialPortController::moveTerminalCursorVertical(int delta)
{
    const qsizetype currentStart = terminalLineStart(m_terminalCursor);
    const qsizetype column = m_terminalCursor - currentStart;
    qsizetype targetStart = currentStart;

    if (delta < 0) {
        for (int i = 0; i < -delta; ++i) {
            if (targetStart == 0) {
                break;
            }
            targetStart = terminalLineStart(targetStart - 1);
        }
    } else {
        for (int i = 0; i < delta; ++i) {
            const qsizetype lineEnd = terminalLineEnd(targetStart);
            if (lineEnd == m_terminalBuffer.size()) {
                break;
            }
            targetStart = lineEnd + 1;
        }
    }

    m_terminalCursor = qMin(targetStart + column, terminalLineEnd(targetStart));
}

void SerialPortController::trimTerminalBuffer()
{
    if (m_terminalBuffer.size() <= kTerminalBufferMaxChars) {
        return;
    }

    const qsizetype removeCount = m_terminalBuffer.size() - kTerminalBufferMaxChars;
    removeTerminalRange(0, removeCount);
    m_terminalCursor = qMax<qsizetype>(0, m_terminalCursor - removeCount);
    m_terminalSavedCursor = qMax<qsizetype>(0, m_terminalSavedCursor - removeCount);
}
