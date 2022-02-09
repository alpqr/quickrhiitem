#ifndef TESTRHIITEM_H
#define TESTRHIITEM_H

#include "rhiitem.h"
#include <QtGui/private/qrhi_p.h>

class TestRenderer : public RhiItemRenderer
{
public:
    void initialize(QRhi *rhi, QRhiTexture *outputTexture) override;
    void synchronize(RhiItem *item) override;
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

class TestRhiItem : public RhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)

public:
    RhiItemRenderer *createRenderer() override { return new TestRenderer; }

signals:

private:

};

#endif
