﻿#pragma once

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>


#ifdef CUVMATERIALAPPBAR_LIB
#define CUVMATERIALAPPBAR_EXPORT Q_DECL_EXPORT
#else
#define CUVMATERIALAPPBAR_EXPORT Q_DECL_IMPORT
#endif

class CUVMaterialAppBarPrivate;

class CUVMATERIALAPPBAR_EXPORT CUVMaterialAppBar final : public QWidget {
	Q_OBJECT

	Q_PROPERTY(QColor foregroundColor WRITE setForegroundColor READ foregroundColor)
	Q_PROPERTY(QColor backgroundColor WRITE setBackgroundColor READ backgroundColor)

public:
	explicit CUVMaterialAppBar(QWidget* parent = nullptr);
	~CUVMaterialAppBar() override;

	[[nodiscard]] QSize sizeHint() const Q_DECL_OVERRIDE;

	void setUseThemeColors(bool value);
	[[nodiscard]] bool useThemeColors() const;

	void setForegroundColor(const QColor& color);
	[[nodiscard]] QColor foregroundColor() const;

	void setBackgroundColor(const QColor& color);
	[[nodiscard]] QColor backgroundColor() const;

	[[nodiscard]] inline QHBoxLayout* appBarLayout() const;

protected:
	void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

	const QScopedPointer<CUVMaterialAppBarPrivate> d_ptr;

private:
	Q_DISABLE_COPY(CUVMaterialAppBar)
	Q_DECLARE_PRIVATE(CUVMaterialAppBar)
};

inline QHBoxLayout* CUVMaterialAppBar::appBarLayout() const {
	return dynamic_cast<QHBoxLayout*>(layout());
}
