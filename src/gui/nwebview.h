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

#ifndef NWEBVIEW_H
#define NWEBVIEW_H

#include <QWebEngineView>
#include "src/gui/nwebpage.h"
#include <QShortcut>
#include <QMenu>
#include <QNetworkRequest>
#include <QAction>
#include <QPoint>
#include <QWebChannel>

#include "src/gui/browserWidgets/ntitleeditor.h"

class NBrowserWindow;
class NWebView;

class NWebViewBridge : public QObject
{
    Q_OBJECT
public:
    explicit NWebViewBridge(NWebView *view, QObject *parent = nullptr);

public slots:
    void editAlert();

private:
    NWebView *view;
};

class NBrowserWindowBridge : public QObject
{
    Q_OBJECT
public:
    explicit NBrowserWindowBridge(NBrowserWindow *browser, QObject *parent = nullptr);

public slots:
    void boldActive();
    void changeDisplayFontName(QString name);
    void changeDisplayFontSize(QString size);
    void decryptText(QString id, QString text, QString hint, QString cipher, int len);
    void imageContextMenu(QString lid, QString filename);
    void insideEncryptionArea();
    void italicsActive();
    void printNodeName(QString node);
    void resourceContextMenu(QString filename);
    void setInsideLink(QString link);
    void setInsideList();
    void setInsidePre();
    void setInsideTable();
    void setTableCellStyle(QString value);
    void setTableCursorPositionBackTab(int currentRow, int currentCol, int tableRows, int tableColumns);
    void setTableCursorPositionTab(int currentRow, int currentCol, int tableRows, int tableColumns);
    void setTableStyle(QString value);
    void underlineActive();

private:
    NBrowserWindow *browser;
};

//****************************************************
//* This inherits everything from QWebEngineView.  It is
//* useful to allow us to setup things that are
//* needed to view & edit notes.
//****************************************************
class NWebView : public QWebEngineView
{
    Q_OBJECT
private:
    QAction *setupColorMenuOption(QString color);
    QMenu *todoMenu;
    QMenu *colorMenu;
    QAction *backgroundColor;
    QWebChannel *webChannel;
    bool contentEditable;

    QString runJavaScriptForString(const QString &script) const;

public:
    explicit NWebView(NBrowserWindow *parent = 0);
    ~NWebView();
    NBrowserWindow *parent;
    NTitleEditor *titleEditor;
    QString pasteSequence;
    QString pasteUnformattedSequence;
    QString fileSaveSequence;
    QMenu *contextMenu;
    QMenu *tableMenu;
    QMenu *imageMenu;
    QAction *cutAction;
    QAction *copyAction;
    QAction *pasteAction;
    //QAction *removeFormattingAction;
    QAction *htmlTidyAction;
    QAction *htmlSimplifyAction;
    QAction *insertDateTimeAction;
    QAction *pasteWithoutFormatAction;
    QAction *todoAction;
    QAction *todoSelectAllAction;
    QAction *todoUnselectAllAction;
    QAction *encryptAction;
    QAction *downloadAttachmentAction();
    QAction *downloadImageAction();
    QAction *rotateImageRightAction;
    QAction *rotateImageLeftAction;
    QAction *insertLinkAction;
    QAction *removeLinkAction;
    QAction *insertLatexAction;
    QAction *attachFileAction;
    QAction *insertTableAction;
    QAction *insertTableRowAction;
    QAction *tablePropertiesAction;
    QAction *insertTableColumnAction;
    QAction *deleteTableColumnAction;
    QAction *deleteTableRowAction;
    QAction *openAction;
    QAction *insertQuickLinkAction;
    QAction *insertHtmlEntitiesAction;
    QAction *copyInAppNoteLinkAction;

    QAction *fontColorAction;
    QAction *fontBackgroundColorAction;


    QShortcut *attachFileShortcut;

    bool isDirty;
    NWebPage *editorPage;
    bool event(QEvent *event);
    void keyPressEvent(QKeyEvent *);
    void downloadAttachment(QNetworkRequest *req);
    void focusLostNotify(QString text);
    void setContent(const QByteArray &data);
    void evaluateJavaScript(const QString &script);
    QString toHtmlSync() const;
    QString toPlainTextSync() const;
    QString selectedHtml() const;
    QString documentElementOuterXmlSync() const;
    QPoint scrollPositionSync() const;
    bool findTextSync(const QString &text,
                      QWebEnginePage::FindFlags options = QWebEnginePage::FindFlags()) const;
    void setScrollPosition(const QPoint &pos);
    void setContentEditable(bool editable);
    bool isContentEditable() const;
    void setHtmlContent(const QString &html);

signals:
    void noteChanged();
    void downloadAttachmentRequested(QNetworkRequest*);
    void htmlEditAlert();
    void escapeKeyPressed();

public slots:
    void editAlert();
    void decryptText(QString id, QString text, QString hint);
    void contextMenuEvent(QContextMenuEvent *event);
    void setBackgroundColor(QString color);
    void printNodeName(QString s);
    void setTitleEditor(NTitleEditor *editor);
    void focusOutEvent(QFocusEvent *e);

private slots:
    void exposeToJavascript();
    void injectEditorBridge();
    void setDefaultTitle();
    void downloadRequested(QNetworkRequest);
    void dropEvent(QDropEvent *e);


};

#endif // NWEBVIEW_H
