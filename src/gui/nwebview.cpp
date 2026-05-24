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


#include "nwebview.h"
#include "nwebpage.h"
#include "src/communication/communicationmanager.h"
#include "src/gui/nbrowserwindow.h"
#include "src/global.h"
#include <QEventLoop>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMutex>
#include <QMutexLocker>
#include <QSaveFile>
#include <QSharedPointer>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QWebEngineFindTextResult>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>
#include "src/settings/colorsettings.h"

//#include "./libencrypt/encrypt_global.h"
//#include "./libencrypt/encrypt.h"
#include "../dialog/endecryptdialog.h"
#include "src/sql/resourcetable.h"

extern Global global;

namespace {
bool isValidEvernoteGuid(const QString &guid)
{
    return !guid.isEmpty() && !QUuid::fromString(guid).isNull();
}

bool downloadResourceFile(qint32 lid, const QString &filePath)
{
    static QMutex downloadMutex;
    QMutexLocker locker(&downloadMutex);

    if (QFileInfo::exists(filePath)) {
        return true;
    }

    ResourceTable resourceTable(global.db);
    Resource localResource;
    if (!resourceTable.get(localResource, lid, false) || !localResource.guid.isSet()) {
        QLOG_WARN() << "Unable to resolve missing editor resource, lid=" << lid;
        return false;
    }

    const QString guid = localResource.guid;
    if (!isValidEvernoteGuid(guid)) {
        QLOG_WARN() << "Editor resource has invalid Evernote guid, lid=" << lid
                    << " guid=" << guid;
        return false;
    }

    CommunicationManager comm(global.db);
    if (!comm.enConnect()) {
        QLOG_WARN() << "Unable to connect to Evernote for missing editor resource, lid=" << lid;
        return false;
    }

    Resource remoteResource;
    bool remoteResourceNotFound = false;
    if (!comm.getResource(remoteResource, guid, true, false, true, false, false,
                          &remoteResourceNotFound)) {
        if (remoteResourceNotFound) {
            QLOG_WARN() << "Editor resource no longer exists on Evernote server, lid=" << lid
                        << " guid=" << guid;
            return false;
        }
        QLOG_WARN() << "Unable to download missing editor resource, lid=" << lid << " guid=" << guid;
        return false;
    }

    Data data;
    if (remoteResource.data.isSet()) {
        data = remoteResource.data;
    }

    QByteArray body;
    if (data.body.isSet()) {
        body = data.body;
    }

    if (body.isEmpty()) {
        QLOG_WARN() << "Downloaded editor resource has empty body, lid=" << lid << " guid=" << guid;
        return false;
    }

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QLOG_WARN() << "Unable to write downloaded editor resource:" << filePath << file.errorString();
        return false;
    }

    if (file.write(body) != body.size()) {
        QLOG_WARN() << "Unable to write complete editor resource:" << filePath << file.errorString();
        return false;
    }

    if (!file.commit()) {
        QLOG_WARN() << "Unable to commit downloaded editor resource:" << filePath << file.errorString();
        return false;
    }

    QLOG_INFO() << "Downloaded missing editor resource:" << filePath;
    return true;
}

class NResourceSchemeHandler : public QWebEngineUrlSchemeHandler
{
public:
    explicit NResourceSchemeHandler(QObject *parent = nullptr) :
        QWebEngineUrlSchemeHandler(parent)
    {
    }

    void requestStarted(QWebEngineUrlRequestJob *request) override
    {
        QString fileName = QUrl::fromPercentEncoding(request->requestUrl().path().toUtf8());
        while (fileName.startsWith(QLatin1Char('/'))) {
            fileName.remove(0, 1);
        }

        fileName = QFileInfo(fileName).fileName();
        if (fileName.isEmpty() || fileName == QStringLiteral(".") || fileName == QStringLiteral("..")) {
            request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }

        const QString filePath = global.fileManager.getDbaDirPath() + fileName;
        if (!QFileInfo::exists(filePath)) {
            bool ok = false;
            const qint32 lid = QFileInfo(fileName).baseName().toInt(&ok);
            if (!ok || lid <= 0 || !downloadResourceFile(lid, filePath)) {
                QLOG_WARN() << "Unable to load editor resource:" << filePath;
                request->fail(QWebEngineUrlRequestJob::UrlNotFound);
                return;
            }
        }

        auto *file = new QFile(filePath, request);
        if (!file->open(QIODevice::ReadOnly)) {
            QLOG_WARN() << "Unable to open editor resource:" << filePath << file->errorString();
            delete file;
            request->fail(QWebEngineUrlRequestJob::UrlNotFound);
            return;
        }

        const QByteArray mimeType = QMimeDatabase().mimeTypeForFile(filePath).name().toUtf8();
        request->reply(mimeType, file);
    }
};

