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
        QVector3D cubeRotation;
        QString message;
        bool transparentBackground = false;
    } itemData;

    void initScene();
    void updateMvp();
    void updateCubeTexture();
};

class TestRhiItem : public QQuickRhiItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TestRhiItem)

    Q_PROPERTY(QVector3D cubeRotation READ cubeRotation WRITE setCubeRotation NOTIFY cubeRotationChanged)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    Q_PROPERTY(bool transparentBackground READ transparentBackground WRITE setTransparentBackground NOTIFY transparentBackgroundChanged)

public:
    QQuickRhiItemRenderer *createRenderer() override { return new TestRenderer; }

    QVector3D cubeRotation() const { return m_cubeRotation; }
    void setCubeRotation(const QVector3D &v);

    QString message() const { return m_message; }
    void setMessage(const QString &s);

    bool transparentBackground() const { return m_transparentBackground; }
    void setTransparentBackground(bool b);

signals:
    void cubeRotationChanged();
    void messageChanged();
    void transparentBackgroundChanged();

private:
    QVector3D m_cubeRotation;
    QString m_message;
    bool m_transparentBackground;
};

#endif
