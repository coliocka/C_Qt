#pragma once

#include <QtWidgets/QMainWindow>
#include <QPlainTextEdit>
#include "ui_readxml.h"
#include "Manipulating_Xml.h"
class readxml : public QMainWindow
{
    Q_OBJECT

public:
    readxml(QWidget *parent = Q_NULLPTR);

private:
    Ui::readxmlClass ui;
	QPlainTextEdit* plainTextEdit;
};
