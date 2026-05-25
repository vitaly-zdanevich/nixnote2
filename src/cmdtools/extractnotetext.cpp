/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2015 Randy Baumgarte

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
***********************************************************************************/


#include "src/global.h"
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QStringList>
#include "extractnotetext.h"

extern Global global;

namespace {

bool isBlockElement(const QString &name) {
    static const QStringList blockElements = {
        QStringLiteral("address"),
        QStringLiteral("article"),
        QStringLiteral("aside"),
        QStringLiteral("blockquote"),
        QStringLiteral("dd"),
        QStringLiteral("div"),
        QStringLiteral("dl"),
        QStringLiteral("dt"),
        QStringLiteral("en-note"),
        QStringLiteral("figcaption"),
        QStringLiteral("figure"),
        QStringLiteral("footer"),
        QStringLiteral("form"),
        QStringLiteral("h1"),
        QStringLiteral("h2"),
        QStringLiteral("h3"),
        QStringLiteral("h4"),
        QStringLiteral("h5"),
        QStringLiteral("h6"),
        QStringLiteral("header"),
        QStringLiteral("hr"),
        QStringLiteral("li"),
        QStringLiteral("main"),
        QStringLiteral("nav"),
        QStringLiteral("ol"),
        QStringLiteral("p"),
        QStringLiteral("pre"),
        QStringLiteral("section"),
        QStringLiteral("table"),
        QStringLiteral("tbody"),
        QStringLiteral("td"),
        QStringLiteral("tfoot"),
        QStringLiteral("th"),
        QStringLiteral("thead"),
        QStringLiteral("tr"),
        QStringLiteral("ul")
    };

    return blockElements.contains(name);
}

void trimTrailingHorizontalWhitespace(QString &text) {
    while (!text.isEmpty()) {
        const QChar ch = text.at(text.size() - 1);
        if (ch != QLatin1Char(' ') && ch != QLatin1Char('\t')) {
            break;
        }
        text.chop(1);
    }
}

void appendForcedLineBreak(QString &text) {
    trimTrailingHorizontalWhitespace(text);
    text.append(QLatin1Char('\n'));
}

void appendBlockLineBreak(QString &text) {
    trimTrailingHorizontalWhitespace(text);
    if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n'))) {
        text.append(QLatin1Char('\n'));
    }
}

void trimTrailingLineBreaks(QString &text) {
    trimTrailingHorizontalWhitespace(text);
    while (!text.isEmpty()) {
        const QChar ch = text.at(text.size() - 1);
        if (ch != QLatin1Char('\n') && ch != QLatin1Char('\r')) {
            break;
        }
        text.chop(1);
        trimTrailingHorizontalWhitespace(text);
    }
}

QString normalizePlainText(QString text) {
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    text.replace(QChar(0x00a0), QLatin1Char(' '));
    trimTrailingLineBreaks(text);
    return text;
}

QString stripTagsFallback(QString content) {
    content.replace(QRegularExpression(QStringLiteral("<\\s*br\\s*/?\\s*>"),
                                       QRegularExpression::CaseInsensitiveOption),
                    QStringLiteral("\n"));
    content.replace(QRegularExpression(
                        QStringLiteral("</\\s*(address|article|aside|blockquote|dd|div|dl|dt|en-note|figcaption|figure|footer|form|h[1-6]|header|hr|li|main|nav|ol|p|pre|section|table|tbody|td|tfoot|th|thead|tr|ul)\\s*>"),
                        QRegularExpression::CaseInsensitiveOption),
                    QStringLiteral("\n"));
    content.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
    return normalizePlainText(content);
}

} // namespace

ExtractNoteText::ExtractNoteText(QObject *parent) :
    QObject(parent)
{
}



QString ExtractNoteText::wrap() {

    QString returnValue;
    QXmlStreamWriter *writer = new QXmlStreamWriter(&returnValue);
    writer->setAutoFormatting(true);
    writer->writeStartDocument();
    writer->writeDTD("<!DOCTYPE NixNote-NoteTextExtract>");
    writer->writeStartElement("nixnote-text-extract");
    writer->writeAttribute("version", "2");
    writer->writeAttribute("application", APP_NNEX_APP_NAME);
    writer->writeAttribute("applicationVersion", "2.x");
    writer->writeStartElement("NoteExtract");
    writer->writeTextElement("ReturnUuid", this->returnUuid);
    if (this->text.trimmed() != "")
        writer->writeTextElement("ReturnText", this->text);
    writer->writeTextElement("Lid", QString::number(this->lid));
    writer->writeEndElement();
    writer->writeEndElement();
    writer->writeEndDocument();
    return returnValue;
}

void ExtractNoteText::unwrap(QString data) {
    lastError = 0;
    QXmlStreamReader reader(data);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.hasError()) {
            errorMessage = reader.errorString();
            QLOG_ERROR() << "************************* ERROR READING IMPORT " << errorMessage;
            lastError = 16;
            return;
        }
        if (reader.name().toString().toLower() == "nixnote-text-extract" && !reader.isEndElement()) {
            reader.readNext();
            while(reader.name().toString().toLower() != "nixnote-text-extract") {
                if (reader.name().toString().toLower() == "returnuuid" && reader.isStartElement()) {
                    reader.readNext();
                    returnUuid = reader.text().toString();
                } else if (reader.name().toString().toLower() == "lid" && reader.isStartElement()) {
                    reader.readNext();
                    lid = reader.text().toString().toInt();
                } else if (reader.name().toString().toLower() == "returntext" && reader.isStartElement()) {
                    reader.readNext();
                    text = reader.text().toString();
                } else
                    reader.readNext();
            }
        }
    }
}


QString ExtractNoteText::stripTags(QString content) {
    QString text;
    QXmlStreamReader reader(content);
    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            const QString name = reader.name().toString().toLower();
            if (name == QStringLiteral("br")) {
                appendForcedLineBreak(text);
            } else if (name == QStringLiteral("hr")) {
                appendBlockLineBreak(text);
            }
        } else if (reader.isEndElement()) {
            const QString name = reader.name().toString().toLower();
            if (isBlockElement(name)) {
                appendBlockLineBreak(text);
            }
        } else if (reader.isCharacters()) {
            text.append(reader.text().toString());
        }
    }

    if (reader.hasError()) {
        QLOG_WARN() << "Unable to parse note ENML while extracting text; using fallback tag stripper:"
                    << reader.errorString();
        return stripTagsFallback(content);
    }

    return normalizePlainText(text);
}
