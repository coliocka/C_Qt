#pragma once

#include <QActionGroup>
#include <QMenuBar>
#include <QSystemTrayIcon>

#include "global/interface_global.h"
#include "framelessmainwindow/framelessmainwindow.h"
#include "framelessMessageBox/uvmessagebox.h"

class Test final : public FramelessMainWindow {
	Q_OBJECT

public:
	explicit Test(WINDOWLANAGUAGE Lanaguage = WINDOWLANAGUAGE::Chinese,
	              WINDOWTHEMESTYLE ThemeStyle = WINDOWTHEMESTYLE::LIGHT,
	              QWidget* parent = nullptr);
	~Test() override;

	[[nodiscard]] QSystemTrayIcon* getTrayIcon() const { return m_ptrayIcon; }

signals:
	void ConfChanged(WINDOWLANAGUAGE, WINDOWTHEMESTYLE);
	void ThemeChanged(WINDOWTHEMESTYLE);

private:
	void createCtrl();
	void customLayout() const;
	void init();
	void initConnection();
	/*
	 * @note: ��������
	 */
	static void restart();
	/*
	 * @note: �ָ���ť״̬
	 */
	void recoveryStateWithAct() const;
	/*
	 * @note: ����ϵͳ����ͼ��
	 */
	void setSystemTrayIcon();
	/*
		 * @note: ����ģ��
		 */
	template<typename T>
	T* createWidget(QWidget* parent = nullptr, const QString& objName = "") {
		T* widget = new T(parent);
		widget->setObjectName(objName);
		return widget;
	}

	template<typename T>
	T* createWidget(const QString& text, QWidget* parent = nullptr, const QString& objName = "") {
		T* widget = new T(text, parent);
		widget->setObjectName(objName);
		return widget;
	}

private slots:
	void onBtnStyleClicked();
	void onActChineseClicked();
	void onActEnglishClicked();
	void onSystemTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

private:
	QWidget* m_pCentralWidget{ nullptr };

	QWidget* m_pTitleBar{ nullptr };
	QPushButton* m_pBtnStyle{ nullptr };
	QPushButton* m_pBtnClose{ nullptr };
	QPushButton* m_pBtnMin{ nullptr };

	QMenuBar* m_pMenuBar{ nullptr };
	QMenu* m_menuPreference{ nullptr };
	QMenu* m_menuLanguage{ nullptr };
	QActionGroup* m_pActGroupLanguage{ nullptr };
	QAction* m_pActChinese{ nullptr };
	QAction* m_pActEnglish{ nullptr };

	QHBoxLayout* m_plyHTitle{ nullptr };
	QVBoxLayout* m_pCentralLayout{ nullptr };

	WINDOWLANAGUAGE m_language{};                  // ����
	WINDOWTHEMESTYLE m_ThemeStyle{};               // ����
	QSystemTrayIcon* m_ptrayIcon{ nullptr };       // ϵͳ����ͼ��
	std::unique_ptr<QMenu> m_ptrayMenu{ nullptr }; // ϵͳ���̲˵�
	QAction* m_pActShow{ nullptr };                // ��ʾ�����水ť
	QAction* m_pActQuit{ nullptr };                // �˳���ť
};
