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
        if(spItem->m_requestDestroy)
            return;
        emit spItem->animationStarted(entry->getTrackIndex(), animationName);
        if(spItem->isVisible() && !spItem->m_animating) {
            spItem->m_animating = true;
            emit spItem->animationUpdated();
        }
        break;
    }
    case spine::EventType_Interrupt: {
        if(spItem->m_requestDestroy)
            return;
        emit spItem->animationInterrupted(entry->getTrackIndex(), animationName);
        break;
    }
    case spine::EventType_End: {
        if(spItem->m_requestDestroy)
            return;
        emit spItem->animationEnded(entry->getTrackIndex(), animationName);
        break;
    }
    case spine::EventType_Complete: {
        if(spItem->m_requestDestroy)
            return;
        emit spItem->animationCompleted(entry->getTrackIndex(), animationName);
        break;
    }
    case spine::EventType_Dispose: {
        if(spItem->m_requestDestroy)
            return;
        emit spItem->animationDisposed(entry->getTrackIndex(), animationName);
        break;
    }
    case spine::EventType_Event: {
        break;
    }
    }
}

SpineItem::SpineItem(QQuickItem *parent) :
    QQuickFramebufferObject(parent),
    m_skeletonScale(1.0),
    m_clipper(new spine::SkeletonClipping),
    m_lazyLoadTimer(new QTimer),
    m_renderCache(new RenderCmdsCache(this, this)),
    m_spWorker(new SpineItemWorker(nullptr, this)),
    m_spWorkerThread(new QThread)
{
    AimyTextureLoader::instance(); // make sure this has been initialized.
    m_blendColor = QColor(255, 255, 255, 255);
    m_lazyLoadTimer->setSingleShot(true);
    m_lazyLoadTimer->setInterval(50);
    m_worldVertices = new float[2000];
    connect(m_lazyLoadTimer.get(), &QTimer::timeout, this, &SpineItem::reloadResource);
    connect(this, &SpineItem::animationUpdated, this, &SpineItem::updateBoundingRect);
    connect(m_renderCache.get(), &RenderCmdsCache::cacheRendered, this, &SpineItem::onCacheRendered);
    connect(this, &SpineItem::visibleChanged, this, &SpineItem::onVisibleChanged);
    if(m_asynchronous)
        m_spWorker->moveToThread(m_spWorkerThread.get());
    m_spWorkerThread->start();
}

SpineItem::~SpineItem()
{
    disconnect(m_renderCache.get(), &RenderCmdsCache::cacheRendered, this, &SpineItem::onCacheRendered);
    disconnect(this, &SpineItem::animationUpdated, this, &SpineItem::updateBoundingRect);
    disconnect(m_lazyLoadTimer.get(), &QTimer::timeout, this, &SpineItem::reloadResource);
    disconnect(this, &SpineItem::visibleChanged, this, &SpineItem::onVisibleChanged);

    if(m_animationState)
        m_animationState->clearTracks();
    m_requestDestroy = true;
    m_spWorkerThread->quit();
    m_spWorkerThread->wait();
    m_spWorker.reset();
    releaseSkeletonRelatedData();
    delete [] m_worldVertices;
}

QQuickFramebufferObject::Renderer *SpineItem::createRenderer() const
{
    if(!window())
        return nullptr;
    auto renderer = new SkeletonRenderer();
    m_renderCache->initShaderProgram();
    renderer->setCache(m_renderCache);
    return renderer;
}

void SpineItem::setToSetupPose()
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setToSetupPose");
}

void SpineItem::setBonesToSetupPose()
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setBonesToSetupPose");
}

void SpineItem::setSlotsToSetupPose()
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setSlotsToSetupPose");
}

bool SpineItem::setAttachment(const QString &slotName, const QString &attachmentName)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setAttachment",
                                  Q_ARG(QString, slotName),
                                  Q_ARG(QString, attachmentName));
    return true;
}

