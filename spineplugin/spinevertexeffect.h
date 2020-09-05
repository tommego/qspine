#ifndef SPINEVERTEXEFFECT_H
#define SPINEVERTEXEFFECT_H

#include <QObject>
#include <QSharedPointer>
#include "spine/spine.h"

class SpineVertexEffect : public QObject
{
    Q_OBJECT
public:
    explicit SpineVertexEffect(QObject *parent = nullptr);
    virtual void begin(spine::Skeleton& skeleton) = 0;
    virtual void transform(float& x, float& y, float &u, float &v, spine::Color &light, spine::Color &dark) = 0;
    virtual void end() = 0;

private:
    spine::VertexEffect* m_proxy = nullptr;

};

class SpineJitterVertexEffect: public SpineVertexEffect{
    Q_OBJECT
    Q_PROPERTY(float jitterX READ jitterX WRITE setJitterX NOTIFY jitterXChanged)
    Q_PROPERTY(float jitterY READ jitterY WRITE setJitterY NOTIFY jitterYChanged)
public:
    explicit SpineJitterVertexEffect(QObject* parent = nullptr);

    virtual void begin(spine::Skeleton& skeleton);
    virtual void transform(float& x, float& y, float &u, float &v, spine::Color &light, spine::Color &dark);
    virtual void end();

    float jitterX() const;
    void setJitterX(const float& val);
    float jitterY() const;
    void setJitterY(const float& val);

signals:
    void jitterXChanged();
    void jitterYChanged();

private:
    QSharedPointer<spine::JitterVertexEffect> m_jitterEffect;
};

class SpineInterpolation: public QObject{
    Q_OBJECT
public:
    SpineInterpolation(QObject* parent = nullptr);
    virtual float apply(float a) = 0;

    virtual float interpolate(float start, float end, float a) {
        return start + (end - start) * apply(a);
    }

    virtual spine::Interpolation* interpolation() = 0;

    virtual int pow() const {return 0;}
    virtual void setPow(int pow){Q_UNUSED(pow)}

signals:
    void powChanged();
};

class SpinePowInterpolation: public SpineInterpolation{
    Q_OBJECT
    Q_PROPERTY(int pow READ pow WRITE setPow NOTIFY powChanged)
public:
    explicit SpinePowInterpolation(QObject* parent = nullptr);
    float apply(float a);
    float interpolate(float start, float end, float a);

    spine::Interpolation* interpolation();

    int pow() const;
    void setPow(int pow);

private:
    int m_pow = 0;
    QSharedPointer<spine::PowInterpolation> m_powInterpolation;
};

class SpinePowOutInterpolation: public SpineInterpolation{
    Q_OBJECT
    Q_PROPERTY(int pow READ pow WRITE setPow NOTIFY powChanged)
public:
    explicit SpinePowOutInterpolation(QObject* parent = nullptr);
    float apply(float a);
    float interpolate(float start, float end, float a);

    spine::Interpolation* interpolation();

    int pow() const;
    void setPow(int pow);

private:
    int m_pow = 0;
    QSharedPointer<spine::PowOutInterpolation> m_powOutInterpolation;
};

class SpineSwrilVertexEffect: public SpineVertexEffect{
    Q_OBJECT
    Q_PROPERTY(QObject* interpolator READ interpolator WRITE setInterpolator NOTIFY interpolatorChanged)
    Q_PROPERTY(float centerX READ centerX WRITE setCenterX NOTIFY centerXChanged)
    Q_PROPERTY(float centerY READ centerY WRITE setCenterY NOTIFY centerYChanged)
    Q_PROPERTY(float radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(float angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(float worldX READ worldX WRITE setWorldX NOTIFY worldXChanged)
    Q_PROPERTY(float worldY READ worldY WRITE setWorldY NOTIFY worldYChanged)

public:
    SpineSwrilVertexEffect(QObject* parent = nullptr);

    virtual void begin(spine::Skeleton& skeleton);
    virtual void transform(float& x, float& y, float &u, float &v, spine::Color &light, spine::Color &dark);
    virtual void end();

    QObject* interpolator();
    void setInterpolator(QObject* val);


    float centerX() const;
    void setCenterX(float centerX);

    float centerY() const;
    void setCenterY(float centerY);

    float radius() const;
    void setRadius(float radius);

    float angle() const;
    void setAngle(float angle);

    float worldX() const;
    void setWorldX(float worldX);

    float worldY() const;
    void setWorldY(float worldY);

signals:
    void interpolatorChanged();
    void centerXChanged();
    void centerYChanged();
    void radiusChanged();
    void angleChanged();
    void worldXChanged();
    void worldYChanged();

private:
    SpineInterpolation *m_interpolator = nullptr;
    float m_centerX = 0;
    float m_centerY = 0;
    float m_radius = 0;
    float m_angle = 0;
    float m_worldX = 0;
    float m_worldY = 0;
    QSharedPointer<spine::SwirlVertexEffect> m_swrilEffect;
};

#endif // SPINEVERTEXEFFECT_H
