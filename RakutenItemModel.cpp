#include "stdafx.h"

#include "RakutenItemModel.hpp"
#include "rakuten_client.hpp"
#include "qt_helper.hpp"
#include "settings.hpp"


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

	const auto item = this->m_items[index.row()];
	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
			case 0: return item->itemUrl;
			case 1: return item->itemName;
			case 2: return QLocale::system().toString(item->itemPrice).append(from_u8(u8" â~"));
			case 3: return item->genreId;
			case 4:
			{
				const qint64 secs = item->registDate.secsTo(QDateTime::currentDateTime());
				if (secs < 60)					return from_u8(u8"%1ïbëO").arg(secs);
				else if (secs < 60 * 60)		return from_u8(u8"%1ï™ëO").arg(secs / 60);
				else if (secs < 60 * 60 * 24)	return from_u8(u8"%1éûä‘ëO").arg(secs / 60 / 60);
				else							return from_u8(u8"%1ì˙ëO").arg(secs / 60 / 60 / 24);
			}
		}
	}
	else if (role == Qt::DecorationRole)
	{
		if (index.column() == 0)
		{
			return QIcon();
		}
	}
	else if (role == Qt::TextColorRole)
	{
		// textcolor
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

	return QVariant();
}
QVariant RakutenItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
			case 0: return from_u8(u8"è§ïiä«óùî‘çÜ");
			case 1: return from_u8(u8"è§ïiñº");
			case 2: return from_u8(u8"âøäi");
			case 3: return from_u8(u8"ÉWÉÉÉìÉãID");
			case 4: return from_u8(u8"ìoò^ì˙");
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
	{
		QFile f_out(dir.filePath(ORDERS_FILE_NAME));
		if (!f_out.open(QIODevice::WriteOnly))
		{
			qWarning() << from_u8(u8"íçï∂èÓïÒÇÃï€ë∂Ç…é∏îsÇµÇ‹ÇµÇΩÅB");
		}
		else
		{
			QJsonObject obj;
			for (auto it = this->m_orders.begin(); it != this->m_orders.end(); ++it)
			{
				obj.insert(it.key(), it.value()->serialize());
			}

			f_out.write(QJsonDocument(obj).toJson());
		}
	}

	// ItemAPI item.search
	QFile f_out(dir.filePath("item.search.xml"));
	if (!f_out.open(QIODevice::WriteOnly))
	{
		qWarning() << from_u8(u8"item.search.xmlÇÃï€ë∂Ç…é∏îsÇµÇ‹ÇµÇΩÅB");
		return;
	}

	QXmlStreamWriter stream(&f_out);
	stream.writeStartDocument();
	stream.writeStartElement("data");
	for (auto it = this->m_allItems.begin(); it != this->m_allItems.end(); ++it)
	{
		const QString& shopCode = it.key();
		const auto& items = it.value();

		stream.writeStartElement("items");
		stream.writeAttribute("shopCode", shopCode);
		for (const auto item : items)
		{
			item->write(stream);
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
}
void RakutenItemModel::load(const QDir& dir)
{
	// orders
	{
		this->m_orders.clear();

		QFile f_in(dir.filePath(ORDERS_FILE_NAME));
		if (f_in.open(QIODevice::ReadOnly))
		{
			const QJsonObject obj = QJsonDocument::fromJson(f_in.readAll()).object();
			for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
			{
				this->m_orders[it.key()] = QSharedPointer<rpay::getOrder::OrderModel>::create(it.value().toObject());
			}
		}
	}

	// shopItems
	this->m_allItems.clear();
	QFile f_in(dir.filePath("item.search.xml"));
	if (!f_in.open(QIODevice::ReadOnly))
		return;

	QDomDocument doc;
	doc.setContent(f_in.readAll());
	const QDomNodeList itemsList = doc.documentElement().elementsByTagName("items");
	for (int i = 0; i < itemsList.count(); i++)
	{
		QMap<QString, item::search::item_ptr> m;
		const QDomNode itemsNode = itemsList.at(i);
		const QString shopCode = itemsNode.attributes().namedItem("shopCode").nodeValue();
		const QDomNodeList itemList = itemsNode.toElement().elementsByTagName("item");
		for (int i = 0; i < itemList.count(); i++)
		{
			item::search::item_ptr item = item::search::item_ptr::create(itemList.at(i));
			m[item->itemUrl] = item;
		}

		this->m_allItems[shopCode] = m;
	}


	// notify changes
	this->beginResetModel();

	this->m_items.clear();
	for (auto it = this->m_allItems.begin(); it != this->m_allItems.end(); ++it)
	{
		if (it.key() == this->m_shopCode)
		{
			for (auto item : it.value())
			{
				this->m_items.push_back(item);
			}
		}
	}

	this->endResetModel();
}


void RakutenItemModel::setShopCode(const QString& shopCode)
{
	if (shopCode == this->m_shopCode)
		return;

	this->m_shopCode = shopCode;

	// notify changes
	this->beginResetModel();

	this->m_items.clear();
	for (auto it = this->m_allItems.begin(); it != this->m_allItems.end(); ++it)
	{
		if (it.key() == this->m_shopCode)
		{
			for (auto item : it.value())
			{
				this->m_items.push_back(item);
			}
		}
	}

	this->endResetModel();
}


bool RakutenItemModel::containsOrder(const QString& orderNumber)
{
	return this->m_orders.contains(orderNumber);
}
void RakutenItemModel::insertOrder(const QString& orderNumber, QSharedPointer<rpay::getOrder::OrderModel> order)
{
	this->m_orders[orderNumber] = order;
}


bool RakutenItemModel::contains(const QString& shopCode, const QString& manageNumber) const
{
	auto it = this->m_allItems.find(shopCode);
	return it != this->m_allItems.end() && it->contains(manageNumber);
}
void RakutenItemModel::addItem(const QString& shopCode, item::search::item_ptr item)
{
	const bool replace = this->m_allItems[shopCode].contains(item->itemUrl);
	this->m_allItems[shopCode][item->itemUrl] = item;
	if (this->m_shopCode != shopCode)
		return;

	if (replace)
	{
		for (int i = 0; i < this->m_items.size(); i++)
		{
			if (this->m_items[i]->itemUrl == item->itemUrl)
			{
				this->m_items[i] = item;
				emit this->dataChanged(this->index(i, 0), this->index(i, this->columnCount() - 1));
				break;
			}
		}
	}
	else
	{
		// emit signal
		this->beginInsertRows(QModelIndex(), this->m_items.count(), this->m_items.count());

		this->m_items.push_back(item);

		// emit signal
		this->endInsertRows();
	}
}


QList<QPair<QString, int>> RakutenItemModel::getItemsSortedByUnits(const QString& shopCode)
{
	QList<QPair<QString, int>> items;
	if (!m_allItems.contains(shopCode))
	{
		return items;
	}


	// íçï∂âÒêîÇranking_mapÇ…éÊìæ
	const QDateTime _30daysb4 = QDateTime::currentDateTime().addDays(-30);
	std::unordered_map<QString, int> ranking_map;
	for (const auto order : this->m_orders)
	{
		if (order->orderDatetime < _30daysb4)
			continue;

		for (const auto item : order->items)
		{
			constexpr bool by_price = false;
			if constexpr (!by_price)
			{
				ranking_map[item->manageNumber] += item->units;
			}
			else
			{
				ranking_map[item->manageNumber] += (item->price * item->units);
			}
		}
	}

	// map > list
	for (const auto& pair : ranking_map)
	{
		items.push_back({ pair.first, pair.second });
	}

	// sort by number
	qSort(items.begin(), items.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b)
	{
		return a.second > b.second;
	});

	return items;
}

