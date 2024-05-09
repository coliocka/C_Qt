#ifndef FRAMELESSHELPER_H
#define FRAMELESSHELPER_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include "qxframeless_global.hpp"

#ifndef QXRESULT
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QXRESULT qintptr
#else
#define QXRESULT long
#endif // QT_VERSION
#endif // QXRESULT

QX_FRAMELESS_BEGIN_NAMESPACE
class FramelessHelperPrivate;

class QX_FRAMELESS_EXPORT FramelessHelper final : public QObject, public QAbstractNativeEventFilter {
	Q_OBJECT

public:
	explicit FramelessHelper(QObject* parent);
	~FramelessHelper() override;

	void addWidget(QWidget* w);
	void removeWidget(QWidget* w);

	[[nodiscard]] bool widgetResizable() const;
	void setWidgetResizable(bool resizable);

	[[nodiscard]] bool widgetMovable() const;
	void setWidgetMovable(bool movable);

	[[nodiscard]] static int titleHeight();
	static void setTitleHeight(int height);

	[[nodiscard]] static int borderWidth();
	static void setBorderWidth(int width);

	void addCaptionClassName(const QString& name);

signals:
	void windowIconChanged(const QIcon& icon);
	void windowTitleChanged(const QString& title);
	void windowStateChanged(Qt::WindowStates state);

protected:
	bool eventFilter(QObject* object, QEvent* event) override;
	bool nativeEventFilter(const QByteArray& eventType, void* message, QXRESULT* result) override;

private:
	QX_DECLARE_PRIVATE(FramelessHelper)
};

QX_FRAMELESS_END_NAMESPACE

#endif // FRAMELESSHELPER_H
