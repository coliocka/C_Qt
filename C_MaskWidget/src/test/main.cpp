﻿#include <QApplication>
#include <QTextCodec>

#include "test.h"

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

#if (QT_VERSION < QT_VERSION_CHECK(5,0,0))
	#if _MSC_VER
    QTextCodec *codec = QTextCodec::codecForName("gbk");
#else
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
#endif
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
#else
	QTextCodec *codec = QTextCodec::codecForName("utf-8");
	QTextCodec::setCodecForLocale(codec);
#endif
	test w;
	w.show();
    return QApplication::exec();
}