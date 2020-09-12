#include "spineitem.h"

#include <QQmlFile>
#include <QtMath>
#include <QtConcurrent>
#include <QFile>
#include <QQuickWindow>

#include "skeletonrenderer.h"
#include <spine/spine.h>
#include <float.h>
#include "rendercmdscache.h"
#include "texture.h"

spine::String qstringtospinestring(const QString& str) {
    return spine::String(str.toStdString().data());
}

spine::String urltospinestring(const QUrl& url) {
    auto rcPath = QQmlFile::urlToLocalFileOrQrc(url);
    auto rePath = url.path();
    rePath = rePath.mid(1, rePath.size() - 1);
    auto abPath = url.path();
    if(QFile::exists(rcPath)) // first search from resource
        return qstringtospinestring(rcPath);
    else if(QFile::exists(rePath)) // then from relative path
        return qstringtospinestring(rePath); // last for absulute
    return qstringtospinestring(abPath);
}

SpineItem::SpineItem(QQuickItem *parent) :
    QQuickFramebufferObject(parent),
    m_skeletonScale(1.0),
    m_lazyLoadTimer(new QTimer),
    m_clipper(new spine::SkeletonClipping)
{
    AimyTextureLoader::instance()->setWindow(window());
    m_lazyLoadTimer->setSingleShot(true);
    m_lazyLoadTimer->setInterval(3);
    m_worldVertices = new float[2000];
    connect(this, &SpineItem::resourceReady, this, &SpineItem::onResourceReady);
    connect(m_lazyLoadTimer.get(), &QTimer::timeout, [=]{releaseSkeletonRelatedData();loadResource();});
}

SpineItem::~SpineItem()
{
    releaseSkeletonRelatedData();
    delete [] m_worldVertices;
}

QQuickFramebufferObject::Renderer *SpineItem::createRenderer() const
{
    return new SkeletonRenderer;
}

void SpineItem::setToSetupPose()
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_skeleton->setToSetupPose();
}

void SpineItem::setBonesToSetupPose()
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setBonesToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_skeleton->setBonesToSetupPose();
}

void SpineItem::setSlotsToSetupPose()
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setSlotsToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_skeleton->setSlotsToSetupPose();
}

bool SpineItem::setAttachment(const QString &slotName, const QString &attachmentName)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setAttachment Error: Skeleton is not ready";
        return false;
    }
    m_skeleton->setAttachment(spine::String(slotName.toStdString().data()), spine::String(attachmentName.toStdString().data()));
    return true;
}

void SpineItem::setMix(const QString &fromAnimation, const QString &toAnimation, float duration)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setMix Error: Skeleton is not ready";
        return;
    }
    m_animationStateData->setMix(qstringtospinestring(fromAnimation),
                                 qstringtospinestring(toAnimation),
                                 duration);
}

void SpineItem::setAnimation(int trackIndex, const QString &name, bool loop)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setAnimation Error: Skeleton is not ready";
        return;
    }
    if(!m_animations.contains(name)) {
        qWarning() << "no " << name << " found, vailable is : " << m_animations;
        return;
    }
    m_animationState->setAnimation(size_t(trackIndex), qstringtospinestring(name), loop);
}

void SpineItem::addAnimation(int trackIndex, const QString &name, bool loop, float delay)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::addAnimation Error: Skeleton is not ready";
        return;
    }
    if(!m_animations.contains(name)) {
        qWarning() << "no " << name << " found, vailable is : " << m_animations;
        return;
    }
    m_animationState->addAnimation(size_t(trackIndex), qstringtospinestring(name), loop, delay);
}

void SpineItem::setSkin(const QString &skinName)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::setSkin Error: Skeleton is not ready";
        return;
    }
    qInfo() << "setting skin " << skinName;
    if(!m_skins.contains(skinName)) {
        qWarning() <<  "no " << skinName << " found, vailable skins is: " << m_skins;
    }
    m_skeleton->setSkin(skinName.toStdString().c_str());
}

