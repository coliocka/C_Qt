#include "readxml.h"

readxml::readxml(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	Config config;
	QString str = config.readConfig();
	ui.plainTextEdit->appendPlainText(str);

	bool err = config.modifyXml();
	if (!err) {
		QMessageBox::warning(NULL, "����", "�޸�ʧ��!");
	}
	else {
		QMessageBox::information(NULL, "��ʾ", "�޸ĳɹ�!");
	}
}
