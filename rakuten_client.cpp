#include "stdafx.h"

#include "rakuten_client.hpp"
#include "RakutenItemModel.hpp"
#include "qt_helper.hpp"
#include "settings.hpp"


rakuten_client::rakuten_client(RakutenItemModel* model, QObject* parent)
	: m_model(model), QNetworkAccessManager(parent)
{
	// ItemAPIへのリクエストは、1秒に1リクエストまでを目安にアクセスしてください。
	// ※ トラフィックが集中した場合には、トラフィック制限をかけさせていただく場合がございます。
	this->m_searchTimerId = this->startTimer(1200);

	// 本APIへのリクエストは、1秒に1リクエストまでを目安にアクセスしてください。
	this->m_payOrderAPITimerId = this->startTimer(1200);
}
rakuten_client::~rakuten_client()
{
}

void rakuten_client::setApplicationId(const QString& id)
{
	this->m_applicationId = id;
}
void rakuten_client::setServiceSecret(const QByteArray& key)
{
	this->m_serviceSecret = key;
}
void rakuten_client::setLicenseKey(const QByteArray& key)
{
	this->m_licenseKey = key;
}
void rakuten_client::setShopCode(const QString& shopCode)
{
	this->m_shopCode = shopCode;
	this->m_model->setShopCode(shopCode);
}


QByteArray rakuten_client::gen_rakuten_auth() const
{
	// ESA Base64(serviceSecret:licenseKey)
	return QByteArray("ESA ").append(
		QByteArray(m_serviceSecret).append(":").append(m_licenseKey).toBase64());
}


void rakuten_client::timerEvent(QTimerEvent* e)
{
	if (e->timerId() == this->m_searchTimerId)
	{
		if (!m_item_search_queue.isEmpty())
		{
			const QString url = m_item_search_queue.dequeue();
			this->item_search(url);
		}
	}
	else if (e->timerId() == this->m_payOrderAPITimerId)
	{
		if (!m_payOrderAPIRequests.isEmpty())
		{
			const auto pair = m_payOrderAPIRequests.dequeue();
			QNetworkReply* reply = this->post(pair.first, pair.second);
			this->m_replyBuffers[reply] = QByteArray();

			QObject::connect(reply, &QNetworkReply::errorOccurred, this, [reply](QNetworkReply::NetworkError e)
			{
				qWarning() << "errorOccurred" << reply->url() << e;
			});
			QObject::connect(reply, &QNetworkReply::finished, this, &rakuten_client::payOrderAPIFinished);
		}
	}
}


void rakuten_client::license_get()
{
	// https://webservice.rms.rakuten.co.jp/merchant-portal/view/ja/common/1-1_service_index/licensemanagementapi/licenseexpirydateget/
	QUrlQuery query;
	query.addQueryItem("licenseKey", this->m_licenseKey);

	QUrl request_url("https://api.rms.rakuten.co.jp/es/1.0/license-management/license-key/expiry-date");
	request_url.setQuery(query);

	QNetworkRequest request(request_url);
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	request.setRawHeader("Authorization", this->gen_rakuten_auth());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

	// reply
	QNetworkReply* reply = this->get(request);
	this->m_replyBuffers[reply] = QByteArray();
	QObject::connect(reply, &QNetworkReply::finished, this, &rakuten_client::licenseExpiryDateFinished);
}


void rakuten_client::queue_searchOrder(struct searchOrder_param& param)
{
	QNetworkRequest request(QUrl("https://api.rms.rakuten.co.jp/es/2.0/order/searchOrder/"));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	request.setRawHeader("Authorization", this->gen_rakuten_auth());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
	this->m_payOrderAPIRequests.enqueue(qMakePair(request, param.toByteArray()));
}
void rakuten_client::queue_searchOrder()
{
	searchOrder_param req;
	this->queue_searchOrder(req);
}

