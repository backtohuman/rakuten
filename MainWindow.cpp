#include "stdafx.h"

#include "MainWindow.hpp"
#include "ControlledWebPage.hpp"
#include "rakuten_client.hpp"
#include "qt_helper.hpp"
#include "RakutenItemModel.hpp"
#include "settings.hpp"

extern QString gen_new_item_row(const QVector<QSharedPointer<rakuten_item>>& new_items, const QString& filepath);
extern QString gen_rank_item_row(const QVector<QSharedPointer<rakuten_item>>& new_items, int rank_start, const QString& filepath);
extern QString gen_entire_html(const QVector<QSharedPointer<rakuten_item>>& new_items, const QVector<QSharedPointer<rakuten_item>>& rankItems, const QDir& dir);
extern void gen_newitem_svg(QSharedPointer<rakuten_item> item, const QString& filename);

// MainWindow
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
	// create folder
	this->m_saveDir = openSubDir(QCoreApplication::applicationDirPath(), "ectool");

	// local variables
	this->m_rank_mode = false;
	this->m_imgTimerId = 0;

	// view
	this->m_rakutenItemView = new QTableView(this);
	this->m_rakutenItemModel = new RakutenItemModel(this->m_rakutenItemView);
	m_rakutenProxyModel = new QSortFilterProxyModel(this->m_rakutenItemView);
	m_rakutenProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_rakutenProxyModel->setSourceModel(m_rakutenItemModel);
	m_rakutenProxyModel->setSortRole(SortRole);
	m_rakutenProxyModel->setFilterKeyColumn(-1); // If the value is -1, the keys will be read from all columns.
	this->m_rakutenItemView->setModel(m_rakutenProxyModel);
	this->m_rakutenItemView->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->m_rakutenItemView->setSortingEnabled(true);
	this->m_rakutenItemView->horizontalHeader()->setStretchLastSection(true);

	// rakuten client
	this->m_rakuten = new rakuten_client(this->m_rakutenItemModel, this->m_rakutenItemModel);

	// action and statusbar
	this->_createActions();
	this->_createStatusBar();

	// main widget
	this->m_mainTabWidget = new QTabWidget(this);
	this->m_mainTabWidget->setTabPosition(QTabWidget::North);
	this->m_mainTabWidget->addTab(this->_createMainTab(), from_u8(u8"メイン")); // something
	this->m_mainTabWidget->addTab(this->_createRankingTab(), from_u8(u8"ランキング")); // for ranking items
	this->m_mainTabWidget->addTab(this->_createNewItemTab(), from_u8(u8"新着商品")); // for new items
	this->m_mainTabWidget->addTab(this->_createProductTab(), from_u8(u8"商品詳細")); // list of production given by item.search
	this->m_mainTabWidget->addTab(this->_createSettingsTab(), from_u8(u8"設定")); // settings
	this->setCentralWidget(this->m_mainTabWidget);

	// load GUI settings
	QSettings settings_global;
	this->loadSettings(settings_global);

	// load cache
	QSettings settings(this->m_saveDir.filePath("orders.ini"), QSettings::IniFormat);
	this->m_rakutenItemModel->load(this->m_saveDir);

	// load last
	QFile f_in(this->m_saveDir.filePath("generated_final.html"));
	if (f_in.open(QIODevice::ReadOnly))
	{
		this->m_viewForSave->setHtml(f_in.readAll());
	}
}
MainWindow::~MainWindow()
{
}


void MainWindow::closeEvent(QCloseEvent* event)
{
	this->m_rakutenItemModel->save(this->m_saveDir);

	QSettings settings;
	this->saveSettings(settings);
	event->accept();
}
void MainWindow::timerEvent(QTimerEvent* event)
{
	if (event->timerId() == this->m_imgTimerId)
	{
		this->m_viewForSave->page()->runJavaScript(R"(var image = document.querySelector('img');image.complete && image.naturalHeight !== 0;)",
			[this](const QVariant& v)
		{
			const bool ok = v.toBool();
			if (!ok)
				return;

			// images have loaded, now kill timer
			this->killTimer(this->m_imgTimerId);
			this->m_imgTimerId = 0;

			// and try to save images
			if (this->m_rank_mode)
			{
				this->async_saveRankItemImages();
			}
			else
			{
				this->async_saveNewItemImages();
			}
		});
	}
}


