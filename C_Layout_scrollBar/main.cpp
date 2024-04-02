#include <QApplication>
#include <QLabel>
#include <QBoxLayout>
#include <QPushButton>
#include <QScrollArea>

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	auto* centerWidget = new QWidget;
	centerWidget->resize(500, 600);

	auto* m_centerVLayout = new QVBoxLayout(centerWidget);
	auto* label = new QLabel("Title");
	m_centerVLayout->addWidget(label);

	// ������������
	auto* scrollArea = new QScrollArea(centerWidget);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea->setWidgetResizable(true);
	m_centerVLayout->addWidget(scrollArea);

	// ��������������Ӳ���
	auto* widget = new QWidget(scrollArea);
	scrollArea->setWidget(widget);

	// �����Ӳ����Ĵ�ֱ���ֹ�����
	auto* layout = new QVBoxLayout(widget);
	layout->setAlignment(Qt::AlignTop);
	widget->setLayout(layout);

	// ���Ӳ�������Ӱ�ť
	for (int i = 0; i < 111; ++i) {
		auto* button = new QPushButton(QString("%1").arg(i));
		layout->addWidget(button);
	}

	centerWidget->show();

	return QApplication::exec();
}
