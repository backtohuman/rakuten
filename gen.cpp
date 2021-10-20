#include "stdafx.h"

#include "rakuten.hpp"
#include "rakuten_client.hpp"
#include "RakutenItemModel.hpp"
#include "qt_helper.hpp"
#include "settings.hpp"
#include <qfont.h>
#include <qsvggenerator.h>

QString get_image_src(item::search::item_ptr item)
{
	if (item->images.isEmpty())
		return QString();

	QUrl img_url(item->images[0]);
	if (img_url.host().contains("thumbnail.image.rakuten.co.jp", Qt::CaseInsensitive)
		|| img_url.host().contains("image.rakuten.co.jp", Qt::CaseInsensitive))
	{
		const QString pixel_size = QString("_ex=%1x%2").arg(CLIENT_WIDTH).arg(CLIENT_WIDTH);
		//img_url.setQuery(pixel_size);
	}
	if (img_url.scheme() == "http")
	{
		img_url.setScheme("https");
	}
	return img_url.toString();
}


QString gen_new_item_row(const QVector<item::search::item_ptr>& new_items, const QString& filepath)
{
	QFile f_out(filepath);
	if (!f_out.open(QIODevice::WriteOnly))
	{
		return QString();
	}

	f_out.write(
		R"(
<style>
    * {
        margin: 0px;
        padding: 0px;
    }

    table {
        border-collapse: collapse;
    }

    .img {
        width: 111px;
    }

    .date {
        width: 111px;
        padding: 3px 0px;
        color: white;
        font-size: 12px;
        background: #d34e6f;
        text-align: center;
    }

    .name {
		max-width: 111px;
        width: 111px;
        height: 70px;
        word-wrap: break-word;
        color: #333;
        color: #1555d5;
        font-size: 12px;
        font-weight: 700;
        text-align: center;
        line-height: 1.3;
        overflow: hidden;
        text-overflow: ellipsis;
        padding: 5px 0px;
    }

    .price {
        width: 111px;
        margin-top: 5px;
        color: #bf0000;
        font-size: 13px;
        text-align: right;
        font-weight: bold;
    }
</style>
)"
	);

	QXmlStreamWriter stream(&f_out);
	stream.setAutoFormatting(false);

	// <table>
	stream.writeStartElement("table");

	const int size = qMin(3, new_items.size());
	for (int i = 0; i < size; i += 3)
	{
		const bool write_0 = true;
		const bool write_1 = (i + 1) < size;
		const bool write_2 = (i + 2) < size;
		const item::search::item_ptr item0 = write_0 ? new_items[i + 0] : nullptr;
		const item::search::item_ptr item1 = write_1 ? new_items[i + 1] : nullptr;
		const item::search::item_ptr item2 = write_2 ? new_items[i + 2] : nullptr;

		// functions
		auto _write_img = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "img");
			stream.writeEmptyElement("img");
			stream.writeAttribute("src", get_image_src(item));
			stream.writeAttribute("width", QString::number(CLIENT_WIDTH));
			stream.writeEndElement();
		};
		auto _write_date = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "date");
			// not working for c++20
			const QString dtstring = item->registDate.toString(QString::fromUtf8(u8"MMŒŽdd“úV’…I"));
			stream.writeCharacters(dtstring);
			stream.writeEndElement();
		};
		auto _write_name = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "name");
			const int max_length = 80;
			const QString title = item->itemName;
			if (max_length < title.length())
				stream.writeCharacters(title.left(max_length).append("..."));
			else
				stream.writeCharacters(title);
			stream.writeEndElement();
		};
		auto _write_price = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "price");
			stream.writeCharacters(QString::number(item->itemPrice).append(
				QString::fromUtf8(reinterpret_cast<const char*>(u8"‰~"))
			));
			stream.writeEndElement();
		};

		// img
		stream.writeStartElement("tr");
		if (write_0) _write_img(stream, item0);
		if (write_1) _write_img(stream, item1);
		if (write_2) _write_img(stream, item2);
		stream.writeEndElement(); // /tr

		// date
		stream.writeStartElement("tr");
		if (write_0) _write_date(stream, item0);
		if (write_1) _write_date(stream, item1);
		if (write_2) _write_date(stream, item2);
		stream.writeEndElement(); // /tr

		// title
		stream.writeStartElement("tr");
		if (write_0) _write_name(stream, item0);
		if (write_1) _write_name(stream, item1);
		if (write_2) _write_name(stream, item2);
		stream.writeEndElement(); // /tr

		// price
		stream.writeStartElement("tr");
		if (write_0) _write_price(stream, item0);
		if (write_1) _write_price(stream, item1);
		if (write_2) _write_price(stream, item2);
		stream.writeEndElement(); // /tr

		// <tr><td colspan="3"><hr></td></tr>
		if ((i + 3) < size && false)
		{
			stream.writeStartElement("tr");
			stream.writeStartElement("td");
			stream.writeAttribute("colspan", "3");
			stream.writeEmptyElement("hr");
			stream.writeEndElement(); // /td
			stream.writeEndElement(); // /tr
		}
	}

	stream.writeEndElement(); // /table

	f_out.flush();
	f_out.close();

	QFile f_in(filepath);
	if (!f_in.open(QIODevice::ReadOnly))
	{
		return QString();
	}

	return f_in.readAll();
}