void installResourceSchemeHandler()
{
    static bool installed = false;
    if (installed) {
        return;
    }

    QWebEngineProfile *profile = QWebEngineProfile::defaultProfile();
    profile->installUrlSchemeHandler(QByteArrayLiteral("nnres"), new NResourceSchemeHandler(profile));
    installed = true;
}
}


NWebViewBridge::NWebViewBridge(NWebView *view, QObject *parent) :
    QObject(parent),
    view(view)
{
}


void NWebViewBridge::editAlert() {
    if (view != nullptr)
        view->editAlert();
}


NBrowserWindowBridge::NBrowserWindowBridge(NBrowserWindow *browser, QObject *parent) :
    QObject(parent),
    browser(browser)
{
}


void NBrowserWindowBridge::boldActive() {
    if (browser != nullptr)
        browser->boldActive();
}


void NBrowserWindowBridge::changeDisplayFontName(QString name) {
    if (browser != nullptr)
        browser->changeDisplayFontName(name);
}


void NBrowserWindowBridge::changeDisplayFontSize(QString size) {
    if (browser != nullptr)
        browser->changeDisplayFontSize(size);
}


void NBrowserWindowBridge::decryptText(QString id, QString text, QString hint, QString cipher, int len) {
    if (browser != nullptr)
        browser->decryptText(id, text, hint, cipher, len);
}


void NBrowserWindowBridge::imageContextMenu(QString lid, QString filename) {
    if (browser != nullptr)
        browser->imageContextMenu(lid, filename);
}


void NBrowserWindowBridge::insideEncryptionArea() {
    if (browser != nullptr)
        browser->insideEncryptionArea();
}


void NBrowserWindowBridge::italicsActive() {
    if (browser != nullptr)
        browser->italicsActive();
}


void NBrowserWindowBridge::printNodeName(QString node) {
    if (browser != nullptr)
        browser->printNodeName(node);
}


void NBrowserWindowBridge::resourceContextMenu(QString filename) {
    if (browser != nullptr)
        browser->resourceContextMenu(filename);
}


void NBrowserWindowBridge::setInsideLink(QString link) {
    if (browser != nullptr)
        browser->setInsideLink(link);
}


void NBrowserWindowBridge::setInsideList() {
    if (browser != nullptr)
        browser->setInsideList();
}


void NBrowserWindowBridge::setInsidePre() {
    if (browser != nullptr)
        browser->setInsidePre();
}


void NBrowserWindowBridge::setInsideTable() {
    if (browser != nullptr)
        browser->setInsideTable();
}


void NBrowserWindowBridge::setTableCellStyle(QString value) {
    if (browser != nullptr)
        browser->setTableCellStyle(value);
}


void NBrowserWindowBridge::setTableCursorPositionBackTab(
        int currentRow, int currentCol, int tableRows, int tableColumns) {
    if (browser != nullptr)
        browser->setTableCursorPositionBackTab(currentRow, currentCol, tableRows, tableColumns);
}


void NBrowserWindowBridge::setTableCursorPositionTab(
        int currentRow, int currentCol, int tableRows, int tableColumns) {
    if (browser != nullptr)
        browser->setTableCursorPositionTab(currentRow, currentCol, tableRows, tableColumns);
}


void NBrowserWindowBridge::setTableStyle(QString value) {
    if (browser != nullptr)
        browser->setTableStyle(value);
}


void NBrowserWindowBridge::underlineActive() {
    if (browser != nullptr)
        browser->underlineActive();
}


//****************************************************
//* This inherits everything from QWebEngineView.  It is
//* useful to allow us to setup things that are
//* needed to view & edit notes.
//****************************************************

