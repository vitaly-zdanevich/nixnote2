/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2013 Randy Baumgarte

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

#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <poppler-qt6.h>
#include <QIcon>
#include <QList>
#include <iostream>
#include <QPainter>
#include <QUrl>
#include "noteformatter.h"
#include "src/sql/resourcetable.h"
#include "src/sql/notebooktable.h"
#include "src/sql/sharednotebooktable.h"
#include "src/sql/linkednotebooktable.h"
#include "src/global.h"
#include "src/filters/filtercriteria.h"
#include "src/filters/filterengine.h"
#include "src/utilities/mimereference.h"
#include "enmlformatter.h"
#include "src/utilities/NixnoteStringUtils.h"

using namespace std;

extern Global global;

namespace {
QString localFileUrl(const QString &path)
{
    return QUrl::fromLocalFile(path).toString();
}

QString localImageUrl(const QString &path, const QString &mimeType)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QLOG_WARN() << "Unable to embed editor image:" << path << file.errorString();
        return localFileUrl(path);
    }

    return QStringLiteral("data:%1;base64,%2")
            .arg(mimeType, QString::fromLatin1(file.readAll().toBase64()));
}

QString resourceImageUrl(const QString &path)
{
    QUrl url;
    url.setScheme(QStringLiteral("nnres"));
    url.setPath(QStringLiteral("/") + QFileInfo(path).fileName());
    return url.toString();
}

QString resourceFilePath(qint32 lid, const QString &extension)
{
    const QString fileName = QString::number(lid) + extension;
    const QString expectedPath = global.fileManager.getDbaDirPath() + fileName;
    if (QFileInfo::exists(expectedPath))
        return expectedPath;

    QDir dbaDir(global.fileManager.getDbaDirPath());
    const QStringList matches = dbaDir.entryList({QString::number(lid) + QStringLiteral(".*")}, QDir::Files);
    if (!matches.isEmpty())
        return dbaDir.filePath(matches.first());

    return expectedPath;
}
}

/* Constructor. */
NoteFormatter::NoteFormatter(QObject *parent) : NoteFormatterBase(parent) {
    thumbnail = false;
    this->setNoteHistory(false);
    this->noteHistory = false;
    this->pdfPreview = true;
    this->readOnly = false;
    this->formatError = false;
    this->inkNote = false;
    this->resourceError = false;
    this->resourceHighlight = false;
}


/*
Set the note we are currently formatting.  The pdfPreview
is an indication if we should generate a picture for the
attachment rather than doing it as an attachment.
*/
void NoteFormatter::setNote(Note n, bool pdfPreview) {
    this->pdfPreview = pdfPreview;
    this->note = n;
    content = "";
    //this->enableHighlight = true;
    readOnly = false;
    inkNote = false;
    NoteAttributes attributes;

    if (note.attributes.isSet()) {
        attributes = note.attributes;
        QString contentClass;
        if (attributes.contentClass.isSet())
            contentClass = attributes.contentClass;
        if (contentClass != "") {
            readOnly = true;
            QLOG_WARN() << "Content class not empty. Setting read-only. Note guid=" << note.guid;
        }
    }
}


/* Return the formatted content */
QString NoteFormatter::getPage() {
    return this->content;
}




/* If we have search criteria, then we highlight the text matching
  those results in the note. */
//void NoteFormatter::setHighlight() {
//    FilterCriteria *criteria = global.getCurrentCriteria();
//    if (criteria->isSearchStringSet())
//        enableHighlight = true;
//    else
//        enableHighlight = false;
//}

/* If we are here because we are viewing note history, then we
  set the flag here.  Note history is almost the same as a regular
  note, but there are some differences. */
void NoteFormatter::setNoteHistory(bool value) {
    this->noteHistory = value;
}


QString NoteFormatter::enmlToNoteHTML(QString enml) {
    HtmlDomDocument page;
    QLOG_TRACE() << "Before preHTMLFormat";
    QString html = "<body></body>";
    if (note.content.isSet()) {
        html = preHtmlFormat(enml);
    }

    html.replace("<en-note", "<body");
    html.replace("</en-note>", "</body>");

    QByteArray htmlPage;
    htmlPage.append(html.toUtf8());
    QLOG_TRACE() << "About to set content";
    page.setContent(htmlPage);

    QLOG_TRACE() << "Starting to modify tags";
    modifyTags(page);
    QLOG_TRACE() << "Done modifying tags";
    html = page.toHtml();

    return html;
}