QVector<item::search::item_ptr> RakutenItemModel::get_ranking_items2(const QString& shopCode)
{
	QVector<item::search::item_ptr> _items;
	if (!m_allItems.contains(shopCode))
	{
		return _items;
	}

	bool _done = false;
	for (const auto& pair : this->getItemsSortedByUnits(shopCode))
	{
		if (_done)
			break;

		for (auto item : this->m_items)
		{
			if (item->itemUrl == pair.first)
			{
				qDebug() << item->itemUrl << item->images[0];
				_items.push_back(item);
				if (RANK_ITEM_MAX_SIZE <= _items.size())
				{
					// break the main loop
					_done = true;
				}

				break;
			}
		}
	}

	if (_items.size() > RANK_ITEM_MAX_SIZE)
	{
		_items.resize(RANK_ITEM_MAX_SIZE);
	}
	return _items;
}

QVector<item::search::item_ptr> RakutenItemModel::new_items(const QString& shopCode)
{
	QVector<item::search::item_ptr> _items;
	if (!m_allItems.contains(shopCode))
	{
		return _items;
	}

	// sort by registDate
	auto _copy = this->m_items;
	qSort(_copy.begin(), _copy.end(), [](const item::search::item_ptr& a, const item::search::item_ptr& b)
	{
		return a->registDate > b->registDate;
	});

	const int len = qMin<>(_copy.size(), NEW_ITEM_MAX_SIZE);
	for (int i = 0; i < len; i++)
	{
		_items.push_back(_copy[i]);
	}

	return _items;
}