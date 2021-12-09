#include "rhiitem_p.h"
#include <QtGui/private/qrhi_p.h>
#include <private/qsgplaintexture_p.h>

/*!
    \class QQuickRhiItem
    \inmodule QtQuick
    \since 6.x

    \brief The QQuickRhiItem class is a convenience class for integrating QRhi
    rendering, that is targeting a 2D texture, with Qt Quick.

    In practice QQuickRhiItem replaces QQuickFramebufferObject from Qt 5. The
    latter was tied to OpenGL, while QQuickRhiItem is functional with any of
    the supported 3D graphics APIs abstracted by QRhi.

    \note QQuickRhiItem is not compatible with the \c software backend of Qt Quick.

    On most platforms, the rendering will occur on a \l {Scene Graph and
    Rendering}{dedicated thread}. For this reason, the QQuickRhiItem class
    enforces a strict separation between the item implementation and the
    rendering working directly with the graphics resources. All item logic,
    such as properties and UI-related helper functions needed by QML should be
    located in a QQuickRhiItem class subclass. Everything that relates to
    rendering must be located in the QQuickRhiItemRenderer class.

    To avoid race conditions and read/write issues from two threads it is
    important that the renderer and the item never read or write shared
    variables. Communication between the item and the renderer should primarily
    happen via the QQuickRhiItemRenderer::synchronize() function. This function
    will be called on the render thread while the GUI thread is blocked.

    Using queued connections or events for communication between item and
    renderer is also possible.

    To render into the 2D texture that is implicitly created and managed by the
    QQuickRhiItem, the user should subclass the QQuickRhiItemRenderer class and
    reimplement its render() function. An instance of the QQuickRhiItemRenderer
    subclass is expected to be returned from createRenderer().

    The size of the texture will by default adapt to the size of the item. If a
    fixed size is preferred, set explicitTextureWidth and
    explicitTextureHeight.

    QQuickRhiItem is a \l{QSGTextureProvider}{texture provider} and can be used
    directly in \l {ShaderEffect}{ShaderEffects} and other classes that consume
    texture providers, without involving an additional render pass.

    An example of a basic QQuickRhiItem implementation could be the following:

    \code
        class ExampleItem : public QQuickRhiItem
        {
            Q_OBJECT
            QML_NAMED_ELEMENT(ExampleItem)
            Q_PROPERTY(bool transparentBackground READ transparentBackground WRITE setTransparentBackground NOTIFY transparentBackgroundChanged)
        public:
            QQuickRhiItemRenderer *createRenderer() override { return new ExampleRenderer; }
            bool transparentBackground() const { return m_transparentBackground; }
            void setTransparentBackground(bool b);
        signals:
            void transparentBackgroundChanged();
        private:
            bool m_transparentBackground;
        };

        void ExampleItem::setTransparentBackground(bool b)
        {
            if (m_transparentBackground == b)
                return;
            m_transparentBackground = b;
            emit transparentBackgroundChanged();
            update();
        }
    \endcode

    Here we demonstrate an item with one custom property, \c
    transparentBackground, which will control the color to which the texture is
    cleared to in the renderer. The value will be synchronized to the renderer
    object in QQuickRhiItemRenderer::synchronize().

    \code
        class ExampleRenderer : public QQuickRhiItemRenderer
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
            bool m_transparentBackground = false;
        };

        void ExampleRenderer::initialize(QRhi *rhi, QRhiTexture *outputTexture)
        {
            m_rhi = rhi;
            m_output = outputTexture;
            if (!m_ds) {
                m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize()));
                m_ds->create();
            } else if (m_ds->pixelSize() != m_output->pixelSize()) {
                m_ds->setPixelSize(m_output->pixelSize());
                m_ds->create();
            }
            if (!m_rt) {
                m_rt.reset(m_rhi->newTextureRenderTarget({ { m_output }, m_ds.data() }));
                m_rp.reset(m_rt->newCompatibleRenderPassDescriptor());
                m_rt->setRenderPassDescriptor(m_rp.data());
                m_rt->create();
            }
        }

        void ExampleRenderer::synchronize(QQuickRhiItem *rhiItem)
        {
            ExampleItem *item = static_cast<ExampleItem *>(rhiItem);
            if (item->transparentBackground() != m_transparentBackground)
                m_transparentBackground = item->transparentBackground();
        }

        void ExampleRenderer::render(QRhiCommandBuffer *cb)
        {
            const QColor clearColor = m_transparentBackground ? Qt::transparent
                                                              : QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
            cb->beginPass(m_rt.data(), clearColor, { 1.0f, 0 });
            cb->endPass();
        }
    \endcode

    The renderer logic starts and ends a render pass targeting the texture,
    which leads to clearing it to the specified color.

    The renderer example above also shows how to set up a depth-stencil buffer.
    When depth testing is not needed by the rendering logic, this can be left
    out and the QRhiTextureRenderTarget can be created with just m_output as a
    color attachment, no depth-stencil buffer.

    ExampleItem can then, assuming the necessary \c qt_add_qml_module in place
    in CMakeLists.txt with the URI \c ExampleUri, be instantiated in the Qt
    Quick scene in QML:

    \badcode
    import QtQuick
    import ExampleUri
    Item {
        ...
        ExampleItem {
            width: 640
            height: 480
            anchors.centerIn: parent
        }
    }
    \endcode

    The result is a quad textured with a texture that is, due to
    ExampleRenderer::render(), either opaque green or fully transparent in this
    example. ExampleItem is a proper visual QQuickItem, and can be transformed
    and manipulated like other Items.

    \sa QQuickRhiItemRenderer, {Scene Graph - Rendering FBOs}, {Scene Graph and Rendering}
 */