QString gen_rank_item_row(const QVector<item::search::item_ptr>& new_items, int rank_start, const QString& filepath)
{
	QFile f_out(filepath);
	if (!f_out.open(QIODevice::WriteOnly))
	{
		return QString();
	}

	f_out.write(R"(
<style>
    * {
        margin: 0px;
        padding: 0px;
    }
    table {
        border-collapse: collapse;
    }
    .item {
        width: 111px;
        font-family: 'Meiryo,Hiragino Kaku Gothic ProN,MS PGothic,sans-serif';
        display: inline-block;
    }
    .img {
        width: 111px;
    }
    .rank {
        background-image: url('https://www.rakuten.ne.jp/gold/kstar/ectool/rrank/shared_resource/rankimg_3-4.jpg');
        -moz-background-size: 100% auto;
        background-size: 100% auto;
        height:29px;
    }
    .rank-text {
        padding-left: 5px;
        color: white;
        font-size: 12px;
        vertical-align:baseline;
    }
    .date {
        width: 111px;
        padding: 3px 0px;
        color: white;
        font-size: 12px;
        background: #d34e6f;
        text-align: center;
    }
    .name {
        width: 111px;
        height: 70px;
        word-wrap: break-word;
        color: #333;
        color: #1555d5;
        font-size: 12px;
        font-weight: 700;
        text-align: center;
        line-height: 1.3;
        overflow: hidden;
        text-overflow: ellipsis;
        padding: 5px 0px;
    }
    .price {
        width: 111px;
        margin-top: 5px;
        color: #bf0000;
        font-size: 13px;
        text-align: right;
        font-weight: bold;
    }
</style>
)");

	QXmlStreamWriter stream(&f_out);
	stream.setAutoFormatting(false);

	// <table>
	stream.writeStartElement("table");

	const int size = qMin(3, new_items.size());
	for (int i = 0; i < size; i += 3)
	{
		const bool write_0 = true;
		const bool write_1 = (i + 1) < size;
		const bool write_2 = (i + 2) < size;
		const item::search::item_ptr item0 = write_0 ? new_items[i + 0] : nullptr;
		const item::search::item_ptr item1 = write_1 ? new_items[i + 1] : nullptr;
		const item::search::item_ptr item2 = write_2 ? new_items[i + 2] : nullptr;

		// functions
		auto _write_rank = [](QXmlStreamWriter& stream, item::search::item_ptr item, int rank)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "rank");
			if (rank == 1)
			{
				stream.writeEmptyElement("img");
				stream.writeAttribute("src", "https://www.rakuten.ne.jp/gold/kstar/ectool/rrank/shared_resource/rankimg_3-1.jpg");
				stream.writeAttribute("width", QString::number(CLIENT_WIDTH));
			}
			else if (rank == 2)
			{
				stream.writeEmptyElement("img");
				stream.writeAttribute("src", "https://www.rakuten.ne.jp/gold/kstar/ectool/rrank/shared_resource/rankimg_3-2.jpg");
				stream.writeAttribute("width", QString::number(CLIENT_WIDTH));
			}
			else if (rank == 3)
			{
				stream.writeEmptyElement("img");
				stream.writeAttribute("src", "https://www.rakuten.ne.jp/gold/kstar/ectool/rrank/shared_resource/rankimg_3-3.jpg");
				stream.writeAttribute("width", QString::number(CLIENT_WIDTH));
			}
			else
			{
				stream.writeStartElement("p");
				stream.writeAttribute("class", "rank-text");
				stream.writeCharacters(from_u8(u8"%1ˆÊ").arg(rank));
				stream.writeEndElement();
			}
			stream.writeEndElement();
		};
		auto _write_img = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "img");
			stream.writeEmptyElement("img");
			stream.writeAttribute("src", get_image_src(item));
			stream.writeAttribute("width", QString::number(CLIENT_WIDTH));
			stream.writeEndElement();
		};
		auto _write_name = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "name");
			const int max_length = 80;
			const QString title = item->itemName;
			if (max_length < title.length())
				stream.writeCharacters(title.left(max_length).append("..."));
			else
				stream.writeCharacters(title);
			stream.writeEndElement();
		};
		auto _write_price = [](QXmlStreamWriter& stream, item::search::item_ptr item)
		{
			stream.writeStartElement("td");
			stream.writeAttribute("class", "price");
			stream.writeCharacters(QString::number(item->itemPrice).append(
				QString::fromUtf8(reinterpret_cast<const char*>(u8"‰~"))
			));
			stream.writeEndElement();
		};

		// rank
		stream.writeStartElement("tr");
		if (write_0) _write_rank(stream, item0, rank_start + i);
		if (write_1) _write_rank(stream, item1, rank_start + i + 1);
		if (write_2) _write_rank(stream, item2, rank_start + i + 2);
		stream.writeEndElement(); // /tr

		// img
		stream.writeStartElement("tr");
		if (write_0) _write_img(stream, item0);
		if (write_1) _write_img(stream, item1);
		if (write_2) _write_img(stream, item2);
		stream.writeEndElement(); // /tr

		// title
		stream.writeStartElement("tr");
		if (write_0) _write_name(stream, item0);
		if (write_1) _write_name(stream, item1);
		if (write_2) _write_name(stream, item2);
		stream.writeEndElement(); // /tr

		// price
		stream.writeStartElement("tr");
		if (write_0) _write_price(stream, item0);
		if (write_1) _write_price(stream, item1);
		if (write_2) _write_price(stream, item2);
		stream.writeEndElement(); // /tr

		// <tr><td colspan="3"><hr></td></tr>
		if ((i + 3) < size && false)
		{
			stream.writeStartElement("tr");
			stream.writeStartElement("td");
			stream.writeAttribute("colspan", "3");
			stream.writeEmptyElement("hr");
			stream.writeEndElement(); // /td
			stream.writeEndElement(); // /tr
		}
	}

	stream.writeEndElement(); // /table

	f_out.flush();
	f_out.close();

	QFile f_in(filepath);
	if (!f_in.open(QIODevice::ReadOnly))
	{
		return QString();
	}

	return f_in.readAll();
}