void rakuten_client::queue_getOrder(const getOrder_param& param)
{
	// https://webservice.rms.rakuten.co.jp/merchant-portal/view?contents=/ja/common/1-1_service_index/rakutenpayorderapi/getorder
	QNetworkRequest request(QUrl("https://api.rms.rakuten.co.jp/es/2.0/order/getOrder/"));
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	request.setRawHeader("Authorization", this->gen_rakuten_auth());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
	this->m_payOrderAPIRequests.enqueue(qMakePair(request, param.toByteArray()));
}
bool rakuten_client::queue_getOrder(const QString& orderNumber)
{
	if (this->m_model->containsOrder(orderNumber))
		return false;

	getOrder_param param;
	param.orderNumberList.append(orderNumber);
	this->queue_getOrder(param);
	return true;
}
int rakuten_client::queue_getOrder(const QJsonArray& orderNumberList)
{
	int queued = 0;

	getOrder_param param;
	for (const QJsonValue& v : orderNumberList)
	{
		const QString orderNumber = v.toString();
		if (!this->m_model->containsOrder(orderNumber))
		{
			param.orderNumberList.append(orderNumber);
			if (param.orderNumberList.size() == 100)
			{
				// 最大 100 件まで指定可能
				queued++;
				this->queue_getOrder(param);
				param.clear();
			}
		}
	}

	// request if not empty
	if (!param.orderNumberList.empty())
	{
		queued++;
		this->queue_getOrder(param);
	}

	return queued;
}
int rakuten_client::queue_getOrder()
{
	if (this->m_orderNumberList.empty())
		return 0;

	QJsonArray orderNumberList;
	for (const QString& orderNumber : this->m_orderNumberList)
		orderNumberList.append(orderNumber);
	return this->queue_getOrder(orderNumberList);
}


void rakuten_client::item_search(const QString& itemUrl)
{
	qDebug() << "item.search" << itemUrl << "requested, remaining" << this->m_item_search_queue.size();

	// https://webservice.rms.rakuten.co.jp/merchant-portal/view?contents=/ja/common/1-1_service_index/itemapi/itemsearch
	QUrlQuery query;
	query.addQueryItem("itemUrl", itemUrl);

	QUrl request_url("https://api.rms.rakuten.co.jp/es/1.0/item/search");
	request_url.setQuery(query);

	// request
	QNetworkRequest request(request_url);
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
	request.setRawHeader("Authorization", this->gen_rakuten_auth());

	// reply
	QNetworkReply* reply = this->get(request);
	this->m_replyBuffers[reply] = QByteArray();
	QObject::connect(reply, &QNetworkReply::finished, this, &rakuten_client::item_search_finished);
}
void rakuten_client::queue_item_search(const QString& itemUrl, bool force)
{
	if (this->m_item_search_queue.contains(itemUrl)
		|| (!force && this->m_itemSearchFails.contains(itemUrl)))
	{
		// already in queue
		return;
	}

	// notify queue
	emit this->queuedItemSearch(1);

	if (!force && this->m_model->contains(this->m_shopCode, itemUrl))
	{
		// use cache if exists
		emit this->signal_itemSearchFinished(itemUrl);
		qDebug() << itemUrl << "cache found";
	}
	else
	{
		// queue otherwise
		this->m_item_search_queue.enqueue(itemUrl);
		qDebug() << "queued item.search" << itemUrl;
	}
}


bool rakuten_client::IchibaItem_Search(int page)
{
	// https://webservice.rakuten.co.jp/api/ichibaitemsearch/
	QUrlQuery query;
	query.addQueryItem("applicationId", this->m_applicationId);
	query.addQueryItem("format", "json");
	query.addQueryItem("formatVersion", "2");
	query.addQueryItem("shopCode", this->m_shopCode);
	query.addQueryItem("sort", QUrl::toPercentEncoding("-updateTimestamp")); // 商品更新日時順（降順）
	query.addQueryItem("page", QString::number(page));
	query.addQueryItem("elements", "pageCount,hits,page,last,itemUrl");

	QUrl request_url("https://app.rakuten.co.jp/services/api/IchibaItem/Search/20170706");
	request_url.setQuery(query);

	// request
	QNetworkRequest request(request_url);
	request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

	// reply
	QNetworkReply* reply = this->get(request);
	this->m_replyBuffers[reply] = QByteArray();
	QObject::connect(reply, &QNetworkReply::finished, this, &rakuten_client::IchibaItem_Search_finished);

	return true;
}


