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

#include "databasestatus.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>
#include "src/sql/notetable.h"
#include "src/sql/resourcetable.h"
#include "src/global.h"

extern Global global;

namespace {
struct ResourceSizeBreakdown {
    qint64 total = 0;
    qint64 images = 0;
    qint64 pdf = 0;
    qint64 audio = 0;
    qint64 video = 0;
    qint64 other = 0;
};

qint64 fileSize(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return 0;
    }
    return fileInfo.size();
}

qint64 databaseFilesSize()
{
    const QString dbFilePath = global.fileManager.getDbDirPath(NN_NIXNOTE_DATABASE_NAME);
    return fileSize(dbFilePath)
           + fileSize(dbFilePath + QStringLiteral("-wal"))
           + fileSize(dbFilePath + QStringLiteral("-shm"))
           + fileSize(dbFilePath + QStringLiteral("-journal"));
}

qint64 directorySize(const QString &dirPath)
{
    qint64 totalSize = 0;
    QDirIterator iterator(
        dirPath,
        QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
        QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        totalSize += iterator.fileInfo().size();
    }
    return totalSize;
}

bool extensionMatches(const QString &extension, const QStringList &extensions)
{
    return extensions.contains(extension, Qt::CaseInsensitive);
}

ResourceSizeBreakdown resourceFilesSize(const QString &dirPath)
{
    static const QStringList imageExtensions = {
        QStringLiteral("bmp"), QStringLiteral("cgm"), QStringLiteral("djvu"),
        QStringLiteral("gif"), QStringLiteral("heic"), QStringLiteral("heif"),
        QStringLiteral("ico"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("png"), QStringLiteral("psd"), QStringLiteral("svg"),
        QStringLiteral("tif"), QStringLiteral("tiff"), QStringLiteral("webp")
    };
    static const QStringList audioExtensions = {
        QStringLiteral("aac"), QStringLiteral("aif"), QStringLiteral("aiff"),
        QStringLiteral("au"), QStringLiteral("flac"), QStringLiteral("m4a"),
        QStringLiteral("mid"), QStringLiteral("midi"), QStringLiteral("mp3"),
        QStringLiteral("mp4a"), QStringLiteral("oga"), QStringLiteral("ogg"),
        QStringLiteral("opus"), QStringLiteral("wav"), QStringLiteral("weba"),
        QStringLiteral("wma")
    };
    static const QStringList videoExtensions = {
        QStringLiteral("3g2"), QStringLiteral("3gp"), QStringLiteral("asf"),
        QStringLiteral("avi"), QStringLiteral("f4v"), QStringLiteral("flv"),
        QStringLiteral("h261"), QStringLiteral("h263"), QStringLiteral("h264"),
        QStringLiteral("m4v"), QStringLiteral("mkv"), QStringLiteral("mov"),
        QStringLiteral("movie"), QStringLiteral("mp4"), QStringLiteral("mpeg"),
        QStringLiteral("mpg"), QStringLiteral("ogv"), QStringLiteral("qt"),
        QStringLiteral("webm"), QStringLiteral("wmv")
    };

    ResourceSizeBreakdown sizes;
    QDirIterator iterator(
        dirPath,
        QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
        QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        qint64 size = iterator.fileInfo().size();
        QString extension = iterator.fileInfo().suffix().toLower();
        sizes.total += size;

        if (extensionMatches(extension, imageExtensions)) {
            sizes.images += size;
        } else if (extension == QStringLiteral("pdf")) {
            sizes.pdf += size;
        } else if (extensionMatches(extension, audioExtensions)) {
            sizes.audio += size;
        } else if (extensionMatches(extension, videoExtensions)) {
            sizes.video += size;
        } else {
            sizes.other += size;
        }
    }
    return sizes;
}

QString formatSize(qint64 bytes)
{
    return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}
}

DatabaseStatus::DatabaseStatus(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *vBoxLayout = new QVBoxLayout(this);
    // Setup window layout, title, & icon
    this->setWindowTitle(tr("Database Status"));
    QGridLayout *textGrid = new QGridLayout();
    setLayout(vBoxLayout);

    NoteTable ntable(global.db);
    ResourceTable rtable(global.db);
    qint32 totalNotes = ntable.getCount();
    qint32 totalResources = rtable.getCount();
    qint32 unindexedNotes = ntable.getUnindexedCount();
    qint32 unindexedResources = rtable.getUnindexedCount();
    qint32 thumbnailsNeeded = ntable.getThumbnailsNeededCount();
    qint64 databaseSize = databaseFilesSize();
    ResourceSizeBreakdown resourceSizes = resourceFilesSize(global.fileManager.getDbaDirPath());
    qint64 thumbnailsSize = directorySize(global.fileManager.getThumbnailDirPath());
    qint64 accountSize = directorySize(global.fileManager.getDbDirPath(QString()));

    textGrid->addWidget(new QLabel(tr("Total notes:")),1,1);
    textGrid->addWidget(new QLabel(QString::number(totalNotes)), 1,2);
    textGrid->addWidget(new QLabel(tr("Total resources:")), 2,1);
    textGrid->addWidget(new QLabel(QString::number(totalResources)), 2, 2);
    textGrid->addWidget(new QLabel(tr("Unindexed notes:")), 3,1);
    textGrid->addWidget(new QLabel(QString::number(unindexedNotes)),3,2);
    textGrid->addWidget(new QLabel(tr("Unindexed resources:")), 4,1);
    textGrid->addWidget(new QLabel(QString::number(unindexedResources)),4,2);
    textGrid->addWidget(new QLabel(tr("Thumbnails needed:")), 5,1);
    textGrid->addWidget(new QLabel(QString::number(thumbnailsNeeded)),5,2);
    textGrid->addWidget(new QLabel(tr("Database files size:")), 6,1);
    textGrid->addWidget(new QLabel(formatSize(databaseSize)),6,2);
    textGrid->addWidget(new QLabel(tr("Resource files size:")), 7,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.total)),7,2);
    textGrid->addWidget(new QLabel(tr("Image resource files:")), 8,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.images)),8,2);
    textGrid->addWidget(new QLabel(tr("PDF resource files:")), 9,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.pdf)),9,2);
    textGrid->addWidget(new QLabel(tr("Audio resource files:")), 10,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.audio)),10,2);
    textGrid->addWidget(new QLabel(tr("Video resource files:")), 11,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.video)),11,2);
    textGrid->addWidget(new QLabel(tr("Other resource files:")), 12,1);
    textGrid->addWidget(new QLabel(formatSize(resourceSizes.other)),12,2);
    textGrid->addWidget(new QLabel(tr("Thumbnail files size:")), 13,1);
    textGrid->addWidget(new QLabel(formatSize(thumbnailsSize)),13,2);
    textGrid->addWidget(new QLabel(tr("Account data size:")), 14,1);
    textGrid->addWidget(new QLabel(formatSize(accountSize)),14,2);


    QHBoxLayout *buttonLayout = new QHBoxLayout();
    ok = new QPushButton(tr("OK"),this);
    connect(ok, &QPushButton::clicked, this, &DatabaseStatus::okPushed);
    buttonLayout->addStretch();
    buttonLayout->addWidget(ok);
    buttonLayout->addStretch();

    vBoxLayout->addLayout(textGrid);
    vBoxLayout->addLayout(buttonLayout);
    this->setFont(global.getGuiFont(font()));
}



// OK button pushed, close the window
void DatabaseStatus::okPushed() {
    this->close();
}
