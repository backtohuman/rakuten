#include "stdafx.h"

#include "rakuten.hpp"


#define deserialize_str(obj, x)			this->x = obj.value(#x).toString()
#define deserialize_int(obj, x)			this->x = obj.value(#x).toInt()
#define deserialize_datetime(obj, x)	this->x = QDateTime::fromString(obj.value(#x).toString(), Qt::ISODate)

#define serialize_var(obj, x)			obj.insert(#x, x)
#define serialize_datetime(obj, x)		obj.insert(#x, x.toString(Qt::ISODate))


namespace item::search
{
	item::item()
	{
		this->itemPrice = 0;
		this->genreId = 0;
	}

	item::item(const QDomNode& n)
	{
		this->read(n);
	}

	void item::readImages(const QDomElement& e)
	{
		this->images.clear();
		for (QDomNode n = e.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			const QDomElement e = n.toElement();
			if (e.tagName() == QLatin1Literal("image"))
			{
				this->images.push_back(e.text());
			}
			else
			{
				qWarning() << "weird.";
			}
		}
	}

	void item::read(const QDomNode& itemNode)
	{
		Q_ASSERT(itemNode.nodeName() == QLatin1Literal("item"));

		for (QDomNode n = itemNode.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			const QDomElement e = n.toElement();
			if (e.tagName() == QLatin1Literal("itemUrl"))			this->itemUrl = e.text();
			else if (e.tagName() == QLatin1Literal("itemNumber"))	this->itemNumber = e.text();
			else if (e.tagName() == QLatin1Literal("itemName"))		this->itemName = e.text();
			else if (e.tagName() == QLatin1Literal("itemPrice"))	this->itemPrice = e.text().toInt();
			else if (e.tagName() == QLatin1Literal("genreId"))		this->genreId = e.text().toInt();
			else if (e.tagName() == QLatin1Literal("registDate"))	this->registDate = QDateTime::fromString(e.text(), Qt::ISODateWithMs);
			else if (e.tagName() == QLatin1Literal("images"))		this->readImages(e);
		}
	}

	void item::write(QXmlStreamWriter& stream) const
	{
		stream.writeStartElement("item");
		stream.writeTextElement("itemUrl", this->itemUrl);
		stream.writeTextElement("itemNumber", this->itemNumber);
		stream.writeTextElement("itemName", this->itemName);
		stream.writeTextElement("itemPrice", QString::number(this->itemPrice));
		stream.writeTextElement("genreId", QString::number(this->genreId));
		stream.writeTextElement("registDate", this->registDate.toString(Qt::ISODateWithMs));
		stream.writeStartElement("images");
		for (const QString& image : this->images)
		{
			stream.writeTextElement("image", image);
		}
		stream.writeEndElement();
		stream.writeEndElement();
	}
}

namespace rpay
{
	// getOrder struct with all personal information excluded
	namespace getOrder
	{
		item::item(const QJsonObject& obj)
		{
			this->deserialize(obj);
		}

		void item::deserialize(const QJsonObject& obj)
		{
			deserialize_int(obj, itemId);
			deserialize_int(obj, price);
			deserialize_int(obj, units);
			deserialize_str(obj, itemName);
			deserialize_str(obj, itemNumber);
			deserialize_str(obj, manageNumber);
		}
		QJsonObject item::serialize() const
		{
			QJsonObject obj;
			serialize_var(obj, itemId);
			serialize_var(obj, price);
			serialize_var(obj, units);
			serialize_var(obj, itemName);
			serialize_var(obj, itemNumber);
			serialize_var(obj, manageNumber);
			return obj;
		}


		// OrderModel
		OrderModel::OrderModel(const QJsonObject& obj)
		{
			// uh....
			if (obj.contains("PackageModelList"))
			{
				this->orderNumber = obj["orderNumber"].toString();
				this->orderDatetime = QDateTime::fromString(obj["orderDatetime"].toString(), Qt::ISODate);
				const QJsonArray PackageModelList = obj["PackageModelList"].toArray();
				for (const QJsonValue& _package : PackageModelList)
				{
					const QJsonObject package = _package.toObject();
					const QJsonArray ItemModelList = package["ItemModelList"].toArray();
					for (const QJsonValue& _item : ItemModelList)
					{
						this->items.push_back(
							QSharedPointer<rpay::getOrder::item>::create(_item.toObject())
						);
					}
				}
			}
			else
			{
				this->deserialize(obj);
			}
		}

		void OrderModel::deserialize(const QJsonObject& obj)
		{
			deserialize_str(obj, orderNumber);
			deserialize_datetime(obj, orderDatetime);

			this->items.clear();
			for (const QJsonValue& item : obj["items"].toArray())
			{
				this->items.push_back(
					QSharedPointer<rpay::getOrder::item>::create(item.toObject()));
			}
		}

		QJsonObject OrderModel::serialize()
		{
			QJsonObject obj;
			serialize_var(obj, orderNumber);
			serialize_datetime(obj, orderDatetime);

			QJsonArray json_items;
			for (const auto item : items)
			{
				json_items.append(item->serialize());
			}
			obj.insert("items", json_items);

			return obj;
		}
	}
}