// Override the constructor so we always use a NWebPage
// rather than a QWebEnginePage
NWebView::NWebView(NBrowserWindow *parent) :
    QWebEngineView(parent)
{
    installResourceSchemeHandler();
    this->parent = parent;
    editorPage = new NWebPage(this);
    setPage(editorPage);
    webChannel = new QWebChannel(this);
    webChannel->registerObject(QStringLiteral("editorWindow"), new NWebViewBridge(this, webChannel));
    webChannel->registerObject(QStringLiteral("browserWindow"), new NBrowserWindowBridge(parent, webChannel));
    editorPage->setWebChannel(webChannel);
    contentEditable = true;
    isDirty = false;
    this->setFont(global.getGuiFont(font()));
    settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

    contextMenu = new QMenu(this);

    openAction = new QAction(tr("Open"), this);
    contextMenu->addAction(openAction);
    contextMenu->addSeparator();
    contextMenu->setFont(global.getGuiFont(font()));

    cutAction = new QAction(tr("Cut"), this);
    global.setupShortcut(cutAction, "Edit_Cut");
    contextMenu->addAction(cutAction);
    connect(cutAction, &QAction::triggered, parent, &NBrowserWindow::cutButtonPressed);

    copyAction = new QAction(tr("Copy"), this);
    global.setupShortcut(copyAction, "Edit_Copy");
    contextMenu->addAction(copyAction);
    connect(copyAction, &QAction::triggered, parent, &NBrowserWindow::copyButtonPressed);

    pasteAction = new QAction(tr("Paste"), this);
    global.setupShortcut(pasteAction, "Edit_Paste");
    contextMenu->addAction(pasteAction);
    connect(pasteAction, &QAction::triggered, parent, &NBrowserWindow::pasteButtonPressed);

    pasteWithoutFormatAction = new QAction(tr("Paste as Unformatted Text"), this);
    global.setupShortcut(pasteWithoutFormatAction, "Edit_Paste_Without_Formatting");
    contextMenu->addAction(pasteWithoutFormatAction);
    connect(pasteWithoutFormatAction, &QAction::triggered,
            parent, &NBrowserWindow::pasteWithoutFormatButtonPressed);

    copyInAppNoteLinkAction = new QAction(tr("Copy In-App Note Link"), this);
    global.setupShortcut(copyInAppNoteLinkAction, "Edit_Copy_Note_Url");
    contextMenu->addAction(copyInAppNoteLinkAction);
    connect(copyInAppNoteLinkAction, &QAction::triggered,
            parent, &NBrowserWindow::copyInAppNoteLink);

    contextMenu->addSeparator();

    htmlSimplifyAction = new QAction(tr("Simplify formatting"), this);
    global.setupShortcut(htmlSimplifyAction, "Edit_HTML_Simplify");
    contextMenu->addAction(htmlSimplifyAction);
    connect(htmlSimplifyAction, &QAction::triggered, parent, &NBrowserWindow::htmlSimplify);

    contextMenu->addSeparator();

    // change background color of WHOLE note (this is different from changing font/text background color)
    colorMenu = new QMenu(tr("Note Background Color"), this);
    colorMenu->setFont(global.getGuiFont(font()));
    // Build the background color menu
    QAction *action;
    ColorSettings colorSettings;
    QList< QPair<QString,QString> > colorList = colorSettings.colorList();
    for (int i=0; i<colorList.size(); i++) {
        action = colorMenu->addAction(colorList[i].first);
        const QString color = colorList[i].second;
        connect(action, &QAction::triggered, this, [this, color]() {
            setBackgroundColor(color);
        });
    }
    // QAction *action = setupColorMenuOption(tr("White"));

    fontColorAction = new QAction(tr("Set text color"), this);
    global.setupShortcut(fontColorAction, "Format_Font_Color");
    contextMenu->addAction(fontColorAction);
    connect(fontColorAction, &QAction::triggered, parent, &NBrowserWindow::fontColorClicked);

    fontBackgroundColorAction = new QAction(tr("Set text background color"), this);
    global.setupShortcut(fontBackgroundColorAction, "Format_Highlight");
    contextMenu->addAction(fontBackgroundColorAction);
    connect(fontBackgroundColorAction, &QAction::triggered, parent, &NBrowserWindow::fontHighlightClicked);

    contextMenu->addMenu(colorMenu);
    contextMenu->addSeparator();

    todoMenu = new QMenu(tr("To-do"), this);
    todoMenu->setFont(global.getGuiFont(font()));
    contextMenu->addMenu(todoMenu);
    todoAction = new QAction(tr("To-do"), this);
    global.setupShortcut(todoAction, "Edit_Insert_Todo");
    todoMenu->addAction(todoAction);
    connect(todoAction, &QAction::triggered, parent, &NBrowserWindow::todoButtonPressed);
    todoMenu->addSeparator();

    todoSelectAllAction = new QAction(tr("Select All"), this);
    todoMenu->addAction(todoSelectAllAction);
    connect(todoSelectAllAction, &QAction::triggered, parent, &NBrowserWindow::todoSelectAll);

    todoUnselectAllAction = new QAction(tr("Unselect All"), this);
    todoMenu->addAction(todoUnselectAllAction);
    connect(todoUnselectAllAction, &QAction::triggered, parent, &NBrowserWindow::todoUnselectAll);

    contextMenu->addSeparator();

    insertHtmlEntitiesAction = new QAction(tr("HTML Entities"),this);
    contextMenu->addAction(insertHtmlEntitiesAction);
    global.setupShortcut(insertHtmlEntitiesAction, "Edit_Insert_Html_Entities");
    connect(insertHtmlEntitiesAction, &QAction::triggered, parent, &NBrowserWindow::insertHtmlEntities);

    contextMenu->addSeparator();

    encryptAction = new QAction(tr("Encrypt Selected Text"), this);
    contextMenu->addAction(encryptAction);
    global.setupShortcut(encryptAction, "Edit_Encrypt_Text");
    connect(encryptAction, &QAction::triggered, parent, &NBrowserWindow::encryptButtonPressed);

    insertDateTimeAction = new QAction(tr("Insert Date Time"), this);
    global.setupShortcut(insertDateTimeAction, "Insert_DateTime");
    contextMenu->addAction(insertDateTimeAction);
    connect(insertDateTimeAction, &QAction::triggered, parent, &NBrowserWindow::insertDatetime);

    insertLinkAction = new QAction(tr("Insert Hyperlink"), this);
    contextMenu->addAction(insertLinkAction);
    global.setupShortcut(insertLinkAction, "Edit_Insert_Hyperlink");
    connect(insertLinkAction, &QAction::triggered, parent, &NBrowserWindow::insertLinkButtonPressed);

    insertQuickLinkAction = new QAction(tr("Quick Link"), this);
    contextMenu->addAction(insertQuickLinkAction);
    global.setupShortcut(insertQuickLinkAction, "Edit_Insert_QuickLink");
    connect(insertQuickLinkAction, &QAction::triggered, parent, &NBrowserWindow::insertQuickLinkButtonPressed);

    removeLinkAction = new QAction(tr("Remove Hyperlink"), this);
    contextMenu->addAction(removeLinkAction);
    global.setupShortcut(removeLinkAction, "Edit_Remove_Hyperlink");
    connect(removeLinkAction, &QAction::triggered, parent, &NBrowserWindow::removeLinkButtonPressed);

    attachFileAction = new QAction(tr("Attach File"), this);
    contextMenu->addAction(attachFileAction);
    global.setupShortcut(attachFileAction, "Edit_Attach_File");
    connect(attachFileAction, &QAction::triggered, parent, &NBrowserWindow::attachFile);
    contextMenu->addSeparator();

    insertLatexAction = new QAction(tr("Insert LaTeX Formula"), this);
    contextMenu->addAction(insertLatexAction);
    global.setupShortcut(insertLatexAction, "Edit_Insert_Latex");
    connect(insertLatexAction, &QAction::triggered, parent, &NBrowserWindow::insertLatexButtonPressed);
    contextMenu->addSeparator();

    tableMenu = new QMenu(tr("Table"), this);
    tableMenu->setFont(global.getGuiFont(font()));
    contextMenu->addMenu(tableMenu);
    insertTableAction = new QAction(tr("Insert Table"), this);
    global.setupShortcut(insertTableAction, "Edit_Insert_Table");
    tableMenu->addAction(insertTableAction);
    connect(insertTableAction, &QAction::triggered, parent, &NBrowserWindow::insertTableButtonPressed);
    tableMenu->addSeparator();

    insertTableRowAction = new QAction(tr("Insert Row"), this);
    global.setupShortcut(insertTableRowAction, "Edit_Insert_Table_Row");
    tableMenu->addAction(insertTableRowAction);
    connect(insertTableRowAction, &QAction::triggered, parent, &NBrowserWindow::insertTableRowButtonPressed);

    insertTableColumnAction = new QAction(tr("Insert Column"), this);
    global.setupShortcut(insertTableColumnAction, "Edit_Insert_Table_Column");
    tableMenu->addAction(insertTableColumnAction);
    connect(insertTableColumnAction, &QAction::triggered, parent, &NBrowserWindow::insertTableColumnButtonPressed);

    tableMenu->addSeparator();

    deleteTableRowAction = new QAction(tr("Delete Row"), this);
    tableMenu->addAction(deleteTableRowAction);
    global.setupShortcut(deleteTableRowAction, "Edit_Delete_Table_Row");
    connect(deleteTableRowAction, &QAction::triggered, parent, &NBrowserWindow::deleteTableRowButtonPressed);
    deleteTableColumnAction = new QAction(tr("Delete Column"), this);
    tableMenu->addAction(deleteTableColumnAction);
    global.setupShortcut(deleteTableColumnAction, "Edit_Delete_Table_Column");
    connect(deleteTableColumnAction, &QAction::triggered, parent, &NBrowserWindow::deleteTableColumnButtonPressed);

    tableMenu->addSeparator();

    tablePropertiesAction = new QAction(tr("Table Properties"), this);
    global.setupShortcut(tablePropertiesAction, "Edit_Table_Properties");
    tableMenu->addAction(tablePropertiesAction);
    connect(tablePropertiesAction, &QAction::triggered, parent, &NBrowserWindow::tablePropertiesButtonPressed);

    contextMenu->addSeparator();

    imageMenu = new QMenu(tr("Image"), this);
    imageMenu->setFont(global.getGuiFont(font()));
    contextMenu->addMenu(imageMenu);
    downloadImageAction()->setText(tr("Save Image"));
    imageMenu->addAction(downloadImageAction());
    // Don't connect this signal.  The download attachmen signal will handle it.  Otherwise
    // the signal fires twice.
    //connect(editorPage, &NWebPage::downloadRequested, this, &NWebView::downloadRequested);

    imageMenu->addSeparator();

    rotateImageLeftAction = new QAction(tr("Rotate Left"), this);
    imageMenu->addAction(rotateImageLeftAction);
    global.setupShortcut(rotateImageLeftAction, "Edit_Image_Rotate_Left");
    connect(rotateImageLeftAction, &QAction::triggered, parent, &NBrowserWindow::rotateImageLeftButtonPressed);
    rotateImageRightAction = new QAction(tr("Rotate Right"), this);
    global.setupShortcut(rotateImageRightAction, "Edit_Image_Rotate_Right");
    imageMenu->addAction(rotateImageRightAction);
    connect(rotateImageRightAction, &QAction::triggered, parent, &NBrowserWindow::rotateImageRightButtonPressed);
    contextMenu->addSeparator();

    downloadAttachmentAction()->setText(tr("Save Attachment"));
    contextMenu->addAction(downloadAttachmentAction());
    connect(editorPage, &NWebPage::linkClicked, parent, &NBrowserWindow::linkClicked);
    connect(this, &QWebEngineView::loadFinished, this, &NWebView::injectEditorBridge);
    setContentEditable(true);

    // Set some of the menus as disabled until a user selects an image or attachment
    downloadAttachmentAction()->setEnabled(false);
    rotateImageRightAction->setEnabled(false);
    rotateImageLeftAction->setEnabled(false);
    openAction->setEnabled(false);
    downloadImageAction()->setEnabled(false);

    exposeToJavascript();

    //    this->setStyleSheet("QWebEngineView,html,body { background-color : red; foreground-color : white; }");
    // ////////////////
    //    QString qss = global.fileManager.getQssDirPathUser("");
    //    if (qss == "")
    //        qss = global.fileManager.getQssDirPath("");
    //    this->settings()->setUserStyleSheetUrl(QUrl("file://"+qss+"editor.css"));

    // style apply: see Global::getEditorCss
    // then NBrowserWindow::setEditorStyle()

    this->pasteSequence = QKeySequence(this->pasteAction->shortcut()).toString().toLower();
    if (pasteSequence.trimmed() == "")
        pasteSequence = "ctrl+v";
    pasteUnformattedSequence = QKeySequence(pasteWithoutFormatAction->shortcut()).toString().toLower();
    if (pasteUnformattedSequence.trimmed() == "")
        pasteUnformattedSequence = "ctrl+shift+v";
    fileSaveSequence = "ctrl+s";

    QString css = global.getThemeCss("noteContentsCss");
    if (css!="")
        this->setStyleSheet(css);

    backgroundColor = nullptr;
}