void SpineItem::setMix(const QString &fromAnimation, const QString &toAnimation, float duration)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setMix",
                                  Q_ARG(QString, fromAnimation),
                                  Q_ARG(QString, toAnimation),
                                  Q_ARG(float, duration));
}

void SpineItem::setAnimation(int trackIndex, const QString &name, bool loop)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setAnimation", Q_ARG(int, trackIndex), Q_ARG(QString, name), Q_ARG(bool, loop));
}

void SpineItem::addAnimation(int trackIndex, const QString &name, bool loop, float delay)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "addAnimation", Q_ARG(int, trackIndex), Q_ARG(QString, name), Q_ARG(bool, loop), Q_ARG(float, delay));
}

void SpineItem::setSkin(const QString &skinName)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "setSkin",
                                  Q_ARG(QString, skinName));
}

void SpineItem::clearTracks()
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "clearTracks");
}

void SpineItem::clearTrack(int trackIndex)
{
    if(m_spWorkerThread->isRunning() && m_spWorker)
        QMetaObject::invokeMethod(m_spWorker.get(), "clearTrack", Q_ARG(int, trackIndex));
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
    float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;
    float vminX = FLT_MAX, vminY = FLT_MAX, vmaxX = -FLT_MAX, vmaxY = -FLT_MAX;
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
            // handle viewport mode
            if(QString(attachment->getName().buffer()).endsWith("viewport")) {
                m_hasViewPort = true;
                for (int ii = 0; ii < verticesCount; ii+=2) {
                    float x = m_worldVertices[ii], y = m_worldVertices[ii + 1];
                    vminX = qMin(vminX, x);
                    vminY = qMin(vminY, y);
                    vmaxX = qMax(vmaxX, x);
                    vmaxY = qMax(vmaxY, y);
                }
                m_viewPortRect = QRectF(vminX, vminY, vmaxX - vminX, vmaxY - vminY);
            }
        } else if(attachment->getRTTI().isExactly(spine::MeshAttachment::rtti)) {
            auto* meshAttachment = static_cast<spine::MeshAttachment*>(slot->getAttachment());
            verticesCount = meshAttachment->getWorldVerticesLength();
            meshAttachment->computeWorldVertices(*slot, m_worldVertices);
            if(QString(attachment->getName().buffer()).endsWith("viewport")) {
                m_hasViewPort = true;
                for (int ii = 0; ii < verticesCount; ii+=2) {
                    float x = m_worldVertices[ii], y = m_worldVertices[ii + 1];
                    vminX = qMin(vminX, x);
                    vminY = qMin(vminY, y);
                    vmaxX = qMax(vmaxX, x);
                    vmaxY = qMax(vmaxY, y);
                }
                m_viewPortRect = QRectF(vminX, vminY, vmaxX - vminX, vmaxY - vminY);
            }
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
    m_hasViewPort = false;
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

static unsigned short quadIndices[] = {0, 1, 2, 2, 3, 0};

void SpineItem::batchRenderCmd()
{
    if(m_requestRender)
        return;
    m_requestRender = true;
    if(!m_renderCache || !m_renderCache->isValid() || !m_componentCompleted)
        return;

    m_batches.clear();

    for(size_t i = 0, n = m_skeleton->getSlots().size(); i < n; ++i) {
        auto slot = m_skeleton->getDrawOrder()[i];

        if (nothingToDraw(*slot)) {
            m_clipper->clipEnd(*slot);
            continue;
        }

        auto* attachment = slot->getAttachment();
        if (!attachment)
            continue;

        RenderCmdBatch batch;

        auto name = QString(attachment->getName().buffer());
//        spine::Vector<SpineVertex> vertices;
//        spine::Vector<GLushort> triangles;
        Texture* texture = nullptr;
        int blendMode;
        blendMode = slot->getData().getBlendMode();


        auto skeletonColor = m_skeleton->getColor();
        auto slotColor = slot->getColor();
        spine::Color attachmentColor(0, 0, 0, 0);
        spine::Color tint(skeletonColor.r * slotColor.r,
                          skeletonColor.g * slotColor.g,
                          skeletonColor.b * slotColor.b,
                          skeletonColor.a * slotColor.a);

        // TODO: for premultiply alpha handling
//        spine::Color darkColor;

//        if(slot->hasDarkColor())
//            darkColor = slot->getDarkColor();
//        else{
//            darkColor.r = 0;
//            darkColor.g = 0;
//            darkColor.b = 0;
//        }
//        darkColor.a = 0;
        if(attachment->getRTTI().isExactly(spine::RegionAttachment::rtti)) {
            auto regionAttachment = (spine::RegionAttachment*)attachment;
            attachmentColor.set(regionAttachment->getColor());

            tint.set(tint.r * attachmentColor.r, tint.g * attachmentColor.g, tint.b * attachmentColor.b, tint.a * attachmentColor.a);
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
            attachmentColor.set(mesh->getColor());
            tint.set(tint.r * attachmentColor.r, tint.g * attachmentColor.g, tint.b * attachmentColor.b, tint.a * attachmentColor.a);
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
            batch.blendMode = blendMode;
            m_batches.push_back(batch);
            m_clipper->clipEnd(*slot);
        }
    }
    m_clipper->clipEnd();
}

void SpineItem::renderToCache(QQuickFramebufferObject::Renderer *renderer)
{
    if(!renderer)
        return;

    if(!m_renderCache || !m_renderCache->isValid() || !m_componentCompleted)
        return;

    if(m_hasViewPort)
        m_renderCache->setSkeletonRect(m_viewPortRect);
    else
        m_renderCache->setSkeletonRect(m_boundingRect);

    m_renderCache->bindShader(RenderCmdsCache::ShaderTexture);
    bool hasBlend = false;
    foreach (const auto& batch, m_batches) {
        if(!batch.texture)
            continue;
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
                m_renderCache->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;
            }
            default:{
                m_renderCache->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                break;
            }
            }
        }

        m_renderCache->drawTriangles(
                    AimyTextureLoader::instance()->getGLTexture(batch.texture,window()),
                    batch.vertices,
                    batch.triangles,
                    m_blendColor,
                    m_blendColorChannel,
                    m_light);
        hasBlend = true;

        // debug drawing
        if(m_debugBones || m_debugSlots || m_debugMesh) {
            m_renderCache->bindShader(RenderCmdsCache::ShaderColor);
            m_renderCache->blendFunc(GL_ONE, GL_ZERO);

            if(m_debugSlots) {
                m_renderCache->drawColor(200, 40, 150, 255);
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

            if(m_debugMesh) {
                spine::Vector<Point> points;
                m_renderCache->drawColor(40, 150, 200, 255);
                for (size_t i = 0, n = m_skeleton->getSlots().size(); i < n; i++) {
                    auto slot = m_skeleton->getSlots()[i];
                    if(!slot->getAttachment() || !slot->getAttachment()->getRTTI().isExactly(spine::MeshAttachment::rtti))
                        continue;
                    auto* mesh = (spine::MeshAttachment*)slot->getAttachment();
                    size_t numVertices = mesh->getWorldVerticesLength() / 2;
                    points.setSize(numVertices, Point());
                    mesh->computeWorldVertices(*slot,
                                               0,
                                               mesh->getWorldVerticesLength(),
                                               (float*)points.buffer(),
                                               0,
                                               sizeof (Point) / sizeof (float));
                    m_renderCache->drawPoly(points.buffer(), points.size());
                }
            }

            if(m_debugBones) {
                m_renderCache->drawColor(200, 150, 40, 255);
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
    m_requestRender = false;
}

bool SpineItem::forceRenderOnHidden() const
{
    return m_forceRenderOnHidden;
}

void SpineItem::setForceRenderOnHidden(bool forceRenderOnHidden)
{
    m_forceRenderOnHidden = forceRenderOnHidden;
    emit forceRenderOnHiddenChanged(m_forceRenderOnHidden);
}

void SpineItem::classBegin()
{
}

void SpineItem::componentComplete()
{
    QQuickFramebufferObject::componentComplete();
    m_componentCompleted = true;
}

bool SpineItem::asynchronous() const
{
    return m_asynchronous;
}

void SpineItem::setAsynchronous(bool asynchronous)
{
    if(m_asynchronous == asynchronous)
        return;

    m_asynchronous = asynchronous;
    if(m_asynchronous) {
        m_spWorker->moveToThread(m_spWorkerThread.get());
    } else{
        m_spWorker->moveToThread(qApp->thread());
    }
    emit asynchronousChanged(m_asynchronous);
}

float SpineItem::light() const
{
    return m_light;
}

void SpineItem::setLight(float light)
{
    m_light = light;
    emit lightChanged(m_light);
}

bool SpineItem::debugMesh() const
{
    return m_debugMesh;
}

void SpineItem::setDebugMesh(bool debugMesh)
{
    m_debugMesh = debugMesh;
    emit debugMeshChanged(m_debugMesh);
}

int SpineItem::blendColorChannel() const
{
    return m_blendColorChannel;
}

void SpineItem::setBlendColorChannel(int blendColorChannel)
{
    m_blendColorChannel = blendColorChannel;
    emit blendColorChannelChanged(m_blendColorChannel);
}

QColor SpineItem::blendColor() const
{
    return m_blendColor;
}

void SpineItem::setBlendColor(const QColor &color)
{
    m_blendColor = color;
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
    if(m_requestDestroy || m_isLoading)
        return;
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
    if(m_requestDestroy || m_isLoading)
        return;
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
    if(m_requestDestroy || m_isLoading)
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

void SpineItem::updateBoundingRect()
{
    if(m_requestDestroy)
        return;
    setSourceSize(QSize(m_boundingRect.width(), m_boundingRect.height()));
    if(m_hasViewPort)
        setImplicitSize(m_viewPortRect.width(), m_viewPortRect.height());
    else
        setImplicitSize(m_boundingRect.width(), m_boundingRect.height());
    update();
}

void SpineItem::onCacheRendered()
{
    if(m_requestDestroy)
        return;
    updateSkeletonAnimation();
}

void SpineItem::onVisibleChanged()
{
    if(isSkeletonReady() && isVisible() && !m_forceRenderOnHidden)
        emit animationUpdated();
}

void SpineItem::reloadResource()
{
    releaseSkeletonRelatedData();
    loadResource();
}

bool SpineItem::isSkeletonReady() const
{
    return m_loaded && m_atlas && m_skeleton && m_animationState && m_spWorkerThread->isRunning() && !m_requestDestroy;
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
        if(sleepCount > 0 && sleepCount < 100)
            QThread::msleep(sleepCount);
    }
    m_spItem->m_tickCounter.restart();

    if(m_spItem->m_animationState->getTracks().size() <= 0 || (!m_spItem->isVisible() && !m_spItem->m_forceRenderOnHidden)) {
        if(m_fadecounter > 0)
            m_fadecounter--;
        else
            return;
    }
    else
        m_fadecounter = 1;

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
    if(!m_spItem->isComponentComplete()) {
        qWarning() << "Spine component is on created completly on qml, you should wait after component has been completed to load the spine resource...";
        m_neadReloadResource = true;
        m_reloadTimerID = startTimer(100);
        return;
    }
    m_spItem->releaseSkeletonRelatedData();
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
        qWarning() << "Failed to load atlas..." << QString(urltospinestring(m_spItem->m_atlasFile).buffer());
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
    m_spItem->m_animationState->setRendererObject(m_spItem);
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

    m_spItem->m_skeleton->setScaleX(m_spItem->m_skeletonScale * m_spItem->m_scaleX);
    m_spItem->m_skeleton->setScaleY(m_spItem->m_skeletonScale * m_spItem->m_scaleY);

    emit m_spItem->skinsChanged(m_spItem->m_skins);
    emit m_spItem->loadedChanged(m_spItem->m_loaded);
    emit m_spItem->isSkeletonReadyChanged(m_spItem->isSkeletonReady());

    m_spItem->m_skeleton->updateWorldTransform();
    m_spItem->m_boundingRect = m_spItem->computeBoundingRect();
    m_spItem->batchRenderCmd();
    QThread::msleep(1);
    emit m_spItem->animationUpdated();
    if(m_spItem->m_requestDestroy)
        return;
    emit m_spItem->resourceReady();
}

void SpineItemWorker::setAnimation(int trackIndex, const QString &name, bool loop)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setAnimation Error: Skeleton is not ready";
        return;
    }
    if(!m_spItem->m_animations.contains(name)) {
        qWarning() << "no " << name << " found, vailable is : " << m_spItem->m_animations;
        return;
    }
    m_spItem->m_animationState->setAnimation(size_t(trackIndex), qstringtospinestring(name), loop);
    m_spItem->m_timer.restart();
}

