#include "rhiitem_p.h"
#include <QtGui/private/qrhi_p.h>
#include <private/qsgplaintexture_p.h>

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
    if (!m_rhi) {
        QSGRendererInterface *rif = m_window->rendererInterface();
        m_rhi = static_cast<QRhi *>(rif->getResource(m_window, QSGRendererInterface::RhiResource));
        if (!m_rhi) {
            qWarning("No QRhi found for window %p, QQuickRhiItem will not be functional", m_window);
            return;
        }
    }

    m_dpr = m_window->effectiveDevicePixelRatio();
    const int minTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMin);
    const QSize newSize = QSize(qMax<int>(minTexSize, m_item->width()),
                                qMax<int>(minTexSize, m_item->height())) * m_dpr;

    bool needsNew = !m_sgWrapperTexture;
    if (newSize != m_pixelSize) {
        needsNew = true;
        m_pixelSize = newSize;
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
                m_sgWrapperTexture->setHasAlphaChannel(true);
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

    QSGRendererInterface *rif = m_window->rendererInterface();
    QRhiCommandBuffer *cb = nullptr;
    QRhiSwapChain *swapchain = static_cast<QRhiSwapChain *>(
        rif->getResource(m_window, QSGRendererInterface::RhiSwapchainResource));
    cb = swapchain ? swapchain->currentFrameCommandBuffer()
                   : static_cast<QRhiCommandBuffer *>(rif->getResource(m_window, QSGRendererInterface::RhiRedirectCommandBuffer));
    if (!cb) {
        qWarning("Neither swapchain nor redirected command buffer are available.");
        return;
    }

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

    // Changing to an empty size should not involve destroying and then later
    // recreating the node, because we do not know how expensive the user's
    // renderer setup is. Rather, keep the node if it already exist, and clamp
    // all accesses to width and height. Hence the unusual !n condition here.
    if (!n && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!n) {
        if (!d->node)
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
    n->setRect(0, 0, qMax<int>(0, width()), qMax<int>(0, height()));

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

void QQuickRhiItem::invalidateSceneGraph()
{
    // called on the render thread when the scenegraph is invalidated

    Q_D(QQuickRhiItem);
    d->node = nullptr;
}

/*!
    \reimp
*/
bool QQuickRhiItem::isTextureProvider() const
{
    return true;
}

/*!
    \reimp
*/
QSGTextureProvider *QQuickRhiItem::textureProvider() const
{
    if (QQuickItem::isTextureProvider()) // e.g. if Item::layer::enabled == true
        return QQuickItem::textureProvider();

    QQuickWindow *w = window();
    if (!w || !w->isSceneGraphInitialized() || QThread::currentThread() != QQuickWindowPrivate::get(w)->context->thread()) {
        qWarning("QQuickRhiItem::textureProvider: can only be queried on the rendering thread of an exposed window");
        return nullptr;
    }

    Q_D(const QQuickRhiItem);
    if (!d->node) // create a node to have a provider, the texture will be null but that's ok
        d->node = new QQuickRhiItemNode(const_cast<QQuickRhiItem *>(this));

    return d->node;
}

/*!
    Call this function when the texture contents should be rendered again. This
    function can be called from render() to force the texture to be rendered to
    again before the next frame, i.e. to request another call to render().

    \note This function should be used from inside the renderer. To update the
    item on the GUI thread, use QQuickRhiItem::update(). Calling this function
    does not trigger invoking synchronize() because it is expected that the
    item properties affecting the renderer do not change and need no
    synchronizing. To also trigger synchronization, call update() on the
    QQuickRhiItem on the GUI thread instead.
 */
void QQuickRhiItemRenderer::update()
{
    if (data)
        static_cast<QQuickRhiItemNode *>(data)->scheduleUpdate();
}

/*!
    Destructor. Called on the render thread of the Qt Quick scenegraph.

    As with QSGNode objects, the QQuickRhiItemRenderer may or may not outlive
    the QQuickRhiItem. Therefore accessing references to the item in the
    destructor is not safe.
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
    size if this must follow the size of \a outputTexture. A compact and
    efficient way for this is the following:

    \code
    m_rhi = rhi;
    m_output = outputTexture;
    bool updateRt = false;
    if (!m_ds) {
        // no depth-stencil buffer yet, create one
        m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize());
        m_ds->create();
        updateRt = true;
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        // the size has changed, update the size and rebuild
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
        updateRt = true;
    }
    if (!m_rt) {
        m_rt = m_rhi->newTextureRenderTarget({ { m_output }, m_ds });
        m_rp = m_rt->newCompatibleRenderPassDescriptor();
        m_rt->setRenderPassDescriptor(m_rp);
        m_rt->create();
    } else if (updateRt) {
        m_rt->create();
    }
    \endcode

    This function is called on the render thread of the Qt Quick scenegraph.
    Called with the GUI (main) thread blocked.

    The created resources are expected to be released in the destructor
    implementation of the subclass. \a rhi and \a outputTexture are not owned
    by, and are guaranteed to outlive the QQuickRhiItemRenderer.

    \sa render(), synchronize()
 */
void QQuickRhiItemRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
{
    Q_UNUSED(rhi);
    Q_UNUSED(outputTexture);
}

/*!
    Called with the GUI (main) thread is blocked on the render thread of the Qt
    Quick scenegraph. This function is the only place when it is safe for the
    renderer and the item to read and write each others members.

    This function is called as a result of QQuickRhiItem::update(). It is not
    triggered by QQuickRhiItemRenderer::update(), however.

    Use this function to update the renderer with changes that have occurred in
    the item. \a item is the item that instantiated this renderer. The function
    is called once before the first call to render(). The call to this function
    always happens after initialize(), if there is one.

    \e {For instance, if the item has a color property which is controlled by
    QML, one should call QQuickRhiItem::update() and use synchronize() to copy
    the new color into the renderer so that it can be used to render the next
    frame.

    \sa render(), initialize()
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

    To request updates from the GUI (main) thread, use QQuickItem::update() on
    the QQuickRhiItem. To schedule an update from the render thread, from
    within render() in order to continously update, call update() on the
    QQuickRhiItemRenderer.

    \a cb is the QRhiCommandBuffer for the current frame of the Qt Quick
    scenegraph. The function is called with a frame being recorded, but without
    an active render pass.

    \sa initialize(), synchronize(), QQuickItem::update(), QQuickRhiItemRenderer::update()
 */
void QQuickRhiItemRenderer::render(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

#include "moc_rhiitem.cpp"
