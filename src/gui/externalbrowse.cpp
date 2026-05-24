/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2014 Randy Baumgarte

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


#include "externalbrowse.h"
#include <QGridLayout>
#include <QLayout>
#include "src/global.h"

extern Global global;

ExternalBrowse::ExternalBrowse(qint32 lid, QWidget *parent) :
    QMdiSubWindow(parent)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    this->setWindowTitle(tr(NN_APP_DISPLAY_NAME_GUI));
    setWindowIcon(global.getIconResource(":windowIcon"));

    browser = new NBrowserWindow(this);
    setWidget(browser);

    // Setup shortcuts
    focusTitleShortcut = new QShortcut(this);
    focusTitleShortcut->setContext(Qt::WidgetShortcut);
    global.setupShortcut(focusTitleShortcut, "Focus_Title");
    connect(focusTitleShortcut, &QShortcut::activated, this, [this]() {
        browser->noteTitle.setFocus();
    });

    focusNoteShortcut = new QShortcut(this);
    focusNoteShortcut->setContext(Qt::WidgetShortcut);
    global.setupShortcut(focusNoteShortcut, "Focus_Note");
    connect(focusNoteShortcut, &QShortcut::activated, this, [this]() {
        browser->editor->setFocus();
    });

    fileSaveShortcut = new QShortcut(this);
    fileSaveShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusNoteShortcut, "File_Save_Content");
    connect(fileSaveShortcut, &QShortcut::activated, browser, &NBrowserWindow::saveNoteContent);

    focusTagShortcut = new QShortcut(this);
    focusTagShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusTagShortcut, "Focus_Tag");
    connect(focusTagShortcut, &QShortcut::activated, browser, &NBrowserWindow::newTagFocusShortcut);

    focusUrlShortcut = new QShortcut(this);
    focusUrlShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusUrlShortcut, "Focus_Url");
    connect(focusUrlShortcut, &QShortcut::activated, browser, &NBrowserWindow::urlFocusShortcut);

    focusAuthorShortcut = new QShortcut(this);
    focusAuthorShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusAuthorShortcut, "Focus_Author");
    connect(focusAuthorShortcut, &QShortcut::activated, browser, &NBrowserWindow::authorFocusShortcut);

    focusNotebookShortcut = new QShortcut(this);
    focusNotebookShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusNotebookShortcut, "Focus_Notebook");
    connect(focusNotebookShortcut, &QShortcut::activated, browser, &NBrowserWindow::notebookFocusShortcut);

    focusFontShortcut = new QShortcut(this);
    focusFontShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusFontShortcut, "Focus_Font");
    connect(focusFontShortcut, &QShortcut::activated, browser, &NBrowserWindow::fontFocusShortcut);

    focusFontSizeShortcut = new QShortcut(this);
    focusFontSizeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(focusFontSizeShortcut, "Focus_Font_Size");
    connect(focusFontSizeShortcut, &QShortcut::activated, browser, &NBrowserWindow::fontSizeFocusShortcut);

    findShortcut = new QShortcut(this);
    findShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(findShortcut, "Edit_Search_Find");
    connect(findShortcut, &QShortcut::activated, browser, &NBrowserWindow::findShortcut);

    findReplaceShortcut = new QShortcut(this);
    findReplaceShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(findReplaceShortcut, "Edit_Search_Find_Replace");
    connect(findReplaceShortcut, &QShortcut::activated, browser, &NBrowserWindow::findReplaceShortcut);

    findNextShortcut = new QShortcut(this);
    findNextShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(findNextShortcut, "Edit_Search_Find_Next");
    connect(findNextShortcut, &QShortcut::activated, browser, &NBrowserWindow::findNextShortcut);

    findPrevShortcut = new QShortcut(this);
    findPrevShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    global.setupShortcut(findPrevShortcut, "Edit_Search_Find_Prev");
    connect(findPrevShortcut, &QShortcut::activated, browser, &NBrowserWindow::findPrevShortcut);

    QString css = global.getThemeCss("externalNoteWindowCss");
    if (css!="")
        this->setStyleSheet(css);

    // Load actual note
    browser->setContent(lid);
}


// We don't really close the window, we just hide it.  This
// solves problems later on if the user wants to re-open the same
// note in an external window.
void ExternalBrowse::closeEvent(QCloseEvent *closeEvent) {
    if (this->browser->editor->isDirty)
        this->browser->saveNoteContent();
    this->setVisible(false);
    closeEvent->ignore();
}



void ExternalBrowse::setTitle(QString text) {
    this->setWindowTitle(tr(NN_APP_DISPLAY_NAME_GUI " - ") +text);
}



