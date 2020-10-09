#ifndef SPINEITEM_H
#define SPINEITEM_H

#include <QQuickFramebufferObject>
#include <QElapsedTimer>
#include <QSGTexture>
#include <QFuture>

#include "rendercmdscache.h"

class SpineItemWorker;

class QTimer;

class RenderCmdsCache;
class AimyTextureLoader;
class Texture;
class SpineVertexEffect;
class SkeletonRenderer;

namespace spine {
class Atlas;
class SkeletonJson;
class SkeletonData;
class AnimationStateData;
class AnimationState;
class Skeleton;
class Attachment;
class SkeletonClipping;
class Slot;
}

struct RenderData{
    spine::Vector<SpineVertex> vertices;
    spine::Vector<GLushort> triangles;
    Texture* texture = nullptr;
    int blendMode;
};

class SpineItem : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_DISABLE_COPY(SpineItem)
    Q_PROPERTY(QUrl atlasFile READ atlasFile WRITE setAtlasFile NOTIFY atlasFileChanged)
    Q_PROPERTY(QUrl skeletonFile READ skeletonFile WRITE setSkeletonFile NOTIFY skeletonFileChanged)
    Q_PROPERTY(bool isSkeletonReady READ isSkeletonReady NOTIFY isSkeletonReadyChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize WRITE setSourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool debugBones READ debugBones WRITE setDebugBones NOTIFY debugBonesChanged)
    Q_PROPERTY(bool debugSlots READ debugSlots WRITE setDebugSlots NOTIFY debugSlotsChanged)
    Q_PROPERTY(bool debugMesh READ debugMesh WRITE setDebugMesh NOTIFY debugMeshChanged)
    Q_PROPERTY(QStringList animations READ animations NOTIFY animationsChanged)
    Q_PROPERTY(QStringList skins READ skins  NOTIFY skinsChanged)
    Q_PROPERTY(qreal skeletonScale READ skeletonScale WRITE setSkeletonScale NOTIFY skeletonScaleChanged)
    Q_PROPERTY(qreal timeScale READ timeScale WRITE setTimeScale NOTIFY timeScaleChanged)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(qreal defaultMix READ defaultMix WRITE setDefaultMix NOTIFY defaultMixChanged)
    Q_PROPERTY(qreal scaleX READ scaleX WRITE setScaleX NOTIFY scaleXChanged)
    Q_PROPERTY(qreal scaleY READ scaleY WRITE setScaleY NOTIFY scaleYChanged)
    Q_PROPERTY(QObject* vertexEfect READ vertexEfect WRITE setVertexEfect NOTIFY vertexEfectChanged)
    Q_PROPERTY(QColor blendColor READ blendColor WRITE setBlendColor NOTIFY blendColorChanged)
    Q_PROPERTY(int blendColorChannel READ blendColorChannel WRITE setBlendColorChannel NOTIFY blendColorChannelChanged) // -1=none, 0=r, 1=g, 2=b, 3=a, 4=gray
    Q_PROPERTY(float light READ light WRITE setLight NOTIFY lightChanged)

public:
    explicit SpineItem(QQuickItem *parent = nullptr);
    ~SpineItem() override;
    virtual Renderer *createRenderer() const override;

    /**
     * @brief setToSetupPose Affect after clearTracks() has been invoked.
     */
    Q_INVOKABLE void setToSetupPose();
    /**
     * @brief setBonesToSetupPose Affect after clearTracks() has been invoked.
     */
    Q_INVOKABLE void setBonesToSetupPose();
    /**
     * @brief setSlotsToSetupPose Affect after clearTracks() has been invoked.
     */
    Q_INVOKABLE void setSlotsToSetupPose();
    /**
     * @brief setAttachment Dynaimicly change attachment for a slot.
     * @param slotName
     * @param attachmentName
     * @return
     */
    Q_INVOKABLE bool setAttachment(const QString& slotName, const QString& attachmentName);
    /**
     * @brief setMix Animation mixing duration.
     * @param fromAnimation
     * @param toAnimation
     * @param duration
     */
    Q_INVOKABLE void setMix(const QString& fromAnimation, const QString& toAnimation, float duration);
    /**
     * @brief setAnimation Sets the current animation for a track, discarding any queued animations.
     * A track entry to allow further customization of animation playback. References to the track entry must not be kept after AnimationState.Dispose.
     * @param trackIndex
     * @param name
     * @param loop If true, the animation will repeat.If false, it will not, instead its last frame is applied if played beyond its duration. In either case TrackEntry.TrackEnd determines when the track is cleared.
     */
    Q_INVOKABLE void setAnimation (int trackIndex, const QString& name, bool loop);
    /**
     * @brief addAnimation Queues an animation by name.
     * @param trackIndex
     * @param name
     * @param loop
     * @param delay
     */
    Q_INVOKABLE void addAnimation (int trackIndex, const QString& name, bool loop, float delay = 0);
    /**
     * @brief setSkin Changes skin by specified skin name at anytime.
     * @param skinName
     */
    Q_INVOKABLE void setSkin(const QString& skinName);
    /**
     * @brief clearTracks Clears all animation tracks, and keeps last rendered frame pixels, and after then you can invoke setToSetupPose to rollback firstly orignal frame immediately.
     */
    Q_INVOKABLE void clearTracks ();
    /**
     * @brief clearTrack Clear specfied animation track by given track id.
     * @param trackIndex
     */
    Q_INVOKABLE void clearTrack(int trackIndex = 0);

    friend class SpineItemWorker;
    friend class SkeletonRenderer;

    QUrl atlasFile() const;
    void setAtlasFile(const QUrl &atlasPath);

    QUrl skeletonFile() const;
    void setSkeletonFile(const QUrl &skeletonPath);

    bool isSkeletonReady() const;

    QSize sourceSize() const;
    void setSourceSize(const QSize &sourceSize);

    bool loaded() const;

    bool debugBones() const;
    void setDebugBones(bool debugBones);

    bool debugSlots() const;
    void setDebugSlots(bool debugSlots);

    QStringList vailableAnimations() const;
    QStringList vailableSkins() const;

    QStringList animations() const;
    QStringList skins() const;

    qreal skeletonScale() const;
    void setSkeletonScale(const qreal &skeletonScale);

    int fps() const;
    void setFps(int fps);

    qreal timeScale() const;
    void setTimeScale(const qreal &timeScale);

    qreal defaultMix() const;
    void setDefaultMix(const qreal &defaultMix);

    qreal scaleX() const;
    void setScaleX(const qreal &value);

    qreal scaleY() const;
    void setScaleY(const qreal &value);

    QObject *vertexEfect() const;
    void setVertexEfect(QObject *vertexEfect);

    QColor blendColor() const;
    void setBlendColor(const QColor &color);

    int blendChannel() const;
    void setBlendChannel(int blendChannel);

    int blendColorChannel() const;
    void setBlendColorChannel(int blendColorChannel);

    bool debugMesh() const;
    void setDebugMesh(bool debugMesh);

    float light() const;
    void setLight(float light);