NWebView::~NWebView() {
    delete contextMenu;
    delete tableMenu;
    delete imageMenu;

    delete openAction;
    delete cutAction;
    delete copyAction;
    delete pasteAction;
    delete pasteWithoutFormatAction;
    delete copyInAppNoteLinkAction;
    delete htmlSimplifyAction;
    delete fontColorAction;

    delete fontBackgroundColorAction;
    delete todoAction;
    delete todoSelectAllAction;
    delete todoUnselectAllAction;
    delete insertHtmlEntitiesAction;
    delete encryptAction;
    delete insertDateTimeAction;
    delete insertLinkAction;

    delete insertQuickLinkAction;
    delete removeLinkAction;
    delete attachFileAction;
    delete insertLatexAction;
    delete insertTableAction;
    delete insertTableRowAction;
    delete insertTableColumnAction;
    delete deleteTableRowAction;
    delete deleteTableColumnAction;
    delete tablePropertiesAction;
    delete rotateImageLeftAction;
    delete rotateImageRightAction;
}


QAction *NWebView::downloadAttachmentAction() {
    return pageAction(QWebEnginePage::DownloadLinkToDisk);
}


QAction *NWebView::downloadImageAction() {
    return pageAction(QWebEnginePage::DownloadImageToDisk);
}

