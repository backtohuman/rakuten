#pragma once

#include <QtFtp>

//
// cd > list > mkdir
//
class RakutenFtp : public QProgressDialog
{
	Q_OBJECT

public:
	RakutenFtp(QWidget* parent = nullptr);
	~RakutenFtp();

	bool isConnected() const;
	bool isLoggedIn() const;

	int pwd();

	//
	void setRelativePath(const QString& path);
	void setLocalDirs(const QDir& ndir, const QDir& rdir);
	void setRemoteDirs(const QString& npath, const QString& rpath);

	//
	QString async_cd(const QString& path);
	void async_login(const QString& user, const QString& pass);


	//
	void async_uploadPNGs(const QString& remote, const QDir& local);

private slots:
	void onStateChanged(int state);
	void onCommandStarted(int id);
	void onCommandFinished(int id, bool error);
	void onDone(bool error);
	void onListInfo(const QUrlInfo& i);
	void onRawCommandReply(int, const QString&);

private:
	QList<QString> m_mkdirCommands;

	// 
	QFtp* m_ftp;
	QDir m_currentDirectory;
	QMap<QString, QList<QUrlInfo>> m_remoteFiles;

	//
	QString npath, rpath;
	QDir ndir, rdir;

	//
	struct
	{
		QString part;
		//QStringList parts;
		QList<QUrlInfo> files;
	} lst;

	//
	int m_commandIndex = 0;
};