#ifndef CUSTOMRHIITEM_H
#define CUSTOMRHIITEM_H

#include "rhiitem.h"

class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;

class TestRenderer : public QQuickRhiItemRenderer
{
public:
    ~TestRenderer() override;
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    QRhiRenderBuffer *m_ds = nullptr;
    QRhiTextureRenderTarget *m_rt = nullptr;
    QRhiRenderPassDescriptor *m_rp = nullptr;
};

class TestRhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)

public:
    QQuickRhiItemRenderer *createRenderer() override { return new TestRenderer; }
};

#endif
