#pragma once

/*
 * @breif ��������
 */
constexpr static int RETCODE_RESTART = 773;
/*
 * @breif ��������
 * �洢�������ļ���
 */
enum class WINDOWLANAGUAGE {
	Chinese,
	English
};
/*
 * @breif ������ʽ
 */
enum class WINDOWTHEMESTYLE {
	DARK,
	LIGHT
};
/*
 * @breif �����ļ�
 */
static const std::string configDir = "config";
static const std::string configName = configDir + "/config.ini";
/*
 * @breif ��־�ļ�����
 */
constexpr static auto logDir = "log";
constexpr static auto logFileName = "user.log";
/*
 * @breif QMenuBar���
 */
constexpr static auto ENGLISH_MENUBAR_WIDTH = 120;
constexpr static auto CHINESE_MENUBAR_WIDTH = 90;
/*
 * @breif ������ʾ����, Ĭ������; ��������ʽ, Ĭ�Ͽ�״; ������ʽ, Ĭ����ɫ
 */
static auto windowLanguage = WINDOWLANAGUAGE::Chinese;
static auto windowThemeStyle = WINDOWTHEMESTYLE::LIGHT;
/*
 * @breif AppID
 */
constexpr static auto AppID = "crucal_app_id";
/*
 * @breif ��Դ�ļ�·��
 */
constexpr static auto qssFilePathLight = "/style/C_testffmpeg_light.qss";
constexpr static auto qssFilePathDark = "/style/C_testffmpeg_dark.qss";
constexpr static auto rccFilePath = "/resource/C_testffmpeg.rcc";
constexpr static auto translationFilePath_CN = "/translation/C_testffmpeg_cn.qm";
constexpr static auto translationFilePath_EN = "/translation/C_testffmpeg_en.qm";
/*
 * @breif �����������ݲ���
 */
static std::string App_arg_dir{};
/*
 * @breif �����ļ�����
 */
struct ConfigData {
 WINDOWLANAGUAGE lanaguage{};
 WINDOWTHEMESTYLE themeStyle{};

 ConfigData();
 ConfigData(WINDOWLANAGUAGE lang, WINDOWTHEMESTYLE theme);
 ConfigData& operator=(const ConfigData& other);
 void reset();
};

inline ConfigData::ConfigData() {
 reset();
}

inline ConfigData::ConfigData(WINDOWLANAGUAGE lang, WINDOWTHEMESTYLE theme) :
lanaguage(lang),  themeStyle(theme) {
}

inline ConfigData& ConfigData::operator=(const ConfigData& other) {
 if (this == &other) {
  return *this;
 }
 lanaguage = other.lanaguage;
 themeStyle = other.themeStyle;
 return *this;
}

inline void ConfigData::reset() {
 lanaguage = WINDOWLANAGUAGE::Chinese;
 themeStyle = WINDOWTHEMESTYLE::LIGHT;
}