/* Take the ENML note and transform it into editor HTML. */
QByteArray NoteFormatter::rebuildNoteHTML() {
    bool haveGuid = note.guid.isSet();
    QString guid = haveGuid ? note.guid.ref() : "unknown";

    // create log file prefix for log file attachments
    QString logfilePrefix("html-");
    logfilePrefix.append(guid).append("-");
    QLOG_DEBUG() << "htmlfmt: rebuilding note HTML, note guid=" << guid;

    formatError = false;
    readOnly = false;

    ResourceTable resTable(global.db);
    if (!haveGuid) {
        formatError = true;
        readOnly = true;
        QLOG_WARN() << "htmlfmt: NOTE GUID IS NOT SET!!! => readOnly";
    } else {
        QLOG_DEBUG() << "htmlfmt: getting resource from note guid=" << guid;
        resTable.getResourceMap(hashMap, resourceMap, guid);
    }
    // original note ENML as we got it from database (should be basically 1:1 what we got from Evernote)
    QString origENML(note.content);
    QLOG_DEBUG_FILE(logfilePrefix + "original-enml.xml", origENML);


    QString html = enmlToNoteHTML(origENML);
    note.content = html;

    QLOG_DEBUG_FILE(logfilePrefix + "modify.html", html);

    content.clear();
    content.append(html.toUtf8());

    qint32 index = content.indexOf("<body");
    content.remove(0, index);
    content.prepend(DEFAULT_HTML_HEAD);

    // OLD:content.prepend("<html>");
    // NEW: EXPERIMENTAL !!!!
    content.prepend(DEFAULT_HTML_TYPE);

    content.append("</html>");

    if (!formatError && !readOnly) {
        NotebookTable ntable(global.db);
        if (note.notebookGuid.isSet()) {
            qint32 notebookLid = ntable.getLid(note.notebookGuid);
            if (ntable.isReadOnly(notebookLid)) {
                readOnly = true;
                QLOG_WARN() << "Notebook is read-only.  Marking note read-only. Note guid=" << guid;
            }
        }
    }
    if (note.active.isSet() && !note.active) {
        readOnly = true;
        QLOG_WARN() << "Note is inactive.  Setting to read-only. Note guid=" << guid;
    }
    QLOG_TRACE() << "Done rebuilding HTML";
    QLOG_DEBUG_FILE(logfilePrefix + "final.html", content);

    return content;
}


// This is to turn the <en-media/> tags into <en-media></en-media> tags because
// The old DOM path tended to miss the /> tag, which can cause text to be missed.
QString NoteFormatter::preHtmlFormat(QString note) {
    QLOG_TRACE_IN();
    int pos;

    // Correct <br></br> because Webkit messes it up.
    //QString content = note.replace("<br></br>", "<br/>");

    QString content = note;

    pos = content.indexOf("<en-media");
    while (pos != -1) {
        int endPos = content.indexOf(">", pos);
        int tagEndPos = content.indexOf("/>", pos);
        if (endPos == -1) {
            break;
        }

        // Convert self-closing media tags into explicit element pairs
        // before QDom parses them.
        if (tagEndPos != -1 && tagEndPos < endPos) {
            const QString replacement = QStringLiteral("></en-media>");
            content = content.mid(0, tagEndPos) +
                    replacement +
                    content.mid(tagEndPos + 2);
            pos = content.indexOf("<en-media", tagEndPos + replacement.size());
        } else {
            pos = content.indexOf("<en-media", pos + 1);
        }
    }
    QLOG_TRACE_OUT();
    return content;
}


/*
  This will go through and modify some of the ENML tags and turn
  them into HTML tags.  Things like en-media & en-crypt have no
  HTML values, so we turn them into HTML.
  */
