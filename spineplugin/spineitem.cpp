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

void animationSateListioner(spine::AnimationState* state, spine::EventType type, spine::TrackEntry* entry, spine::Event* event) {
    auto spItem = static_cast<SpineItem*>(state->getRendererObject());
    if(!spItem)
        return;
    QString animationName = QString(entry->getAnimation()->getName().buffer());
    switch (type) {
    case spine::EventType_Start:{
        emit spItem->animationStarted();
        break;
    }
    case spine::EventType_Interrupt: {
        emit spItem->animationInterrupted();
        break;
    }
    case spine::EventType_End: {
        emit spItem->animationEnded();
        break;
    }
    case spine::EventType_Complete: {
        emit spItem->animationCompleted();
        break;
    }
    case spine::EventType_Dispose: {
        emit spItem->animationDisposed();
        break;
    }
    default:{
        qWarning() << "unknown event type " << type;
        break;
    }
    }
}

SpineItem::SpineItem(QQuickItem *parent) :
    QQuickFramebufferObject(parent),
    m_skeletonScale(1.0),
    m_clipper(new spine::SkeletonClipping),
    m_lazyLoadTimer(new QTimer),
    m_renderCache(new RenderCmdsCache(this)),
    m_spWorker(new SpineItemWorker(nullptr, this)),
    m_spWorkerThread(new QThread)
{
    m_blendColor = QColor(255, 255, 255, 255);
    AimyTextureLoader::instance()->setWindow(window());
    m_lazyLoadTimer->setSingleShot(true);
    m_lazyLoadTimer->setInterval(50);
    m_worldVertices = new float[2000];
    connect(this, &SpineItem::resourceReady, this, &SpineItem::onAnythingReady);
    connect(m_lazyLoadTimer.get(), &QTimer::timeout, [=]{releaseSkeletonRelatedData();loadResource();});
    connect(this, &SpineItem::animationUpdated, this, &SpineItem::updateBoundingRect);
    connect(this, &SpineItem::cacheRendered, this, &SpineItem::onCacheRendered);
    m_spWorker->moveToThread(m_spWorkerThread.get());
    m_spWorkerThread->start();
}

SpineItem::~SpineItem()
{
    m_spWorkerThread->quit();
    m_spWorkerThread->wait();
    m_spWorker.reset();
    releaseSkeletonRelatedData();
    delete [] m_worldVertices;
}

QQuickFramebufferObject::Renderer *SpineItem::createRenderer() const
{
    auto renderer = new SkeletonRenderer();
    m_renderCache->initShaderProgram();
    renderer->setCache(m_renderCache);
    return renderer;
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

static unsigned short quadIndices[] = {0, 1, 2, 2, 3, 0};

void SpineItem::renderToCache(QQuickFramebufferObject::Renderer *renderer)
{
    Q_UNUSED(renderer)
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
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "loadResource");
}

void SpineItem::updateSkeletonAnimation()
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "updateSkeletonAnimation");
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

