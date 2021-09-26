#pragma once


#define LinkRole	(Qt::UserRole + 0)
#define SortRole	(Qt::UserRole + 1)
#define CodeRole	(Qt::UserRole + 2)
#define SoldoutRole (Qt::UserRole + 3)
#define PriceRole	(Qt::UserRole + 4)
#define TitleRole	(Qt::UserRole + 5)

#define NewRole		(Qt::UserRole + 6)
#define ChangedRole	(Qt::UserRole + 7)
#define ErrorRole	(Qt::UserRole + 8)



// getOrder struct with all personal information excluded
struct getOrder_resp
{
	// 注文番号
	QString orderNumber;

	// 注文日時
	QDateTime orderDatetime;

	// 商品ID, 単価, 個数
	int itemId, price, units;

	// 商品名, 商品番号(maybe null), 商品管理番号
	QString itemName, itemNumber, manageNumber;

	void deserialize(const QSettings& settings)
	{
#define _value(x)  this->x = settings.value(#x)
		_value(orderNumber).toString();
		_value(orderDatetime).toDateTime();
		_value(itemId).toInt();
		_value(price).toInt();
		_value(units).toInt();
		_value(itemName).toString();
		_value(itemNumber).toString();
		_value(manageNumber).toString();
#undef _value
	}
	void serialize(QSettings& settings) const
	{
#define _setValue(x)  settings.setValue(#x, x)
		_setValue(orderNumber);
		_setValue(orderDatetime);
		_setValue(itemId);
		_setValue(price);
		_setValue(units);
		_setValue(itemName);
		_setValue(itemNumber);
		_setValue(manageNumber);
#undef _setValue
	}
};


//
struct rakuten_item
{
	// itemUrl is NOT url
	QString itemUrl, itemNumber, itemName;
	int itemPrice, genreId;
	QString catalogId, catchCopyForPC;
	QList<QString> images;
	QDateTime registDate;

	rakuten_item()
	{
		this->itemPrice = 0;
		this->genreId = 0;
	}
};


class RakutenItemModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit RakutenItemModel(QObject* parent = nullptr);
	~RakutenItemModel();

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

public:
	void save(const QDir& dir);
	void load(const QDir& dir);

	// orders
	bool containsOrder(const QString& orderNumber);
	void insertOrder(const QString& orderNumber, QVector<QSharedPointer<getOrder_resp>>&& orders);

	// shopItems
	bool contains(const QString& shopCode, const QString& manageNumber) const;
	void addItem(const QString& shopCode, const QJsonObject& obj);

	// generators
	QVector<QSharedPointer<rakuten_item>> ranking(const QString& shopCode);
	QVector<QSharedPointer<rakuten_item>> new_items(const QString& shopCode);

	//
	void setShopCode(const QString& shopCode)
	{
		// lazy fix
		this->m_shopCode = shopCode;
	}

public slots:
	//void saveToOld();
	//void complete();

private:
	QString m_shopCode;
	QList<QSharedPointer<rakuten_item>> m_items;

	// <店舗コード, <商品管理番号, struct>>
	QMap<QString, QMap<QString, QJsonObject>> m_shopItems;

	// <orderNumber, orders>
	QMap<QString, QVector<QSharedPointer<getOrder_resp>>> m_orders;
};