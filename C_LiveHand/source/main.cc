#include <filesystem>
#include <fstream>
#include <QApplication>
#include <QProcess>
#include <QResource>
#include <QSharedMemory>
#include <QThread>
#include <QTranslator>
#ifdef Q_OS_LINUX
#include <QTextCodec>
#endif

#include <QSettings>

#include "clivehand.h"
#include "global/interface_global.h"
#include "logger/logger.h"
#include "splashscreen/SplashScreen.h"

QTranslator* g_translator{ nullptr };
QSharedMemory g_sharedMemory{ nullptr };
SplashScreen* g_splashScreen{ nullptr };

/*
 * @note: ���ء�ж����Դ�ļ���������ʽ�����ء�ж�ط����ļ�
 * @return: ����true��ʾ���سɹ�������false��ʾ����ʧ��
 */
bool loadResources(const QString& strPath);
bool unloadResources(const QString& strPath);
bool loadStyle(QApplication& app, const QString& filePath);
bool loadTranslations(QApplication& app, const QString& filePath);
void unLoadTranslations();
/*
 * @note: �����ļ�����
 */
void initializeConfigFile();
ConfigData readConf();
void readConf(std::vector<std::string>& lines);
void writeConf(const std::vector<std::string>& lines);
void changeConf(WINDOWLANAGUAGE newLanguage, WINDOWTHEMESTYLE newThemeStyle);
/*
 * @note: Linux ͨ��ϵͳ�����ж��Ƿ�Ϊ��ɫ����
 */
#ifdef Q_OS_LINUX
std::string exec(const char* cmd); // ִ��ϵͳ������������
bool isDarkTheme();                // ��鵱ǰ����ģʽ
#endif
/*
 * @note: ��һ����������Ĭ��ʹ��ϵͳ��ɫ
 */
void checkWindowThemeStyle();
/*
 * @note: ����Ψһ�Լ��, ���ù����ڴ���ƣ�����һ��Ψһ��Key�����Խ������ڴ渽�ӵ���ǰ���̣���������ڴ���ڣ���˵�������Ѵ��ڣ�����false����������ڴ治���ڣ���˵������Ψһ������true
 * @param: �����ڴ�
 * @return: ����true��ʾ����Ψһ, ����false��ʾ�����Ѵ���
 */
inline bool checkSingleInstance(QSharedMemory& shared_memory);
/*
 * @note: ������������ҳ��
 */
SplashScreen* createSplashScreen(const QPixmap& pixmap, int w = 400, int h = 400, Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio);

int main(int argc, char* argv[]) {
#ifdef Q_OS_LINUX
 QTextCodec* codec = QTextCodec::codecForName("utf-8");
 QTextCodec::setCodecForLocale(codec);
#endif
	QApplication app(argc, argv);
	checkWindowThemeStyle();
	const QFileInfo appFile(QApplication::applicationFilePath());
	// ��·���л����ϼ�Ŀ¼
	const QString appParPath = appFile.absolutePath();
	const QString strStyle_light = appParPath + QString::fromLatin1(qssFilePathLight);
	const QString strStyle_dark = appParPath + QString::fromLatin1(qssFilePathDark);
	const QString strRcc = appParPath + QString::fromLatin1(rccFilePath);
	const QString strtrans_cn = appParPath + QString::fromLatin1(translationFilePath_CN);
	const QString strtrans_en = appParPath + QString::fromLatin1(translationFilePath_EN);
	// ����rcc
	Logger::instance().logInfo(loadResources(strRcc) ? "Load Resource Success!" : "Load Resource Failed!");
	g_splashScreen = createSplashScreen(QPixmap(":/icon/start.png"));
	// ��ʼ�������ļ�
	initializeConfigFile();
	auto [language, themeStyle] = readConf();
	windowLanguage = language;
	windowThemeStyle = themeStyle;
	// ������ʽ��
	QString str = (windowThemeStyle == WINDOWTHEMESTYLE::LIGHT) ? strStyle_light : strStyle_dark;
	g_splashScreen->showMessage(loadStyle(app, str) ? "Load Style Success!" : "Load Style Failed!", Qt::AlignBottom);
	QThread::sleep(1);
	// ���ط��� & ����Label��С
	str = (windowLanguage == WINDOWLANAGUAGE::Chinese) ? strtrans_cn : strtrans_en;
	g_splashScreen->showMessage(loadTranslations(app, str) ? "Load Translation Success!" : "Load Translation Failed!", Qt::AlignBottom);
	QThread::sleep(1);

	CLiveHand w;
	// �ͷ���Դ
	QObject::connect(&w, &CLiveHand::destroyed, [&]() {
		unloadResources(strRcc);
		unLoadTranslations();
		g_sharedMemory.detach();
		g_splashScreen->deleteLater();
	});
	// ����Ψһ�Լ��
	if (!checkSingleInstance(g_sharedMemory)) {
		w.getTrayIcon()->showMessage(QObject::tr("InfoMation"),
									 QObject::tr("The program already exists, do not start again!"),
									 QSystemTrayIcon::Information, 1000);
		qDebug() << QObject::tr("The program already exists, do not start again!");
		return -1;
	}
	g_splashScreen->hide();
	w.show();
	const int nRet = QApplication::exec();
	if (nRet == RETCODE_RESTART) {
		// ���� qApp->applicationFilePath()�������Լ�
		QProcess::startDetached(qApp->applicationFilePath(), QStringList());
		return 0;
	}
	return nRet;
}