void SpineItem::clearTracks()
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::clearTracks Error: Skeleton is not ready";
        return;
    }
    m_animationState->clearTracks();
}

void SpineItem::clearTrack(int trackIndex)
{
    if(!isSkeletonReady()) {
        qWarning() << "SpineItem::clearTrack Error: Skeleton is not ready";
        return;
    }
    m_animationState->clearTrack(size_t(trackIndex));
}

static spine::Vector<SpineVertex> vertices;
static unsigned short quadIndices[] = {0, 1, 2, 2, 3, 0};

void SpineItem::renderToCache(QQuickFramebufferObject::Renderer *renderer, QSharedPointer<RenderCmdsCache> cache)
{
    if(!isSkeletonReady())
        return;

    if(cache.isNull())
        return;
    cache->clear();
    if(!renderer)
        return;

    if(m_shouldReleaseCacheTexture) {
        m_shouldReleaseCacheTexture = false;
    }

    cache->setSkeletonRect(m_boundingRect);

    cache->bindShader(RenderCmdsCache::ShaderTexture);

    unsigned short* triangles = nullptr;
    size_t trianglesCount = 0;
    bool hasBlend = false;

    for(size_t i = 0, n = m_skeleton->getSlots().size(); i < n; ++i) {
        auto slot = m_skeleton->getDrawOrder()[i];

        if (nothingToDraw(*slot)) {
            m_clipper->clipEnd(*slot);
            continue;
        }

        auto* attachment = slot->getAttachment();
        if (!attachment)
            continue;

        auto name = QString(attachment->getName().buffer());
        spine::BlendMode engineBlendMode = slot->getData().getBlendMode();

        auto skeletonColor = m_skeleton->getColor();
        auto slotColor = slot->getColor();
        spine::Color tint(skeletonColor.r * slotColor.r,
                          skeletonColor.g * slotColor.g,
                          skeletonColor.b * slotColor.b,
                          skeletonColor.a * slotColor.a);
        Texture* texture = nullptr;
        triangles = nullptr;
        trianglesCount = 0;
        if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
            auto regionAttachment = (spine::RegionAttachment*)attachment;
            texture = getTexture(regionAttachment);
            vertices.setSize(4, SpineVertex());
            regionAttachment->computeWorldVertices(slot->getBone(),
                                                   (float*)vertices.buffer(),
                                                   0,
                                                   sizeof (SpineVertex) / sizeof (float));
            for(size_t j = 0, l = 0; j < 4; j++,l+=2) {
                auto &vertex = vertices[j];
                vertex.color.set(tint);
                vertex.u = regionAttachment->getUVs()[l];
                vertex.v = regionAttachment->getUVs()[l + 1];
            }
            triangles = quadIndices;
            trianglesCount = 6;
        } else if (attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
            auto mesh = (spine::MeshAttachment*)attachment;
            size_t numVertices = mesh->getWorldVerticesLength() / 2;
            vertices.setSize(numVertices, SpineVertex());
            texture = getTexture(mesh);
            mesh->computeWorldVertices(*slot,
                                       0,
                                       mesh->getWorldVerticesLength(),
                                       (float*)vertices.buffer(),
                                       0,
                                       sizeof (SpineVertex) / sizeof (float));
            for (size_t j = 0, l = 0; j < numVertices; j++, l+=2) {
                auto& vertex = vertices[j];
                vertex.color.set(tint);
                vertex.u = mesh->getUVs()[l];
                vertex.v = mesh->getUVs()[l+1];
            }
            triangles = mesh->getTriangles().buffer();
            trianglesCount = mesh->getTriangles().size();

            if (m_vertexEfect) {
                // todo
            }

        } else if(attachment->getRTTI().isExactly(spine::ClippingAttachment::rtti)) {
            auto clip = (spine::ClippingAttachment*)attachment;
            m_clipper->clipStart(*slot, clip);
            continue;
        } else{
            m_clipper->clipEnd(*slot);
            continue;
        }

        if(tint.a == 0) {
            m_clipper->clipEnd(*slot);
            continue;
        }

        if(texture) {
            if(m_clipper->isClipping()) {

                auto tmpVerticesCount = vertices.size() * 2;
                spine::Vector<float> tmpVertices;
                spine::Vector<float> tmpUvs;
                tmpVertices.setSize(tmpVerticesCount, 0);
                tmpUvs.setSize(tmpVerticesCount, 0);

                for(size_t i = 0; i < vertices.size(); i++) {
                    tmpVertices[i * 2] = vertices[i].x;
                    tmpVertices[i * 2 + 1] = vertices[i].y;
                    tmpUvs[i * 2] = vertices[i].u;
                    tmpUvs[i * 2 + 1] = vertices[i].v;
                }
                m_clipper->clipTriangles(tmpVertices.buffer(), triangles, trianglesCount, tmpUvs.buffer(), sizeof (short));
                tmpVertices.setSize(0, 0);
                tmpUvs.setSize(0, 0);

                auto vertCount = m_clipper->getClippedVertices().size() / 2;

                if(m_clipper->getClippedTriangles().size() == 0) {
                    m_clipper->clipEnd(*slot);
                    continue;
                }
                triangles = m_clipper->getClippedTriangles().buffer();
                trianglesCount = m_clipper->getClippedTriangles().size();
                auto newUvs = m_clipper->getClippedUVs();
                auto newVertices = m_clipper->getClippedVertices();
                vertices.setSize(vertCount, SpineVertex());
                for(size_t i = 0; i < vertCount; i++) {
                    vertices[i].x = newVertices[i * 2];
                    vertices[i].y = newVertices[i * 2 + 1];
                    vertices[i].u = newUvs[i * 2];
                    vertices[i].v = newUvs[i * 2 + 1];
                    vertices[i].color.set(tint);
                }
            }
            if(!triangles) {
                m_clipper->clipEnd(*slot);
                hasBlend = false;
                continue;
            }

            if(hasBlend) {
                switch (engineBlendMode) {
                case spine::BlendMode_Additive: {
                    cache->blendFunc(GL_ONE, GL_ONE);
                    break;
                }
                case spine::BlendMode_Multiply: {
                    cache->blendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
                    break;
                }
                case spine::BlendMode_Screen: {
                    cache->blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                }
                default:{
                    cache->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                }
                }
            }
            cache->drawTriangles( AimyTextureLoader::instance()->getGLTexture(texture,window()), vertices, triangles, trianglesCount);
            hasBlend = true;
            m_clipper->clipEnd(*slot);
        }
    }
    m_clipper->clipEnd();

    // debug drawing
    if(m_debugBones || m_debugSlots) {
        cache->bindShader(RenderCmdsCache::ShaderColor);
        cache->blendFunc(GL_ONE, GL_ONE);

        if(m_debugSlots) {
            cache->drawColor(0, 100, 0, 255);
            cache->lineWidth(1);
            Point points[4];
            for (size_t i = 0, n = m_skeleton->getSlots().size(); i < n; i++) {
                auto slot = m_skeleton->getSlots()[i];
                if(!slot->getAttachment() || !slot->getAttachment()->getRTTI().isExactly(spine::RegionAttachment::rtti))
                    continue;
                auto* regionAttachment = (spine::RegionAttachment*)slot->getAttachment();
                regionAttachment->computeWorldVertices(slot->getBone(), m_worldVertices, 0, 2);
                points[0] = Point(m_worldVertices[0], m_worldVertices[1]);
                points[1] = Point(m_worldVertices[2], m_worldVertices[3]);
                points[2] = Point(m_worldVertices[4], m_worldVertices[5]);
                points[3] = Point(m_worldVertices[6], m_worldVertices[7]);
                cache->drawPoly(points, 4);
            }
        }

        if(m_debugBones) {
//            cache->drawColor(0, 40, 0, 255);
            cache->lineWidth(2);
            for(int i = 0, n = m_skeleton->getBones().size(); i < n; i++) {
                m_skeleton->updateWorldTransform();
                auto bone = m_skeleton->getBones()[i];
                if(!bone->isActive()) continue;
                float x = bone->getData().getLength() * bone->getA() + bone->getWorldX();
                float y = bone->getData().getLength() * bone->getC() + bone->getWorldY();
                Point p0(bone->getWorldX(), bone->getWorldY());
                Point p1(x, y);
                cache->drawLine(p0, p1);
            }
//            cache->drawColor(0, 0, 255, 255);
            cache->pointSize(4.0);
            for(int i = 0, n = m_skeleton->getBones().size(); i < n; i++) {
                auto bone = m_skeleton->getBones()[i];
                cache->drawPoint(Point(bone->getWorldX(), bone->getWorldY()));
                if(i == 0) cache->drawColor(0, 255, 0,255);
            }
        }
    }
}

