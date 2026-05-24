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

#include "ntableviewheader.h"
#include "src/global.h"

extern Global global;

//************************************************************
//* This overrides QHeaderView and is used so we can have
//* things like custom context menus
//************************************************************
NTableViewHeader::NTableViewHeader(Qt::Orientation orientation, QWidget *parent) :
    QHeaderView(orientation, parent) {

    setSectionsClickable(true);
    setSortIndicatorShown(true);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    createdDateAction = new QAction(this);
    createdDateAction->setText(tr("Date Created"));
    createdDateAction->setCheckable(true);
    addAction(createdDateAction);

    changedDateAction = new QAction(this);
    changedDateAction->setText(tr("Date Updated"));
    changedDateAction->setCheckable(true);
    addAction(changedDateAction);

    titleAction = new QAction(this);
    titleAction->setText(tr("Title"));
    titleAction->setCheckable(true);
    addAction(titleAction);

    notebookAction = new QAction(this);
    notebookAction->setText(tr("Notebook"));
    notebookAction->setCheckable(true);
    addAction(notebookAction);

    tagsAction = new QAction(this);
    tagsAction->setText(tr("Tags"));
    tagsAction->setCheckable(true);
    addAction(tagsAction);

    authorAction = new QAction(this);
    authorAction->setText(tr("Author"));
    authorAction->setCheckable(true);
    addAction(authorAction);

    subjectDateAction = new QAction(this);
    subjectDateAction->setText(tr("Subject Date"));
    subjectDateAction->setCheckable(true);
    addAction(subjectDateAction);

    sourceAction = new QAction(this);
    sourceAction->setText(tr("Source"));
    sourceAction->setCheckable(true);
    addAction(sourceAction);

    urlAction = new QAction(this);
    urlAction->setText(tr("Source URL"));
    urlAction->setCheckable(true);
    addAction(urlAction);

    latitudeAction = new QAction(this);
    latitudeAction->setText(tr("Latitude"));
    latitudeAction->setCheckable(true);
    addAction(latitudeAction);

    longitudeAction = new QAction(this);
    longitudeAction->setText(tr("Longitude"));
    longitudeAction->setCheckable(true);
    addAction(longitudeAction);

    altitudeAction = new QAction(this);
    altitudeAction->setText(tr("Altitude"));
    altitudeAction->setCheckable(true);
    addAction(altitudeAction);

    hasEncryptionAction = new QAction(this);
    hasEncryptionAction->setText(tr("Has Encryption"));
    hasEncryptionAction->setCheckable(true);
    addAction(hasEncryptionAction);

    hasTodoAction = new QAction(this);
    hasTodoAction->setText(tr("Has To-do"));
    hasTodoAction->setCheckable(true);
    addAction(hasTodoAction);

    synchronizedAction = new QAction(this);
    synchronizedAction->setText(tr("Synchronized"));
    synchronizedAction->setCheckable(true);
    addAction(synchronizedAction);

    sizeAction = new QAction(this);
    sizeAction->setText(tr("Size"));
    sizeAction->setCheckable(true);
    addAction(sizeAction);

    reminderOrderAction = new QAction(this);
    reminderOrderAction->setText(tr("Reminder"));
    reminderOrderAction->setCheckable(true);
    reminderOrderAction->setVisible(false);  // Hidden because it isn't really needed
    addAction(reminderOrderAction);

    reminderTimeAction = new QAction(this);
    reminderTimeAction->setText(tr("Reminder Due"));
    reminderTimeAction->setCheckable(true);
    addAction(reminderTimeAction);

    reminderTimeDoneAction = new QAction(this);
    reminderTimeDoneAction->setText(tr("Reminder Completed"));
    reminderTimeDoneAction->setCheckable(true);
    addAction(reminderTimeDoneAction);

    pinnedAction = new QAction(this);
    pinnedAction->setText(tr("Pinned"));
    pinnedAction->setCheckable(true);
    addAction(pinnedAction);

    thumbnailAction = new QAction(this);
    thumbnailAction->setText(tr("Thumbnail"));
    thumbnailAction->setCheckable(true);
    addAction(thumbnailAction);

    relevanceAction = new QAction(this);
    relevanceAction->setText(tr("Relevance"));
    relevanceAction->setCheckable(true);
    addAction(relevanceAction);


    this->setMouseTracking(true);

    connect(this, &QHeaderView::sortIndicatorChanged, this, &NTableViewHeader::saveSort);

    connect(createdDateAction, &QAction::toggled, this, &NTableViewHeader::createdDateChecked);
    connect(titleAction, &QAction::toggled, this, &NTableViewHeader::titleChecked);
    connect(changedDateAction, &QAction::toggled, this, &NTableViewHeader::changedDateChecked);
    connect(subjectDateAction, &QAction::toggled, this, &NTableViewHeader::subjectDateChecked);
    connect(notebookAction, &QAction::toggled, this, &NTableViewHeader::notebookChecked);
    connect(tagsAction, &QAction::toggled, this, &NTableViewHeader::tagsChecked);
    connect(urlAction, &QAction::toggled, this, &NTableViewHeader::urlChecked);
    connect(authorAction, &QAction::toggled, this, &NTableViewHeader::authorChecked);
    connect(hasTodoAction, &QAction::toggled, this, &NTableViewHeader::hasTodoChecked);
    connect(hasEncryptionAction, &QAction::toggled, this, &NTableViewHeader::hasEncryptionChecked);
    connect(sizeAction, &QAction::toggled, this, &NTableViewHeader::sizeChecked);
    connect(thumbnailAction, &QAction::toggled, this, &NTableViewHeader::thumbnailChecked);
    connect(relevanceAction, &QAction::toggled, this, &NTableViewHeader::relevanceChecked);
    connect(latitudeAction, &QAction::toggled, this, &NTableViewHeader::latitudeChecked);
    connect(longitudeAction, &QAction::toggled, this, &NTableViewHeader::longitudeChecked);
    connect(altitudeAction, &QAction::toggled, this, &NTableViewHeader::altitudeChecked);
    connect(synchronizedAction, &QAction::toggled, this, &NTableViewHeader::synchronizedChecked);
    connect(sourceAction, &QAction::toggled, this, &NTableViewHeader::sourceChecked);
    connect(reminderTimeAction, &QAction::toggled, this, &NTableViewHeader::reminderTimeChecked);
    connect(reminderTimeDoneAction, &QAction::toggled, this, &NTableViewHeader::reminderTimeDoneChecked);
    connect(reminderOrderAction, &QAction::toggled, this, &NTableViewHeader::reminderOrderChecked);
    connect(pinnedAction, &QAction::toggled, this, &NTableViewHeader::pinnedChecked);

    this->setFont(global.getGuiFont(font()));

    QString css = global.getThemeCss("noteTableViewHeaderCss");
    if (css != "")
        this->setStyleSheet(css);

    this->setDefaultAlignment(Qt::AlignLeft);
}


