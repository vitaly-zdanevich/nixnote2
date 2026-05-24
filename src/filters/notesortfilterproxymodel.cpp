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

#include "notesortfilterproxymodel.h"
#include "src/global.h"

NoteSortFilterProxyModel::NoteSortFilterProxyModel() :
    QSortFilterProxyModel(),
    lidMap(std::make_unique<QMap<qint32, qint32>>()) {
}


NoteSortFilterProxyModel::~NoteSortFilterProxyModel() = default;


bool NoteSortFilterProxyModel::filterAcceptsRow(qint32 source_row, const QModelIndex &source_parent) const {

    // index(row, column, parent_index) Returns the index of the item in the model specified by the
    // given row, column and parent index.
    QModelIndex idx = sourceModel()->index(source_row, NOTE_TABLE_LID_POSITION, source_parent);
    qint32 rowLid = sourceModel()->data(idx).toInt();
    if (lidMap->contains(rowLid)) {
        //QLOG_DEBUG() << "filterAcceptsRow lid=" << rowLid << "OK";
        lidMap->remove(rowLid);
        lidMap->insert(rowLid, source_row);
        return true;
    }
    //QLOG_DEBUG() << "filterAcceptsRow lid=" << rowLid << "failed";
    return false;
}


// obsolete/unused

bool NoteSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    QLOG_DEBUG() << "lessThan";
    QVariant leftData = this->sourceModel()->data(left);
    QVariant rightData = this->sourceModel()->data(right);

    if (left.column() == NOTE_TABLE_IS_DIRTY_POSITION)
        return leftData.toBool() < rightData.toBool();

    const int leftType = leftData.typeId();
    const int rightType = rightData.typeId();

    if (leftType == QMetaType::UnknownType || rightType == QMetaType::UnknownType)
        return true;

    if (leftType == QMetaType::QIcon || rightType == QMetaType::QIcon)
        return true;

    if (leftType == QMetaType::QImage || rightType == QMetaType::QImage)
        return true;

    if (leftType == QMetaType::QString && rightType == QMetaType::QString) {
        QString left = leftData.toString();
        QString right = rightData.toString();
        return left.toLower() < right.toLower();
    }

    if (leftType == QMetaType::Int && rightType == QMetaType::Int)
        return leftData.toInt() < rightData.toInt();

    if (leftType == QMetaType::LongLong && rightType == QMetaType::LongLong)
        return leftData.toLongLong() < rightData.toLongLong();

    if (leftType == QMetaType::Double && rightType == QMetaType::Double)
        return leftData.toDouble() < rightData.toDouble();

    return true;
}
