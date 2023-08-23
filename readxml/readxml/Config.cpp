#include "config.h"

Config::Config() {}

QString Config::readConfig() {
	QFile file;
	QString nodename;
	QString Output;

	file.setFileName(QApplication::applicationDirPath() + "/Config/testconfig.xml");
	qDebug() << QApplication::applicationDirPath();
	if (!file.exists()) {
		qDebug() << "testconfig.xml �ļ�������";
	}
	else {
		qDebug() << "testconfig.xml �ļ�����";
	}
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "���ļ�ʧ��";
	}
	else {
		QXmlStreamReader xmlreader(&file);
		qDebug() << "xml�ļ����ڣ��򿪳ɹ�!";
		while (!xmlreader.atEnd() || !xmlreader.hasError()) {
			xmlreader.readNextStartElement();
			nodename = xmlreader.name().toString();
			if (nodename == "Data" && xmlreader.isStartElement()) {
				xmlreader.attributes().value("id").toString();
				Output.append(QString("ID:%1").arg(xmlreader.attributes().value("id").toString()));
				Output.push_back("\n");
				while (!(nodename == "Data" && xmlreader.isEndElement())) {
					xmlreader.readNextStartElement();
					nodename = xmlreader.name().toString();
					if (nodename == "Number" && xmlreader.isStartElement()) {
						while (!(nodename == "Number" && xmlreader.isEndElement())) {
							Output.append(QString("Number:%1").arg(xmlreader.readElementText()));
							Output.push_back("\n");
						}
					}
					else if (nodename == "Name" && xmlreader.isStartElement()) {
						while (!(nodename == "Name" && xmlreader.isEndElement())) {
							Output.append(QString("Name:%1").arg(xmlreader.readElementText()));
							Output.push_back("\n");
						}
					}
					else if (nodename == "email" && xmlreader.isStartElement()) {
						while (!(nodename == "email" && xmlreader.isEndElement())) {
							Output.append(QString("email:%1").arg(xmlreader.readElementText()));
							Output.push_back("\n");
						}
					}
					else if (nodename == "website" && xmlreader.isStartElement()) {
						while (!(nodename == "website" && xmlreader.isEndElement())) {
							Output.append(QString("website:%1").arg(xmlreader.readElementText()));
							Output.push_back("\n");
							Output.append("------------------\n");
						}
					}
				}
			}
		}
	}
	file.close();
	return Output;
}

bool Config::modifyXml()
{
	QFile file;
	file.setFileName(QApplication::applicationDirPath() + "/Config/test.xml");
	if (!file.open(QFile::ReadOnly))
		return false;

	//����һ����ǩ��,���֪��xml�Ľṹ��ֱ�Ӷ�λ���Ǹ���ǩ�϶������
	//�����ñ����ķ���ȥƥ��tagname����attribut��value������
	QDomDocument doc;
	if (!doc.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();

	QDomElement root = doc.documentElement();
	QDomNodeList list = root.elementsByTagName("book");
	QDomNode node = list.at(list.size() - 1).firstChild(); //��λ��������һ���ӽڵ����Ԫ��
	QDomNode oldnode = node.firstChild(); //��ǩ֮���������Ϊ�ڵ���ӽڵ����,��ǰ��Pride and Projudice
	node.firstChild().setNodeValue("Crucal");
	QDomNode newnode = node.firstChild();
	node.replaceChild(newnode, oldnode);

	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;
	//������ļ�
	QTextStream out_stream(&file);
	doc.save(out_stream, 4); //����4��

	file.close();
	return true;
}