void SpineItem::batchRenderCmd()
{
    if(!m_renderCache || !m_renderCache->isValid())
        return;

    // batching
    m_renderCache->clearCache();
    m_renderCache->setSkeletonRect(m_boundingRect);
    m_batches.clear();

    m_renderCache->bindShader(RenderCmdsCache::ShaderTexture);

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
        RenderData batch;
        batch.blendMode = slot->getData().getBlendMode();

        auto skeletonColor = m_skeleton->getColor();
        auto slotColor = slot->getColor();
        spine::Color tint(skeletonColor.r * slotColor.r,
                          skeletonColor.g * slotColor.g,
                          skeletonColor.b * slotColor.b,
                          skeletonColor.a * slotColor.a);

        Texture* texture = nullptr;
        if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
            auto regionAttachment = (spine::RegionAttachment*)attachment;
            texture = getTexture(regionAttachment);
            batch.vertices.setSize(4, SpineVertex());
            regionAttachment->computeWorldVertices(slot->getBone(),
                                                   (float*)batch.vertices.buffer(),
                                                   0,
                                                   sizeof (SpineVertex) / sizeof (float));
            for(size_t j = 0, l = 0; j < 4; j++,l+=2) {
                auto &vertex = batch.vertices[j];
                vertex.color.set(tint);
                vertex.u = regionAttachment->getUVs()[l];
                vertex.v = regionAttachment->getUVs()[l + 1];
            }
            batch.triangles.setSize(6, 0);
            memcpy(batch.triangles.buffer(), quadIndices, 6 * sizeof (GLushort));
        } else if (attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
            auto mesh = (spine::MeshAttachment*)attachment;
            size_t numVertices = mesh->getWorldVerticesLength() / 2;
            batch.vertices.setSize(numVertices, SpineVertex());
            texture = getTexture(mesh);
            mesh->computeWorldVertices(*slot,
                                       0,
                                       mesh->getWorldVerticesLength(),
                                       (float*)batch.vertices.buffer(),
                                       0,
                                       sizeof (SpineVertex) / sizeof (float));
            for (size_t j = 0, l = 0; j < numVertices; j++, l+=2) {
                auto& vertex = batch.vertices[j];
                vertex.color.set(tint);
                vertex.u = mesh->getUVs()[l];
                vertex.v = mesh->getUVs()[l+1];
            }
            batch.triangles.setSize(mesh->getTriangles().size(), 0);
            memcpy(batch.triangles.buffer(), mesh->getTriangles().buffer(), mesh->getTriangles().size() * sizeof (GLushort));

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

                auto tmpVerticesCount = batch.vertices.size() * 2;
                spine::Vector<float> tmpVertices;
                spine::Vector<float> tmpUvs;
                tmpVertices.setSize(tmpVerticesCount, 0);
                tmpUvs.setSize(tmpVerticesCount, 0);

                for(size_t i = 0; i < batch.vertices.size(); i++) {
                    tmpVertices[i * 2] = batch.vertices[i].x;
                    tmpVertices[i * 2 + 1] = batch.vertices[i].y;
                    tmpUvs[i * 2] = batch.vertices[i].u;
                    tmpUvs[i * 2 + 1] = batch.vertices[i].v;
                }
                m_clipper->clipTriangles(tmpVertices.buffer(), batch.triangles.buffer(), batch.triangles.size(), tmpUvs.buffer(), sizeof (short));
                tmpVertices.setSize(0, 0);
                tmpUvs.setSize(0, 0);

                auto vertCount = m_clipper->getClippedVertices().size() / 2;

                if(m_clipper->getClippedTriangles().size() == 0) {
                    m_clipper->clipEnd(*slot);
                    continue;
                }
                batch.triangles.setSize(m_clipper->getClippedTriangles().size(), 0);
                memcpy(batch.triangles.buffer(), m_clipper->getClippedTriangles().buffer(), batch.triangles.size() * sizeof (unsigned short));
                auto newUvs = m_clipper->getClippedUVs();
                auto newVertices = m_clipper->getClippedVertices();
                batch.vertices.setSize(vertCount, SpineVertex());
                for(size_t i = 0; i < vertCount; i++) {
                    batch.vertices[i].x = newVertices[i * 2];
                    batch.vertices[i].y = newVertices[i * 2 + 1];
                    batch.vertices[i].u = newUvs[i * 2];
                    batch.vertices[i].v = newUvs[i * 2 + 1];
                    batch.vertices[i].color.set(tint);
                }
            }
            if(batch.triangles.size() == 0)
                m_clipper->clipEnd(*slot);
            batch.texture = texture;
            m_clipper->clipEnd(*slot);

            if(batch.triangles.size() == 0) {
                hasBlend = false;
                continue;
            }
            if(hasBlend) {
                switch (batch.blendMode) {
                case spine::BlendMode_Additive: {
                    m_renderCache->blendFunc(GL_ONE, GL_ONE);
                    break;
                }
                case spine::BlendMode_Multiply: {
                    m_renderCache->blendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
                    break;
                }
                case spine::BlendMode_Screen: {
                    m_renderCache->blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
                    break;
                }
                default:{
                    m_renderCache->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                    break;
                }
                }
            }
            m_batches << batch;
            m_renderCache->drawTriangles(
                        AimyTextureLoader::instance()->getGLTexture(batch.texture,window()),
                        batch.vertices,
                        batch.triangles,
                        m_blendColor);
            hasBlend = true;
            m_clipper->clipEnd(*slot);
        }
    }
    m_clipper->clipEnd();

    // debug drawing
    if(m_debugBones || m_debugSlots) {
        m_renderCache->bindShader(RenderCmdsCache::ShaderColor);
        m_renderCache->blendFunc(GL_ONE, GL_ONE);

        if(m_debugSlots) {
            m_renderCache->drawColor(0, 100, 0, 255);
            m_renderCache->lineWidth(1);
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
                m_renderCache->drawPoly(points, 4);
            }
        }

        if(m_debugBones) {
//            cache->drawColor(0, 40, 0, 255);
            m_renderCache->lineWidth(2);
            for(int i = 0, n = m_skeleton->getBones().size(); i < n; i++) {
                m_skeleton->updateWorldTransform();
                auto bone = m_skeleton->getBones()[i];
                if(!bone->isActive()) continue;
                float x = bone->getData().getLength() * bone->getA() + bone->getWorldX();
                float y = bone->getData().getLength() * bone->getC() + bone->getWorldY();
                Point p0(bone->getWorldX(), bone->getWorldY());
                Point p1(x, y);
                m_renderCache->drawLine(p0, p1);
            }
//            cache->drawColor(0, 0, 255, 255);
            m_renderCache->pointSize(4.0);
            for(int i = 0, n = m_skeleton->getBones().size(); i < n; i++) {
                auto bone = m_skeleton->getBones()[i];
                m_renderCache->drawPoint(Point(bone->getWorldX(), bone->getWorldY()));
                if(i == 0) m_renderCache->drawColor(0, 255, 0,255);
            }
        }
    }
}

