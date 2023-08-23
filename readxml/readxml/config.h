#ifdef WIN32
#pragma execution_character_set("utf-8")
#endif
#pragma once
#include <QXmlStreamReader>
#include <QFile>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>

class Config
{
public:
	Config();

	QString readConfig(); // ��ȡXml �ļ�
	bool modifyXml(); // �޸�Xml �ļ�
	void addXml(); // ����Xml �ļ�
	void removeXml(); // ɾ��xml �ļ�
};