QUrl SpineItem::atlasFile() const
{
    return m_atlasFile;
}

void SpineItem::setAtlasFile(const QUrl &atlasPath)
{
    m_atlasFile = atlasPath;
    emit atlasFileChanged(atlasPath);
    m_lazyLoadTimer->stop();
    m_lazyLoadTimer->start();
}

QUrl SpineItem::skeletonFile() const
{
    return m_skeletonFile;
}

void SpineItem::setSkeletonFile(const QUrl &skeletonPath)
{
    m_skeletonFile = skeletonPath;
    emit skeletonFileChanged(skeletonPath);
    m_lazyLoadTimer->stop();
    m_lazyLoadTimer->start();
}

void SpineItem::loadResource()
{
    AimyTextureLoader::instance()->setWindow(window());
    m_isLoading = true;
    if(m_timerId > 0) {
        killTimer(m_timerId);
        m_timerId = 0;
    }

    QtConcurrent::run([=]{
        m_loaded = false;
        m_animations.clear();
        m_skins.clear();
        m_scaleX = 1.0;
        m_scaleY = 1.0;
        emit scaleXChanged(m_scaleX);
        emit scaleYChanged(m_scaleY);
        emit animationsChanged(m_animations);
        emit skinsChanged(m_skins);
        emit loadedChanged(m_loaded);
        emit isSkeletonReadyChanged(isSkeletonReady());

        if(m_atlasFile.isEmpty() || !m_atlasFile.isValid()) {
            qWarning() << m_atlasFile << " not exists...";
            return;
        }

        if(m_skeletonFile.isEmpty() || !m_skeletonFile.isValid()) {
            qWarning() << m_skeletonFile << " not exists...";
            return;
        }

        m_atlas = QSharedPointer<spine::Atlas>(new spine::Atlas(urltospinestring(m_atlasFile),
                                                                AimyTextureLoader::instance()));

        if(m_atlas->getPages().size() == 0) {
            qWarning() << "Failed to load atlas...";
            return;
        }

        spine::SkeletonJson json(m_atlas.get());
        json.setScale(1);
        m_skeletonData.reset(json.readSkeletonDataFile(urltospinestring(m_skeletonFile)));
        if(m_skeletonData.isNull()) {
            qWarning() << json.getError().buffer();
            return;
        }

        m_skeleton.reset(new spine::Skeleton(m_skeletonData.get()));
        m_skeleton->setX(0);
        m_skeleton->setY(0);
        m_animationStateData.reset(new spine::AnimationStateData(m_skeletonData.get()));
        m_animationStateData->setDefaultMix(m_defaultMix);

        m_animationState.reset(new spine::AnimationState(m_animationStateData.get()));
        m_animationState->setRendererObject(this);
        m_loaded = true;
        m_isLoading = false;
        emit loadedChanged(m_loaded);
        emit isSkeletonReadyChanged(isSkeletonReady());
        emit resourceReady();

        auto animations = m_skeletonData->getAnimations();
        for(int i = 0; i < animations.size(); i++) {
            auto aniName = QString(animations[i]->getName().buffer());
            m_animations << aniName;
        }
        emit animationsChanged(m_animations);

        auto skins = m_skeletonData->getSkins();
        for(int i = 0; i < skins.size(); i++) {
            auto skinName = QString(skins[i]->getName().buffer());
            m_skins << skinName;
        }
//        if(m_skins.contains("default"))
//            m_skeleton->setSkin(m_skins.last().toStdString().c_str());
        emit skinsChanged(m_skins);

        m_skeleton->setScaleX(m_skeletonScale * m_scaleX);
        m_skeleton->setScaleY(m_skeletonScale * m_scaleY);
    });
}

