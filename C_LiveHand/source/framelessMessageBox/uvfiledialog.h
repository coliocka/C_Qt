#pragma once

#include <QFileDialog>
#include <QDebug>
#include <utility>
#include <QComboBox>

#include "uvbasedialog.h"

#ifdef FRAMELESSFILEDIALOG_LIB
#define FRAMELESSFILEDIALOG_EXPORT Q_DECL_EXPORT
#else
#define FRAMELESSFILEDIALOG_EXPORT Q_DECL_IMPORT
#endif

class FRAMELESSFILEDIALOG_EXPORT CUVFileBase final : public CUVBaseDialog {
	Q_OBJECT

public:
	explicit CUVFileBase(QString strRegisterName, QWidget* parent = Q_NULLPTR,
	                     const QString& caption = QString(), const QString& directory = QString(),
	                     const QString& filter = QString());
	~CUVFileBase() override;

public:
	void setLabelText(QFileDialog::DialogLabel label, const QString& strText) const;
	void setFilter(QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot) const;
	void setFileMode(QFileDialog::FileMode mode = QFileDialog::ExistingFile) const;

	void setAcceptMode(QFileDialog::AcceptMode mode = QFileDialog::AcceptOpen) const;
	[[nodiscard]] QFileDialog::AcceptMode acceptMode() const;

	void setDefaultSuffix(const QString& suffix) const;
	[[nodiscard]] QString defaultSuffix() const;

	void setDirectory(const QString& directory) const;
	inline void setDirectory(const QDir& directory) const;
	[[nodiscard]] QDir directory() const;

	void setDirectoryUrl(const QUrl& directory) const;
	[[nodiscard]] QUrl directoryUrl() const;

	void setViewMode(QFileDialog::ViewMode mode = QFileDialog::Detail) const;

	void selectFile(const QString& filename) const;
	[[nodiscard]] QStringList selectedFiles() const;

	QString getOpenFileName(QString* selectedFilter = nullptr,
	                        QFileDialog::Options options = QFileDialog::Options());

	QUrl getOpenFileUrl(const QUrl& dir = QUrl(),
	                    QString* selectedFilter = nullptr,
	                    QFileDialog::Options options = QFileDialog::Options(),
	                    const QStringList& supportedSchemes = QStringList());

	QString getSaveFileName(QString* selectedFilter = nullptr,
	                        QFileDialog::Options options = QFileDialog::Options());

	QUrl getSaveFileUrl(const QUrl& dir = QUrl(),
	                    QString* selectedFilter = nullptr,
	                    QFileDialog::Options options = QFileDialog::Options(),
	                    const QStringList& supportedSchemes = QStringList());

	QString getExistingDirectory(QFileDialog::Options options = QFileDialog::ShowDirsOnly);

	QString getExistingDirectoryUrl(const QUrl& dir = QUrl(),
	                                QFileDialog::Options options = QFileDialog::ShowDirsOnly,
	                                const QStringList& supportedSchemes = QStringList());

	QStringList getOpenFileNames(QString* selectedFilter = nullptr,
	                             QFileDialog::Options options = QFileDialog::Options());

	QList<QUrl> getOpenFileUrls(const QUrl& dir = QUrl(),
	                            QString* selectedFilter = nullptr,
	                            QFileDialog::Options options = QFileDialog::Options(),
	                            const QStringList& supportedSchemes = QStringList());

	void getOpenFileContent(const QString& nameFilter,
	                        const std::function<void(const QString&, const QByteArray&)>& fileContentsReady);

	void saveFileContent(const QByteArray& fileContent, const QString& fileNameHint = QString());

private:
	void init();

private:
	QFileDialog* m_pFileDialog{ nullptr };
	QString m_strRegisterName{ "" };
};

class FRAMELESSFILEDIALOG_EXPORT CUVFileDialog final : public CUVBaseDialog {
	Q_OBJECT

public:
	explicit CUVFileDialog(QWidget* parent = nullptr);
	~CUVFileDialog() override;

	static QString getOpenFileName(QWidget* parent = nullptr,
	                               const QString& caption = QString(),
	                               const QString& dir = QString(),
	                               const QString& filter = QString(),
	                               QString* selectedFilter = nullptr,
	                               QFileDialog::Options options = QFileDialog::Options());

	static QUrl getOpenFileUrl(QWidget* parent = nullptr,
	                           const QString& caption = QString(),
	                           const QUrl& dir = QUrl(),
	                           const QString& filter = QString(),
	                           QString* selectedFilter = nullptr,
	                           QFileDialog::Options options = QFileDialog::Options(),
	                           const QStringList& supportedSchemes = QStringList());

	static QString getSaveFileName(QWidget* parent = nullptr,
	                               const QString& caption = QString(),
	                               const QString& dir = QString(),
	                               const QString& filter = QString(),
	                               QString* selectedFilter = nullptr,
	                               QFileDialog::Options options = QFileDialog::Options());

	static QUrl getSaveFileUrl(QWidget* parent = nullptr,
	                           const QString& caption = QString(),
	                           const QUrl& dir = QUrl(),
	                           const QString& filter = QString(),
	                           QString* selectedFilter = nullptr,
	                           QFileDialog::Options options = QFileDialog::Options(),
	                           const QStringList& supportedSchemes = QStringList());

	static QString getExistingDirectory(QWidget* parent = nullptr,
	                                    const QString& caption = QString(),
	                                    const QString& dir = QString(),
	                                    QFileDialog::Options options = QFileDialog::ShowDirsOnly |
		                                    QFileDialog::ReadOnly);

	static QUrl getExistingDirectoryUrl(QWidget* parent = nullptr,
	                                    const QString& caption = QString(),
	                                    const QUrl& dir = QUrl(),
	                                    QFileDialog::Options options = QFileDialog::ShowDirsOnly |
		                                    QFileDialog::ReadOnly,
	                                    const QStringList& supportedSchemes = QStringList());

	static QStringList getOpenFileNames(QWidget* parent = nullptr,
	                                    const QString& caption = QString(),
	                                    const QString& dir = QString(),
	                                    const QString& filter = QString(),
	                                    QString* selectedFilter = nullptr,
	                                    QFileDialog::Options options = QFileDialog::Options());

	static QList<QUrl> getOpenFileUrls(QWidget* parent = nullptr,
	                                   const QString& caption = QString(),
	                                   const QUrl& dir = QUrl(),
	                                   const QString& filter = QString(),
	                                   QString* selectedFilter = nullptr,
	                                   QFileDialog::Options options = QFileDialog::Options(),
	                                   const QStringList& supportedSchemes = QStringList());

	static void getOpenFileContent(const QString& nameFilter,
	                               const std::function<void(const QString&, const QByteArray&)>& fileContentsReady);

	static void
	saveFileContent(const QByteArray& fileContent, const QString& fileNameHint = QString());
};