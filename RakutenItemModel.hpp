#pragma once

#include "rakuten.hpp"


#define LinkRole	(Qt::UserRole + 0)
#define SortRole	(Qt::UserRole + 1)
#define CodeRole	(Qt::UserRole + 2)


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
	void setShopCode(const QString& shopCode);

	// orders
	bool containsOrder(const QString& orderNumber);
	void insertOrder(const QString& orderNumber, QSharedPointer<rpay::getOrder::OrderModel> order);

	// shopItems
	bool contains(const QString& shopCode, const QString& manageNumber) const;
	void addItem(const QString& shopCode, item::search::item_ptr item);

	// generators
	QList<QPair<QString, int>> get_ranking_items(const QString& shopCode);
	QVector<item::search::item_ptr> get_ranking_items2(const QString& shopCode);
	QVector<item::search::item_ptr> new_items(const QString& shopCode);

public slots:
	//void saveToOld();
	//void complete();

private:
	QString m_shopCode;
	QList<item::search::item_ptr> m_items;

	// <店舗コード, <商品管理番号, struct>>
	QMap<QString, QMap<QString, item::search::item_ptr>> m_allItems;

	// <orderNumber, orders>
	QMap<QString, QSharedPointer<rpay::getOrder::OrderModel>> m_orders;
};