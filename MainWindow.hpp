#pragma once

#include "rakuten.hpp"

// forward declare
class RakutenItemModel;
class rakuten_client;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	QString getShopCode() const;

protected:
	void closeEvent(QCloseEvent* event) override;
	void timerEvent(QTimerEvent* event) override;

private:
	void _createActions();
	void _createStatusBar();

	QWidget* _createMainTab();
	QWidget* _createNewItemTab();
	QWidget* _createRankingTab();
	QWidget* _createProductTab();
	QWidget* _createSettingsTab();

	void renderDiv(const QString& html);

public slots:
	void loadSettings(QSettings& settings);
	void saveSettings(QSettings& settings);

private slots:
	// file
	void on_new();
	void on_open();
	void on_save();
	void on_save_as();

	void get_orders();
	void newitems();
	void gen_png();
	void gen_html();
	void upload_pngs();

private:
	void async_saveRankItemImages();
	void async_saveNewItemImages();

private:

	// folder
	QDir m_saveDir;

	// workers
	rakuten_client* m_rakuten;

	// controls
	QLabel* m_statusbarLabel;
	QProgressBar* m_progressBar;
	QSplitter* m_splitter;
	QWebEngineView* m_viewForSave;
	//

	int m_imgTimerId;


	// rakuten view
	QSortFilterProxyModel* m_rakutenProxyModel;
	RakutenItemModel* m_rakutenItemModel;
	QTableView* m_rakutenItemView;


	// generator
	bool m_rank_mode;
	int m_rankIndex, m_newIndex;
	QVector<item::search::item_ptr> m_rankItems, m_newItems;


	// settings
	QLineEdit* m_applicationIdEdit, *m_serviceSecretEdit, * m_licenseKeyEdit, *m_shopCodeEdit,

		// ftp
		*m_ftpUserEdit, *m_ftpPasswordEdit, *m_ftpRelativePathEdit;


	//
	QTabWidget* m_mainTabWidget;
};