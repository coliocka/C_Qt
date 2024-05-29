#pragma once

#include <QtWidgets/QWidget>

#ifdef CUVMATERIAL_LIB
#define CUVMATERIAL_EXPORT Q_DECL_EXPORT
#else
#define CUVMATERIAL_EXPORT Q_DECL_IMPORT
#endif

/*
 * @brief �ڸ�����ϴ���һ�����ǲ㣬�����ݸ�������ƶ��͵�����С�¼�ʵʱ���¸��ǲ��λ�úʹ�С
 */
class CUVMATERIAL_EXPORT CUVMaterialOverlayWidget : public QWidget {
	Q_OBJECT

public:
	explicit CUVMaterialOverlayWidget(QWidget* parent = nullptr);
	~CUVMaterialOverlayWidget() override;

protected:
	bool event(QEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

	[[nodiscard]] virtual QRect overlayGeometry() const;

private:
	Q_DISABLE_COPY(CUVMaterialOverlayWidget)
};
