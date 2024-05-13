#pragma once

#include <QBasicTimer>
#include <QFile>
#include <QSettings>

#include "logger.hpp"
#include "logger_p_global.hpp"

namespace Logger_p {
/*
 * @brief ʹ���ı��ļ���Ϊ����ļ�¼�������ô�ʹ��QSettings���󴴽������ļ�������������ʱ������������
 * <p>
 * ����ʾ��:
 * <code><pre>
 * fileName = logs/QtWebApp.log
 * maxSize = 1000000
 * maxBackups = 10
 * bufferSize = 0
 * minLevel = WARNING
 * msgformat = {timestamp} {typeNr} {type}
 * thread = {thread} : {message}
 * timestampFormat=dd.MM.  yyyy hh:mm:ss.zzz
 * </pre></code>
 * - ���ܵ���־����Ϊ:ALL/DEBUG=0, WARN/WARNING=1, ERROR/CRITICAL=2, FATAL=3, INFO=4
 * - "fileName"Ϊ��־�ļ�����������ļ�����Ŀ¼�����ơ�����windows�����������ע����У���·������ڵ�ǰ����Ŀ¼
 * - maxSizeΪ���ļ�������С(���ֽ�Ϊ��λ)���ļ���������, ������ļ����ڴ����ƣ��������ļ��滻����ע��ʵ���ļ���С���ܻ����������Դ�һЩ��Ĭ��ֵΪ0=������
 * - maxBackups����Ҫ�����ı����ļ�������Ĭ��ֵΪ0=������
 * - bufferSize���廷�λ������Ĵ�С��Ĭ��Ϊ0=����
 * - minLevel���bufferSize=0����ʾ�����ͼ������Ϣ
 * - msgFormat������־��Ϣ��װ�Σ��μ�LogMessage�ࡣĬ��Ϊ"{timestamp} {type} {message}"
 *
 * @see set()���������������־��¼������
 * @see LogMessage��ȡ��Ϣװ�ε�����
 * @see Logger��ȡ������������
 */
class LOGGER_P_EXPORT FileLogger final : public Logger {
	Q_OBJECT
	Q_DISABLE_COPY(FileLogger)

public:
	/*
	 * @note: �������ã�ͨ���洢��INI�ļ��С�������0�������Ǵӵ�ǰ���ȡ�ģ���˵����߱������Settings ->beginGroup()��
	 * ��Ϊ��������ʱ�ڼ䲻�ܸ��ģ����Խ����ṩ������QSettingsʵ������ʵ�������������������ʹ�á�
	 * FileLogger���ӹ�QSettingsʵ��������Ȩ����˵�����Ӧ���ڹػ�ʱ��������
	 * @param refreshInterval ˢ�����õ�ʱ����(�Ժ���Ϊ��λ)��Ĭ��ֵΪ10000, 0 = ����
	 */
	explicit FileLogger(QSettings* settings, int refreshInterval = 10000, QObject* parent = nullptr);
	~FileLogger() override;
	/*
	 * @note: д����־�ļ�
	 */
	void write(const LogMessage* logMessage) override;

protected:
	/*
	 * @note: �����¼�ˢ���������û�ͬ��I/O��������
	 * ����������̰߳�ȫ�ġ�
	 */
	void timerEvent(QTimerEvent* event) override;

private:
	QString m_fileName{};          // ��־�ļ���
	qlonglong m_maxSize{};         // �ļ�������С(���ֽ�Ϊ��λ), 0 =����
	int m_maxBackups{};            // �����ļ����������, 0 = ����
	QSettings* m_settings{};       // ��������
	QFile* m_file{};               // ��־�ļ�, 0 = ����
	QBasicTimer* m_refreshTimer{}; // ˢ���������õĶ�ʱ��
	QBasicTimer* m_flushTimer{};   // ˢ���ļ�I/O�������Ķ�ʱ��
	/*
	 * @note: ������ļ�
	 */
	void open();
	/*
	 * @note: �ر�����ļ�
	 */
	void close();
	/*
	 * @note: Rotate �ļ���ɾ��һЩ���ݣ������̫��
	 */
	void rotate() const;
	/*
	 * @note: ˢ���������á�
	 * ����������̰߳�ȫ�ġ�
	 */
	void refreshSettings();
};
}
