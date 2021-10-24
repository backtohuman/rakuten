#pragma once

// https://webservice.rms.rakuten.co.jp/merchant-portal/view/ja/common/1-1_service_index/functionsCommonDefinition
struct APIResponseStatus
{
	QString interfaceId, systemStatus, message, requestId;
	QDomElement requests;

	APIResponseStatus()
	{
	}
	APIResponseStatus(const QByteArray& data)
	{
		QDomDocument doc;
		doc.setContent(data);
		const QDomElement e_status = doc.documentElement().firstChildElement("status");
		for (QDomNode n = e_status.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			const QDomElement e = n.toElement();
			if (e.tagName() == QLatin1String("interfaceId"))
			{
				this->interfaceId = e.text();
			}
			else if (e.tagName() == QLatin1String("systemStatus"))
			{
				this->systemStatus = e.text();
			}
			else if (e.tagName() == QLatin1String("message"))
			{
				this->message = e.text();
			}
			else if (e.tagName() == QLatin1String("requestId"))
			{
				this->requestId = e.text();
			}
			else if (e.tagName() == QLatin1String("requests"))
			{
				this->requests = e;
			}
		}
	}

	bool isOk() const
	{
		return this->systemStatus == QLatin1String("OK");
	}
	bool isNG() const
	{
		return this->systemStatus == QLatin1String("NG");
	}
};
Q_DECLARE_METATYPE(APIResponseStatus)



struct PaginationRequestModel
{
	// 1ページあたりの取得結果数 required
	// 最大 1000 件まで指定可能
	int requestRecordsAmount;

	// リクエストページ番号 required
	int requestPage;

	PaginationRequestModel()
	{
		this->requestRecordsAmount = 30;
		this->requestPage = 1;
	}

	QJsonObject toObject() const
	{
		QJsonObject obj;
		obj["requestRecordsAmount"] = requestRecordsAmount;
		obj["requestPage"] = requestPage;
		return obj;
	}
};
struct searchOrder_param
{
	QJsonArray orderProgressList;
	int dateType;
	QDateTime startDatetime, endDatetime;
	PaginationRequestModel pagination;

	searchOrder_param()
	{
		// 500: 発送済
		this->orderProgressList.append(500);

		// 1: 注文日
		this->dateType = 1;

		// YYYY-MM-DDThh:mm:ss+09:00
		// ex: "2017-10-14T00:00:00+0900"
		// 過去 730 日(2年)以内の注文を指定可能
		this->startDatetime = QDateTime::currentDateTime().addDays(-60);

		// 開始日から 63 日以内
		this->endDatetime = QDateTime::currentDateTime();
	}

	QByteArray toByteArray() const
	{
		QJsonObject params;
		params["orderProgressList"] = orderProgressList;
		params["dateType"] = dateType;
		params["startDatetime"] = startDatetime.toString("yyyy-MM-ddThh:mm:ss+0900");
		params["endDatetime"] = endDatetime.toString("yyyy-MM-ddThh:mm:ss+0900");
		params["PaginationRequestModel"] = pagination.toObject();
		return QJsonDocument(params).toJson(QJsonDocument::Compact);
	}
};
struct getOrder_param
{
	// 	最大 100 件まで指定可能
	// 過去 730 日(2年)以内の注文を取得可能
	QJsonArray orderNumberList;
	int version;

	getOrder_param()
	{
		this->version = 5;
	}

	void clear()
	{
		if constexpr (true)
		{
			orderNumberList = QJsonArray();
		}
		else
		{
			while (!orderNumberList.isEmpty())
			{
				orderNumberList.removeFirst();
			}
		}
	}

	QByteArray toByteArray() const
	{
		QJsonObject params;
		params["orderNumberList"] = orderNumberList;
		params["version"] = version;
		return QJsonDocument(params).toJson(QJsonDocument::Compact);
	}
};







class RakutenItemModel;


class rakuten_client : public QNetworkAccessManager
{
	Q_OBJECT

public:
	rakuten_client(RakutenItemModel* model, QObject* parent = nullptr);
	~rakuten_client();

	void setApplicationId(const QString& id);
	void setServiceSecret(const QByteArray& key);
	void setLicenseKey(const QByteArray& key);
	void setShopCode(const QString& shopCode);

private:
	QByteArray gen_rakuten_auth() const;

protected:
	void timerEvent(QTimerEvent* e) override;


public:
	bool isFinished()
	{
		return this->m_item_search_queue.isEmpty()
			&& this->m_payOrderAPIRequests.isEmpty()
			&& this->m_replyBuffers.isEmpty();
	}


	void license_get();

	void queue_searchOrder(struct searchOrder_param& param);
	void queue_searchOrder();

	// returns queued number
	void queue_getOrder(const getOrder_param& param);
	bool queue_getOrder(const QString& orderNumber);
	int queue_getOrder(const QJsonArray& orderNumberList);
	int queue_getOrder();

	void queue_item_search(const QString& itemUrl, bool force = false);

	// IchibaAPI
	bool IchibaItem_Search(int page = 1);




private:
	// itemUrl=商品管理番号
	void item_search(const QString& itemUrl);

	void onSearchOrder(QNetworkReply* reply);
	void onGetOrder(QNetworkReply* reply);


private slots:
	void IchibaItem_Search_finished();
	void licenseExpiryDateFinished();
	void item_search_finished();
	void payOrderAPIFinished();

signals:
	void signal_itemAPIFinished(const APIResponseStatus& status);

	void signal_error(const QString& error);
	void signal_searchOrderFinished(bool);
	void signal_getOrderFinished(bool);
	void signal_IchibaItemSearchFinished(bool);
	void signal_itemSearchFinished(const QString& itemUrl);

	//
	void queuedItemSearch(int n);

private:
	QString m_applicationId;
	QByteArray m_serviceSecret, m_licenseKey;
	QString m_shopCode;
	
	// set of ordernumber given by searchOrder API
	std::set<QString> m_orderNumberList;

	// queue
	int m_searchTimerId, m_payOrderAPITimerId;
	QQueue<QString> m_item_search_queue;
	QSet<QString> m_itemSearchFails;

	// <request, postdata>
	QQueue<QPair<QNetworkRequest, QByteArray>> m_payOrderAPIRequests;

	// <店舗コード, Vec<商品管理番号>>
	QMap<QString, QVector<QString>> m_ichiba_items;
	RakutenItemModel* m_model;

	//
	QMap<QNetworkReply*, QByteArray> m_replyBuffers;
};