QAction* NWebView::setupColorMenuOption(QString color) {
    if (backgroundColor != nullptr) {
        backgroundColor = new QAction(color, this);
    }
    color = color.replace(" ", "");
    //connect(backgroundColor, &QAction::triggered, this, ...)
    return backgroundColor;
}


void NWebView::focusOutEvent(QFocusEvent *e) {
    QWebEngineView::focusOutEvent(e);
    titleEditor->checkNoteTitleChange();
}

void NWebView::editAlert() {

    // If this is the first time the note is dirty, set the dirty
    // flag (this is checked elsewhere) and signal the change
    if (!isDirty) {
        isDirty = true;
        emit(noteChanged());
    }
    emit (htmlEditAlert());

    // This should already be set, but just in case...
    isDirty = true;
}


void NWebView::exposeToJavascript() {
    page()->setWebChannel(webChannel);
}


void NWebView::injectEditorBridge() {
    QString source;
    QFile webChannelScript(QStringLiteral(":/qtwebchannel/qwebchannel.js"));
    if (webChannelScript.open(QIODevice::ReadOnly)) {
        source = QString::fromUtf8(webChannelScript.readAll());
    }

    source += QStringLiteral(
        "\n(function() {"
        "  function install() {"
        "    if (typeof QWebChannel === 'undefined' || typeof qt === 'undefined') return;"
        "    new QWebChannel(qt.webChannelTransport, function(channel) {"
        "      window.editorWindow = channel.objects.editorWindow;"
        "      window.browserWindow = channel.objects.browserWindow;"
        "      window.browser = window.browserWindow;"
        "      document.addEventListener('input', function() {"
        "        if (window.editorWindow && window.editorWindow.editAlert) window.editorWindow.editAlert();"
        "      }, true);"
        "    });"
        "  }"
        "  install();"
        "})();");

    page()->runJavaScript(source);
    setContentEditable(contentEditable);
}


