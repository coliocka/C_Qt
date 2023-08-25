#include "Manipulating_Xml.h"

Manipulating_Xml::Manipulating_Xml() {}

QString Manipulating_Xml::readXml() {
	QFile file;
	QString nodename;
	QString Output;

	file.setFileName(QApplication::applicationDirPath() + "/Config/testconfig.xml");
	qDebug() << QApplication::applicationDirPath();
	if (!file.exists()) {
		qDebug() << "testconfig.xml �ļ�������";
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

bool Manipulating_Xml::modifyXml()
{
	QFile file;
	QString filePath = QApplication::applicationDirPath() + "/Config/test.xml";
	file.setFileName(filePath);
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

bool Manipulating_Xml::addXml()
{
	QFile file;
	QString filePath = QApplication::applicationDirPath() + "/Config/test.xml";
	file.setFileName(filePath);
	if (!file.open(QFile::ReadOnly))
		return false;

	//����һ��һ���ӽڵ��Լ�Ԫ��
	QDomDocument doc;
	if (!doc.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();

	QDomElement root = doc.documentElement();
	QDomElement book = doc.createElement("book");
	book.setAttribute("id", 3);
	book.setAttribute("time", "1813/1/27");
	QDomElement title = doc.createElement("title");
	QDomText text;
	text = doc.createTextNode("Pride and Prejudice");
	title.appendChild(text);
	book.appendChild(title);
	QDomElement author = doc.createElement("author");
	text = doc.createTextNode("Jane Austen");
	author.appendChild(text);
	book.appendChild(author);
	root.appendChild(book);

	if (!file.open(QFile::WriteOnly | QFile::Truncate)) //�ȶ�����������д���������truncate�����ں���׷�����ݣ�����Ч��
		return false;
	//������ļ�
	QTextStream out_stream(&file);
	doc.save(out_stream, 4); //����4��
	file.close();
	return true;
}

bool Manipulating_Xml::removeXml()
{    
	QFile file;
	QString filePath = QApplication::applicationDirPath() + "/Config/test.xml";
	file.setFileName(filePath);
	if (!file.open(QFile::ReadOnly))
		return false;

	//ɾ��һ��һ���ӽڵ㼰��Ԫ�أ����ڵ�ɾ���ڲ�ڵ��ڴ���ͬ
	QDomDocument doc;
	if (!doc.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();  //һ��Ҫ�ǵùص�������Ȼ�޷���ɲ���

	QDomElement root = doc.documentElement();
	QDomNodeList list = doc.elementsByTagName("book"); //�ɱ�ǩ����λ
	for (int i = 0; i<list.count(); i++)
	{
		QDomElement e = list.at(i).toElement();
		if (e.attribute("time") == "1813/1/27")  //����������λ��������hash�ķ�ʽ��warning���������ɾ��һ���ڵ㣬��ʵ���ԼӸ�break
			root.removeChild(list.at(i));
	}

	if (!file.open(QFile::WriteOnly | QFile::Truncate))
		return false;
	//������ļ�
	QTextStream out_stream(&file);
	doc.save(out_stream, 4); //����4��
	file.close();
	return true;
}
