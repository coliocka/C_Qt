#include "readxml.h"

readxml::readxml(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	QString filePath = QApplication::applicationDirPath() + "/Config/testconfig.xml";
	QString str = Manipulating_Xml::readXml(filePath);
	ui.plainTextEdit->appendPlainText(str);

	filePath = QApplication::applicationDirPath() + "/Config/test.xml";
	qDebug() << (Manipulating_Xml::modifyXml(filePath) ? "�޸ĳɹ�" : "�޸�ʧ��");
	qDebug() << (Manipulating_Xml::addXml(filePath) ? "��ӳɹ�" : "���ʧ��");
	qDebug() << (Manipulating_Xml::removeXml(filePath) ? "ɾ���ɹ�" : "ɾ��ʧ��");
}
