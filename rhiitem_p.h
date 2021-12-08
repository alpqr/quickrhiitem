#ifndef RHIITEM_P_H
#define RHIITEM_P_H

#include "rhiitem.h"
#include <QSGSimpleTextureNode>
#include <QtQuick/private/qquickitem_p.h>

class QSGPlainTexture;
class QRhiTexture;
class QRhi;

class QQuickRhiItemNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

public:
    QQuickRhiItemNode(QQuickRhiItem *item);
    ~QQuickRhiItemNode();

    QSGTexture *texture() const override;

    void sync();
    bool isValid() const { return m_rhi && m_texture && m_sgWrapperTexture; }
    void scheduleUpdate();
    bool hasRenderer() const { return m_renderer; }
    void setRenderer(QQuickRhiItemRenderer *r) { m_renderer = r; }

private slots:
    void render();

private:
    void createNativeTexture();
    void releaseNativeTexture();

    QQuickRhiItem *m_item;
    QQuickWindow *m_window;
    QSize m_pixelSize;
    qreal m_dpr = 0.0f;
    QRhi *m_rhi = nullptr;
    QRhiTexture *m_texture = nullptr;
    QSGPlainTexture *m_sgWrapperTexture = nullptr;
    bool m_renderPending = true;
    QQuickRhiItemRenderer *m_renderer = nullptr;
};

class QQuickRhiItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickRhiItem)
public:
    mutable QQuickRhiItemNode *node = nullptr;
};

#endif
