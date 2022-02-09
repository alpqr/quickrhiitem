#ifndef CUSTOMRHIITEM_H
#define CUSTOMRHIITEM_H

#include "rhiitem.h"
#include <QtGui/private/qrhi_p.h>

class TestRenderer : public QQuickRhiItemRenderer
{
public:
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_output = nullptr;
    QScopedPointer<QRhiRenderBuffer> m_ds;
    QScopedPointer<QRhiTextureRenderTarget> m_rt;
    QScopedPointer<QRhiRenderPassDescriptor> m_rp;

    struct {
        QRhiResourceUpdateBatch *resourceUpdates = nullptr;
        QScopedPointer<QRhiBuffer> vbuf;
        QScopedPointer<QRhiBuffer> ubuf;
        QScopedPointer<QRhiShaderResourceBindings> srb;
        QScopedPointer<QRhiGraphicsPipeline> ps;
        QScopedPointer<QRhiSampler> sampler;
        QScopedPointer<QRhiTexture> cubeTex;
        QMatrix4x4 mvp;
    } scene;

    struct {
    } itemData;

    void initScene();
    void updateMvp();
};

class TestRhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)

public:
    QQuickRhiItemRenderer *createRenderer() override { return new TestRenderer; }

signals:

private:

};

#endif