QString NWebView::runJavaScriptForString(const QString &script) const {
    struct ScriptResult {
        QString value;
        bool finished;
    };
    QSharedPointer<ScriptResult> result(new ScriptResult{QString(), false});
    QSharedPointer<QEventLoop> loop(new QEventLoop);

    page()->runJavaScript(script, [result, loop](const QVariant &value) {
        result->value = value.toString();
        result->finished = true;
        loop->quit();
    });

    if (!result->finished) {
        QTimer::singleShot(5000, loop.data(), &QEventLoop::quit);
        loop->exec();
    }

    return result->value;
}


void NWebView::evaluateJavaScript(const QString &script) {
    page()->runJavaScript(script);
}


QString NWebView::toHtmlSync() const {
    struct HtmlResult {
        QString value;
        bool finished;
    };
    QSharedPointer<HtmlResult> result(new HtmlResult{QString(), false});
    QSharedPointer<QEventLoop> loop(new QEventLoop);

    page()->toHtml([result, loop](const QString &html) {
        result->value = html;
        result->finished = true;
        loop->quit();
    });

    if (!result->finished) {
        QTimer::singleShot(5000, loop.data(), &QEventLoop::quit);
        loop->exec();
    }

    return result->value;
}


QString NWebView::toPlainTextSync() const {
    struct TextResult {
        QString value;
        bool finished;
    };
    QSharedPointer<TextResult> result(new TextResult{QString(), false});
    QSharedPointer<QEventLoop> loop(new QEventLoop);

    page()->toPlainText([result, loop](const QString &text) {
        result->value = text;
        result->finished = true;
        loop->quit();
    });

    if (!result->finished) {
        QTimer::singleShot(5000, loop.data(), &QEventLoop::quit);
        loop->exec();
    }

    return result->value;
}


QString NWebView::selectedHtml() const {
    return runJavaScriptForString(QStringLiteral(
        "(function() {"
        "  var selection = window.getSelection();"
        "  if (!selection || selection.rangeCount === 0) return '';"
        "  var container = document.createElement('div');"
        "  for (var i = 0; i < selection.rangeCount; ++i) {"
        "    container.appendChild(selection.getRangeAt(i).cloneContents());"
        "  }"
        "  return container.innerHTML;"
        "})();"));
}


QString NWebView::documentElementOuterXmlSync() const {
    return runJavaScriptForString(QStringLiteral("document.documentElement.outerHTML"));
}


QPoint NWebView::scrollPositionSync() const {
    const QString value = runJavaScriptForString(QStringLiteral("window.scrollX + ',' + window.scrollY"));
    const QStringList parts = value.split(QStringLiteral(","));
    if (parts.size() != 2) {
        return QPoint();
    }
    return QPoint(parts[0].toInt(), parts[1].toInt());
}