void SpineItem::updateSkeletonAnimation()
{
    if(!isSkeletonReady())
        return;

    qint64 msecs = 0;
    if(!m_timer.isValid())
        m_timer.start();
    else
        msecs = m_timer.restart();
    const float deltaTime = msecs / 1000.0 * m_timeScale;
    m_animationState->update(deltaTime);
    m_animationState->apply(*m_skeleton.get());
    m_skeleton->updateWorldTransform();

    m_boundingRect = computeBoundingRect();
    setImplicitSize(m_boundingRect.width(), m_boundingRect.height());
    setSourceSize(QSize(m_boundingRect.width(), m_boundingRect.height()));
    update();
}

QRectF SpineItem::computeBoundingRect()
{
    if(!isSkeletonReady())
        return QRectF();
    float minX = FLT_MAX, minY = FLT_MAX, maxX = FLT_MIN, maxY = FLT_MIN;
    for(int i = 0; i < m_skeleton->getSlots().size(); i++) {
        auto slot = m_skeleton->getSlots()[i];
        if(!slot->getAttachment())
            continue;
        int verticesCount;
        auto* attachment = slot->getAttachment();
        if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
            auto* regionAttachment = static_cast<spine::RegionAttachment*>(slot->getAttachment());
            regionAttachment->computeWorldVertices(slot->getBone(), m_worldVertices, 0, 2);
            verticesCount = 8;
        } else if(attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
            auto* meshAttachment = static_cast<spine::MeshAttachment*>(slot->getAttachment());
            verticesCount = meshAttachment->getWorldVerticesLength();
            meshAttachment->computeWorldVertices(*slot, m_worldVertices);
        } else
            continue;
        for (int ii = 0; ii < verticesCount; ii+=2) {
            float x = m_worldVertices[ii], y = m_worldVertices[ii + 1];
            minX = qMin(minX, x);
            minY = qMin(minY, y);
            maxX = qMax(maxX, x);
            maxY = qMax(maxY, y);
        }
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

Texture *SpineItem::getTexture(spine::Attachment *attachment) const
{
    if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
        return (Texture*)((spine::AtlasRegion*)static_cast<spine::RegionAttachment*>(attachment)->getRendererObject())->page->getRendererObject();
    } else if(attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
        return (Texture*)((spine::AtlasRegion*)static_cast<spine::MeshAttachment*>(attachment)->getRendererObject())->page->getRendererObject();
    }
    return nullptr;
}