void MainWindow::_createActions()
{
	QMenu* fileMenu = new QMenu(from_u8(u8"ファイル"), this);
	QMenu* editMenu = new QMenu(from_u8(u8"編集"), this);
	QMenu* viewMenu = new QMenu(from_u8(u8"表示"), this);
	QMenu* helpMenu = new QMenu(from_u8(u8"ヘルプ"), this);

	this->menuBar()->addMenu(fileMenu);
	this->menuBar()->addMenu(editMenu);
	this->menuBar()->addMenu(viewMenu);
	this->menuBar()->addMenu(helpMenu);

	// ファイル
	QAction* new_act = fileMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_FileIcon), from_u8(u8"新規"), this, &MainWindow::on_new, QKeySequence::New);
	QAction* open_act = fileMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton), from_u8(u8"開く"), this, &MainWindow::on_open, QKeySequence::Open);
	QAction* save_act = fileMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton), from_u8(u8"上書き保存"), this, &MainWindow::on_save, QKeySequence::Save);
	QAction* save_as_act = fileMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton), from_u8(u8"名前を付けて保存"), this, &MainWindow::on_save_as, QKeySequence::SaveAs);
	fileMenu->addSeparator();
	QAction* exitAction = fileMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton), from_u8(u8"終了"), this, &MainWindow::close, QKeySequence::Quit);

	// ファイルバー
	QToolBar* fileToolBar = new QToolBar("&File", this);
	fileToolBar->setObjectName("&File");
	fileToolBar->addAction(new_act);
	fileToolBar->addAction(open_act);
	fileToolBar->addAction(save_act);
	fileToolBar->addAction(save_as_act);
	fileToolBar->addSeparator();
	this->addToolBar(Qt::TopToolBarArea, fileToolBar);

	// 編集バー
	QToolBar* editToolBar = new QToolBar("&Edit", this);
	editToolBar->setObjectName("&Edit");
	this->addToolBar(Qt::TopToolBarArea, editToolBar);
}
void MainWindow::_createStatusBar()
{
	this->m_statusbarLabel = new QLabel;
	this->m_statusbarLabel->setText(from_u8(u8"準備完了"));

	this->m_progressBar = new QProgressBar;
	this->m_progressBar->setRange(0, 100);

	this->statusBar()->addWidget(this->m_statusbarLabel, 1);
	this->statusBar()->addPermanentWidget(this->m_progressBar);
}


