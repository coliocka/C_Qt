#pragma once

#include <QMutex>
#include <QObject>
#include <QThreadStorage>
#include "logger_p_global.hpp"
#include "logmessage.hpp"

namespace Logger_p {
/*
 * װ����־��Ϣ������д�����̨, stderr
 * װ����ʹ��Ԥ����� msgFormat �ַ������ḻ��־��Ϣ
 * ������Ϣ(��ʱ���)��
 * msgFormat�ַ�������Ϣ�ı����ܰ��������, ��ʽΪ<i>{name}</i>�ı�������ֵ���, ȡ�Ծ�̬�̱߳����ֵ䡣
 * ��־��¼���������̱߳����ռ���������������Ϣ, FIFO�������������� >= minLevel����־��Ϣ��ˢ�»�������
 * ������Ϣ�ͱ�д�����ˡ���һ������: INFO ��Ϣ����ΪDEBUG��Ϣ(����0)��
 * ʾ��:������û�������ʹ�� minLevel = 2����Ӧ�ó���ȴ���ֱ����������Ȼ����ͬ������Ϣһ��д����
 * ʹ��ͬһ�̵߳����л���ĵͼ���Ϣ����ֻҪ���������ˣ�ʲôҲ����д����
 * ��������������ã���ôֻ�������� >= minLevel����Ϣ��д����
 * @see: �μ� LogMessage ��ȡ��Ϣװ�ε�������
 */
class LOGGER_P_EXPORT Logger : public QObject {
	Q_OBJECT
	Q_DISABLE_COPY(Logger)

public:
	explicit Logger(QObject* parent = nullptr);
	/*
	 * log levels: 0 = DEBUG, 1 = WARNING, 2 = CRITICAL, 3 = FATAL, 4 = INFO
	 * @param msgFormat ��Ϣ��ʽ�ַ���
	 * @param timestampFormat ʱ�����ʽ�ַ���
	 * @param minLevel �����־����, ���bufferSize = 0, �ϵͼ������Ϣ��������, ���bufferSize > 0, �ϵͼ�������������壬��Ȼ���߼������Ϣ����д�뻺������
	 * @param bufferSize ��������С, 0 = ���û�����, ��������
	 * @see: �μ� LogMessage ��ȡ��Ϣװ�ε�������
	 */
	explicit Logger(QString msgFormat, QString timestampFormat, QtMsgType minLevel, int bufferSize, QObject* parent = nullptr);

	~Logger() override;
	/*
	 * @note: ���type >= minLevel, װ�β���¼��Ϣ���˷������̰߳�ȫ��
	 * @param: type ��Ϣ����
	 * @param: message ��Ϣ�ı�
	 * @param: file �ļ���
	 * @param: function ������
	 * @param: line �к�
	 * @see: �μ� LogMessage ��ȡ��Ϣװ�ε�������
	 */
	virtual void log(QtMsgType type, const QString& message, const QString& file, const QString& function, int line);
	/*
	 * @note: ������־��¼����װΪĬ����Ϣ������򣬰�װ����Ϳ�ʹ�� qDebug() �Ⱥ�����¼��־
	 */
	void installMsgHandler();
	/*
	 * @note: ���ÿ�����������־��Ϣ���ֲ߳̾��������˷������̰߳�ȫ��
	 * @param: name ������
	 * @param: value ����ֵ
	 */
	static void set(const QString& name, const QString& value);
	/*
	 * @note: �����ǰ�̵߳��̱߳������ݣ��˷������̰߳�ȫ��
	 * @param: buffer �Ƿ�������ݻ�����
	 * @param: variables �Ƿ������־����
	 */
	virtual void clear(bool buffer, bool variables);

protected:
	QString msgFormat{};       // ��Ϣ��ʽ�ַ���
	QString timestampFormat{}; // ʱ�����ʽ�ַ���
	QtMsgType minLevel{};      // �����־����
	int bufferSize{};          // ��������С
	static QMutex mutex;       // ����ͬ�������̵߳ķ���
	/*
	 * @note: װ�β���дһ����־��Ϣ�� stderr
	 */
	virtual void write(const LogMessage* logMessage);

private:
	static Logger* defaultLogger; // Ĭ��ʹ�� msgHandler
	/*
	 * @note: ȫ�־�̬��־��������Ϣ�������, ת����Ĭ�ϼ�¼���ĵ���, �˷������̰߳�ȫ��
	 * �������������Ϣ��������ֹ����Ϣ�еı����������ǵ�ֵ�滻��
	 * @param: type ��Ϣ����
	 * @param: message ��Ϣ�ı�
	 * @param: file �ļ���
	 * @param: function ������
	 * @param: line �к�
	 */
	static void msgHandler(QtMsgType type, const QString& message, const QString& file = "", const QString& function = "", int line = 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	/*
	 * @note: Qt5�İ�װ��
	 * @param: type ��Ϣ����
	 * @param: context ��Ϣ������
	 * @param: message ��Ϣ�ı�
	 * @see: msgHandler()
	 */
	static void msgHandler5(QtMsgType type, const QMessageLogContext& context, const QString& message);
#else
	/*
	 * @note: Qt4�İ�װ��
	 * @param: type ��Ϣ����
	 * @param: message ��Ϣ�ı�
	 * @see: msgHandler()
	 */
	static void msgHandler4(QtMsgType type, const char* message);
#endif
	static QThreadStorage<QHash<QString, QString>*> logVars; // ����־��Ϣ��ʹ�õ��ֲ߳̾�����
	QThreadStorage<QList<LogMessage*>*> buffers;             // �̱߳��ػ��ݻ�����
};
}