void NoteFormatter::modifyTags(HtmlDomDocument &doc) {
    QLOG_TRACE_IN();
    tempFiles.clear();

    // Modify en-media tags
    QLOG_TRACE() << "Searching for all en-media tags;";
    QList<HtmlDomElement> anchors = doc.findAllElements("en-media");
    QLOG_TRACE() << "Search complete: " << anchors.size();
    for (HtmlDomElement enmedia : anchors) {
        if (enmedia.hasAttribute("type")) {
            QString attr = enmedia.attribute("type");
            QString hash = enmedia.attribute("hash");
            QStringList type = attr.split("/");
            if (type.size() >= 2) {
                QString appl = type[1];
                QLOG_TRACE() << "En-Media tag type: " << type[0];
                if (type[0] == "image")
                    modifyImageTags(enmedia, hash);
                else
                    modifyApplicationTags(enmedia, hash, appl);
                QLOG_TRACE() << "Type modified";
            }
        }
    }

    // Modify todo tags
    anchors = doc.findAllElements("en-todo");
    qint32 enTodoCount = anchors.count();
    for (qint32 i = enTodoCount - 1; i >= 0; i--) {
        HtmlDomElement enmedia = anchors.at(i);
        modifyTodoTags(enmedia);
    }

    anchors = doc.findAllElements("en-crypt");
    qint32 enCryptLen = anchors.count();
    for (qint32 i = enCryptLen - 1; i >= 0; i--) {
        HtmlDomElement enmedia = anchors.at(i);
        QString hint = enmedia.attribute("hint");
        QString cipher = enmedia.attribute("cipher", "RC2");
        QString length = enmedia.attribute("length", "64");

        enmedia.setAttribute("contentEditable", "false");
        enmedia.setAttribute("src", localImageUrl(global.fileManager.getImageDirPath("encrypt.png"),
                                                  QStringLiteral("image/png")));
        enmedia.setAttribute("en-tag", "en-crypt");
        enmedia.setAttribute("cipher", cipher);
        enmedia.setAttribute("length", length);
        enmedia.setAttribute("hint", hint);
        enmedia.setAttribute("alt", enmedia.toInnerXml());
        global.cryptCounter++;
        enmedia.setAttribute("id", "crypt" + QString().number(global.cryptCounter));
        QString encryptedText = enmedia.toInnerXml();

        // If the encryption string contains crlf at the end, remove them because they mess up the javascript.
        if (encryptedText.endsWith("\n"))
            encryptedText.truncate(encryptedText.length() - 1);
        if (encryptedText.endsWith("\r"))
            encryptedText.truncate(encryptedText.length() - 1);

        // Add the commands
        hint = hint.replace("'", "&apos;");
        enmedia.setAttribute("onClick", "window.browserWindow.decryptText('crypt" +
                                        QString().number(global.cryptCounter) +
                                        "', '" + encryptedText + "', '" +
                                        hint + "', '" +
                                        cipher + "', " +
                                        length +
                                        ");");
        enmedia.setAttribute("onMouseOver", "style.cursor='hand'");
        enmedia.setInnerXml("");
        QString k = enmedia.toOuterXml();
        k.replace("<en-crypt", "<img");
        k.replace("img>", "<en-crypt");
        enmedia.setOuterXml(k);
    }


    // Modify link tags
    anchors = doc.findAllElements("a");
    enCryptLen = anchors.count();
    for (qint32 i = 0; i < anchors.count(); i++) {
        HtmlDomElement element = anchors.at(i);
        QString href = element.attribute("href");
        QLOG_DEBUG() << "link #" << i << " href=" << href;

        if (!NixnoteStringUtils::isLatexFormulaResourceUrl(href)) {
            element.setAttribute("title", href);
        } else {
            QString encodedFormula = NixnoteStringUtils::extractLatexFormulaFromResourceUrl(href, true);
            element.setAttribute("title", encodedFormula);

            QLOG_DEBUG() << "link #" << i << " encodedFormula=" << encodedFormula;

            QString resLid = element.firstChild().attribute("lid", "");
            element.setAttribute("href", "latex:///" + resLid);
        }
    }
    QLOG_TRACE_OUT();
}


/* This function works the same as the addHighlight, but instead of highlighting
  text in a note, it highlights the text in an image. */
