#include "customrhiitem.h"
#include "cube.h"
#include <QFile>
#include <QPainter>

static const QSize CUBE_TEX_SIZE(512, 512);

void TestRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    m_rhi = rhi;
    m_output = outputTexture;

    bool updateRt = false;
    if (!m_ds) {
        m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize()));
        m_ds->create();
        updateRt = true;
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
        updateRt = true;
    }

    if (!m_rt) {
        m_rt.reset(m_rhi->newTextureRenderTarget({ { m_output }, m_ds.data() }));
        m_rp.reset(m_rt->newCompatibleRenderPassDescriptor());
        m_rt->setRenderPassDescriptor(m_rp.data());
        m_rt->create();
    } else if (updateRt) {
        m_rt->create();
    }

    if (!scene.vbuf) {
        initScene();
        updateCubeTexture();
    }

    const QSize outputSize = m_output->pixelSize();
    scene.mvp = m_rhi->clipSpaceCorrMatrix();
    scene.mvp.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    scene.mvp.translate(0, 0, -4);
    updateMvp();
}

void TestRenderer::updateMvp()
{
    QMatrix4x4 mvp = scene.mvp * QMatrix4x4(QQuaternion::fromEulerAngles(itemData.cubeRotation).toRotationMatrix());
    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->updateDynamicBuffer(scene.ubuf.data(), 0, 64, mvp.constData());
}

void TestRenderer::updateCubeTexture()
{
    QImage image(CUBE_TEX_SIZE, QImage::Format_RGBA8888);
    const QRect r(QPoint(0, 0), CUBE_TEX_SIZE);
    QPainter p(&image);
    p.fillRect(r, QGradient::DeepBlue);
    QFont font;
    font.setPointSize(24);
    p.setFont(font);
    p.drawText(r, itemData.message);
    p.end();

    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->uploadTexture(scene.cubeTex.data(), image);
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void TestRenderer::initScene()
{
    scene.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube)));
    scene.vbuf->create();

    scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->uploadStaticBuffer(scene.vbuf.data(), cube);

    scene.ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68));
    scene.ubuf->create();

    const qint32 flip = m_rhi->isYUpInFramebuffer() ? 1 : 0;
    scene.resourceUpdates->updateDynamicBuffer(scene.ubuf.data(), 64, 4, &flip);

    scene.cubeTex.reset(m_rhi->newTexture(QRhiTexture::RGBA8, CUBE_TEX_SIZE));
    scene.cubeTex->create();

    scene.sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                               QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    scene.sampler->create();

    scene.srb.reset(m_rhi->newShaderResourceBindings());
    scene.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, scene.ubuf.data()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, scene.cubeTex.data(), scene.sampler.data())
    });
    scene.srb->create();

    scene.ps.reset(m_rhi->newGraphicsPipeline());
    scene.ps->setDepthTest(true);
    scene.ps->setDepthWrite(true);
    scene.ps->setDepthOp(QRhiGraphicsPipeline::Less);
    scene.ps->setCullMode(QRhiGraphicsPipeline::Back);
    scene.ps->setFrontFace(QRhiGraphicsPipeline::CCW);
    QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    Q_ASSERT(fs.isValid());
    scene.ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });
    scene.ps->setVertexInputLayout(inputLayout);
    scene.ps->setShaderResourceBindings(scene.srb.data());
    scene.ps->setRenderPassDescriptor(m_rp.data());
    scene.ps->create();
}

void TestRenderer::synchronize(QQuickRhiItem *rhiItem)
{
    TestRhiItem *item = static_cast<TestRhiItem *>(rhiItem);
    if (item->cubeRotation() != itemData.cubeRotation) {
        itemData.cubeRotation = item->cubeRotation();
        updateMvp();
    }
    if (item->message() != itemData.message) {
        itemData.message = item->message();
        updateCubeTexture();
    }
}

void TestRenderer::render(QRhiCommandBuffer *cb)
{
    QRhiResourceUpdateBatch *rub = scene.resourceUpdates;
    if (rub)
        scene.resourceUpdates = nullptr;

    cb->beginPass(m_rt.data(), QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f), { 1.0f, 0 }, rub);

    cb->setGraphicsPipeline(scene.ps.data());
    const QSize outputSize = m_output->pixelSize();
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { scene.vbuf.data(), 0 },
        { scene.vbuf.data(), quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    cb->endPass();
}

void TestRhiItem::setCubeRotation(const QVector3D &v)
{
    if (m_cubeRotation == v)
        return;

    m_cubeRotation = v;
    emit cubeRotationChanged();
    update();
}

void TestRhiItem::setMessage(const QString &s)
{
    if (m_message == s)
        return;

    m_message = s;
    emit messageChanged();
    update();
}
