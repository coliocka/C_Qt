#pragma once

/*
 * @note: �����궨��
 * ��Windowsƽ̨�£���̬���еķ���Ĭ��������ǲ��ɼ��ģ�����ζ���ڱ��붯̬��ʱ�����ڲ��ĺ������಻�ᱻ�Զ��������ⲿʹ�õĳ����С���������Ŀ����Ϊ�˱��⶯̬���е��ڲ�ʵ��ϸ�ڱ��ⲿ������ʵ���
 * �Ӷ�����˳���ķ�װ�ԺͰ�ȫ�ԡ�Ϊ�������������ܹ�ʹ�ö�̬���еĺ������࣬��Ҫ�ڵ���ʱ��ʾ��ʹ��__declspec(dllexport)��ָ�����ŵĿɼ��ԡ�
 * ����Linuxƽ̨�£�Ĭ������·����ǿɼ��ģ�����Ĭ��������������������ġ�
 * Linuxϵͳ���õ���һ��"�ͽ�ԭ��"���������ţ���������ʱ������ѡ�������ҵ��ķ��š���ˣ������ͬһ������߿��д��ڶ��ͬ���ķ��ţ�Ĭ������£���������ѡ�������ҵ��ķ��ţ���Щ���Ŷ��ǿɼ��ġ�
 * ������Ƽ��˳���Ŀ��������ӹ��̣���������Unix/Linuxϵͳ�������ѧ��
 */
#ifdef Q_OS_WIN
#ifdef LOGGER_P_LIB
#define LOGGER_P_EXPORT Q_DECL_EXPORT
#else
#define LOGGER_P_EXPORT Q_DECL_IMPORT
#endif
#elif Q_OS_UNIX
#define LOGGER_P_EXPORT
#endif

using i64 = long long;
using ui64 = unsigned long long;
/*
 * @breif: �����ļ�Ĭ������
 */
struct LoggerConfigData {
	std::wstring group{};
	std::wstring fileName{};
	std::wstring minLevel{};
	ui64 bufferSize{};
	ui64 maxSize{};
	int maxBackups{};
	std::string timestampFormat{};
	std::string msgFormat{};

	LoggerConfigData();
	LoggerConfigData& operator=(const LoggerConfigData& other);
	void reset();
};

inline LoggerConfigData::LoggerConfigData() {
	reset();
}

inline LoggerConfigData& LoggerConfigData::operator=(const LoggerConfigData& other) {
	if (this == &other) {
		return *this;
	}
	this->group = other.group;
	this->fileName = other.fileName;
	this->minLevel = other.minLevel;
	this->bufferSize = other.bufferSize;
	this->maxSize = other.maxSize;
	this->maxBackups = other.maxBackups;
	this->timestampFormat = other.timestampFormat;
	this->msgFormat = other.msgFormat;

	return *this;
}

inline void LoggerConfigData::reset() {
	group = L"logging";
	fileName = L"../log/stdout.log";
	minLevel = L"DEBUG";
	bufferSize = 100;
	maxSize = 1000000;
	maxBackups = 10;
	timestampFormat = "dd.MM.yyyy hh:mm:ss.zzz";
	msgFormat = "{timestamp} {typeNr} {type} {thread} {message}\\n  in {file} line {line} function {function}";
}
