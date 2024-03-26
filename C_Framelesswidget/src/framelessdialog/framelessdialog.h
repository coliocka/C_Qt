﻿#pragma once

#include <QDialog>

#ifdef FRAMELESSDIALOG_LIB
#define FRAMELESSDIALOG_EXPORT Q_DECL_EXPORT
#else
#define FRAMELESSDIALOG_EXPORT Q_DECL_IMPORT
#endif

class FRAMELESSDIALOG_EXPORT FramelessDialog : public QDialog {
  Q_OBJECT
  public:
	explicit FramelessDialog(QWidget* parent = nullptr);

  protected:
	//窗体显示的时候触发
	void showEvent(QShowEvent* event) override;

	//事件过滤器识别拖动拉伸等
	void doWindowStateChange(QEvent* event);
	void doResizeEvent(QEvent* event);
	bool eventFilter(QObject* watched, QEvent* event) override;

	//拦截系统事件用于修复系统休眠后唤醒程序的bug
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
	bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
#endif

  private:
	//边距+可移动+可拉伸
	int padding;
	bool moveEnable;
	bool resizeEnable;

	//标题栏控件
	QWidget* titleBar;

	//鼠标是否按下+按下坐标+按下时窗体区域
	bool mousePressed;
	QPoint mousePoint;
	QRect mouseRect;

	//鼠标是否按下某个区域+按下区域的大小
	//依次为 左侧+右侧+上侧+下侧+左上侧+右上侧+左下侧+右下侧
	QList<bool> pressedArea;
	QList<QRect> pressedRect;

	//记录是否最小化
	bool isMin;
	//存储窗体默认的属性
	Qt::WindowFlags flags;

  public Q_SLOTS:
	//设置边距+可拖动+可拉伸
	void setPadding(int _padding);
	void setMoveEnable(bool _moveEnable);
	void setResizeEnable(bool _resizeEnable);

	//设置标题栏控件
	void setTitleBar(QWidget* _titleBar);

  Q_SIGNALS:
	void titleDblClick();
	void windowStateChange(bool max);
};
