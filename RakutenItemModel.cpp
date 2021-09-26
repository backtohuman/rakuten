#include "stdafx.h"

#include "RakutenItemModel.hpp"
#include "rakuten_client.hpp"
#include "qt_helper.hpp"
#include "settings.hpp"


QSharedPointer<rakuten_item> item_from_obj(QSharedPointer<rakuten_item> item, const QJsonObject& obj)
{
	item->itemUrl = obj.value("itemUrl").toString();
	item->itemNumber = obj.value("itemNumber").toString();
	item->itemName = obj.value("itemName").toString();
	item->itemPrice = obj.value("itemPrice").toString().toInt();
	item->genreId = obj.value("genreId").toString().toInt();
	item->registDate = QDateTime::fromString(obj.value("registDate").toString(), Qt::ISODateWithMs);
	item->images.clear();
	for (const auto& v : obj.value("images").toObject())
	{
		item->images.push_back(v.toString());
	}
	return item;
}


RakutenItemModel::RakutenItemModel(QObject* parent) : QAbstractTableModel(parent)
{
}
RakutenItemModel::~RakutenItemModel()
{
}

int RakutenItemModel::rowCount(const QModelIndex& parent) const
{
	return this->m_items.size();
}
int RakutenItemModel::columnCount(const QModelIndex& parent) const
{
	return 5;
}

QVariant RakutenItemModel::data(const QModelIndex& index, int role) const
{
	if (index.row() >= this->m_items.size())
		return QVariant();

	const auto& item = this->m_items[index.row()];
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
			case 0: return item->itemUrl;
			case 1: return item->itemName;
			case 2: return QLocale::system().toString(item->itemPrice).append(from_u8(u8" ‰~"));
			case 3: return item->genreId;
			case 4:
			{
				const qint64 secs = item->registDate.secsTo(QDateTime::currentDateTime());
				if (secs < 60)					return from_u8(u8"%1•b‘O").arg(secs);
				else if (secs < 60 * 60)		return from_u8(u8"%1•ª‘O").arg(secs / 60);
				else if (secs < 60 * 60 * 24)	return from_u8(u8"%1ŽžŠÔ‘O").arg(secs / 60 / 60);
				else							return from_u8(u8"%1“ú‘O").arg(secs / 60 / 60 / 24);
			}
		}
	}
	else if (role == Qt::DecorationRole)
	{
		if (index.column() == 0)
		{
			/*if (item.detail.loaded)
				return QIcon(":/images/icons8-checked-48.png");
			else if (item.search.loaded)
				return QIcon(":/images/icons8-loading-48.png");*/
			return QIcon();
		}
	}
	else if (role == Qt::TextColorRole)
	{
		/*if (item._error)
			return QBrush(Qt::red);*/
	}
	else if (role == LinkRole)
	{
		return QString("https://item.rakuten.co.jp/%1/%2").arg(m_shopCode).arg(item->itemUrl);
	}
	else if (role == SortRole)
	{
		switch (index.column())
		{
			case 0: return item->itemUrl;
			case 1: return item->itemName;
			case 2: return item->itemPrice;
			case 3: return item->genreId;
			case 4: return item->registDate;
		}

	}
	else if (role == CodeRole)
	{
		return item->itemUrl;
	}
	else if (role == SoldoutRole)
	{
		//return item.search.item.soldout;
	}
	else if (role == PriceRole)
	{
		//return item.search.item.price;
	}
	else if (role == TitleRole)
	{
		//return item.search.item.title;
	}

	// runtime
	else if (role == NewRole)
	{
		//return item._new;
	}
	else if (role == ChangedRole)
	{
		//return item._changed;
	}
	else if (role == ErrorRole)
	{
		//return item._error;
	}

	return QVariant();
}
QVariant RakutenItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
			case 0: return from_u8(u8"¤•iŠÇ—”Ô†");
			case 1: return from_u8(u8"¤•i–¼");
			case 2: return from_u8(u8"‰¿Ši");
			case 3: return from_u8(u8"ƒWƒƒƒ“ƒ‹ID");
			case 4: return from_u8(u8"“o˜^“ú");
		}
	}
	else if (orientation == Qt::Vertical && role == Qt::DisplayRole)
	{
		// return section + 1;
	}

	return QVariant();
}