void SpineItem::releaseSkeletonRelatedData(){
    m_animationStateData.reset();
    m_animationState.reset();
    m_skeletonData.reset();
    m_atlas.reset();
    m_skeleton.reset();
    m_loaded = false;
    m_shouldReleaseCacheTexture = true;
}

bool SpineItem::nothingToDraw(spine::Slot &slot)
{
    auto attachMent = slot.getAttachment();
    if(!attachMent ||
        !slot.getBone().isActive() ||
        slot.getColor().a == 0)
        return true;
    if(attachMent->getRTTI().isExactly(spine::RegionAttachment::rtti)){
        if(static_cast<spine::RegionAttachment*>(attachMent)->getColor().a == 0)
            return true;
    }
    else if (attachMent->getRTTI().isExactly(spine::MeshAttachment::rtti)){
            if(static_cast<spine::MeshAttachment*>(attachMent)->getColor().a == 0)
                return true;
    }
    return false;
}

QObject *SpineItem::vertexEfect() const
{
    return (QObject*)m_vertexEfect;
}

void SpineItem::setVertexEfect(QObject *vertexEfect)
{
    m_vertexEfect = (SpineVertexEffect*)vertexEfect;
    emit vertexEfectChanged();
}

qreal SpineItem::scaleY() const
{
    return m_scaleY;
}