/*!
    \class QQuickRhiItemRenderer
    \inmodule QtQuick
    \since 6.x

    The QQuickRhiItemRenderer class is used to implement the rendering logic of
    a QQuickRhiItem. It is instantiated and returned from the QQuickRhiItem
    subclass' reimplementation of createRenderer().

    The QQuickRhiItemRenderer always lives on the rendering thread of the Qt
    Quick scenegraph. All its functions are called on the render thread.

    \note Due to different lifetimes and thread affinities, care must be taken
    to only access the QQuickRhiItem from the renderer when it is safe to do
    so: in synchronize() and initialize(). Keeping references to the item and
    dereferencing it elsewhere, in render() or in the destructor is unsafe and
    will can lead to unspecified behavior.

    \sa QQuickRhiItem
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

    QSize newSize(m_item->explicitTextureWidth(), m_item->explicitTextureHeight());
    if (newSize.isEmpty()) {
        m_dpr = m_window->effectiveDevicePixelRatio();
        const int minTexSize = m_rhi->resourceLimit(QRhi::TextureSizeMin);
        newSize = QSize(qMax<int>(minTexSize, m_item->width()),
                        qMax<int>(minTexSize, m_item->height())) * m_dpr;
    }

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
                m_sgWrapperTexture->setHasAlphaChannel(m_item->alphaBlending());
                setTexture(m_sgWrapperTexture);
            }
        }
        QQuickRhiItemPrivate::get(m_item)->effectiveTextureSize = m_pixelSize;
        emit m_item->effectiveTextureSizeChanged();
        if (m_texture)
            m_renderer->initialize(m_rhi, m_texture);
    }

    if (m_sgWrapperTexture && m_sgWrapperTexture->hasAlphaChannel() != m_item->alphaBlending()) {
        m_sgWrapperTexture->setHasAlphaChannel(m_item->alphaBlending());
        // hasAlphaChannel is mapped to QSGMaterial::Blending in setTexture() so that has to be called again
        setTexture(m_sgWrapperTexture);
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

    n->setTextureCoordinatesTransform(d->mirrorVertically ? QSGSimpleTextureNode::MirrorVertically : QSGSimpleTextureNode::NoTransform);
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
    \property QQuickRhiItem::explicitTextureWidth

    This property allows specifying the width (in pixels) of the
    QQuickRhiItem's associated texture. By default the texture follows the size
    of the QQuickRhiItem. When this is not desired, set explicitTextureWidth
    and explicitTextureHeight to a value larger than 0. The texture will then
    always have that size.

    The default value is 0.
 */