bool loadResources(const QString& strPath) {
	qDebug() << ("Resource filePath:\t" + strPath);
	return QResource::registerResource(strPath);
}

bool unloadResources(const QString& strPath) {
	if (QResource::unregisterResource(strPath)) {
		qDebug() << "unregister resource success";
		return true;
	}
	Logger::instance().logError("resource filePath:\t" + strPath);
	Logger::instance().logError("unregister resource failed");
	return false;
}

bool loadStyle(QApplication& app, const QString& filePath) {
	qDebug() << ("Style filePath:\t" + filePath);
	QFile file(filePath);
	if (file.open(QFile::ReadOnly)) {
		const QString strStyleSheet = QLatin1String(file.readAll());
		app.setStyleSheet(strStyleSheet);
		file.close();
		return true;
	}
	Logger::instance().logError(file.errorString());
	return false;
}

bool loadTranslations(QApplication& app, const QString& filePath) {
	unLoadTranslations();
	qDebug() << "Translation filePath:\t" << filePath;
	g_translator = new QTranslator(&app);
	if (!g_translator->load(filePath)) {
		return false;
	}
	QApplication::installTranslator(g_translator);
	return true;
}

void unLoadTranslations() {
	QApplication::removeTranslator(g_translator);
	if (g_translator) {
		delete g_translator;
		g_translator = nullptr;
		qDebug() << "unLoadTranslations, g_translator delete success";
	} else {
		qDebug() << "unLoadTranslations, g_translator == nullptr";
	}
}

void initializeConfigFile() {
	const int language = static_cast<int>(windowLanguage);
	const int themeStyle = static_cast<int>(windowThemeStyle);
	// ���config�ļ����Ƿ���ڣ�����������򴴽�
	if (!std::filesystem::exists(configDir)) {
		if (!std::filesystem::create_directory(configDir)) {
			Logger::instance().logError("Error: Unable to create directory " + QString::fromStdString(configDir));
			return;
		}
		g_splashScreen->showMessage("Directory " + QString::fromStdString(configDir) + " created.", Qt::AlignBottom);
		QThread::sleep(1);
	}

	if (const std::ifstream configFile(configName); !configFile) {
		// �����ļ�������
		if (std::ofstream outputFile(configName); outputFile) {
			// д�����Ժͽ�������ʽ
			outputFile << "Language: " << language << "\n";
			outputFile << "ThemeStyle: " << themeStyle;
			outputFile.close();
			g_splashScreen->showMessage("Config file created.", Qt::AlignBottom);
			QThread::sleep(1);
		} else {
			Logger::instance().logError("Error: Unable to create config file.");
		}
	} else {
		g_splashScreen->showMessage("Config file already exists.", Qt::AlignBottom);
		QThread::sleep(1);
	}
}

