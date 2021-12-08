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

    Q_PROPERTY(int explicitTextureWidth READ explicitTextureWidth WRITE setExplicitTextureWidth NOTIFY explicitTextureWidthChanged)
    Q_PROPERTY(int explicitTextureHeight READ explicitTextureHeight WRITE setExplicitTextureHeight NOTIFY explicitTextureHeightChanged)
    Q_PROPERTY(QSize effectiveTextureSize READ effectiveTextureSize NOTIFY effectiveTextureSizeChanged)
    Q_PROPERTY(bool alphaBlending READ alphaBlending WRITE setAlphaBlending NOTIFY alphaBlendingChanged)

public:
    QQuickRhiItem(QQuickItem *parent = nullptr);

    virtual QQuickRhiItemRenderer *createRenderer() = 0;

    int explicitTextureWidth() const;
    void setExplicitTextureWidth(int w);
    int explicitTextureHeight() const;
    void setExplicitTextureHeight(int h);

    QSize effectiveTextureSize() const;

    bool alphaBlending() const;
    void setAlphaBlending(bool b);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;
    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void explicitTextureWidthChanged();
    void explicitTextureHeightChanged();
    void effectiveTextureSizeChanged();
    void alphaBlendingChanged();

private Q_SLOTS:
    void invalidateSceneGraph();
};

#endif