QWidget* MainWindow::_createMainTab()
{
	this->m_viewForSave = new QWebEngineView(this);
	this->m_viewForSave->setPage(new ControlledWebPage);
	this->m_viewForSave->layout()->setSpacing(0);
	this->m_viewForSave->layout()->setMargin(0);

	// for g00d
	this->m_progressDlg = new QProgressDialog(this);
	this->m_progressDlg->setAutoReset(false);
	this->m_progressDlg->hide();

	QPushButton* getOrderBtn = new QPushButton(from_u8(u8"注文を取得"));
	QPushButton* newItemBtn = new QPushButton(from_u8(u8"新着アイテム"));
	QPushButton* genBtn = new QPushButton(from_u8(u8"画像生成"));
	QPushButton* genFinalBtn = new QPushButton(from_u8(u8"HTML生成"));

	// ranking
	QObject::connect(getOrderBtn, &QPushButton::clicked, this, [this]()
	{
		searchOrder_param param;
		param.pagination.requestRecordsAmount = 1000;
		this->m_rakuten->searchOrder(param);

		this->m_progressDlg->setLabelText(from_u8(u8"注文を取得しています..."));
		this->m_progressDlg->setCancelButtonText(from_u8(u8"取消"));
		this->m_progressDlg->setRange(0, 1);
		this->m_progressDlg->setWindowModality(Qt::WindowModal);
		this->m_progressDlg->exec();
	});
	QObject::connect(this->m_rakuten, &rakuten_client::signal_searchOrderFinished, this, [this](bool ok)
	{
		// autoclose
		if (ok)
		{
			const int queued = this->m_rakuten->queue_getOrder();
			qDebug() << "queued:" << queued;

			if (queued == 0)
			{
				this->m_progressDlg->setRange(0, 1);
				this->m_progressDlg->setValue(1);
				this->m_progressDlg->setLabelText(from_u8(u8"注文を取得しました。"));
				this->m_progressDlg->setCancelButtonText(from_u8(u8"OK"));
				QTimer::singleShot(2500, [this]() { this->m_progressDlg->reset(); });
			}
			else
			{
				this->m_progressDlg->setRange(0, queued);
				this->m_progressDlg->setValue(0);
				this->m_progressDlg->setLabelText(from_u8(u8"注文を取得しています..."));
				this->m_progressDlg->setCancelButtonText(from_u8(u8"取消"));
			}
		}
		else
		{
			this->m_progressDlg->setLabelText(from_u8(u8"失敗しました。"));
			this->m_progressDlg->setCancelButtonText(from_u8(u8"閉じる"));
		}
	});
	QObject::connect(this->m_rakuten, &rakuten_client::signal_getOrderFinished, this, [this](bool ok)
	{
		if (ok)
			this->m_progressDlg->setValue(this->m_progressDlg->value() + 1);
		else
		{
			this->m_progressDlg->setValue(this->m_progressDlg->value() + 1);
		}

		if (this->m_progressDlg->value() >= this->m_progressDlg->maximum())
		{
			this->m_progressDlg->setLabelText(from_u8(u8"注文を取得しました。"));
			this->m_progressDlg->setCancelButtonText(from_u8(u8"OK"));
			QTimer::singleShot(2500, [this]() { this->m_progressDlg->reset(); });
		}
	});

	// newitem
	QObject::connect(newItemBtn, &QPushButton::clicked, this, [this]()
	{
		this->m_rakuten->IchibaItem_Search();
	});
	QObject::connect(this->m_rakuten, &rakuten_client::signal_itemSearchFinished, this, [this](const QString& itemUrl)
	{
	});

	// gen
	QObject::connect(genBtn, &QPushButton::clicked, this, [this](bool checked)
	{
		if (this->m_imgTimerId != 0)
		{
			// cannot start
			QMessageBox::warning(this, QString(), from_u8(u8"現在画像を生成中です。"));
			return;
		}

		this->m_rank_mode = true;
		this->m_rankIndex = 0;
		this->m_rankItems = this->m_rakutenItemModel->ranking(this->m_shopCodeEdit->text());
		const QString html = gen_rank_item_row(m_rankItems, 1, this->m_saveDir.filePath("ranking_generated.html"));
		if (!html.isEmpty())
		{
			this->renderDiv(html);
		}
	});
	QObject::connect(genFinalBtn, &QPushButton::clicked, this, [this](bool checked)
	{
		if (this->m_imgTimerId != 0)
		{
			// cannot start
			QMessageBox::warning(this, QString(), from_u8(u8"現在画像を生成中です。"));
			return;
		}

		this->m_rankItems = this->m_rakutenItemModel->ranking(this->m_shopCodeEdit->text());
		this->m_newItems = this->m_rakutenItemModel->new_items(this->m_shopCodeEdit->text());
		qApp->clipboard()->setText(gen_entire_html(this->m_newItems, this->m_rankItems, this->m_saveDir));
		QMessageBox::information(this, QString(), from_u8(u8"クリップボードにコピーしました。"));
	});

	QHBoxLayout* layout = new QHBoxLayout;
	layout->addWidget(getOrderBtn);
	layout->addWidget(newItemBtn);
	layout->addWidget(genBtn);
	layout->addWidget(genFinalBtn);

	QWidget* widget = new QWidget(this);
	widget->setLayout(layout);

	this->m_splitter = new QSplitter(this);
	this->m_splitter->setOrientation(Qt::Vertical);
	this->m_splitter->addWidget(this->m_viewForSave);
	this->m_splitter->addWidget(widget);

	return this->m_splitter;
}
QWidget* MainWindow::_createNewItemTab()
{
	// IchibaItem_Search > itemName itemUrl itemPrice manageNumber
	// item.search > genreId reviewNum reviewAverage image registDate
	QWidget* wid = new QWidget(this);

	return wid;
}
QWidget* MainWindow::_createRankingTab()
{
	// searchOrder > num of orders
	// getOrder > itemName manageNumber price units
	// item.search > genreId reviewNum reviewAverage image registDate
	QWidget* wid = new QWidget(this);

	return wid;
}
QWidget* MainWindow::_createProductTab()
{
	QWidget* widget = new QWidget(this);

	QLineEdit* edit = new QLineEdit(widget);
	QObject::connect(edit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		m_rakutenProxyModel->setFilterFixedString(text);
	});
	QObject::connect(this->m_rakutenItemView, &QTableView::doubleClicked, this, [this](const QModelIndex& index)
	{
		const QModelIndex sourceIndex = m_rakutenProxyModel->mapToSource(index);
		if (sourceIndex.isValid())
		{
			const QString number = this->m_rakutenItemModel->data(sourceIndex, CodeRole).toString();
			this->m_rakuten->queue_item_search(number, true);
			QDesktopServices::openUrl(this->m_rakutenItemModel->data(sourceIndex, LinkRole).toUrl());
		}
	});

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(edit);
	layout->addWidget(this->m_rakutenItemView);
	widget->setLayout(layout);

	return widget;
}
QWidget* MainWindow::_createSettingsTab()
{
	QWidget* widget = new QWidget(this);

	this->m_applicationIdEdit = new QLineEdit(widget);
	this->m_serviceSecretEdit = new QLineEdit(widget);
	this->m_licenseKeyEdit = new QLineEdit(widget);
	this->m_shopCodeEdit = new QLineEdit(widget);

	QObject::connect(m_applicationIdEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		this->m_rakuten->setApplicationId(text.toUtf8());
	});
	QObject::connect(m_serviceSecretEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		this->m_rakuten->setServiceSecret(text.toUtf8());
	});
	QObject::connect(m_licenseKeyEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		this->m_rakuten->setLicenseKey(text.toUtf8());
	});
	QObject::connect(m_shopCodeEdit, &QLineEdit::textChanged, this, [this](const QString& text)
	{
		this->m_rakuten->setShopCode(text);
		this->m_rakuten;
	});


	QFormLayout* layout = new QFormLayout;
	layout->addRow("applicationId: ", m_applicationIdEdit);
	layout->addRow("serviceSecret: ", m_serviceSecretEdit);
	layout->addRow("licenseKey: ", m_licenseKeyEdit);
	layout->addRow("shopCode: ", m_shopCodeEdit);
	widget->setLayout(layout);

	return widget;
}

