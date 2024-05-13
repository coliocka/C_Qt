#pragma once

/*
 * @note: �����궨��
 * ��Windowsƽ̨�£���̬���еķ���Ĭ��������ǲ��ɼ��ģ�����ζ���ڱ��붯̬��ʱ�����ڲ��ĺ������಻�ᱻ�Զ��������ⲿʹ�õĳ����С���������Ŀ����Ϊ�˱��⶯̬���е��ڲ�ʵ��ϸ�ڱ��ⲿ������ʵ���
 * �Ӷ�����˳���ķ�װ�ԺͰ�ȫ�ԡ�Ϊ�������������ܹ�ʹ�ö�̬���еĺ������࣬��Ҫ�ڵ���ʱ��ʾ��ʹ��__declspec(dllexport)��ָ�����ŵĿɼ��ԡ�
 * ����Linuxƽ̨�£�Ĭ������·����ǿɼ��ģ�����Ĭ��������������������ġ�
 * Linuxϵͳ���õ���һ��"�ͽ�ԭ��"���������ţ���������ʱ������ѡ�������ҵ��ķ��š���ˣ������ͬһ������߿��д��ڶ��ͬ���ķ��ţ�Ĭ������£���������ѡ�������ҵ��ķ��ţ���Щ���Ŷ��ǿɼ��ġ�
 * ������Ƽ��˳���Ŀ��������ӹ��̣���������Unix/Linuxϵͳ�������ѧ��
 */
#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#ifdef LOGGER_REDIRECTION_LIB
#define LOGGER_REDIRECTION_EXPORT Q_DECL_EXPORT
#else
#define LOGGER_REDIRECTION_EXPORT Q_DECL_IMPORT
#endif
#elif defined(__unix__)
#define LOGGER_REDIRECTION_EXPORT
#endif

/*
 * @breif ��־�ļ�����
 */
constexpr static auto logDirectory = "log";
/*
 * @breif ����log�ļ����ֵ
 */
constexpr static auto LOG_MAXSIZE = 5 * 1024 * 1024;