void SpineItemWorker::addAnimation(int trackIndex, const QString &name, bool loop, float delay)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::addAnimation Error: Skeleton is not ready";
        return;
    }
    if(!m_spItem->m_animations.contains(name)) {
        qWarning() << "no " << name << " found, vailable is : " << m_spItem->m_animations;
        return;
    }
    m_spItem->m_animationState->addAnimation(size_t(trackIndex), qstringtospinestring(name), loop, delay);
    m_spItem->m_timer.restart();
}

void SpineItemWorker::setToSetupPose()
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_skeleton->setToSetupPose();
    m_spItem->m_animationState->apply(*m_spItem->m_skeleton.get());
    m_spItem->m_skeleton->updateWorldTransform();
}

void SpineItemWorker::setBonesToSetupPose()
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setBonesToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_skeleton->setBonesToSetupPose();
}

void SpineItemWorker::setSlotsToSetupPose()
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setSlotsToSetupPose Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_skeleton->setSlotsToSetupPose();
}

void SpineItemWorker::setAttachment(const QString &slotName, const QString &attachmentName)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setAttachment Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_skeleton->setAttachment(spine::String(slotName.toStdString().data()), spine::String(attachmentName.toStdString().data()));
}

void SpineItemWorker::setMix(const QString &fromAnimation, const QString &toAnimation, float duration)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setMix Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_animationStateData->setMix(qstringtospinestring(fromAnimation),
                                 qstringtospinestring(toAnimation),
                                 duration);
}