signals:

    // property signals
    void atlasFileChanged(const QUrl& path);
    void skeletonFileChanged(const QUrl& path);
    void isSkeletonReadyChanged(const bool& ready);
    void sourceSizeChanged(const QSize& size);
    void loadedChanged(const bool& loaded);
    void premultipliedAlphaChanged(const bool& ret);
    void debugBonesChanged(const bool& ret);
    void debugSlotsChanged(const bool& ret);
    void debugMeshChanged(const bool& ret);
    void animationsChanged(const QStringList& animations);
    void skinsChanged(const QStringList& skins);
    void skeletonScaleChanged(const qreal& scale);
    void fpsChanged(const int& fps);
    void timeScaleChanged(const qreal& timesCale);
    void defaultMixChanged(const qreal& defaultMix);
    void scaleXChanged(const qreal& scaleX);
    void scaleYChanged(const qreal& scaleY);
    void vertexEfectChanged();
    void animationUpdated();
    void blendColorChanged(const QColor& color);
    void resourceLoadFailed();
    void blendColorChannelChanged(const int& channel);
    void lightChanged(const float& light);

    // rumtime signals
    void animationStarted(int trackId, QString animationName);
    void animationCompleted(int trackId, QString animationName);
    void animationInterrupted(int trackId, QString animationName);
    void animationEnded(int trackId, QString animationName);
    void animationDisposed(int trackId, QString animationName);
    void cacheRendered();
    void resourceReady();

protected:
    virtual void componentComplete() override;

private slots:
    void updateBoundingRect();
    void onCacheRendered();
    void onVisibleChanged();

private:
    void loadResource();
    void updateSkeletonAnimation();
    QRectF computeBoundingRect();
    Texture* getTexture(spine::Attachment* attachment) const;
    void releaseSkeletonRelatedData();
    bool nothingToDraw(spine::Slot& slot);
    void batchRenderCmd();
    void renderToCache(QQuickFramebufferObject::Renderer* renderer);

private:
    QUrl m_atlasFile;
    QUrl m_skeletonFile;
    bool m_loaded = false;
    bool m_debugBones = false;
    bool m_debugSlots = false;
    bool m_debugMesh = false;
    bool m_isLoading = false;
    bool m_hasViewPort = false;
    qreal m_scaleX = 1.0;
    qreal m_scaleY = 1.0;
    qreal m_timeScale = 1.0;
    float m_light = 1.0;
    int m_fps = 25;
    qreal m_defaultMix = 0.1;
    QSize m_sourceSize;
    float* m_worldVertices;
    bool m_shouldReleaseCacheTexture = false;
    qreal m_skeletonScale;
    QStringList m_animations;
    QStringList m_skins;
    QRectF m_boundingRect;
    QRectF m_viewPortRect;
    QElapsedTimer m_timer;
    QSharedPointer<spine::Atlas> m_atlas;
    QSharedPointer<spine::SkeletonData> m_skeletonData;
    QSharedPointer<spine::AnimationStateData> m_animationStateData;
    QSharedPointer<spine::AnimationState> m_animationState;
    QSharedPointer<spine::Skeleton> m_skeleton;
    QSharedPointer<spine::SkeletonClipping> m_clipper;
    SpineVertexEffect* m_vertexEfect = nullptr;
    QSharedPointer<QTimer> m_lazyLoadTimer;
    QElapsedTimer m_tickCounter;
    QSharedPointer<RenderCmdsCache> m_renderCache;
    QList<RenderData> m_batches;
    QSharedPointer<SpineItemWorker> m_spWorker;
    QSharedPointer<QThread> m_spWorkerThread;
    QColor m_blendColor;
    bool m_componentCompleted = false;
    int m_blendColorChannel = -1;
    bool m_requestDestroy = false;
};

class SpineItemWorker: public QObject{
    Q_OBJECT
public:
    SpineItemWorker(QObject* parent = nullptr, SpineItem* spItem = nullptr);

public slots:
    void updateSkeletonAnimation();
    void loadResource();

private:
    SpineItem* m_spItem = nullptr;
    int m_fadecounter = 1; // make sure last state textue has been render.
};

#endif // SPINEITEM_H