QString NoteFormatter::addImageHighlight(qint32 resLid, QString imgfile) {
    QLOG_TRACE_IN();
    if (highlightWords.size() == 0)
        return "";

    // Get the image resource recognition data.  This tells where to highlight the image
    ResourceTable resTable(global.db);
    Resource recoResource;
    resTable.getResourceRecognition(recoResource, resLid);
    Data recognition;
    if (recoResource.recognition.isSet())
        recognition = recoResource.recognition;
    if (!recognition.size.isSet() || !recognition.body.isSet() ||
        recognition.size == 0) {
        return "";
    }

    QString filename = global.fileManager.getTmpDirPath() + QString::number(resLid) + ".png";
    // Now we have the recognition data.  We need to go through it
    QByteArray recoData;
    if (recognition.body.isSet())
        recoData = recognition.body;
    QString xml(recoData);

    // Create a transparent pixmap.  The only non transparent piece is the
    // highlight that will be overlaid on the old image
    if (imgfile.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))
        imgfile = QUrl(imgfile).toLocalFile();
    if (imgfile.isEmpty())
        return "";
    QPixmap originalFile(imgfile, findImageFormat(imgfile));
    QPixmap overlayPix(originalFile.size());
    overlayPix.fill(Qt::transparent);
    QPainter p2(&overlayPix);
    p2.save();
    p2.setBackgroundMode(Qt::TransparentMode);
    p2.setRenderHint(QPainter::Antialiasing, true);
    QColor yellow(Qt::yellow);
    p2.setBrush(yellow);

    // Now, we have the image.  We need to go through all the recognition data to highlight
    // what we've found.
    QDomDocument doc;
    doc.setContent(xml);

    // Go through the "item" nodes
    bool found = false;
    QDomNodeList anchors = doc.elementsByTagName("item");
    for (int i = 0; i < anchors.length(); i++) {
        QDomElement element = anchors.at(i).toElement();
        int x = element.attribute("x").toInt();
        int y = element.attribute("y").toInt();
        int w = element.attribute("w").toInt();
        int h = element.attribute("h").toInt();

        // Get all children ("t" nodes)
        QDomNodeList children = element.childNodes();
        for (int j = 0; j < children.length(); j++) {
            QDomElement child = children.at(j).toElement();
            if (child.nodeName().toLower() == "t") {
                QString text = child.text();
                int weight = child.attribute("w").toInt(); // Image weight
                if (weight >= global.getMinimumRecognitionWeight()) {

                    // Check to see if this word matches something we're looking for
                    for (int k = 0; k < highlightWords.size(); k++) {
                        QString searchWord = highlightWords[k].toLower();
                        if (searchWord.endsWith("*"))
                            searchWord.chop(1);
                        if (text.toLower().contains(searchWord)) {
                            found = true;
                            p2.drawRect(x, y, w, h);
                        }
                    }
                }
            }
        }
    }

    // If nothing was found, we exit
    if (!found)
        return "";


    // Paint the highlight onto the background & save over the original
    p2.setOpacity(0.4);
    p2.drawPixmap(0, 0, overlayPix);
    p2.restore();
    //p2.end();

    // Create the actual overlay.  We do this in two steps to avoid
    // constantly painting the same area
    QPixmap finalPix(originalFile.size());
    finalPix.fill(Qt::transparent);
    QPainter p3(&finalPix);
    p3.save();
    p3.setBackgroundMode(Qt::TransparentMode);
    p3.setRenderHint(QPainter::Antialiasing, true);
    p3.drawPixmap(0, 0, originalFile);
    p3.setOpacity(0.4);
    p3.drawPixmap(0, 0, overlayPix);
    p3.restore();
    finalPix.save(filename);

    QLOG_TRACE_OUT();
    return localImageUrl(filename, QStringLiteral("image/png"));
//    return "this.src='file://"+filename+"';";
}


const char *NoteFormatter::findImageFormat(QString file) {
    QByteArray b;
    QFile f(file);
    if (!f.open(QFile::ReadOnly))
        return nullptr;
    b = f.read(10);
    f.close();

    // Try to determine the type of image from the "magic bytes"
    if (b.startsWith("\xFF\xD8\xFF"))
        return "JPG";
    if (b.startsWith("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"))
        return "PNG";
    if (b.startsWith("GIF87a"))
        return "GIF";
    if (b.startsWith("GIF89a"))
        return "GIF";
    return nullptr;
}


/* Modify an image tag.  Basically we turn it back into a picture, write out the file, and
  modify the ENML */