void SpineItem::setScaleY(const qreal &value)
{
    m_scaleY = value;
    if(isSkeletonReady())
        m_skeleton->setScaleY(m_scaleY * m_skeletonScale);
    emit scaleYChanged(m_scaleY);
}

qreal SpineItem::scaleX() const
{
    return m_scaleX;
}

void SpineItem::setScaleX(const qreal &value)
{
    m_scaleX = value;
    if(isSkeletonReady())
        m_skeleton->setScaleX(m_scaleX * m_skeletonScale);
    emit scaleXChanged(m_scaleX);
}

qreal SpineItem::defaultMix() const
{
    return m_defaultMix;
}

void SpineItem::setDefaultMix(const qreal &defaultMix)
{
    m_defaultMix = defaultMix;
    emit defaultMixChanged(m_defaultMix);
    if(isSkeletonReady()) {
        m_animationStateData->setDefaultMix(m_defaultMix);
        return;
    }
    else{
        m_lazyLoadTimer->stop();
        m_lazyLoadTimer->start();
    }
}

qreal SpineItem::timeScale() const
{
    return m_timeScale;
}

void SpineItem::setTimeScale(const qreal &timeScale)
{
    m_timeScale = timeScale;
    emit timeScaleChanged(m_timeScale);
}

int SpineItem::fps() const
{
    return m_fps;
}

void SpineItem::setFps(int fps)
{
    m_fps = fps;
    fpsChanged(fps);
    if(m_timerId > 0)
        killTimer(m_timerId);
    m_timerId = startTimer(1000 / m_fps);
}

qreal SpineItem::skeletonScale() const
{
    return m_skeletonScale;
}

void SpineItem::setSkeletonScale(const qreal &skeletonScale)
{
    if(m_isLoading)
        return;
    m_skeletonScale = skeletonScale;
    if(isSkeletonReady()) {
        m_skeleton->setScaleX(m_scaleX * m_skeletonScale);
        m_skeleton->setScaleY(m_scaleY * m_skeletonScale);
    }
    emit skeletonScaleChanged(m_skeletonScale);
//    m_lazyLoadTimer->stop();
//    m_lazyLoadTimer->start();
}

QStringList SpineItem::animations() const
{
    return m_animations;
}

QStringList SpineItem::skins() const
{
    return m_skins;
}

bool SpineItem::debugSlots() const
{
    return m_debugSlots;
}

void SpineItem::setDebugSlots(bool debugSlots)
{
    m_debugSlots = debugSlots;
    emit debugSlotsChanged(m_debugSlots);
}

bool SpineItem::debugBones() const
{
    return m_debugBones;
}

void SpineItem::setDebugBones(bool debugBones)
{
    m_debugBones = debugBones;
    emit debugBonesChanged(m_debugBones);
}

bool SpineItem::loaded() const
{
    return m_loaded;
}

QSize SpineItem::sourceSize() const
{
    return m_sourceSize;
}

void SpineItem::setSourceSize(const QSize &sourceSize)
{
    m_sourceSize = sourceSize;
    emit sourceSizeChanged(m_sourceSize);
}

void SpineItem::timerEvent(QTimerEvent *event)
{
    if(m_timerId == event->timerId())
        updateSkeletonAnimation();
}

void SpineItem::onResourceReady()
{
    m_timerId = startTimer(1);
}

bool SpineItem::isSkeletonReady() const
{
    return m_loaded && m_atlas && m_skeleton && m_animationState;
}
