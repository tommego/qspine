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

    animation->renderToCache(this);
}

QSharedPointer<RenderCmdsCache> SkeletonRenderer::getCache() const
{
    return m_cache;
}