void RakutenItemModel::save(const QDir& dir)
{
	// orders
	QSettings settings(dir.filePath("orders.ini"), QSettings::IniFormat);
	for (auto it = this->m_orders.begin(); it != this->m_orders.end(); ++it)
	{
		settings.beginGroup(it.key());
		settings.beginWriteArray("orders");
		const auto& vec = it.value();
		for (int i = 0; i < vec.size(); i++)
		{
			settings.setArrayIndex(i);
			vec[i]->serialize(settings);
		}
		settings.endArray();
		settings.endGroup();
	}


	// shop items
	QJsonObject obj;
	for (auto it = this->m_shopItems.begin(); it != this->m_shopItems.end(); ++it)
	{
		const QString& shopCode = it.key();
		const QMap<QString, QJsonObject>& items = it.value();

		QJsonObject save;
		for (auto it = items.begin(); it != items.end(); ++it)
		{
			save.insert(it.key(), it.value());
		}
		obj[shopCode] = save;
	}

	QFile f_out(dir.filePath("item.search.json"));
	if (f_out.open(QIODevice::WriteOnly))
	{
		QJsonDocument doc(obj);
		f_out.write(doc.toJson(QJsonDocument::Compact));
	}
}
void RakutenItemModel::load(const QDir& dir)
{
	QSettings settings(dir.filePath("orders.ini"), QSettings::IniFormat);

	// orders
	const QStringList orderNumberList = settings.childGroups();
	for (const QString& orderNumber : orderNumberList)
	{
		auto& vec = this->m_orders[orderNumber];
		settings.beginGroup(orderNumber);
		const int size = settings.beginReadArray("orders");
		for (int i = 0; i < size; i++)
		{
			settings.setArrayIndex(i);

			QSharedPointer<getOrder_resp> resp = QSharedPointer<getOrder_resp>::create();
			resp->deserialize(settings);
			vec.push_back(std::move(resp));
		}
		settings.endArray();
		settings.endGroup();
	}

	// shopItems
	QFile f_in(dir.filePath("item.search.json"));
	if (!f_in.open(QIODevice::ReadOnly))
		return;

	const QJsonObject all_obj = QJsonDocument::fromJson(f_in.readAll()).object();
	this->m_shopItems.clear();
	for (const QString& key : all_obj.keys())
	{
		QMap<QString, QJsonObject> shopData;
		if (key != this->m_shopCode)
		{
			// shopCode : { id : { data }, ... }
			const QJsonObject d = all_obj.value(key).toObject();
			for (const QString& key : d.keys())
				shopData[key] = d.value(key).toObject();
		}
		else
		{
			this->beginResetModel();

			this->m_items.clear();

			// shopCode : { id : { data }, ... }
			const QJsonObject d = all_obj.value(key).toObject();
			for (const QString& key : d.keys())
			{
				const QJsonObject obj = d.value(key).toObject();
				shopData[key] = obj;

				QSharedPointer<rakuten_item> item = QSharedPointer<rakuten_item>::create();
				item_from_obj(item, obj);
				this->m_items.push_back(std::move(item));
			}

			this->endResetModel();
		}

		this->m_shopItems[key] = shopData;
	}
}


bool RakutenItemModel::containsOrder(const QString& orderNumber)
{
	return this->m_orders.contains(orderNumber);
}
void RakutenItemModel::insertOrder(const QString& orderNumber, QVector<QSharedPointer<getOrder_resp>>&& orders)
{
	this->m_orders[orderNumber] = std::move(orders);
}


bool RakutenItemModel::contains(const QString& shopCode, const QString& manageNumber) const
{
	auto it = this->m_shopItems.find(shopCode);
	return it != this->m_shopItems.end() && it->contains(manageNumber);
}
void RakutenItemModel::addItem(const QString& shopCode, const QJsonObject& obj)
{
	if (!obj.contains("itemUrl"))
		return;

	bool replace = false;
	const QString itemUrl = obj["itemUrl"].toString();
	if (this->m_shopItems[shopCode].contains(itemUrl))
	{
		// update
		replace = true;
	}

	this->m_shopItems[shopCode][itemUrl] = obj;
	if (this->m_shopCode == shopCode)
	{
		if (replace)
		{
			for (int i = 0; i < this->m_items.size(); i++)
			{
				auto& item = this->m_items[i];
				if (item->itemUrl == itemUrl)
				{
					item_from_obj(item, obj);
					emit this->dataChanged(this->index(i, 0), this->index(i, this->columnCount() - 1));
					break;
				}
			}
		}
		else
		{
			// emit signal
			this->beginInsertRows(QModelIndex(), this->m_items.count(), this->m_items.count());

			QSharedPointer<rakuten_item> item = QSharedPointer<rakuten_item>::create();
			item_from_obj(item, obj);
			this->m_items.push_back(std::move(item));

			// emit signal
			this->endInsertRows();
		}
	}
}


QVector<QSharedPointer<rakuten_item>> RakutenItemModel::ranking(const QString& shopCode)
{
	const int ranking_max_size = 3 * 3;
	QVector<QSharedPointer<rakuten_item>> _items;
	if (!m_shopItems.contains(shopCode))
	{
		return _items;
	}

	const QDateTime _30daysb4 = QDateTime::currentDateTime().addDays(-30);

	std::unordered_map<QString, int> ranking_map;
	for (const auto& orderedItems : this->m_orders)
	{
		for (const auto item : orderedItems)
		{
			if (_30daysb4 <= item->orderDatetime)
			{
				ranking_map[item->manageNumber] += item->units;
			}
		}
	}

	QVector<QPair<QString, int>> v;
	for (const auto& pair : ranking_map)
	{
		v.push_back({ pair.first, pair.second });
	}

	qSort(v.begin(), v.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b)
	{
		return a.second > b.second;
	});


	bool _done = false;
	for (const auto& pair : v)
	{
		if (_done)
			break;

		// error or what
		for (auto item : this->m_items)
		{
			if (item->itemUrl == pair.first)
			{
				qDebug() << item->itemUrl << item->images[0];
				_items.push_back(item);
				if (ranking_max_size <= _items.size())
				{
					// break the main loop
					_done = true;
				}

				break;
			}
		}
	}

	if (_items.size() > ranking_max_size)
	{
		_items.resize(ranking_max_size);
	}
	return _items;
}
QVector<QSharedPointer<rakuten_item>> RakutenItemModel::new_items(const QString& shopCode)
{
	QVector<QSharedPointer<rakuten_item>> _items;
	if (!m_shopItems.contains(shopCode))
	{
		return _items;
	}

	// sort by registDate
	_items = this->m_items.toVector();
	qSort(_items.begin(), _items.end(), [](const QSharedPointer<rakuten_item>& a, const QSharedPointer<rakuten_item>& b)
	{
		return a->registDate > b->registDate;
	});

	const int newitem_max_size = 3 * 3;
	if (_items.size() > newitem_max_size)
	{
		_items.resize(newitem_max_size);
	}
	return _items;
}