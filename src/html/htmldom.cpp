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

#include "htmldom.h"

#include <QTextStream>

HtmlDomElement::HtmlDomElement()
{
}

HtmlDomElement::HtmlDomElement(const QDomElement &element) :
    element(element)
{
}

bool HtmlDomElement::isNull() const
{
    return element.isNull();
}

QString HtmlDomElement::tagName() const
{
    return element.tagName();
}

QString HtmlDomElement::attribute(const QString &name, const QString &defaultValue) const
{
    return element.attribute(name, defaultValue);
}

QStringList HtmlDomElement::attributeNames() const
{
    QStringList names;
    QDomNamedNodeMap attributes = element.attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        names.append(attributes.item(i).nodeName());
    }
    return names;
}

bool HtmlDomElement::hasAttribute(const QString &name) const
{
    return element.hasAttribute(name);
}

void HtmlDomElement::setAttribute(const QString &name, const QString &value)
{
    element.setAttribute(name, value);
}

void HtmlDomElement::removeAttribute(const QString &name)
{
    element.removeAttribute(name);
}

HtmlDomElement HtmlDomElement::firstChild() const
{
    return HtmlDomElement(element.firstChildElement());
}

HtmlDomElement HtmlDomElement::lastChild() const
{
    QDomNode child = element.lastChild();
    while (!child.isNull() && !child.isElement()) {
        child = child.previousSibling();
    }
    return HtmlDomElement(child.toElement());
}

HtmlDomElement HtmlDomElement::nextSibling() const
{
    return HtmlDomElement(element.nextSiblingElement());
}

HtmlDomElement HtmlDomElement::findFirst(const QString &tagName) const
{
    return HtmlDomElement(element.elementsByTagName(tagName).at(0).toElement());
}

void HtmlDomElement::appendInside(const QString &markup)
{
    const QList<QDomNode> nodes = parseFragment(markup);
    QDomDocument owner = ownerDocument();
    for (const QDomNode &node : nodes) {
        element.appendChild(owner.importNode(node, true));
    }
}

void HtmlDomElement::removeAllChildren()
{
    while (element.hasChildNodes()) {
        element.removeChild(element.firstChild());
    }
}

void HtmlDomElement::removeFromDocument()
{
    QDomNode parent = element.parentNode();
    if (!parent.isNull()) {
        parent.removeChild(element);
    }
}

void HtmlDomElement::setInnerXml(const QString &markup)
{
    removeAllChildren();
    appendInside(markup);
}

void HtmlDomElement::setOuterXml(const QString &markup)
{
    QDomNode parent = element.parentNode();
    if (parent.isNull()) {
        return;
    }

    const QList<QDomNode> nodes = parseFragment(markup);
    QDomDocument owner = ownerDocument();
    for (const QDomNode &node : nodes) {
        parent.insertBefore(owner.importNode(node, true), element);
    }
    parent.removeChild(element);
}

void HtmlDomElement::setPlainText(const QString &text)
{
    removeAllChildren();
    if (!text.isEmpty()) {
        element.appendChild(ownerDocument().createTextNode(text));
    }
}

QString HtmlDomElement::toInnerXml() const
{
    QString result;
    QTextStream stream(&result);
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        child.save(stream, -1);
        child = child.nextSibling();
    }
    return result;
}

QString HtmlDomElement::toOuterXml() const
{
    QString result;
    QTextStream stream(&result);
    element.save(stream, -1);
    return result;
}

QDomDocument HtmlDomElement::ownerDocument() const
{
    return element.ownerDocument();
}

QList<QDomNode> HtmlDomElement::parseFragment(const QString &markup) const
{
    QList<QDomNode> nodes;

    QDomDocument fragmentDoc;
    QString wrapped = QStringLiteral("<fragment>") + markup + QStringLiteral("</fragment>");
    if (!fragmentDoc.setContent(wrapped)) {
        return nodes;
    }

    QDomNode child = fragmentDoc.documentElement().firstChild();
    while (!child.isNull()) {
        nodes.append(child);
        child = child.nextSibling();
    }
    return nodes;
}

HtmlDomDocument::HtmlDomDocument() :
    doc(QStringLiteral("html"))
{
}

bool HtmlDomDocument::setContent(const QByteArray &content)
{
    return static_cast<bool>(doc.setContent(QString::fromUtf8(content)));
}

HtmlDomElement HtmlDomDocument::documentElement() const
{
    return HtmlDomElement(doc.documentElement());
}

HtmlDomElement HtmlDomDocument::findFirst(const QString &tagName) const
{
    return HtmlDomElement(doc.elementsByTagName(tagName).at(0).toElement());
}

QList<HtmlDomElement> HtmlDomDocument::findAllElements(const QString &tagName) const
{
    QList<HtmlDomElement> result;
    QDomNodeList nodes = doc.elementsByTagName(tagName);
    for (int i = 0; i < nodes.count(); ++i) {
        result.append(HtmlDomElement(nodes.at(i).toElement()));
    }
    return result;
}

QString HtmlDomDocument::toHtml() const
{
    return doc.toString(-1);
}
