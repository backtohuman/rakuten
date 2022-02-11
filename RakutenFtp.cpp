#include "stdafx.h"

#include "RakutenFtp.hpp"
#include "qt_helper.hpp"


/*
connect > login > mkdir > cd > put
	cd > mkdir > cd > put
*/



RakutenFtp::RakutenFtp(QWidget* parent) : QProgressDialog(parent)
{
	this->m_ftp = new QFtp(this);

	QObject::connect(this->m_ftp, &QFtp::stateChanged, this, &RakutenFtp::onStateChanged);
	QObject::connect(this->m_ftp, &QFtp::listInfo, this, &RakutenFtp::onListInfo);
	QObject::connect(this->m_ftp, &QFtp::rawCommandReply, this, &RakutenFtp::onRawCommandReply);

	QObject::connect(this->m_ftp, &QFtp::commandStarted, this, &RakutenFtp::onCommandStarted);
	QObject::connect(this->m_ftp, &QFtp::commandFinished, this, &RakutenFtp::onCommandFinished);
	QObject::connect(this->m_ftp, &QFtp::done, this, &RakutenFtp::onDone);
}
RakutenFtp::~RakutenFtp()
{
}


bool RakutenFtp::isConnected() const
{
	const auto ftp_state = this->m_ftp->state();
	return ftp_state == QFtp::Connected || ftp_state == QFtp::LoggedIn;
}
bool RakutenFtp::isLoggedIn() const
{
	return this->m_ftp->state() == QFtp::LoggedIn;
}

int RakutenFtp::pwd()
{
	return this->m_ftp->rawCommand("PWD");
}

void RakutenFtp::setRelativePath(const QString& path)
{
	this->lst.part = path;
}

void RakutenFtp::setLocalDirs(const QDir& ndir, const QDir& rdir)
{
	this->ndir = ndir;
	this->rdir = rdir;
}

void RakutenFtp::setRemoteDirs(const QString& npath, const QString& rpath)
{
	this->npath = npath;
	this->rpath = rpath;

	/*QString absolutePath, testDir;
	if (npath.startsWith('/'))
	{
		absolutePath = QDir::cleanPath(npath);
		testDir = "/";
		m_mkdirCommands.push_back(absolutePath);
	}
	else
	{
		absolutePath = QDir::cleanPath(this->m_currentDirectory.absoluteFilePath(npath));
		testDir = m_currentDirectory.absolutePath();
		m_mkdirCommands.push_back(absolutePath);
	}

	auto it = this->m_remoteFiles.find(testDir);
	if (it != this->m_remoteFiles.end())
	{
		if (it.value().contains(""))
		{

		}
		else
		{
			this->m_ftp->mkdir("");
		}
	}
	else
	{
		// list
		this->async_list(testDir);
	}*/
}

QString RakutenFtp::async_cd(const QString& path)
{
	if (path.startsWith('/'))
	{
		// absolute path
		/*this->m_cdCommands[
			this->m_ftp->cd(path)
		] = path;*/
		return path;
	}
	else
	{
		if (!this->m_currentDirectory.isEmpty())
		{
			qWarning() << "cd while current directory is not set";
		}

		// relative path
		const QString absolutePath = this->m_currentDirectory.absoluteFilePath(path);
		/*this->m_cdCommands[
			this->m_ftp->cd(path)
		] = absolutePath;*/
		return absolutePath;
	}
}

void RakutenFtp::async_login(const QString& user, const QString& pass)
{
	this->m_ftp->connectToHost("ftp.rakuten.ne.jp", 16910);
	this->m_ftp->login(user, pass);
	this->m_ftp->list("/m_top/n");
}

void RakutenFtp::async_uploadPNGs(const QString& remote, const QDir& local)
{
	for (const QFileInfo& fi : local.entryInfoList())
	{
		// only png file
		if (!fi.isFile() || fi.suffix().compare(QLatin1String("png"), Qt::CaseInsensitive) != 0)
			continue;

		QFile* f_in = new QFile(fi.absoluteFilePath(), this);
		if (!f_in->open(QIODevice::ReadOnly))
		{
			qWarning() << "file open error" << f_in->errorString();
			continue;
		}

		this->m_ftp->put(f_in, fi.fileName());
	}
}