QString gen_entire_html(
	const QVector<item::search::item_ptr>& new_items,
	const QVector<item::search::item_ptr>& rankItems,
	const QDir& dir,
	const QString& shopCode, const QString& new_item_path, const QString& rank_item_path)
{
	QFile f_out(dir.filePath(ENTIRE_HTML_FILENAME));
	if (!f_out.open(QIODevice::WriteOnly))
	{
		return QString();
	}

	QXmlStreamWriter stream(&f_out);
	stream.setAutoFormatting(false);

	// <table>
	stream.writeStartElement("table");
	stream.writeAttribute("align", "center");
	stream.writeStartElement("tr");
	stream.writeStartElement("th");
	stream.writeAttribute("colspan", "3");
	stream.writeAttribute("align", "center");
	stream.writeAttribute("bgcolor", "#222");
	stream.writeAttribute("height", "36");

	stream.writeStartElement("font");
	stream.writeAttribute("color", "#fff");
	stream.writeAttribute("size", "3");
	stream.writeCharacters("NEW ITEM");
	stream.writeEndElement();

	stream.writeEndElement();
	stream.writeEndElement();

	const QString rakuten_url = QString("https://item.rakuten.co.jp/%1/").arg(shopCode);
	const QString rakuten_gold_url = QString("https://www.rakuten.ne.jp/gold/%1/").arg(shopCode);

	const int size = new_items.size();
	for (int i = 0; i < size; i += 3)
	{
		const bool write_0 = true;
		const bool write_1 = (i + 1) < size;
		const bool write_2 = (i + 2) < size;
		const item::search::item_ptr item0 = write_0 ? new_items[i + 0] : nullptr;
		const item::search::item_ptr item1 = write_1 ? new_items[i + 1] : nullptr;
		const item::search::item_ptr item2 = write_2 ? new_items[i + 2] : nullptr;

		auto _write_item = [](QXmlStreamWriter& stream, const QString& href, const QString& src)
		{
			QUrl src_url(src);
			src_url.setQuery(QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()));

			stream.writeStartElement("td");
			stream.writeStartElement("a");
			stream.writeAttribute("href", href);
			stream.writeEmptyElement("img");
			stream.writeAttribute("src", src_url.toString());
			stream.writeEndElement();
			stream.writeEndElement();
		};

		// new item
		QString path;
		for (const QString& part : new_item_path.split(QRegularExpression("\\\\/"), Qt::SkipEmptyParts))
		{
			if (!path.isEmpty())
				path.append('/');
			path.append(part);
		}
		const QString parent_path = rakuten_gold_url + path;

		stream.writeStartElement("tr");
		if (write_0) _write_item(stream, QString(rakuten_url).append(item0->itemUrl), QString("%1/%2.png").arg(parent_path).arg(i + 1));
		if (write_1) _write_item(stream, QString(rakuten_url).append(item1->itemUrl), QString("%1/%2.png").arg(parent_path).arg(i + 2));
		if (write_2) _write_item(stream, QString(rakuten_url).append(item2->itemUrl), QString("%1/%2.png").arg(parent_path).arg(i + 3));
		stream.writeEndElement(); // /tr
	}

	// ranking
	stream.writeStartElement("tr");
	stream.writeStartElement("th");
	stream.writeAttribute("colspan", "3");
	stream.writeAttribute("align", "center");
	stream.writeAttribute("bgcolor", "#222");
	stream.writeAttribute("height", "36");
	stream.writeStartElement("font");
	stream.writeAttribute("color", "#fff");
	stream.writeAttribute("size", "3");
	stream.writeCharacters("RANKING");
	stream.writeEndElement(); // /font
	stream.writeEndElement(); // /th
	stream.writeEndElement(); // /tr

	const int ranking_size = rankItems.size();
	for (int i = 0; i < ranking_size; i += 3)
	{
		const bool write_0 = true;
		const bool write_1 = (i + 1) < size;
		const bool write_2 = (i + 2) < size;
		const item::search::item_ptr item0 = write_0 ? rankItems[i + 0] : nullptr;
		const item::search::item_ptr item1 = write_1 ? rankItems[i + 1] : nullptr;
		const item::search::item_ptr item2 = write_2 ? rankItems[i + 2] : nullptr;

		auto _write_item = [](QXmlStreamWriter& stream, const QString& href, const QString& src)
		{
			QUrl src_url(src);
			src_url.setQuery(QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()));

			stream.writeStartElement("td");
			stream.writeStartElement("a");
			stream.writeAttribute("href", href);
			stream.writeEmptyElement("img");
			stream.writeAttribute("src", src_url.toString());
			stream.writeEndElement();
			stream.writeEndElement();
		};

		// rank
		QString path;
		for (const QString& part : rank_item_path.split(QRegularExpression("\\\\/"), Qt::SkipEmptyParts))
		{
			if (!path.isEmpty())
				path.append('/');
			path.append(part);
		}
		const QString parent_path = rakuten_gold_url + path;

		stream.writeStartElement("tr");
		if (write_0) _write_item(stream, rakuten_url + item0->itemUrl, QString("%1/%2.png").arg(parent_path).arg(i + 1));
		if (write_1) _write_item(stream, rakuten_url + item1->itemUrl, QString("%1/%2.png").arg(parent_path).arg(i + 2));
		if (write_2) _write_item(stream, rakuten_url + item2->itemUrl, QString("%1/%2.png").arg(parent_path).arg(i + 3));
		stream.writeEndElement(); // /tr
	}

	stream.writeEndElement(); // /table

	f_out.flush();
	f_out.close();


	// open as readonly
	if (!f_out.open(QIODevice::ReadOnly))
		return QString();

	return f_out.readAll();
}


