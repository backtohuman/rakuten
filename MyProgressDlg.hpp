#pragma once

class MyProgressDlg : public QProgressDialog
{
	Q_OBJECT

public:
	MyProgressDlg(QWidget* parent) : QProgressDialog(parent)
	{

	}

public slots:
	void incValue()
	{
		this->setValue(this->value() + 1);
	}
	void addTask(int n)
	{
		this->setMaximum(this->maximum() + n);
	}
};