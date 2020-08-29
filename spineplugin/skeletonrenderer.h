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

    QSGTexture* getGLTexture(Texture*texture, QQuickWindow*window);
    void releaseTextures();

    QSharedPointer<RenderCmdsCache> getCache() const;

private:
private:
    QSharedPointer<RenderCmdsCache> m_cache;
    QHash<QString, QSGTexture*> m_textureHash;

};

#endif // SKELETONRENDERER_H
