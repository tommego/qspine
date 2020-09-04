#ifndef SKELETONRENDERER_H
#define SKELETONRENDERER_H
#include <QQuickFramebufferObject>
#include <QSGTexture>
#include <QSharedPointer>
#include <QHash>

class Texture;
class RenderCmdsCache;

class SkeletonRenderer : public QQuickFramebufferObject::Renderer
{
public:
    SkeletonRenderer();
    ~SkeletonRenderer() override;

    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize & size) override;
    virtual void render() override;
    virtual void synchronize(QQuickFramebufferObject * item) override;

    QSharedPointer<RenderCmdsCache> getCache() const;

private:
    QSharedPointer<RenderCmdsCache> m_cache;

};

#endif // SKELETONRENDERER_H
