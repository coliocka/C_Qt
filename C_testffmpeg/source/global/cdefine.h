#pragma once

#include <QString>

#define  SAFE_DELETE(p) {if(p){delete p;p=nullptr;}}

// ����״̬
enum eProcessState {
	eProcessState_Unknown = -1, // δ֪
	eProcessState_Online,       // ����
	eProcessState_Online_Push,  // ��������
	eProcessState_Offline,      // ����
};

// ��������Ϣ
struct CPushStreamInfo {
	// ����������
	enum PushStream {
		Video = 0x1,
		Audio = 0x2
	};

	QString strAddress{ "" };                        // ���͵�ַ
	float fVideoBitRate{ 0.0 };                      // ��Ƶ������
	PushStream eStream{ PushStream(Video | Audio) }; // ��������
	int nWidth{ 0 };                                 // ��Ƶ���
	int nHeight{ 0 };                                // ��Ƶ�߶�
	int nFrameRateNum{ 1 };                          // ��Ƶ֡�ʷ���
	int nFrameRateDen{ 25 };                         // ��Ƶ֡�ʷ�ĸ
	int nColorDepth{ 24 };                           // ��Ƶ��ɫ���
	int nAudioBitRate{ 0 };                          // ��Ƶ������
	int nAudioSampleRate{ 0 };                       // ��Ƶ������
};

// ѭ������
template<typename T>
struct CircularQueue {
public:
	// ����ָ����С��ѭ������
	explicit CircularQueue(int size);
	~CircularQueue();
	// �ж�ѭ�������Ƿ�����
	bool isFull() const;
	// �ж�ѭ�������Ƿ�Ϊ��
	bool isEmpty() const;
	// ��Ԫ������ѭ������ĩβ
	bool push(T);
	// ��������
	T pop();
	// ��ȡ����
	T first();
	// ��ն���
	void clear();
	int size() const;

private:
	// ѭ�������ܴ�С
	std::atomic_int m_nSize{ 0 };
	// ѭ������ͷ������
	std::atomic_int m_nFront{ 0 };
	// ѭ������β������
	std::atomic_int m_nRear{ 0 };
	// �洢ѭ������Ԫ�ص�����
	T* m_pData{ nullptr };
};

template<typename T>
inline CircularQueue<T>::CircularQueue(const int size)
: m_nSize(size) {
	m_pData = new T[size];
}

template<typename T>
inline CircularQueue<T>::~CircularQueue() {
	delete[]m_pData;
}

template<typename T>
inline bool CircularQueue<T>::isFull() const {
	return m_nFront == (m_nRear + 1) % m_nSize;
}

template<typename T>
inline bool CircularQueue<T>::isEmpty() const {
	return m_nFront == m_nRear;
}

template<typename T>
inline bool CircularQueue<T>::push(T data) {
	if (isFull()) {
		return false;
	}
	m_pData[m_nRear] = data;
	m_nRear = (m_nRear + 1) % m_nSize;
	return true;
}

template<typename T>
inline T CircularQueue<T>::pop() {
	if (isEmpty()) {
		return nullptr;
	}
	T data = m_pData[m_nFront];
	m_nFront = (m_nFront + 1) % m_nSize;
	return data;
}

template<typename T>
inline T CircularQueue<T>::first() {
	if (isEmpty()) {
		return nullptr;
	}
	T data = m_pData[m_nFront];
	return data;
}

template<typename T>
inline void CircularQueue<T>::clear() {
	m_nFront = 0;
	m_nRear = 0;
}

template<typename T>
inline int CircularQueue<T>::size() const {
	return (m_nRear - m_nFront + m_nSize) % m_nSize;
}
