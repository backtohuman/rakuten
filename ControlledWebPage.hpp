#pragma once

class ControlledWebPage : public QWebEnginePage
{
public:
	ControlledWebPage(QObject* parent = Q_NULLPTR) : QWebEnginePage(parent) {}
	virtual ~ControlledWebPage() { }

protected:
	bool acceptNavigationRequest(const QUrl& url, QWebEnginePage::NavigationType type, bool isMainFrame)
	{
		if (type == QWebEnginePage::NavigationTypeTyped)
			return true;

		return false;
	}
};