QColor SpineItem::blendColor() const
{
    return m_blendColor;
}

void SpineItem::setBlendColor(const QColor &color)
{
    m_blendColor = color;
    qDebug() << "SpineItem::setBlendColor" << color;
    emit blendColorChanged(color);
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

void SpineItem::onAnythingReady()
{
    updateSkeletonAnimation();
}

void SpineItem::updateBoundingRect()
{
    setImplicitSize(m_boundingRect.width(), m_boundingRect.height());
    setSourceSize(QSize(m_boundingRect.width(), m_boundingRect.height()));
    update();
}

void SpineItem::onCacheRendered()
{
    updateSkeletonAnimation();
}

bool SpineItem::isSkeletonReady() const
{
    return m_loaded && m_atlas && m_skeleton && m_animationState && m_spWorkerThread->isRunning();
}

SpineItemWorker::SpineItemWorker(QObject *parent, SpineItem *spItem) :
    QObject(parent),
    m_spItem(spItem)
{

}

void SpineItemWorker::updateSkeletonAnimation()
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::updateSkeletonAnimation(): skeleton is not ready";
        return;
    }

    if(!m_spItem->m_tickCounter.isValid())
        m_spItem->m_tickCounter.restart();
    else{
        auto sleepCount = 1000.0 / m_spItem->m_fps - m_spItem->m_tickCounter.elapsed();
        if(sleepCount > 0)
            QThread::msleep(sleepCount);
    }
    m_spItem->m_tickCounter.restart();

    qint64 msecs = 0;
    if(!m_spItem->m_timer.isValid())
        m_spItem->m_timer.start();
    else
        msecs = m_spItem->m_timer.restart();
    const float deltaTime = msecs / 1000.0 * m_spItem->m_timeScale;
    m_spItem->m_animationState->update(deltaTime);
    m_spItem->m_animationState->apply(*m_spItem->m_skeleton.get());
    m_spItem->m_skeleton->updateWorldTransform();

    m_spItem->m_boundingRect = m_spItem->computeBoundingRect();
    m_spItem->batchRenderCmd();
    emit m_spItem->animationUpdated();
}

