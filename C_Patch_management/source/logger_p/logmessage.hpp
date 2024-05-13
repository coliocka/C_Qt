#pragma once

#include <QDateTime>
#include <qglobal.h>
#include <QHash>

namespace Logger_p {
/*
 * @brief: һ����־��Ϣ
 * ���±�����������Ϣ�� msgFormat ��ʹ��:
 * - {timestamp} �������ں�ʱ��
 * - {typeNr} ���ָ�ʽ����Ϣ����(0-3)
 * - {type} �ַ�����ʽ����Ϣ����(DEBUG, WARNING, CRITICAL, FATAL)
 * - {thread} �߳�ID��
 * - {message} ��Ϣ�ı�
 * - {xxx} �����κ��û�����ļ�¼������
 * - {file} �ļ���
 * - {function} ������
 * - {line} �к�
 */
class LOGGER_P_EXPORT LogMessage {
	Q_DISABLE_COPY(LogMessage)

public:
	/*
	 * @param: type ��Ϣ����
	 * @param: message ��Ϣ�ı�
	 * @param: logVars �û�����ļ�¼������������Ϊ0
	 * @param: file �ļ���
	 * @param: function ������
	 * @param: line �к�
	 */
	LogMessage(QtMsgType type, const QString& message, const QHash<QString, QString>* logVars, const QString& file,
	           const QString& function, int line);

	/*
	 * @note: ����Ϣת��Ϊ�ַ���
	 * @param: msgFormatװ�θ�ʽ�����ܰ��������;�̬�ı�
	 * @param: timestampFormatʱ�����ʽ��
	 * @return: ��ʽ������ַ���
	 */
	QString toString(const QString& msgFormat, const QString& timestampFormat) const;

	/*
	 * @note: ��ȡ��Ϣ����
	 */
	QtMsgType getType() const;

private:
	QHash<QString, QString> m_logVars{}; // �û�����ļ�¼������
	QDateTime m_timestamp{};             // �������ں�ʱ��
	QtMsgType m_type{};                  // ��Ϣ����
	Qt::HANDLE m_threadId{};             // �߳�ID��
	QString m_message{};                 // ��Ϣ�ı�
	QString m_file{};                    // �ļ���
	QString m_function{};                // ������
	int m_line{};                        // �к�
};
}