void MainWindow::renderDiv(const QString& html)
{
	if (this->m_imgTimerId == 0)
	{
		this->m_viewForSave->setHtml(html);
		this->m_imgTimerId = this->startTimer(1000);
	}
}

void MainWindow::loadSettings(QSettings& settings)
{
	// load mainwindow
	settings.beginGroup("mainWindow");
	this->restoreGeometry(settings.value("geometry").toByteArray());
	this->restoreState(settings.value("state").toByteArray());

	// splitter
	settings.beginGroup("splitter");
	this->m_splitter->restoreGeometry(settings.value("geometry").toByteArray());
	this->m_splitter->restoreState(settings.value("state").toByteArray());
	settings.endGroup();

	// rakuten
	settings.beginGroup("rakuten");
	this->m_rakutenItemView->restoreGeometry(settings.value("geometry").toByteArray());
	this->m_rakutenItemView->horizontalHeader()->restoreGeometry(settings.value("h/geometry").toByteArray());
	this->m_rakutenItemView->horizontalHeader()->restoreState(settings.value("h/state").toByteArray());
	settings.endGroup();

	settings.endGroup();

	// keys
	settings.beginGroup("keys");
	this->m_applicationIdEdit->setText(settings.value("applicationId").toByteArray());
	this->m_serviceSecretEdit->setText(settings.value("serviceSecret").toByteArray());
	this->m_licenseKeyEdit->setText(settings.value("licenseKey").toByteArray());
	this->m_shopCodeEdit->setText(settings.value("shopCode").toByteArray());
	settings.endGroup();
}
void MainWindow::saveSettings(QSettings& settings)
{
	// save mainwindow
	settings.beginGroup("mainWindow");
	settings.setValue("geometry", this->saveGeometry());
	settings.setValue("state", this->saveState());

	// splitter
	settings.beginGroup("splitter");
	settings.setValue("geometry", this->m_splitter->saveGeometry());
	settings.setValue("state", this->m_splitter->saveState());
	settings.endGroup();
	//

	settings.beginGroup("rakuten");
	settings.setValue("geometry", this->m_rakutenItemView->saveGeometry());
	settings.setValue("h/geometry", this->m_rakutenItemView->horizontalHeader()->saveGeometry());
	settings.setValue("h/state", this->m_rakutenItemView->horizontalHeader()->saveState());
	settings.endGroup();

	settings.endGroup();

	// keys
	settings.beginGroup("keys");
	settings.setValue("applicationId", this->m_applicationIdEdit->text());
	settings.setValue("serviceSecret", this->m_serviceSecretEdit->text());
	settings.setValue("licenseKey", this->m_licenseKeyEdit->text());
	settings.setValue("shopCode", this->m_shopCodeEdit->text());
	settings.endGroup();
}

void MainWindow::on_new()
{

}
void MainWindow::on_open()
{
}
void MainWindow::on_save()
{
}
void MainWindow::on_save_as()
{
}

