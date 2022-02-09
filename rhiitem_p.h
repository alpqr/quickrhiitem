#ifndef RHIITEM_P_H
#define RHIITEM_P_H

#include "rhiitem.h"
#include <QSGSimpleTextureNode>
#include <QtQuick/private/qquickitem_p.h>

class QSGPlainTexture;
class QRhiTexture;
class QRhi;

class RhiItemNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    RhiItemNode(RhiItem *item);
    ~RhiItemNode();

    QSGTexture *texture() const override;

    void sync();
    bool isValid() const { return m_rhi && m_texture && m_sgWrapperTexture; }
    void scheduleUpdate();
    bool hasRenderer() const { return m_renderer; }
    void setRenderer(RhiItemRenderer *r) { m_renderer = r; }

private slots:
    void render();

private:
    void createNativeTexture();
    void releaseNativeTexture();

    RhiItem *m_item;
    QQuickWindow *m_window;
    QSize m_pixelSize;
    qreal m_dpr = 0.0f;
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_texture = nullptr;
    QSGPlainTexture *m_sgWrapperTexture = nullptr;
    bool m_renderPending = true;
    RhiItemRenderer *m_renderer = nullptr;
};

class RhiItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(RhiItem)
public:
    static RhiItemPrivate *get(RhiItem *item) { return item->d_func(); }
    mutable RhiItemNode *node = nullptr;
    int explicitTextureWidth = 0;
    int explicitTextureHeight = 0;
    bool blend = true;
    bool mirrorVertically = false;
    QSize effectiveTextureSize;
};

#endif
