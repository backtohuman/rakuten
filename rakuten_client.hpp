#pragma once


#define CLIENT_WIDTH (111)


struct PaginationRequestModel
{
	// 1�y�[�W������̎擾���ʐ� required
	// �ő� 1000 ���܂Ŏw��\
	int requestRecordsAmount;

	// ���N�G�X�g�y�[�W�ԍ� required
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
		// 500: ������
		this->orderProgressList.append(500);

		// 1: ������
		this->dateType = 1;

		// YYYY-MM-DDThh:mm:ss+09:00
		// ex: "2017-10-14T00:00:00+0900"
		// �ߋ� 730 ��(2�N)�ȓ��̒������w��\
		this->startDatetime = QDateTime::currentDateTime().addDays(-60);

		// �J�n������ 63 ���ȓ�
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
	// 	�ő� 100 ���܂Ŏw��\
	// �ߋ� 730 ��(2�N)�ȓ��̒������擾�\
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
	void searchOrder(struct searchOrder_param& param);
	void searchOrder();

	// returns queued number
	void queue_getOrder(const getOrder_param& param);
	bool queue_getOrder(const QString& orderNumber);
	int queue_getOrder(const QJsonArray& orderNumberList);
	int queue_getOrder();

	// ItemAPI
private:
	// itemUrl=���i�Ǘ��ԍ�
	void item_search(const QString& itemUrl);

public:
	void queue_item_search(const QString& itemUrl, bool force = false);

	// IchibaAPI
	void IchibaItem_Search(int page = 1);


private:
	void onSearchOrder(QNetworkReply* reply);
	void onGetOrder(QNetworkReply* reply);


private slots:
	void IchibaItem_Search_finished();
	void item_search_finished();
	void payOrderAPIFinished();

signals:
	void signal_error(const QString& error);
	void signal_searchOrderFinished(bool);
	void signal_getOrderFinished(bool);
	void signal_IchibaItemSearchFinished(bool);
	void signal_itemSearchFinished(const QString& itemUrl);

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

	// <�X�܃R�[�h, Vec<���i�Ǘ��ԍ�>>
	QMap<QString, QVector<QString>> m_ichiba_items;
	RakutenItemModel* m_model;
};