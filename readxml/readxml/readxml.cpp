#include "readxml.h"

readxml::readxml(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	Manipulating_Xml config;
	QString str = config.readXml();
	ui.plainTextEdit->appendPlainText(str);
	qDebug() << (config.modifyXml() ? "�޸ĳɹ�" : "�޸�ʧ��");
	qDebug() << (config.addXml() ? "��ӳɹ�" : "���ʧ��");
	qDebug() << (config.removeXml() ? "ɾ���ɹ�" : "ɾ��ʧ��");
}
