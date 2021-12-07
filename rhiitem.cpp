#include "rhiitem_p.h"
#include <QtGui/private/qrhi_p.h>
#include <private/qsgplaintexture_p.h>
#include <private/qquickwindow_p.h>
#include <private/qsgdefaultrendercontext_p.h>

/*!
    \class QQuickRhiItem

    Replaces QQuickFramebufferObject in the modern world.

 */

QQuickRhiItemNode::QQuickRhiItemNode(QQuickRhiItem *item)
    : m_item(item)
{
    m_window = m_item->window();
    Q_ASSERT(m_window);
    connect(m_window, &QQuickWindow::beforeRendering, this, &QQuickRhiItemNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });
}

QQuickRhiItemNode::~QQuickRhiItemNode()
{
    delete m_renderer;
    delete m_sgWrapperTexture;
    releaseNativeTexture();
}

QSGTexture *QQuickRhiItemNode::texture() const
{
    return m_sgWrapperTexture;
}

void QQuickRhiItemNode::createNativeTexture()
{
    Q_ASSERT(!m_texture);
    Q_ASSERT(!m_pixelSize.isEmpty());

    m_texture = m_rhi->newTexture(QRhiTexture::RGBA8, m_pixelSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!m_texture->create()) {
        qWarning("Failed to create QQuickRhiItem texture of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
        delete m_texture;
        m_texture = nullptr;
    }
}

void QQuickRhiItemNode::releaseNativeTexture()
{
    if (m_texture) {
        m_texture->deleteLater();
        m_texture = nullptr;
    }
}

void QQuickRhiItemNode::sync()
{
    m_dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_window->size() * m_dpr;
    bool needsNew = false;

    if (!m_sgWrapperTexture)
        needsNew = true;

    if (newSize != m_pixelSize) {
        needsNew = true;
        m_pixelSize = newSize;
    }

    if (!m_rhi) {
        QSGRendererInterface *rif = m_window->rendererInterface();
        m_rhi = static_cast<QRhi *>(rif->getResource(m_window, QSGRendererInterface::RhiResource));
        if (!m_rhi) {
            qWarning("No QRhi found for window %p, QQuickRhiItem will not be functional", m_window);
            return;
        }
    }

    if (needsNew) {
        if (m_texture && m_sgWrapperTexture) {
            m_texture->setPixelSize(m_pixelSize);
            if (m_texture->create())
                m_sgWrapperTexture->setTextureSize(m_pixelSize);
            else
                qWarning("Failed to recreate QQuickRhiItem texture of size %dx%d", m_pixelSize.width(), m_pixelSize.height());
        } else {
            delete m_sgWrapperTexture;
            m_sgWrapperTexture = nullptr;
            releaseNativeTexture();
            createNativeTexture();
            if (m_texture) {
                m_sgWrapperTexture = new QSGPlainTexture;
                m_sgWrapperTexture->setOwnsTexture(false);
                m_sgWrapperTexture->setTexture(m_texture);
                m_sgWrapperTexture->setTextureSize(m_pixelSize);
                //m_sgWrapperTexture->setHasAlphaChannel(options & QQuickWindow::TextureHasAlphaChannel);
                setTexture(m_sgWrapperTexture);
            }
        }
        if (m_texture)
            m_renderer->initialize(m_rhi, m_texture);
    }

    m_renderer->synchronize(m_item);
}

void QQuickRhiItemNode::render()
{
    // called before Qt Quick starts recording its main render pass

    if (!m_rhi || !m_texture || !m_renderer)
        return;

    if (!m_renderPending)
        return;

    QQuickWindowPrivate *wd = QQuickWindowPrivate::get(m_window);
    QRhiCommandBuffer *cb = static_cast<QSGDefaultRenderContext *>(wd->context)->currentFrameCommandBuffer();
    Q_ASSERT(cb);

    m_renderPending = false;
    m_renderer->render(cb);

    markDirty(QSGNode::DirtyMaterial);
    emit textureChanged();
}

void QQuickRhiItemNode::scheduleUpdate()
{
    m_renderPending = true;
    m_window->update(); // ensure getting to beforeRendering() at some point
}

QQuickRhiItem::QQuickRhiItem(QQuickItem *parent)
    : QQuickItem(*new QQuickRhiItemPrivate, parent)
{
    setFlag(ItemHasContents);
}

/*!
    \internal
 */
QSGNode *QQuickRhiItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    Q_D(QQuickRhiItem);
    QQuickRhiItemNode *n = static_cast<QQuickRhiItemNode *>(node);

    if (width() <= 0 || height() <= 0) {
        delete n;
        d->node = nullptr;
        return nullptr;
    }

    if (!n) {
        d->node = new QQuickRhiItemNode(this);
        n = d->node;
    }

    if (!n->hasRenderer()) {
        QQuickRhiItemRenderer *r = createRenderer();
        if (r) {
            r->data = n;
            n->setRenderer(r);
        } else {
            qWarning("No QQuickRhiItemRenderer was created; the item will not render");
            delete n;
            d->node = nullptr;
            return nullptr;
        }
    }

    n->sync();

    if (!n->isValid()) {
        delete n;
        d->node = nullptr;
        return nullptr;
    }

    n->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    n->setFiltering(QSGTexture::Linear);
    n->setRect(0, 0, width(), height());

    n->scheduleUpdate();

    return n;
}