void NoteFormatter::modifyImageTags(HtmlDomElement &enMedia, QString &hash) {
    QLOG_TRACE_IN();
    QString mimetype = enMedia.attribute("type");
    qint32 resLid = 0;
    resLid = hashMap[hash];
    QString highlightString = "";

    if (resLid > 0) {
        Resource r = resourceMap[resLid];
        QLOG_TRACE() << "resource retrieved";

        MimeReference ref;
        QString filename;
        ResourceAttributes attributes;
        if (r.attributes.isSet()) {
            attributes = r.attributes;
        }
        if (attributes.fileName.isSet()) {
            filename = attributes.fileName;
        }
        if (r.mime.isSet() && (mimetype != r.mime)) {
            QLOG_WARN() << "htmlfmt: image tag - overriding mime type from ENML style attribute=" << mimetype
              << " by mimetype from resource=" << r.mime;

            // override the mime type from "type" attribute in enml by mime info from resource
            mimetype = r.mime;
        }



        QString type = ref.getExtensionFromMime(mimetype, filename);
        QLOG_DEBUG() << "htmlfmt: image tag - getting resource hash=" << hash << ", lid=" << resLid << ", type="
                     << type << ", mimetype=" << mimetype;

        Data data;
        if (r.data.isSet())
            data = r.data;

        if (data.size.isSet() && data.size > 0) {
            const QString imagePath = resourceFilePath(resLid, type);
            QLOG_DEBUG() << "htmlfmt: image tag - imgfile=" << imagePath;

            enMedia.setAttribute("src", resourceImageUrl(imagePath));

            // Check if this is a LaTeX image
            ResourceAttributes attributes;
            if (r.attributes.isSet()) {
                attributes = r.attributes;
            }

            QString sourceUrl = "";
            if (attributes.sourceURL.isSet()) {
                sourceUrl = attributes.sourceURL;
            }

            if (NixnoteStringUtils::isLatexFormulaResourceUrl(sourceUrl)) {
                enMedia.appendInside("<img/>");
                HtmlDomElement newText = enMedia.lastChild();
                enMedia.setAttribute("en-tag", "en-latex");
                newText.setAttribute("onMouseOver", "style.cursor='pointer'");

                QString encodedFormula = NixnoteStringUtils::extractLatexFormulaFromResourceUrl(sourceUrl, true);
                newText.setAttribute("title", encodedFormula);
                QLOG_DEBUG() << "setting title=" << encodedFormula;

                newText.setAttribute("href", "latex:///" + QString::number(resLid));
            }
            enMedia.setAttribute("onContextMenu", "window.browserWindow.imageContextMenu('"
                                                  + QString::number(resLid) + "', '"
                                                  + QString::number(resLid) + type + "');");

            if (!global.disableImageHighlight()) {
                highlightString = addImageHighlight(resLid, imagePath);

                if (highlightString != "")
                    enMedia.setAttribute("src", highlightString);

                //if (highlightString != "")
                //    enMedia.setAttribute("onload", highlightString);
            }
        }
    } else {
        resourceError = true;
        readOnly = true;
        QLOG_WARN() << "Image tag - resource error. Setting note to read-only . Note guid=" << note.guid
                    << ". Resource hash " << hash << ")";
    }

    // Reset the tags to something the editor can render.
    enMedia.setAttribute("en-tag", "en-media");
    enMedia.setPlainText("");
    enMedia.setAttribute("lid", QString::number(resLid));

    // rename the <enmedia> tag to <img>
    QString mediaXml = enMedia.toOuterXml();
    mediaXml.replace("<en-media", "<img");
    mediaXml.replace("</en-media>", "</img>");
    enMedia.setOuterXml(mediaXml);
    QLOG_TRACE_OUT();
}


