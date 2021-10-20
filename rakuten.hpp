#pragma once

namespace item::search
{
	// https://webservice.rms.rakuten.co.jp/merchant-portal/view/ja/common/1-1_service_index/itemApi/itemSearch
	// XML : item
	struct item
	{
		// itemUrl is NOT url
		QString itemUrl, itemNumber, itemName;
		int itemPrice, genreId;
		QString catalogId, catchCopyForPC;
		QList<QString> images;
		QDateTime registDate;

		item();
		item(const QDomNode& n);

	private:
		void readImages(const QDomElement& e);

	public:
		void read(const QDomNode& itemNode);
		void write(QXmlStreamWriter& stream) const;
	};

	typedef QSharedPointer<item> item_ptr;
}

namespace rpay
{
	// getOrder struct with all personal information excluded
	namespace getOrder
	{
		// PackageModel > ItemModel
		struct item
		{
			// 商品ID, 単価, 個数
			int itemId, price, units;

			// 商品名, 商品番号(maybe null), 商品管理番号
			QString itemName, itemNumber, manageNumber;

			item(const QJsonObject& obj);

			void deserialize(const QJsonObject& obj);
			QJsonObject serialize() const;
		};


		// OrderModel
		struct OrderModel
		{
			// 注文番号
			QString orderNumber;
			
			// 注文日時
			QDateTime orderDatetime;

			//
			QVector<QSharedPointer<rpay::getOrder::item>> items;


			OrderModel(const QJsonObject& obj);
			void deserialize(const QJsonObject& obj);
			QJsonObject serialize();
		};
	}
}