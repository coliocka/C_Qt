#pragma once

#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSlider>
#include <QAudioOutput>
#include <QTime>
#include <QPushButton>

class CCodecThread;
class CAspectRatioItem;
class CPlayBackWidget final : public QWidget {
	Q_OBJECT

public:
	// ������״̬
	enum State {
		Play,
		Pause,
		Stop
	};

public:
	explicit CPlayBackWidget(QWidget* parent = nullptr);
	~CPlayBackWidget() override;

public:
	void openFile(const QString& strFile, bool bLoop = false, bool bPicture = false);
	void stop();
	void pause();
	void resume();

public slots:
	void onNotifyAudio(const QByteArray& data);            // ��Ƶ����
	void onNotifyClipInfo(const quint64& nDuration) const; // ý����ʱ��
	void onNotifyAudioPara(const quint64& nSampleRate, const quint64& nChannels);
	void onNotifyCountDown(const quint64& nCountDown) const;
	void onNotifyImage(const QPixmap& pixmap) const;
	void onCodecStarted();
	void onCodecFinished();
	void enablePlay(bool bEnable) const;
	[[nodiscard]] CPlayBackWidget::State state() const;
	[[nodiscard]] QSize playSize() const;
	void setPlayState(bool bPlay) const;

signals:
	void notifyState(CPlayBackWidget::State);
	void notifyPlayClicked(bool);

protected:
	void timerEvent(QTimerEvent* event) override;

private slots:
	void onSliderDurationPressed() const;
	void onSliderDurationMoved(int nPosition) const;
	void onSliderDurationReleased();
	void onBtnPlayClicked(bool bChecked);

private:
	void createCtrl();
	void layOut();
	// void destroy();
	void initConnect();

private:
	QLabel* m_pLbDisplay{ nullptr };

	QPushButton* m_pBtnPlay{ nullptr };
	QSlider* m_pSliderDuration{ nullptr };
	QLabel* m_pLbDuration{ nullptr };
	QLabel* m_pLbCountDown{ nullptr };

	CAspectRatioItem* m_pAspectRatioItem{ nullptr };
	QHBoxLayout* m_pLyHPlay{ nullptr };
	QFormLayout* m_pLyFormDuration{ nullptr };
	QFormLayout* m_pLyFormCountDown{ nullptr };
	QHBoxLayout* m_pLyHDuration{ nullptr };
	QVBoxLayout* m_pLyVTotal{ nullptr };

private:
	QAudioOutput* m_pAudioOutput{ nullptr };
	QIODevice* m_pAudioDevice{ nullptr };
	QByteArray m_audioByteBuffer; //��Ƶ����

	CCodecThread* m_pCodecThread{ nullptr };
	State m_eState{ Stop };
	int m_nTimerId{ -1 };
};

class CAspectRatioItem final : public QWidgetItem {
	Q_DISABLE_COPY(CAspectRatioItem)

public:
	explicit CAspectRatioItem(QWidget* parent);
	explicit CAspectRatioItem(QWidget* parent, int nWidthAspect, int nHeightAspect);
	~CAspectRatioItem() override;

	// ���ÿ�߱�
	void setAspectRatio(int nWidthAspect, int nHeightAspect);
	// ��д����� setGeometry ��������ȷ�������óߴ�ʱ���ֿ�߱�
	void setGeometry(const QRect&) override;
	// ���õ�ǰ��߱ȵĿ���
	[[nodiscard]] int widthAspect() const;
	// ���õ�ǰ��߱ȵĸ߲���
	[[nodiscard]] int heightAspect() const;

protected:
	// ά�ֿ�߱�
	void doLayout(const QRect& rect) const;

private:
	int m_nWidthAspect{ 16 };
	int m_nHeightAspect{ 9 };
};