void SpineItemWorker::loadResource()
{
    AimyTextureLoader::instance()->setWindow(m_spItem->window());
    m_spItem->m_isLoading = true;

    m_spItem->m_loaded = false;
    m_spItem->m_animations.clear();
    m_spItem->m_skins.clear();
    m_spItem->m_scaleX = 1.0;
    m_spItem->m_scaleY = 1.0;
    emit m_spItem->scaleXChanged(m_spItem->m_scaleX);
    emit m_spItem->scaleYChanged(m_spItem->m_scaleY);
    emit m_spItem->loadedChanged(m_spItem->m_loaded);
    emit m_spItem->isSkeletonReadyChanged(m_spItem->isSkeletonReady());

    if(m_spItem->m_atlasFile.isEmpty() || !m_spItem->m_atlasFile.isValid()) {
        qWarning() << m_spItem->m_atlasFile << " not exists...";
        emit m_spItem->resourceLoadFailed();
        return;
    }

    if(m_spItem->m_skeletonFile.isEmpty() || !m_spItem->m_skeletonFile.isValid()) {
        qWarning() << m_spItem->m_skeletonFile << " not exists...";
        emit m_spItem->resourceLoadFailed();
        return;
    }

    m_spItem->m_atlas.reset(new spine::Atlas(urltospinestring(m_spItem->m_atlasFile),
                                   AimyTextureLoader::instance()));

    if(m_spItem->m_atlas->getPages().size() == 0) {
        qWarning() << "Failed to load atlas...";
        emit m_spItem->resourceLoadFailed();
        return;
    }

    spine::SkeletonJson json(m_spItem->m_atlas.get());
    json.setScale(1);
    m_spItem->m_skeletonData.reset(json.readSkeletonDataFile(urltospinestring(m_spItem->m_skeletonFile)));
    if(m_spItem->m_skeletonData.isNull()) {
        qWarning() << json.getError().buffer();
        emit m_spItem->resourceLoadFailed();
        return;
    }

    m_spItem->m_skeleton.reset(new spine::Skeleton(m_spItem->m_skeletonData.get()));
    m_spItem->m_skeleton->setX(0);
    m_spItem->m_skeleton->setY(0);
    m_spItem->m_animationStateData.reset(new spine::AnimationStateData(m_spItem->m_skeletonData.get()));
    m_spItem->m_animationStateData->setDefaultMix(m_spItem->m_defaultMix);

    m_spItem->m_animationState.reset(new spine::AnimationState(m_spItem->m_animationStateData.get()));
    m_spItem->m_animationState->setRendererObject(this);
    m_spItem->m_animationState->setListener(animationSateListioner);
    m_spItem->m_loaded = true;
    m_spItem->m_isLoading = false;

    auto animations = m_spItem->m_skeletonData->getAnimations();
    for(int i = 0; i < animations.size(); i++) {
        auto aniName = QString(animations[i]->getName().buffer());
        m_spItem->m_animations << aniName;
    }
    emit m_spItem->animationsChanged(m_spItem->m_animations);

    auto skins = m_spItem->m_skeletonData->getSkins();
    for(int i = 0; i < skins.size(); i++) {
        auto skinName = QString(skins[i]->getName().buffer());
        m_spItem->m_skins << skinName;
    }
//        if(m_skins.contains("default"))
//            m_skeleton->setSkin(m_skins.last().toStdString().c_str());

    m_spItem->m_skeleton->setScaleX(m_spItem->m_skeletonScale * m_spItem->m_scaleX);
    m_spItem->m_skeleton->setScaleY(m_spItem->m_skeletonScale * m_spItem->m_scaleY);

    m_spItem->m_boundingRect = m_spItem->computeBoundingRect();
    emit m_spItem->skinsChanged(m_spItem->m_skins);
    emit m_spItem->loadedChanged(m_spItem->m_loaded);
    emit m_spItem->isSkeletonReadyChanged(m_spItem->isSkeletonReady());
    emit m_spItem->resourceReady();
}
