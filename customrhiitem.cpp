#include "customrhiitem.h"
#include <QtGui/private/qrhi_p.h>

TestRenderer::~TestRenderer()
{
    delete m_rt;
    delete m_rp;
    delete m_ds;
}

void TestRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    qDebug() << rhi << outputTexture << outputTexture->pixelSize();
    m_rhi = rhi;
    m_output = outputTexture;

    bool sizeChanged = false;
    if (!m_ds) {
        m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize());
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
        sizeChanged = true;
    }

    if (!m_rt) {
        QRhiTextureRenderTargetDescription rtDesc({ m_output }, m_ds);
        m_rt = m_rhi->newTextureRenderTarget(rtDesc);
        m_rp = m_rt->newCompatibleRenderPassDescriptor();
        m_rt->setRenderPassDescriptor(m_rp);
        m_rt->create();
    } else if (sizeChanged) {
        m_rt->create();
    }
}

void TestRenderer::synchronize(QQuickRhiItem *item)
{
    qDebug() << "sync" << item;
}

void TestRenderer::render(QRhiCommandBuffer *cb)
{
    qDebug() << "render";
    cb->beginPass(m_rt, Qt::green, { 1.0f, 0 });
    cb->endPass();
}
