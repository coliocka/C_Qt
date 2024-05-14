#pragma once

#include <QSettings>

#include "filelogger.hpp"
#include "logger.hpp"
#include "logger_p_global.hpp"

namespace Logger_p {
/*
 * @brief ͬʱ����־��Ϣд��������־�ļ�
 * @see FileLogger ��ϸ��־����
 */
class LOGGER_P_EXPORT DualFileLogger final : public Logger {
	Q_OBJECT
	Q_DISABLE_COPY(DualFileLogger)

public:
	/*
	 * @note: �������ڼ䲻�ܸ��ģ����������ṩ������QSettingsʵ������ʵ�������������������ʹ��
	 * FileLogger ���ӹ�QSettingsʵ��������Ȩ����˵�����Ӧ���ڹػ�ʱ������
	 * @param: firstSettings ��һ����־�ļ�����
	 * @param: secondSettings �ڶ�����־�ļ�����
	 * @param: refreshInterval ˢ�¼������λ����, 0 = ����
	 */
	explicit DualFileLogger(QSettings* firstSettings, QSettings* secondSettings, int refreshInterval = 10000, QObject* parent = nullptr);
	~DualFileLogger() override;

	/*
	 * @note: ��� type >= minLevel, װ�β���¼��Ϣ
	 * ����������̰߳�ȫ��
	 * @param: type ��Ϣ����
	 * @param: message ��Ϣ����
	 * @param: file �ļ���
	 * @param: function ������
	 * @param: line �к�
	 * @see LogMessage ��ϸ��־����
	 */
	void log(QtMsgType type, const QString& message, const QString& file, const QString& function, int line) override;
	/*
	 * @note: �����־�ļ�
	 * ����������̰߳�ȫ��
	 * @param: buffer �Ƿ�������ݻ�����
	 * @param: variables �Ƿ������־����
	 */
	void clear(bool buffer, bool variables) override;

private:
	FileLogger* m_firstLogger{ nullptr };
	FileLogger* m_secondLogger{ nullptr };
};
}