ConfigData readConf() {
	auto language = windowLanguage;     // Ĭ������
	auto themeStyle = windowThemeStyle; // Ĭ��������ʽ

	if (std::ifstream configFile(configName); configFile) {
		std::string line;
		while (std::getline(configFile, line)) {
			if (line.find("Language:") != std::string::npos) {
				// ��ȡ��ʾ����
				std::string languageValue = line.substr(line.find(':') + 1);
				int languageint = std::stoi(languageValue);
				language = static_cast<WINDOWLANAGUAGE>(languageint);
			} else if (line.find("ThemeStyle:") != std::string::npos) {
				// ������ʽ
				std::string themeValue = line.substr(line.find(':') + 1);
				int themeInt = std::stoi(themeValue);
				themeStyle = static_cast<WINDOWTHEMESTYLE>(themeInt);
			}
		}
	}
	return { language, themeStyle };
}

void readConf(std::vector<std::string>& lines) {
	// ��ȡ�����ļ����ݵ��ڴ���
	if (std::ifstream configFile(configName); configFile) {
		std::string line;
		while (std::getline(configFile, line)) {
			lines.push_back(line);
		}
		configFile.close();
	} else {
		Logger::instance().logError("Error: Unable to open config file for reading.");
	}
}

void writeConf(const std::vector<std::string>& lines) {
	if (std::ofstream outputFile(configName, std::ios::trunc); outputFile) {
		for (const std::string& line : lines) {
			outputFile << line << std::endl;
		}
		outputFile.close();
	} else {
		Logger::instance().logError("Error: Unable to open config file for writing.");
	}
}

void changeConf(WINDOWLANAGUAGE newLanguage, WINDOWTHEMESTYLE newThemeStyle) {
	const int newLanguageInt = static_cast<int>(newLanguage);
	const int newThemeStyleInt = static_cast<int>(newThemeStyle);

	std::vector<std::string> lines;
	readConf(lines);
	// ���Ҳ��޸� "Language:", "ProgressbarStyle:", "ThemeStyle:"
	bool Found = true;
	for (std::string& line : lines) {
		if (line.find("Language:") != std::string::npos) {
			line = "Language: " + std::to_string(newLanguageInt); // �޸�����ֵ
		} else if (line.find("ThemeStyle:") != std::string::npos) {
			line = "ThemeStyle: " + std::to_string(newThemeStyleInt); // �޸�����ֵ
		} else {
			Found = false;
			break;
		}
	}
	// ����ҵ������Ժ���ʽ�У���д�ص��ļ���
	if (Found) {
		writeConf(lines);
	} else {
		Logger::instance().logError("Error: config line not found in config file.");
	}
}

#ifdef Q_OS_LINUX
std::string exec(const char* cmd) {
	std::array<char, 128> buffer{};
	std::string result{};
	const std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		// ��buffer�е����ݴ�β�����뵽result��
		std::copy(buffer.begin(), buffer.end(), std::back_inserter(result));
	}
	return result;
}

bool isDarkTheme() {
	const std::string output = exec("gsettings get org.gnome.desktop.interface gtk-theme");
	return output.find("dark") != std::string::npos;
}
#endif

void checkWindowThemeStyle() {
#ifdef Q_OS_WIN
	const QSettings settings(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", QSettings::NativeFormat);
	windowThemeStyle = settings.value("AppsUseLightTheme").toBool() ? WINDOWTHEMESTYLE::LIGHT : WINDOWTHEMESTYLE::DARK;
#elif defined(Q_OS_LINUX)
	windowThemeStyle = isDarkTheme() ? WINDOWTHEMESTYLE::DARK : WINDOWTHEMESTYLE::LIGHT;
#endif
}

inline bool checkSingleInstance(QSharedMemory& shared_memory) {
	shared_memory.setKey(QString::fromLatin1(AppID));
	if (shared_memory.attach()) {
		return false;
	}
	if (shared_memory.create(1)) {
		return true;
	}
	return false;
}

SplashScreen* createSplashScreen(const QPixmap& pixmap, int w, int h, Qt::AspectRatioMode mode) {
	auto* splash = new SplashScreen(pixmap.scaled(w, h, mode));
	splash->show();
	QApplication::processEvents();
	return splash;
}