void gen_newitem_svg(item::search::item_ptr item, const QString& filename)
{
	const int width = 100;

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);

	// QSvgGenerator
	QSvgGenerator generator;
	generator.setOutputDevice(&buffer);
	generator.setTitle("ectool svg");
	generator.setDescription("generated by svg generator");

	// font
	QFont font(from_u8(u8"ƒƒCƒŠƒI"));
	font.setPixelSize(9);

	// doc
	QTextDocument td;
	td.setTextWidth(width);
	td.setDefaultFont(font);
	td.setDefaultStyleSheet(R"(
* {
	margin: 0px;
	padding: 0px;
}
.img {
    width: 100px;
}
.date {
    width: 100px;
	padding: 5px 0px !important;
	color: white;
    font-size: 9px;
	background: #d34e6f;
	text-align: center;
}
.name {
    width: 100px;
    height: 70px;
    word-wrap: break-word;
    color: #1555d5;
    font-size: 9px;
    font-weight: 700;
    text-align: center;
    line-height: 1.0;
    overflow: hidden;
    text-overflow: ellipsis;
    padding: 5px 0px;
}
.price {
    width: 100px;
    margin-top: 5px;
    color: #bf0000;
    font-size: 9px;
    text-align: right;
    font-weight: bold;
}
)");
	const QString dtstring = item->registDate.toString(QString::fromUtf8(u8"MMŒŽdd“úV’…I"));
	const int max_length = 80;
	QString title = item->itemName;
	if (max_length < title.length())
		title = title.left(max_length).append("...");

	td.setDocumentMargin(0);
	td.setHtml(from_u8(
		u8"<a href=\"https://www.google.co.jp\">"
		u8"<p class=\"img\"><img src=\"%1\" width=\"100\" height=\"100\"></p>"
		u8"<p class=\"date\">%2</p>"
		u8"<p class=\"name\">%3</p>"
		u8"<p class=\"price\">%4‰~</p>"
		u8"</a>"
	).arg("c:\\users\\lily\\desktop\\nct-t020-19.webp").arg(dtstring).arg(title).arg(item->itemPrice));

	QTextOption to(td.defaultTextOption());
	to.setAlignment(Qt::AlignCenter);
	to.setWrapMode(QTextOption::WordWrap);
	td.setDefaultTextOption(to);

	const QSizeF docsize = td.size();
	generator.setSize(docsize.toSize());
	generator.setViewBox(QRect(QPoint(0, 0), docsize.toSize()));

	// paint
	QPainter painter;
	painter.begin(&generator);
	td.drawContents(&painter, QRectF(QPoint(0, 0), docsize));
	painter.end();


	// png > url
	//const QString yo = QString(byteArray).replace(QRegularExpression("data:image/png;base64,[^\"]+"), get_image_src(item));

	QFile f_out(filename);
	f_out.open(QIODevice::WriteOnly);
	f_out.write(byteArray);
}