void rakuten_client::onSearchOrder(QNetworkReply* reply)
{
	bool ok = false;
	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	switch (statusCode)
	{
		case 200:
		{
			// OK
			ok = true;
			break;
		}
		case 400:
		case 404:
		case 405:
		case 500:
		case 503:
		{
			// expected error
			ok = false;
			break;
		}
		default:
		{
			// unexpected status code
			qDebug() << reply->readAll();
			emit this->signal_searchOrderFinished(false);
			return;
		}
	}

	const QByteArray data = reply->readAll();
	if (data.isEmpty())
	{
		emit this->signal_searchOrderFinished(false);
		return;
	}

	const QJsonObject obj = QJsonDocument::fromJson(data).object();
	if (!ok)
	{
		const QJsonArray MessageModelList = obj["MessageModelList"].toArray(); // always not null
		for (const QJsonValue& v : MessageModelList)
		{
			const QJsonObject MessageModel = v.toObject();
			const QString type = MessageModel["messageType"].toString();
			const QString code = MessageModel["messageCode"].toString();
			const QString message = MessageModel["message"].toString();
			const QString errorstr = QString("searchOrder failed, type: %1, code: %2, message: %3").arg(type, code, message);
			emit this->signal_error(errorstr);
		}

		emit this->signal_searchOrderFinished(false);
	}
	else
	{
		const QJsonArray orderNumberList = obj["orderNumberList"].toArray();
		//const QJsonObject PaginationResponseModel = obj["PaginationResponseModel"].toObject();

		// merge list
		std::set<QString> _local_set;
		for (const QJsonValue& v : orderNumberList)
		{
			const QString num = v.toString();
			_local_set.insert(num);
		}
		this->m_orderNumberList.merge(_local_set);
		qDebug() << "m_orderNumberList.size" << m_orderNumberList.size();

		emit this->signal_searchOrderFinished(true);
	}


#ifdef _DEBUG
	const QString filename = global_dir.filePath("searchOrder.json");
	QFile f_out(filename);
	if (f_out.open(QIODevice::WriteOnly))
	{
		f_out.write(data);
		f_out.flush();
	}
#endif
}
void rakuten_client::onGetOrder(QNetworkReply * reply)
{
	bool ok = false;
	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	switch (statusCode)
	{
		case 200:
		{
			// OK
			ok = true;
			break;
		}
		case 400:
		{
			// Bad Request
			qDebug() << reply->readAll();
			break;
		}
		case 404:
		case 405:
		case 500:
		case 503:
		{
			// expected error
			ok = false;
			break;
		}
		default:
		{
			// unexpected status code
			emit this->signal_getOrderFinished(false);
			return;
		}
	}

	const QByteArray data = reply->readAll();
	if (data.isEmpty())
	{
		emit this->signal_getOrderFinished(false);
		return;
	}

	// OrderModelList -> PackageModelList -> ItemModelList -> 
	const QJsonObject obj = QJsonDocument::fromJson(data).object();
	if (!ok)
	{
		const QJsonArray MessageModelList = obj["MessageModelList"].toArray(); // always not null
		for (const QJsonValue& v : MessageModelList)
		{
			const QJsonObject MessageModel = v.toObject();
			const QString type = MessageModel["messageType"].toString();
			const QString code = MessageModel["messageCode"].toString();
			const QString message = MessageModel["message"].toString();
			const QString errorstr = QString("getOrder failed, type: %1, code: %2, message: %3").arg(type, code, message);
			emit this->signal_error(errorstr);
		}

		emit this->signal_getOrderFinished(false);
	}
	else
	{
		// Note: OrderModelList might be null
		if (!obj.contains("OrderModelList"))
		{
			emit this->signal_getOrderFinished(false);
			return;
		}

		const QJsonArray OrderModelList = obj["OrderModelList"].toArray();
		for (const QJsonValue& _order : OrderModelList)
		{
			const QJsonObject json_obj = _order.toObject();
			auto order = QSharedPointer<rpay::getOrder::OrderModel>::create(json_obj);
			this->m_model->insertOrder(order->orderNumber, std::move(order));
		}

		emit this->signal_getOrderFinished(true);
	}

#ifdef _DEBUG
	const QString filename = global_dir.filePath("getOrder.json");
	QFile f_out(filename);
	if (f_out.open(QIODevice::WriteOnly))
	{
		f_out.write(data);
		f_out.flush();
	}
#endif
}


