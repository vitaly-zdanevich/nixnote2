/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2026 NixNote contributors

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

#ifndef HTMLDOM_H
#define HTMLDOM_H

#include <QDomDocument>
#include <QList>
#include <QString>
#include <QStringList>

class HtmlDomElement
{
public:
    HtmlDomElement();
    explicit HtmlDomElement(const QDomElement &element);

    bool isNull() const;
    QString tagName() const;

    QString attribute(const QString &name, const QString &defaultValue = QString()) const;
    QStringList attributeNames() const;
    bool hasAttribute(const QString &name) const;
    void setAttribute(const QString &name, const QString &value);
    void removeAttribute(const QString &name);

    HtmlDomElement firstChild() const;
    HtmlDomElement lastChild() const;
    HtmlDomElement nextSibling() const;
    HtmlDomElement findFirst(const QString &tagName) const;

    void appendInside(const QString &markup);
    void removeAllChildren();
    void removeFromDocument();
    void setInnerXml(const QString &markup);
    void setOuterXml(const QString &markup);
    void setPlainText(const QString &text);

    QString toInnerXml() const;
    QString toOuterXml() const;

private:
    QDomElement element;

    QDomDocument ownerDocument() const;
    QList<QDomNode> parseFragment(const QString &markup) const;
};

class HtmlDomDocument
{
public:
    HtmlDomDocument();

    bool setContent(const QByteArray &content);
    HtmlDomElement documentElement() const;
    HtmlDomElement findFirst(const QString &tagName) const;
    QList<HtmlDomElement> findAllElements(const QString &tagName) const;
    QString toHtml() const;

private:
    QDomDocument doc;
};

#endif // HTMLDOM_H