// Modify the en-media tag into an attachment
void NoteFormatter::modifyApplicationTags(HtmlDomElement &enmedia, QString &hash, QString appl) {
    QLOG_TRACE_IN();
    if (appl.toLower() == "vnd.evernote.ink") {
        readOnly = true;
        QLOG_INFO() << "Note is ink-note.  Setting to read-only. Note guid=" << note.guid;
        inkNote = true;
        buildInkNote(enmedia, hash);
        return;
    }

    ResourceTable resTable(global.db);
    QString contextFileName;
    QLOG_DEBUG() << "htmlfmt: fetching for note: " << note.guid << " hash: " << hash;
    qint32 resLid = resTable.getLidByHashHex(note.guid, hash);
    Resource r;
    resTable.get(r, resLid, false);
    if (!r.data.isSet()) {
        resourceError = true;
    } else {
        // If we are running the formatter and we are not generating a thumbnail
        QString mimetype = "";
        if (r.mime.isSet()) {
            mimetype = r.mime;
        }

        // Check that we don't have a locked PDF.  If we do, then disable PDF previews.
        if (mimetype == "application/pdf") {
            QString file = global.fileManager.getDbaDirPath() + QString::number(resLid) + ".pdf";
            if (!QFile::exists(file)) {
                pdfPreview = false;
            } else if (auto doc = Poppler::Document::load(file); doc != nullptr && doc->isLocked()) {
                pdfPreview = false;
            }
        }

        if (mimetype == "application/pdf" && pdfPreview && !thumbnail) {
            modifyPdfTags(resLid, enmedia);
            return;
        }


        // If we are running the formatter so we can generate a thumbnail and it is a PDF
        if (mimetype == "application/pdf" && pdfPreview && thumbnail) {
            QString printImageFile =
                    global.fileManager.getTmpDirPath() + QString::number(resLid) + QString("-print.jpg");
            QString file = global.fileManager.getDbaDirPath() + QString::number(resLid) + ".pdf";
            if (!QFile::exists(file))
                return;

            auto doc = Poppler::Document::load(file);
            if (doc == nullptr)
                return;

            QImage image = doc->page(0)->renderToImage();
            image.save(printImageFile, "jpg");

            enmedia.setAttribute("src", localImageUrl(printImageFile, QStringLiteral("image/jpeg")));
            enmedia.removeAttribute("hash");
            enmedia.removeAttribute("type");
            QString mediaXml = enmedia.toOuterXml();
            mediaXml.replace("<en-media", "<img");
            mediaXml.replace("</en-media>", "</img>");
            enmedia.setOuterXml(mediaXml);
            return;
        }

        QString fileDetails = "";
        MimeReference ref;
        ResourceAttributes attributes;
        if (r.attributes.isSet()) {
            attributes = r.attributes;
        }

        if (attributes.fileName.isSet()) {
            fileDetails = ref.getExtensionFromMime(r.mime, fileDetails);
        }

        enmedia.setAttribute("href", QString("nnres:") + global.fileManager.getDbaDirPath() + QString::number(resLid)
                                     + fileDetails);
        contextFileName =
                global.fileManager.getTmpDirPath("") + QString::number(resLid) + global.attachmentNameDelimeter +
                fileDetails;

        // Setup the context menu.  This is useful if we want to do a "save as" or such
        contextFileName = contextFileName.replace("\\", "/");
        enmedia.setAttribute("onContextMenu", "window.browserWindow.resourceContextMenu('" + contextFileName + "');");
        enmedia.setAttribute("en-tag", "en-media");
        enmedia.setAttribute("lid", QString::number(resLid));

        enmedia.appendInside("<img/>");
        HtmlDomElement newText = enmedia.lastChild();

        // Build an icon of the image
        QString fileExt;
        if (attributes.fileName.isSet())
            fileExt = attributes.fileName;
        else
            fileExt = appl;

        QString fn;
        QString mime;
        if (attributes.fileName.isSet()) {
            fn = attributes.fileName;
        }
        if (r.mime.isSet()) {
            mime = r.mime;
        }
        fileExt = ref.getExtensionFromMime(mime, fn);

        QString icon = findIcon(resLid, r, fileExt);
        newText.setAttribute("src", localImageUrl(icon, QStringLiteral("image/png")));
        if (attributes.fileName.isSet()) {
            newText.setAttribute("title", attributes.fileName);
        }
        newText.setAttribute("en-tag", "temporary");
        //Rename the tag to a <a> link
        QString mediaXml = enmedia.toOuterXml();
        mediaXml.replace("<en-media", "<a");
        mediaXml.replace("</en-media>", "</a>");
        enmedia.setOuterXml(mediaXml);
    }
    QLOG_TRACE_OUT();
}


