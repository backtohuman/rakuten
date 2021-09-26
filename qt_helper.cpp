#include "stdafx.h"

#include "qt_helper.hpp"


QDir openSubDir(const QDir& parent, const QString& dirName)
{
	QDir ret = parent;
	if (!ret.cd(dirName))
	{
		ret.mkdir(dirName);
		ret.cd(dirName);
	}
	return ret;
}


void xml_to_json(const QDomNode& node, QJsonObject& obj)
{
	const QDomNodeList children = node.childNodes();
	for (int i = 0; i < children.count(); i++)
	{
		const QDomNode child = children.at(i);
		if (child.isElement())
		{
			if (!child.hasChildNodes())
				continue;

			const QDomNodeList children2 = child.childNodes();
			if (children2.size() == 1 && children2.item(0).isText())
			{
				const QDomText text = children2.item(0).toText();

				const QString key = child.nodeName();
				if (obj.contains(key))
				{
					// no overwrite?
					//obj.insert(child.nodeName(), text.data());
				}
				else
				{
					obj.insert(child.nodeName(), text.data());
				}
			}
			else
			{
				QJsonObject sub;
				xml_to_json(child, sub);
				const QString key = child.nodeName();
				if (obj.contains(key))
				{
					// no overwrite?
					//obj.insert(child.nodeName(), sub);
				}
				else
				{
					obj.insert(child.nodeName(), sub);
				}
			}
		}
	}
}