int QQuickRhiItem::explicitTextureWidth() const
{
    Q_D(const QQuickRhiItem);
    return d->explicitTextureWidth;
}

void QQuickRhiItem::setExplicitTextureWidth(int w)
{
    Q_D(QQuickRhiItem);
    if (d->explicitTextureWidth == w)
        return;

    d->explicitTextureWidth = w;
    emit explicitTextureWidthChanged();
    update();
}

/*!
    \property QQuickRhiItem::explicitTextureHeight

    This property allows specifying the height (in pixels) of the
    QQuickRhiItem's associated texture. By default the texture follows the size
    of the QQuickRhiItem. When this is not desired, set explicitTextureWidth
    and explicitTextureHeight to a value larger than 0. The texture will then
    always have that size.

    The default value is 0.
 */

int QQuickRhiItem::explicitTextureHeight() const
{
    Q_D(const QQuickRhiItem);
    return d->explicitTextureHeight;
}

void QQuickRhiItem::setExplicitTextureHeight(int h)
{
    Q_D(QQuickRhiItem);
    if (d->explicitTextureHeight == h)
        return;

    d->explicitTextureHeight = h;
    emit explicitTextureHeightChanged();
    update();
}

/*!
    \property QQuickRhiItem::effectiveTextureSize

    This read-only property contains the size of the QQuickRhiItem's associated
    texture, in pixels. In practice this is the same as the pixelSize() of the
    \c outputTexture passed to QQuickRhiItemRenderer::initialize().

    \note The value is only up-to-date when the QQuickRhiItem has rendered at
    least once.
 */

QSize QQuickRhiItem::effectiveTextureSize() const
{
    Q_D(const QQuickRhiItem);
    return d->effectiveTextureSize;
}

/*!
    \property QQuickRhiItem::alphaBlending

    This property controls if blending is enabled for the item even when
    nothing else, such as the opacity, implies that alpha blending is required.

    The default value is true.

    The value plays no role when the item's effective opacity is smaller than
    1.0, because blending is then enabled implicitly.

    Setting the property to false can serve as an optimization when the content
    rendered to the QQuickRhiItem's associated texture is fully opaque and no
    semi-transparency is involved. The result on screen is not affected in this
    case, but disabling blending can be more efficient depending on the
    underlying graphics API and hardware.
 */

bool QQuickRhiItem::alphaBlending() const
{
    Q_D(const QQuickRhiItem);
    return d->blend;
}

void QQuickRhiItem::setAlphaBlending(bool enable)
{
    Q_D(QQuickRhiItem);
    if (d->blend == enable)
        return;

    d->blend = enable;
    emit alphaBlendingChanged();
    update();
}

/*!
    \property QQuickRhiItem::mirrorVertically

    This property controls if the shader that is used when drawing the quad
    textured with the QQuickRhiItem's associated texture should flip the V
    texture coordinate.

    The default value is false.
 */

bool QQuickRhiItem::mirrorVertically() const
{
    Q_D(const QQuickRhiItem);
    return d->mirrorVertically;
}

void QQuickRhiItem::setMirrorVertically(bool enable)
{
    Q_D(QQuickRhiItem);
    if (d->mirrorVertically == enable)
        return;

    d->mirrorVertically = enable;
    emit mirrorVerticallyChanged();
    update();
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
    if (!m_ds) {
        // no depth-stencil buffer yet, create one
        m_ds = m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_output->pixelSize());
        m_ds->create();
    } else if (m_ds->pixelSize() != m_output->pixelSize()) {
        // the size has changed, update the size and rebuild
        m_ds->setPixelSize(m_output->pixelSize());
        m_ds->create();
    }
    if (!m_rt) {
        m_rt = m_rhi->newTextureRenderTarget({ { m_output }, m_ds });
        m_rp = m_rt->newCompatibleRenderPassDescriptor();
        m_rt->setRenderPassDescriptor(m_rp);
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

#include "rhiitem.moc"
#include "moc_rhiitem.cpp"
