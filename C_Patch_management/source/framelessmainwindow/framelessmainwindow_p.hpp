#pragma once

#include <QWidget>

class FramelessMainWindow;

class FramelessMainWindowPrivate {
	Q_DISABLE_COPY(FramelessMainWindowPrivate)
	Q_DECLARE_PUBLIC(FramelessMainWindow)

public:
	explicit FramelessMainWindowPrivate(FramelessMainWindow* q);
	~FramelessMainWindowPrivate();

	void init();

	FramelessMainWindow* const q_ptr{ nullptr };
	// �߾�+���ƶ�+������
	int padding{};
	bool moveEnable{};
	bool resizeEnable{};

	// �������ؼ�
	QWidget* titleBar{ nullptr };

	// ����Ƿ��¡��������ꡢ����ʱ��������
	bool mousePressed{};
	QPoint mousePoint{};
	QRect mouseRect{};

	// ����Ƿ���ĳ�����򡢰�������Ĵ�С
	// ����Ϊ ���ҡ��ϡ��¡����ϡ����ϡ����¡�����
	QList<bool> pressedArea{};
	QList<QRect> pressedRect{};

	// ��¼�Ƿ���С��
	bool isMin{};
	// �洢����Ĭ�ϵ�����
	Qt::WindowFlags flags{};
};