bool NWebView::findTextSync(const QString &text, QWebEnginePage::FindFlags options) const {
    struct FindResult {
        bool found;
        bool finished;
    };
    QSharedPointer<FindResult> result(new FindResult{false, false});
    QSharedPointer<QEventLoop> loop(new QEventLoop);

    page()->findText(text, options, [result, loop](const QWebEngineFindTextResult &findResult) {
        result->found = findResult.numberOfMatches() > 0;
        result->finished = true;
        loop->quit();
    });

    if (!result->finished) {
        QTimer::singleShot(5000, loop.data(), &QEventLoop::quit);
        loop->exec();
    }

    return result->found;
}


void NWebView::setScrollPosition(const QPoint &pos) {
    evaluateJavaScript(QStringLiteral("window.scrollTo(%1, %2);").arg(pos.x()).arg(pos.y()));
}


void NWebView::setContentEditable(bool editable) {
    contentEditable = editable;
    evaluateJavaScript(QStringLiteral(
        "document.designMode = '%1';"
        "if (document.body) document.body.contentEditable = '%2';"
        "document.documentElement.contentEditable = '%2';")
        .arg(editable ? QStringLiteral("on") : QStringLiteral("off"),
             editable ? QStringLiteral("true") : QStringLiteral("false")));
}


bool NWebView::isContentEditable() const {
    return contentEditable;
}


void NWebView::setHtmlContent(const QString &html) {
    page()->setHtml(html, QUrl::fromLocalFile(global.fileManager.getDbaDirPath()));
}



void NWebView::decryptText(QString id, QString text, QString hint) {
    /* Suppress unused */
    Q_UNUSED(id);
    Q_UNUSED(text);
    Q_UNUSED(hint);


//    EnCrypt crypt;
//    QString plainText;
/*
//    // First, try to decrypt with any keys we already have
    for (int i=0; i<global.passwordRemember.size(); i++) {
        QString key = global.passwordRemember[i].second;
        bool retVal = crypt.decrypt(text, plainText, key);
        if (retVal && plainText != "") {
            //slot = new String(Long.toString(l));
            //Global.passwordSafe.put(slot, Global.passwordRemember.get(i));
            //removeEncryption(id, plainText, false, slot);
            return;
        }
    }


    EnDecryptDialog dialog;
    dialog.hint.setText(hint);
    bool goodReturn = true;
    while (goodReturn && (plainText == "" || !dialog.okPressed)) {
        dialog.exec();
        if (!dialog.okPressed) {
            return;
        }
        QString pswd = dialog.password.text().trimmed();
        crypt.decrypt(text, plainText, pswd);
        if (plainText == "") {
            QMessageBox mb;
            mb.warning(this, tr("Incorrect Password"), tr("The password entered is not correct"));
        }
    }
//    Pair<String,String> passwordPair = new Pair<String,String>();
//    passwordPair.setFirst(dialog.getPassword());
//    passwordPair.setSecond(dialog.getHint());
//    Global.passwordSafe.put(slot, passwordPair);
//    removeEncryption(id, plainText, dialog.permanentlyDecrypt(), slot);
//    if (dialog.rememberPassword()) {
//        Pair<String, String> pair = new Pair<String,String>();
//        pair.setFirst(dialog.getPassword());
//        pair.setSecond(dialog.getHint());
//        Global.passwordRemember.add(pair);
//    }
*/
}



void NWebView::contextMenuEvent(QContextMenuEvent *event) {
    if (event != nullptr)
        contextMenu->exec(event->globalPos());
    downloadAttachmentAction()->setEnabled(false);
    rotateImageRightAction->setEnabled(false);
    rotateImageLeftAction->setEnabled(false);
    openAction->setEnabled(false);
    downloadImageAction()->setEnabled(false);}


bool NWebView::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent*)event;
        if (ke->key() == Qt::Key_Tab) {
            parent->tabPressed();
            ke->accept();
            return true;
        }
        if (ke->key() == Qt::Key_Backtab) {
            parent->backtabPressed();
            ke->accept();
            return true;
        }
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            bool retval = parent->enterPressed();
            if (retval) {
                ke->accept();
                return true;
            }
        }

    }
    if (event->type() == QEvent::MouseButtonDblClick) {
        QLOG_DEBUG() << "NWebView::event: DOUBLE CLICK!!!";
    }
    return QWebEngineView::event(event);
}



void NWebView::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        const int direction = e->key() == Qt::Key_PageDown ? 1 : -1;
        evaluateJavaScript(QStringLiteral("window.scrollBy(0, %1);").arg(direction * height()));
        e->accept();
        return;
    }

    // Exit presentation mode
    if (e->key() == Qt::Key_Escape) {
        emit escapeKeyPressed();

        // Return without going forward or the current selected text is removed
        return;
    }

    // Hard override of paste because I can't seem to get it any other way.