void MainWindow::async_saveRankItemImages()
{
	this->m_viewForSave->page()->runJavaScript(R"(document.querySelector('table').clientHeight)", [this](const QVariant& v)
	{
		const int clientHeight = v.toInt();
		const int clientWidth = CLIENT_WIDTH;

		const QDir rankingDir = openSubDir(this->m_saveDir, "ranking");

		// save as jpg
		if (!this->m_rankItems.isEmpty())
		{
			QPixmap pixmap(QSize(clientWidth, clientHeight));
			this->m_viewForSave->render(&pixmap, QPoint());
			pixmap.save(rankingDir.filePath(QString("%1.png").arg(++m_rankIndex)), "PNG");
			this->m_rankItems.pop_front();
		}
		if (!this->m_rankItems.isEmpty())
		{
			QPixmap pixmap2(QSize(clientWidth, clientHeight));
			this->m_viewForSave->render(&pixmap2, QPoint(), QRegion(clientWidth, 0, clientWidth, clientHeight));
			pixmap2.save(rankingDir.filePath(QString("%1.png").arg(++m_rankIndex)), "PNG");
			this->m_rankItems.pop_front();
		}
		if (!this->m_rankItems.isEmpty())
		{
			QPixmap pixmap3(QSize(clientWidth, v.toInt()));
			this->m_viewForSave->render(&pixmap3, QPoint(), QRegion(clientWidth * 2, 0, clientWidth, clientHeight));
			pixmap3.save(rankingDir.filePath(QString("%1.png").arg(++m_rankIndex)), "PNG");
			this->m_rankItems.pop_front();
		}

		// recursive call
		if (!this->m_rankItems.isEmpty())
		{
			const QString html_code = gen_rank_item_row(m_rankItems, m_rankIndex + 1, this->m_saveDir.filePath("ranking_generated.html"));
			if (!html_code.isEmpty())
			{
				this->renderDiv(html_code);
			}
		}
		else
		{
			// render new items next
			constexpr bool use_svg = false;

			this->m_rank_mode = false;
			this->m_newIndex = 0;

			const QDir newitemDir = openSubDir(this->m_saveDir, "newitem");
			this->m_newItems = this->m_rakutenItemModel->new_items(this->m_shopCodeEdit->text());

			if (!use_svg)
			{
				const QString html = gen_new_item_row(m_newItems, this->m_saveDir.filePath("new_item_row.html"));
				if (!html.isEmpty())
				{
					this->renderDiv(html);
				}
			}
			else
			{
				for (auto item : m_newItems)
				{
					gen_newitem_svg(
						item,
						newitemDir.filePath(QString("%1.svg").arg(++m_newIndex))
					);
				}
			}
		}
	});
}
void MainWindow::async_saveNewItemImages()
{
	this->m_viewForSave->page()->runJavaScript(R"(document.querySelector('table').clientHeight)", [this](const QVariant& v)
	{
		const int clientHeight = v.toInt();
		const int clientWidth = CLIENT_WIDTH;
		const QDir newitemDir = openSubDir(this->m_saveDir, "newitem");

		// save as jpg
		if (!this->m_newItems.isEmpty())
		{
			QPixmap pixmap(QSize(clientWidth, clientHeight));
			this->m_viewForSave->render(&pixmap, QPoint());
			pixmap.save(newitemDir.filePath(QString("%1.png").arg(++m_newIndex)), "PNG");
			m_newItems.pop_front();
		}
		if (!this->m_newItems.isEmpty())
		{
			QPixmap pixmap2(QSize(clientWidth, clientHeight));
			this->m_viewForSave->render(&pixmap2, QPoint(), QRegion(clientWidth, 0, clientWidth, clientHeight));
			pixmap2.save(newitemDir.filePath(QString("%1.png").arg(++m_newIndex)), "PNG");
			m_newItems.pop_front();
		}
		if (!this->m_newItems.isEmpty())
		{
			QPixmap pixmap3(QSize(clientWidth, v.toInt()));
			this->m_viewForSave->render(&pixmap3, QPoint(), QRegion(clientWidth * 2, 0, clientWidth, clientHeight));
			pixmap3.save(newitemDir.filePath(QString("%1.png").arg(++m_newIndex)), "PNG");
			m_newItems.pop_front();
		}

		// recursive call
		if (!this->m_newItems.isEmpty())
		{
			const QString html = gen_new_item_row(m_newItems, this->m_saveDir.filePath("new_item_row.html"));
			if (!html.isEmpty())
			{
				this->renderDiv(html);
			}
		}
	});
}