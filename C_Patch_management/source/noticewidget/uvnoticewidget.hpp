#pragma once

#include <QLabel>
#include <QList>
#include <QtGlobal>
#include <QTimer>

//��ʽ��������ɫ����ɫ��Բ�ǣ�����ɫ͸����
#define STYLE_SHEET "color:%1;border-radius:8px;background-color:rgba(80, 80, 80, %2);"

#ifdef NOTICEWIDGET_LIB
#define NOTICEWIDGET_EXPORT Q_DECL_EXPORT
#else
#define NOTICEWIDGET_EXPORT Q_DECL_IMPORT
#endif

/*
 * @brief ��ʾ�ؼ���������ʧ
 */

class NOTICEWIDGET_EXPORT CUVNoticeWidget final : public QLabel {
	Q_OBJECT

public:
	explicit CUVNoticeWidget(QWidget* parent = nullptr);
	~CUVNoticeWidget() override;

	/*
	 * @note ��ʾ��ʾ��Ϣ
	 * @param parent ���ؼ�
	 * @param msg ��ʾ��Ϣ
	 * @param delay_ms ��ʾʱ�䣬��λms��Ĭ��2000ms
	 * @param bsuccess �Ƿ�ɹ����ɹ�Ϊ��ɫ��ʧ��Ϊ��ɫ
	 */
	void notice(QWidget* parent, const QString& msg, int delay_ms = 2000, bool bsuccess = true);

private:
	/*
	 * @note ������ʾ��Ϣ��ȷ��ʼ���ڸ������м���ʾ��������С
	 * @param msg ��ʾ��Ϣ
	 * @param delay_ms ��ʾʱ�䣬��λms��Ĭ��2000ms
	 * @param bsuccess �Ƿ�ɹ����ɹ�Ϊ��ɫ��ʧ��Ϊ��ɫ
	 */
	void setMessage(const QString& msg, int delay_ms, bool bsuccess = true);
	/*
	 * @note ������С
	 */
	void changeSize();

public slots:
	/*
	 * @note ������ʧ����
	 */
	void OnTimerTimeout();

private:
	QWidget* m_Parent{ nullptr };
	QTimer* m_Timer{ nullptr };
	int m_TimerCount{};
	int m_BaseWidth{};              //��һ��ʱ��Ŀ��
	int m_BaseHeight{};             //һ�и߶�
	int m_MinHeight{};              //��С�߶�
	int m_TransparentVal{ 255 };    //͸����0~255��ֵԽСԽ͸��
	QString m_fontcolor{ "green" }; // ������ɫ
	QList<int> m_ListLinesLen{};

	int m_timer_interval_ms{ 50 };                                                   // ��ʱ���������λms
	int m_notice_def_delay_cnt{ 1000 / m_timer_interval_ms };                        // Ĭ����ʾʱ��1s
	int m_transparent_max_val{ 255 };                                                // ͸�������ֵ255��Ҳ���ǲ�͸��
	int m_transparent_cut_val{ m_transparent_max_val / m_notice_def_delay_cnt + 1 }; // ͸���ȵݼ�ֵ
	double m_size_scale{ 0.2 };                                                      // ��С����
	int m_patch_height{ 10 };                                                        // �߶Ȳ���
};
