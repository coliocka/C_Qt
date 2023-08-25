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

/*
 ���� Xml �ļ�����ɾ��Ĳ���
*/
class Manipulating_Xml
{
public:
	Manipulating_Xml();

public:
	/*************************************************
	* @arg: ��Ҫ��ȡ�� Xml �ļ�·��
	* @Fun: ��ȡXml�ļ��� plainTextEdit �ؼ���
	* @return: ��ȡ�����ַ��� QString
	* **********************************************/
	static QString readXml(const QString filePath); 
	/*************************************************
	* @arg: ��Ҫ�޸ĵ� Xml �ļ�·��
	* @Fun: �޸� Xml �ļ�������
	* @return: �޸ĳɹ� true���޸�ʧ�� false
	* **********************************************/
	static bool modifyXml(const QString filePath);
	/*************************************************
	* @arg: ��Ҫ��ӵ� Xml �ļ�·��
	* @Fun: ������ݵ� Xml �ļ���
	* @return: ��ӳɹ� true�����ʧ�� false
	* **********************************************/
	static bool addXml(const QString filePath);
	/*************************************************
	* @arg: ��Ҫɾ���� Xml �ļ�·��
	* @Fun: ɾ�� Xml �ļ��е�����
	* @return: ɾ���ɹ� true��ɾ��ʧ�� false
	* **********************************************/
	static bool removeXml(const QString filePath);
};