/*!
    \internal
 */
void QQuickRhiItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}

/*!
    \internal
 */
void QQuickRhiItem::releaseResources()
{
    // called on the gui thread if the item is removed from scene

    Q_D(QQuickRhiItem);
    d->node = nullptr;
}

void QQuickRhiItemPrivate::_q_invalidateSceneGraph()
{
    // called on the render thread when the scenegraph is invalidated

    node = nullptr;
}

/*!

 */
void QQuickRhiItemRenderer::update()
{
    if (data)
        static_cast<QQuickRhiItemNode *>(data)->scheduleUpdate();
}

/*!
    Destructor. Called on the render thread of the Qt Quick scenegraph.
 */
QQuickRhiItemRenderer::~QQuickRhiItemRenderer()
{
}

/*!
    Called when the item is initialized and every time its size changes.

    The implementation should be prepared that both \a rhi and \a outputTexture
    can change between invocations of this function, although this is not
    guaranteed to happen in practice. For example, when the item size changes,
    it is likely that this function is called with the same \a rhi and \a
    outputTexture as before, but \a outputTexture may have been rebuilt,
    meaning its \l{QRhiTexture::pixelSize()}{size} and the underlying native
    texture resource may be different than in the last invocation.

    Implementations will typically create or rebuild a QRhiTextureRenderTarget
    in order to allow the subsequent render() call to render into the texture.
    When a depth buffer is necessary create a QRhiRenderBuffer as well. The
    size if this must follow the size of \a outputTexture. The most efficient
    way for this is the following:

    \code
    m_rhi = rhi;
    m_output = outputTexture;
    if (!m_ds) {
        // no depth-stencil buffer yet, create one
        m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize());
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        // the size has changed, update the size and rebuild
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
    }
    // ... similarly create/update a QRhiTextureRenderTarget
    \endcode

    This function is called on the render thread of the Qt Quick scenegraph.
    Called with the gui (main) thread blocked.

    The created resources are expected to be released in the destructor
    implementation of the subclass.

    \sa render()
 */
void QQuickRhiItemRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    Q_UNUSED(rhi);
    Q_UNUSED(outputTexture);
}

/*!
    Called while gui (main) thread is blocked etc.
 */
void QQuickRhiItemRenderer::synchronize(QQuickRhiItem *item)
{
    Q_UNUSED(item);
}

/*!
    Called when the item contents (i.e. the contents of the texture) need
    updating.

    There is always at least one call to initialize() before this function is
    called.

    This function is called on the render thread of the Qt Quick scenegraph.

    To request updates from the gui (main) thread, use QQuickItem::update() on
    the QQuickRhiItem. To schedule an update from the render thread, from
    within render() in order to continously update, call update() on the
    QQuickRhiItemRenderer.

    \a cb is the QRhiCommandBuffer for the current frame of the Qt Quick
    scenegraph. The function is called with a frame being recorded, but without
    an active render pass.

    \sa initialize()
 */
void QQuickRhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

#include "moc_rhiitem.cpp"