void SpineItemWorker::setSkin(const QString &skinName)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::setSkin Error: Skeleton is not ready";
        return;
    }
    qInfo() << "setting skin " << skinName;
    if(!m_spItem->m_skins.contains(skinName)) {
        qWarning() <<  "no " << skinName << " found, vailable skins is: " << m_spItem->m_skins;
    }
    m_spItem->m_skeleton->setSkin(skinName.toStdString().c_str());
}

void SpineItemWorker::clearTracks()
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::clearTracks Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_animationState->clearTracks();
    m_spItem->m_animating = false;
}

void SpineItemWorker::clearTrack(int trackIndex)
{
    if(!m_spItem->isSkeletonReady()) {
        qWarning() << "SpineItem::clearTrack Error: Skeleton is not ready";
        return;
    }
    m_spItem->m_animationState->clearTrack(size_t(trackIndex));
    if(m_spItem->m_animationState->getTracks().size() <= 0)
        m_spItem->m_animating = false;
}

void SpineItemWorker::timerEvent(QTimerEvent *event)
{
    qDebug() << "SpineItemWorker::timerEvent(QTimerEvent *event) " << m_reloadTimerID << event->timerId() << m_neadReloadResource;
    if(m_reloadTimerID == event->timerId() && m_neadReloadResource){
        m_neadReloadResource = false;
        loadResource();
        killTimer(m_reloadTimerID);
        m_reloadTimerID = 0;
    }
}