void RakutenFtp::onStateChanged(int state)
{
	qDebug() << "onStateChanged" << state;
}

void RakutenFtp::onCommandStarted(int id)
{
	qDebug() << "onCommandStarted" << id;
}

void RakutenFtp::onCommandFinished(int id, bool error)
{
	if (error)
	{
		switch (this->m_ftp->currentCommand())
		{
			case QFtp::Command::List:
			{
				break;
			}
			case QFtp::Command::Cd:
			{
				//this->m_ftp->mkdir("");
				//this->m_ftp->cd("");
				break;
			}
			default:
			{
				qWarning() << "FTP Command" << id << "failed with error:" << this->m_ftp->errorString();
				break;
			}
		}

		// don't fallthrough
		return;
	}

	switch (this->m_ftp->currentCommand())
	{
		case QFtp::Command::List:
		{
			break;
		}
		case QFtp::Command::Cd:
		{
			this->pwd();
			break;
		}
		case QFtp::Command::RawCommand:
		{
			break;
		}
		default:
		{
			qDebug() << "onCommandFinished" << id << "error" << error;
			break;
		}
	}

	if (1)
	{
		const int newval = qMin<>(this->value() + 1, this->maximum() - 1);
		this->setValue(newval);
	}
}

void RakutenFtp::onDone(bool error)
{
	// This signal is emitted when the last pending command has finished; (it is emitted after the last command's commandFinished() signal).
	// error is true if an error occurred during the processing; otherwise error is false.
	qDebug() << "done error:" << error;

	if (error)
	{
		this->setLabelText(this->m_ftp->errorString());
		return;
	}
	else
	{
		return;
	}

	//
	if (m_commandIndex == 0)
	{
		// login done
		if (!this->isLoggedIn())
		{
			// wut
			qWarning() << "client is not logged in";
			return;
		}

		// check
		bool _exists = false;
		for (const QUrlInfo& i : lst.files)
		{
			if (i.isDir() && i.name().compare(lst.part, Qt::CaseInsensitive) == 0)
			{
				_exists = true;
				break;
			}

		}

		++m_commandIndex;
		if (!_exists)
		{
			this->m_ftp->mkdir(lst.part);
			this->async_cd(lst.part);
			this->m_ftp->mkdir("r");
			this->m_ftp->mkdir("n");
		}
		else
		{
			this->onDone(0);
		}
	}
	else if (m_commandIndex == 1)
	{
		// mkdirs done
		async_uploadPNGs(npath, this->ndir);
		async_uploadPNGs(rpath, this->rdir);

		// close cmd
		const int lastCommandId = this->m_ftp->close();

		// update ui
		this->setRange(0, lastCommandId);
		this->setValue(this->m_ftp->currentId());
		this->setLabelText(from_u8(u8"ファイルをアップロードしています..."));

		++m_commandIndex;
	}
	else
	{
		// all done
		this->setValue(this->maximum());
	}
}

void RakutenFtp::onListInfo(const QUrlInfo& i)
{
	const QString absolutePath = this->m_currentDirectory.absolutePath();
	if (absolutePath.isEmpty())
	{
		qWarning() << "current directory is not set";
		return;
	}

	this->m_remoteFiles[absolutePath].push_back(i);
	qDebug() << this->m_currentDirectory.absoluteFilePath(i.name());
}

void RakutenFtp::onRawCommandReply(int code, const QString &text)
{
	qDebug() << "rawcommandreply" << code << text;

	if (code == 257)
	{
		// mkdir or pwd
		const QString absolutePath = text.section("\"", 0);
		if (absolutePath.isEmpty())
		{
			this->m_currentDirectory = "/";
		}
		else
		{
			this->m_currentDirectory = absolutePath;
		}
	}
}