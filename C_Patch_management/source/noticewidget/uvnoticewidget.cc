#include "uvnoticewidget.hpp"

#include <QFontMetrics>
#include <QTime>

CUVNoticeWidget::CUVNoticeWidget(QWidget* parent) : QLabel(parent) {
	m_TimerCount = m_notice_def_delay_cnt;
	setAlignment(Qt::AlignCenter);

	m_Timer = new QTimer(this);
	connect(m_Timer, &QTimer::timeout, this, &CUVNoticeWidget::OnTimerTimeout, Qt::UniqueConnection);
}

CUVNoticeWidget::~CUVNoticeWidget() {
	if (m_Timer->isActive()) {
		m_Timer->stop();
	}

	delete m_Timer;
	deleteLater();
}

void CUVNoticeWidget::notice(QWidget* parent, const QString& msg, const int delay_ms, const bool bsuccess) {
	if (m_Timer->isActive()) {
		m_Timer->stop();
		setVisible(false);
	}

	if (msg.isEmpty() || delay_ms <= 0) {
		return;
	}

	setParent(parent);
	setMessage(msg, delay_ms, bsuccess);
	m_Timer->start(m_timer_interval_ms);
}

void CUVNoticeWidget::setMessage(const QString& msg, const int delay_ms, const bool bsuccess) {
	QStringList string_list = msg.split("\n");
	const QFontMetrics font_metrics(font());
	m_ListLinesLen.clear();

	int maxLinelen = 1; // �����һ��
	for (const auto& s : string_list) {
		int tmpW = font_metrics.horizontalAdvance(s);
		m_ListLinesLen.append(tmpW);
		if (maxLinelen < tmpW) {
			maxLinelen = tmpW;
		}
	}

	m_Parent = parentWidget();
	m_BaseWidth = font_metrics.horizontalAdvance(msg);
	m_BaseHeight = font_metrics.lineSpacing() + m_patch_height;
	m_MinHeight = (m_BaseWidth * m_BaseHeight) / maxLinelen + 1; // ���������Ŀ������С�ĸ�

	changeSize();
	setWordWrap(true); // ����
	setText(msg);
	// ����
	if (m_Parent) {
		move((m_Parent->width() - width()) >> 1, (m_Parent->height() - height()) >> 1);
	}

	setVisible(true);
	m_fontcolor = bsuccess ? "green" : "red";
	setStyleSheet(QString(STYLE_SHEET).arg(m_fontcolor).arg(m_transparent_max_val)); // ��͸��
	m_TimerCount = delay_ms / m_timer_interval_ms + 1;                               // ��ʱ��������
	m_TransparentVal = m_transparent_max_val;
}

void CUVNoticeWidget::changeSize() {
	if (m_Parent) {
		const double wd = m_Parent->width() * m_size_scale;
		// ��ʾ���ݶ��پ�����ʾ���������������� s = m_BaseHeight * m_BaseWidth
		// ��� s �̶����� m_BaseWidth ���洰���ȱ���������ô m_BaseHeight Ӧ����С����ά�� s �̶�
		int newH = static_cast<int>((m_BaseHeight * m_BaseWidth) / wd) + m_patch_height;
		if (newH < (m_MinHeight + m_BaseHeight)) { // �趨��С�߶�
			newH = m_MinHeight + m_BaseHeight;
		} else {
			foreach(const int lineLen, m_ListLinesLen) {
				if (lineLen > wd) { // ĳһ�г��ȴ��ڵ�ǰ��Ⱦͻᷢ�����У��߶���Ҫ����
					newH += m_BaseHeight;
				}
			}
		}

		setFixedSize(static_cast<int>(wd), newH);
	}
}

void CUVNoticeWidget::OnTimerTimeout() {
	--m_TimerCount;
	if (m_TimerCount) {
		if (m_Parent) {
			if (const QPoint pt((m_Parent->width() - width()) >> 1, (m_Parent->height() - height()) >> 1); pos() != pt) { // ������λ�ñ仯
				changeSize();
				move(pt);
			}
		}
		// ���1s��ʼ������ʧ
		if (m_TimerCount <= m_notice_def_delay_cnt && 0 < m_TransparentVal) {
			m_TransparentVal -= m_transparent_cut_val;
			if (0 > m_TransparentVal) {
				m_TransparentVal = 0;
			}
			// ����͸����
			setStyleSheet(QString(STYLE_SHEET).arg(m_fontcolor).arg(m_TransparentVal));
		}
	} else {
		m_Timer->stop();
		setVisible(false);
	}
}