//    if (e->key() == Qt::Key_V && e->modifiers().testFlag(Qt::ControlModifier)) {
    QKeySequence ks(e->modifiers()|e->key());
    if (ks.toString().toLower() == pasteSequence) {
        parent->pasteButtonPressed();
        e->accept();
        return;
    }
    if (ks.toString().toLower() == pasteUnformattedSequence) {
        parent->pasteWithoutFormatButtonPressed();
        e->accept();
        return;
    }
    if (ks.toString().toLower() == fileSaveSequence) {
        parent->saveNoteContent();
        e->accept();
        return;
    }

    QWebEngineView::keyPressEvent(e);
}




void NWebView::downloadAttachment(QNetworkRequest *req) {
    QLOG_DEBUG() << req->url().toString();
    emit(downloadAttachmentRequested(req));
}



void NWebView::setBackgroundColor(QString color) { parent->setBackgroundColor(color); }

void NWebView::printNodeName(QString s) {
    QLOG_DEBUG() << s;
}


void NWebView::setTitleEditor(NTitleEditor *editor) {
    titleEditor = editor;
    if (editor == nullptr)
        return;
    connect(this, &NWebView::htmlEditAlert, this, &NWebView::setDefaultTitle);
}


void NWebView::setDefaultTitle() {
    QString body = toPlainTextSync();
    titleEditor->setTitleFromContent(body);
}

void NWebView::downloadRequested(QNetworkRequest req) {
    QString urlString = req.url().toString();

    if (urlString == "")  {
        return;
    }

    if (urlString.startsWith("nnres:")) {
        int pos = urlString.indexOf(global.attachmentNameDelimeter);
        QString extension = "";
        if (pos != -1) {
            extension = urlString.mid(pos+global.attachmentNameDelimeter.length());
            urlString = urlString.mid(0,pos);
        }
        urlString = urlString.mid(6);
        if (urlString.lastIndexOf("/") > 0)
        	urlString = urlString.mid(urlString.lastIndexOf("/")+1);
        if (urlString.indexOf(".")>0)
        	urlString = urlString.mid(0,urlString.indexOf("."));

        qint32 lid = urlString.toInt();
        ResourceTable resTable(global.db);
        Resource r;
        resTable.get(r, lid, true);
        QString filename;
        ResourceAttributes attributes;
        if (r.attributes.isSet())
            attributes = r.attributes;
        if (attributes.fileName.isSet())
            filename = attributes.fileName;
        else
            filename = urlString + QString(".") + extension;

        QFileDialog fd;
        fd.setFileMode(QFileDialog::AnyFile);
        fd.setWindowTitle(tr("Save File"));
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.selectFile(filename);
        fd.setOption(QFileDialog::DontConfirmOverwrite, false);
        if (fd.exec()) {
            if (fd.selectedFiles().size() == 0)
                return;
            filename = fd.selectedFiles()[0];
            QFile newFile(filename);
            if (!newFile.open(QIODevice::WriteOnly)) {
                QMessageBox::critical(this, tr("Error"), tr("Unable to save file: %1").arg(filename));
                return;
            }
            Data d;
            if (r.data.isSet())
                d = r.data;
            QByteArray body;
            if (d.body.isSet())
                body = d.body;
            int size = 0;
            if (d.size.isSet())
                size = d.size;
            newFile.write(body, size);
            newFile.close();
            return;
        }
    }
    if (urlString.startsWith("file:///")) {
        if (!req.url().isValid())
            return;
        urlString = urlString.mid(8);
        QFileDialog fd;
        fd.setFileMode(QFileDialog::AnyFile);
        fd.setWindowTitle(tr("Save File"));
        fd.setAcceptMode(QFileDialog::AcceptSave);
        QString oldname = urlString;
        fd.selectFile(urlString.replace(global.fileManager.getDbaDirPath(), ""));
        fd.setOption(QFileDialog::DontConfirmOverwrite, false);
        if (fd.exec()) {
            if (fd.selectedFiles().size() == 0)
                return;
            QString newname = fd.selectedFiles()[0];
            QFile::remove(urlString);
            QFile::copy(oldname, newname);
            return;
        }

    }
}



void NWebView::dropEvent(QDropEvent *e) {
        setFocus();
        const QMimeData *mime = e->mimeData();
        parent->handleUrls(mime);
        parent->contentChanged();
}


void NWebView::setContent(const QByteArray &data) {
    QByteArray content = data;
    content.replace("<body", "<body onunload=_function() {} ");
    QWebEngineView::setContent(content, QStringLiteral("text/html"),
                               QUrl::fromLocalFile(global.fileManager.getDbaDirPath()));

    this->history()->clear();
}