// Build an icon for any attachments
QString NoteFormatter::findIcon(qint32 lid, Resource r, QString fileExt) {
    QLOG_TRACE_IN();

    FilterCriteria *criteria = global.getCurrentCriteria();
    // First get the icon for this type of file
    resourceHighlight = false;
    if (criteria->isSearchStringSet() && criteria->getSearchString() != "") {
        FilterEngine engine;
        resourceHighlight = engine.resourceContains(lid, criteria->getSearchString(), nullptr);
    }

    QString fileName = global.fileManager.getDbaDirPath() + QString::number(lid) + fileExt;
    QIcon icon;
    QFileInfo info(fileName);
    QFileIconProvider provider;
    icon = provider.icon(info);

    // Build a string name for the display
    QString displayName;
    ResourceAttributes attributes;
    if (r.attributes.isSet())
        attributes = r.attributes;
    if (attributes.fileName.isSet())
        displayName = attributes.fileName;
    else
        displayName = fileExt.toUpper() + " " + QString(tr("File"));

    // Setup the painter
    QPainter p;

    // Setup the font
    QFont font; // =p.font() ;
    global.getGuiFont(font);
    QPen fontPen;
    fontPen.setColor(QColor(global.getEditorFontColor()));
//    font.setFamily("Arial");
    QFontMetrics fm(font);
    int width = fm.horizontalAdvance(displayName);
    if (width < 40)  // steup a minimum width
        width = 40;
    width = width + 50;  // Add 10 px for padding & 40 for the icon

    // Start drawing a new pixmap for  the image in the note
    QPoint textPoint(40, 15);
    QPoint sizePoint(40, 29);
    QPixmap pixmap(width, 37);
    if (resourceHighlight) {
        pixmap.fill(Qt::yellow);
    } else
        pixmap.fill(QColor(global.getEditorBackgroundColor()));

    p.begin(&pixmap);
    p.setPen(fontPen);
    p.setFont(font);
    p.drawPixmap(QPoint(3, 3), icon.pixmap(QSize(30, 40)));

    // Write out the attributes of the file
    p.drawText(textPoint, displayName);

    QString unit = QString(tr("Bytes"));
    qint64 size = QFileInfo(fileName).size();
    if (size > 1024) {
        size = size / 1024;
        unit = QString(tr("KB"));
    }
    if (size > 1024) {
        size = size / 1024;
        unit = QString("MB");
    }

    p.drawText(sizePoint, QString::number(size).trimmed() + " " + unit);
    p.drawRect(0, 0, width - 1, 37 - 1);   // Draw a rectangle around the image.
    p.end();

    // Now that it is drawn, we write it out to a temporary file
    QString tmpFile = global.fileManager.getTmpDirPath(QString::number(lid) + QString("_icon.png"));
    pixmap.save(tmpFile, "png");
    QLOG_TRACE_OUT();
    return tmpFile;
}


// Modify the en-to tag into an input field
void NoteFormatter::modifyTodoTags(HtmlDomElement &todo) {
    QLOG_TRACE_IN();

    // Checks the en-to tag wheter or not the todo-item is checked or not
    // and sets up the HTML to keep storing the information in value
    QString checked = todo.attribute("checked");
    bool isChecked = false;
    if (checked.toLower() == "true") {
        isChecked = true;
    }

    QString xml = global.getCheckboxElement(isChecked, false);
    todo.setOuterXml(xml + todo.toInnerXml());
    QLOG_TRACE_OUT();
}


/* If we have an ink note, then we need to pull the image and display it */
bool NoteFormatter::buildInkNote(HtmlDomElement &docElem, QString &hash) {
    QLOG_TRACE_IN();

    ResourceTable resTable(global.db);
    qint32 resLid = resTable.getLidByHashHex(note.guid, hash);
    if (resLid <= 0)
        return false;
    docElem.setAttribute("en-tag", "en-media");
    docElem.setAttribute("lid", QString::number(resLid));
    docElem.setAttribute("type", "application/vnd.evernote.ink");
    const QString imagePath = resourceFilePath(resLid, QStringLiteral(".png"));
    QString filename = resourceImageUrl(imagePath);
    docElem.setAttribute("src", filename);
    QString k = docElem.toOuterXml();
    k.replace("<en-media", "<img");
    k.replace("enmedia>", "img>");;
    docElem.setOuterXml(k);

    QLOG_TRACE_OUT();
    return true;
}


void NoteFormatter::modifyPdfTags(qint32 resLid, HtmlDomElement &enmedia) {
    QLOG_TRACE_IN();

    enmedia.setAttribute("style", "width:100%; height: 600px");
    enmedia.setAttribute("lid", QString::number(resLid));
    QString x = enmedia.toOuterXml();
    x.replace("en-media", "object");
    enmedia.setOuterXml(x);
    x = enmedia.toOuterXml();
    QLOG_TRACE_OUT();
}


void NoteFormatter::setHighlightText(QString text) {
    QLOG_TRACE_IN();
    QStringList temp = text.split(" ");
    for (int i = 0; i < temp.size(); i++) {
        if (temp[i].trimmed() != "")
            highlightWords.append(temp[i]);
    }
    QLOG_TRACE_OUT();
}