// slots
void rakuten_client::IchibaItem_Search_finished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(this->sender());
	this->m_replyBuffers.remove(reply);
	reply->deleteLater();

	if (reply->error() != QNetworkReply::NoError)
	{
		emit this->signal_IchibaItemSearchFinished(false);
		return;
	}

	const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();

	// 現在のページ番号
	const int page = obj["page"].toInt();

	// ヒット件数番
	const int hits = obj["hits"].toInt();

	// 総ページ数(最大100)
	const int pageCount = obj["pageCount"].toInt();
	const int last = obj["last"].toInt();

	//
	if (page == 1)
	{
		this->m_ichiba_items.clear();
	}

	for (const auto& _item : obj["Items"].toArray())
	{
		// thumbnail, datetime, title price, url
		const QJsonObject item = _item.toObject();

		// 従来の商品名を表示させたい場合は、「catchcopy＋itemname」で表示してください。
		//const QString itemName = item["itemName"].toString();
		//const QString catchcopy = item["catchcopy"].toString();
		const QString itemUrl = item["itemUrl"].toString();
		//const int itemPrice = item["itemPrice"].toInt();

		// itemUrl > 商品管理番号
		// https://.../shopid/number/
		const QStringList parts = QUrl(itemUrl).path().split('/', Qt::SkipEmptyParts);
		if (parts.size() != 2)
		{
			qWarning() << "IchibaItem_Search unexpected parts:" << parts;
		}
		else
		{
			const QString shopCode = parts.first();
			const QString manageNumber = parts.last();
			this->m_ichiba_items[shopCode].push_back(manageNumber);

			// item.search
			this->queue_item_search(manageNumber);
		}
	}


	// request next page? 最大50件まで検索する
	if (page < pageCount && last < 50)
	{
		this->IchibaItem_Search(page + 1);
	}
	else
	{
		emit this->signal_IchibaItemSearchFinished(true);
	}
}


void rakuten_client::licenseExpiryDateFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(this->sender());
	this->m_replyBuffers.remove(reply);
	reply->deleteLater();

	const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QByteArray data = reply->readAll();
	if (data.isEmpty())
	{
		// network error or smth
		return;
	}

	const QJsonObject obj = QJsonDocument::fromJson(data).object();
	if (obj.contains("errors"))
	{
		const QJsonArray errors = obj["errors"].toArray(); // always not null
		for (const QJsonValue& v : errors)
		{
			const QJsonObject error = v.toObject();
			const QString type = error["code"].toString();
			const QString code = error["message"].toString();
		}
	}
	else
	{
		const QString expiryDate = obj["expiryDate"].toString();
	}


#ifdef _DEBUG
	const QString filename = global_dir.filePath("license.expiryDate.get.json");
	QFile f_out(filename);
	if (f_out.open(QIODevice::WriteOnly))
	{
		f_out.write(data);
		f_out.flush();
	}
#endif
}


void rakuten_client::item_search_finished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(this->sender());
	this->m_replyBuffers.remove(reply);
	reply->deleteLater();

	//
	const QUrlQuery query(reply->url());
	const QString itemUrl = query.queryItemValue("itemUrl");

	const QByteArray responseBody = reply->readAll();
	const APIResponseStatus status(responseBody);
	if (reply->error() != QNetworkReply::NoError)
	{
		if (status.isNG())
		{
			emit this->signal_itemAPIFinished(status);
		}
		else
		{
			// network error or smth
		}

		qWarning() << reply->url() << "failed.";
		return;
	}

	QDomDocument doc;
	doc.setContent(responseBody);
	const QDomNodeList items = doc.documentElement().elementsByTagName("item");
	if (items.isEmpty())
	{
		if (!itemUrl.isEmpty())
		{
			this->m_itemSearchFails.insert(itemUrl);
			emit this->signal_itemSearchFinished(itemUrl);
		}

		qWarning() << "item.search" << itemUrl << "failed";
	}
	else
	{
		for (int i = 0; i < items.count(); i++)
		{
			item::search::item_ptr item = item::search::item_ptr::create(items.at(i));
			this->m_model->addItem(this->m_shopCode, item);
			emit this->signal_itemSearchFinished(item->itemUrl);
		}
	}
}


void rakuten_client::payOrderAPIFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(this->sender());
	const QString path = reply->url().path();
	if (path.contains("searchOrder", Qt::CaseInsensitive))
	{
		this->onSearchOrder(reply);
	}
	else if (path.contains("getOrder", Qt::CaseInsensitive))
	{
		this->onGetOrder(reply);
	}

	// delete later
	this->m_replyBuffers.remove(reply);
	reply->deleteLater();
}