void NTableViewHeader::saveSort(int logicalIndex, Qt::SortOrder order) {
    global.settings->beginGroup(INI_GROUP_SAVE_STATE);
    global.settings->setValue("sortOrder", order);
    global.settings->setValue("sortColumn", logicalIndex);
    global.settings->endGroup();
}


void NTableViewHeader::saveHeader() {
    QByteArray state = this->saveState();
    QByteArray geometry = this->saveGeometry();
    global.settings->beginGroup(INI_GROUP_SAVE_STATE);
    global.settings->setValue("listViewWideHeaderState", state);
    global.settings->setValue("listViewWideHeaderGeometry", geometry);
    global.settings->endGroup();
}


void NTableViewHeader::checkActions() {
    int count = 0;
    QList<QAction*> actions = this->actions();
    for (int i=0; i<actions.size(); i++) {
        if (actions[i]->isChecked())
            count++;
        actions[i]->setEnabled(true);
    }
    if (count <= 1) {
        for (int i=0; i<actions.size(); i++) {
            if (actions[i]->isChecked())
                actions[i]->setEnabled(false);
        }
    }
}


void NTableViewHeader::createdDateChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_DATE_CREATED_POSITION, checked));
    checkActions();
}

void NTableViewHeader::changedDateChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_DATE_UPDATED_POSITION, checked));
    checkActions();
}
void NTableViewHeader::subjectDateChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_DATE_SUBJECT_POSITION, checked));
    checkActions();
}
void NTableViewHeader::tagsChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_TAGS_POSITION, checked));
    checkActions();
}
void NTableViewHeader::titleChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_TITLE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::notebookChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_NOTEBOOK_POSITION, checked));
    checkActions();
}
void NTableViewHeader::synchronizedChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_IS_DIRTY_POSITION, checked));
    checkActions();
}
void NTableViewHeader::sourceChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_SOURCE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::authorChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_AUTHOR_POSITION, checked));
    checkActions();
}
void NTableViewHeader::urlChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_SOURCE_URL_POSITION, checked));
    checkActions();
}
void NTableViewHeader::altitudeChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_ALTITUDE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::longitudeChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_LONGITUDE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::latitudeChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_LATITUDE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::hasTodoChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_HAS_TODO_POSITION, checked));
    checkActions();
}
void NTableViewHeader::hasEncryptionChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_HAS_ENCRYPTION_POSITION, checked));
    checkActions();
}
void NTableViewHeader::sizeChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_SIZE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::thumbnailChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_THUMBNAIL_POSITION, checked));
    checkActions();
    global.disableThumbnails = !checked;
}
void NTableViewHeader::relevanceChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_SEARCH_RELEVANCE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::reminderTimeChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_REMINDER_TIME_POSITION, checked));
    checkActions();
}
void NTableViewHeader::reminderTimeDoneChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_REMINDER_TIME_DONE_POSITION, checked));
    checkActions();
}
void NTableViewHeader::reminderOrderChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_REMINDER_ORDER_POSITION, checked));
    checkActions();
}

void NTableViewHeader::pinnedChecked(bool checked) {
    emit (setColumnVisible(NOTE_TABLE_PINNED_POSITION, checked));
    checkActions();
}

bool NTableViewHeader::isThumbnailVisible() {
    return thumbnailAction->isChecked();
}

bool NTableViewHeader::isRelevanceVisible() {
    return relevanceAction->isChecked();
}
