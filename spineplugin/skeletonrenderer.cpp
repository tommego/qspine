#include "skeletonrenderer.h"
#include <QOpenGLFramebufferObject>
#include "rendercmdscache.h"
#include "spineitem.h"
#include "texture.h"
#include <QQuickWindow>

SkeletonRenderer::SkeletonRenderer():
    m_cache(new RenderCmdsCache)
{
}

SkeletonRenderer::~SkeletonRenderer()
{
    releaseTextures();
}

QOpenGLFramebufferObject *SkeletonRenderer::createFramebufferObject(const QSize &size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void SkeletonRenderer::render()
{
    m_cache->render();
}

void SkeletonRenderer::synchronize(QQuickFramebufferObject *item)
{
    SpineItem* animation = qobject_cast<SpineItem*>(item);
    if (!animation)
        return;

    animation->renderToCache(this, m_cache);
}

QSGTexture *SkeletonRenderer::getGLTexture(Texture *texture, QQuickWindow *window)
{
    if (!texture || texture->name().isEmpty())
        return nullptr;

    if (!window)
        return nullptr;

    if (m_textureHash.contains(texture->name()))
        return m_textureHash.value(texture->name());

    if (!texture->image())
        return nullptr;
    qDebug() << "uploading texture to gpu: " << texture->name();

    QSGTexture* tex = window->createTextureFromImage(*texture->image());
    tex->setFiltering(QSGTexture::Linear);
    tex->setMipmapFiltering(QSGTexture::Linear);
    m_textureHash.insert(texture->name(), tex);
    return tex;
}

void SkeletonRenderer::releaseTextures()
{
    if (m_textureHash.isEmpty())
        return;

    QHashIterator<QString, QSGTexture*> i(m_textureHash);
    while (i.hasNext()) {
        i.next();
        if (i.value())
            delete i.value();
    }

    m_textureHash.clear();
}

QSharedPointer<RenderCmdsCache> SkeletonRenderer::getCache() const
{
    return m_cache;
}
