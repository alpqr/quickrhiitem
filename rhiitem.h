#ifndef RHIITEM_H
#define RHIITEM_H

#include <QQuickItem>

class QQuickRhiItem;
class QQuickRhiItemPrivate;
class QRhi;
class QRhiTexture;
class QRhiCommandBuffer;

class QQuickRhiItemRenderer
{
public:
    virtual ~QQuickRhiItemRenderer();
    virtual void initialize(QRhi *rhi, QRhiTexture *outputTexture);
    virtual void synchronize(QQuickRhiItem *item);
    virtual void render(QRhiCommandBuffer *cb);

    void update();

private:
    void *data;
    friend class QQuickRhiItem;
};

class QQuickRhiItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRhiItem)

public:
    QQuickRhiItem(QQuickItem *parent = nullptr);

    virtual QQuickRhiItemRenderer *createRenderer() = 0;

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;

private:
    Q_PRIVATE_SLOT(d_func(), void _q_invalidateSceneGraph())
};

#endif
