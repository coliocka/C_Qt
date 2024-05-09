//
// Created by crucal on 2024-03-17.
//
#pragma once

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "framelesswidget/framelesswidget.hpp"

class framelesswidget_p final : public FramelessWidget {
	Q_OBJECT

public:
	explicit framelesswidget_p(QWidget* parent = nullptr);

private slots:
	void onBtnMinClicked();
	void onBtnMaxClicked();
	void onBtnCloseClicked();

private:
	void createCtrl();
	void layoutcustom() const;
	void initConnection();
	void initStyle() const;

	QLabel* m_pLbTitle{ nullptr };

	QLabel* m_pLbTitleText{ nullptr };
	QPushButton* m_pBtnMin{ nullptr };
	QPushButton* m_pBtnMax{ nullptr };
	QPushButton* m_pBtnClose{ nullptr };

	QWidget* m_pCenterWidget{ nullptr };

	QHBoxLayout* m_pHBoxLayTitle{ nullptr };
	QVBoxLayout* m_pVBoxLayCenter{ nullptr };
};
