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
			// ���iID, �P��, ��
			int itemId, price, units;

			// ���i��, ���i�ԍ�(maybe null), ���i�Ǘ��ԍ�
			QString itemName, itemNumber, manageNumber;

			item(const QJsonObject& obj);

			void deserialize(const QJsonObject& obj);
			QJsonObject serialize() const;
		};


		// OrderModel
		struct OrderModel
		{
			// �����ԍ�
			QString orderNumber;
			
			// ��������
			QDateTime orderDatetime;

			//
			QVector<QSharedPointer<rpay::getOrder::item>> items;


			OrderModel(const QJsonObject& obj);
			void deserialize(const QJsonObject& obj);
			QJsonObject serialize();
		};
	}
}