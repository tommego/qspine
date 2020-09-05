#include "spinevertexeffect.h"

SpineVertexEffect::SpineVertexEffect(QObject *parent) : QObject(parent)
{

}

SpineJitterVertexEffect::SpineJitterVertexEffect(QObject *parent):
    SpineVertexEffect(parent),
    m_jitterEffect(new spine::JitterVertexEffect(0, 0))
{

}

void SpineJitterVertexEffect::begin(spine::Skeleton &skeleton)
{
    m_jitterEffect->begin(skeleton);
}

void SpineJitterVertexEffect::transform(float &x, float &y, float &u, float &v, spine::Color &light, spine::Color &dark)
{
    m_jitterEffect->transform(x, y, u, v, light, dark);
}

void SpineJitterVertexEffect::end()
{
    m_jitterEffect->end();
}

float SpineJitterVertexEffect::jitterX() const
{
    return m_jitterEffect->getJitterX();
}

void SpineJitterVertexEffect::setJitterX(const float &val)
{
    m_jitterEffect->setJitterX(val);
    emit jitterXChanged();
}

float SpineJitterVertexEffect::jitterY() const
{
    return m_jitterEffect->getJitterY();
}

void SpineJitterVertexEffect::setJitterY(const float &val)
{
    m_jitterEffect->setJitterY(val);
    emit jitterYChanged();
}

SpineInterpolation::SpineInterpolation(QObject *parent): QObject(parent)
{

}

SpinePowInterpolation::SpinePowInterpolation(QObject *parent):
    SpineInterpolation(parent),
    m_powInterpolation(new spine::PowInterpolation(0))
{

}

float SpinePowInterpolation::apply(float a)
{
    return m_powInterpolation->apply(a);
}

float SpinePowInterpolation::interpolate(float start, float end, float a)
{
    return m_powInterpolation->interpolate(start, end, a);
}

spine::Interpolation *SpinePowInterpolation::interpolation()
{
    return m_powInterpolation.get();
}

int SpinePowInterpolation::pow() const
{
    return m_pow;
}

void SpinePowInterpolation::setPow(int pow)
{
    m_pow = pow;
    m_powInterpolation.reset(new spine::PowInterpolation(m_pow));
    emit powChanged();
}

SpinePowOutInterpolation::SpinePowOutInterpolation(QObject *parent):
    SpineInterpolation(parent),
    m_powOutInterpolation(new spine::PowOutInterpolation(0))
{

}

float SpinePowOutInterpolation::apply(float a)
{
    return m_powOutInterpolation->apply(a);
}

float SpinePowOutInterpolation::interpolate(float start, float end, float a)
{
    return m_powOutInterpolation->interpolate(start, end, a);
}

spine::Interpolation *SpinePowOutInterpolation::interpolation()
{
    return m_powOutInterpolation.get();
}

int SpinePowOutInterpolation::pow() const
{
    return m_pow;
}

void SpinePowOutInterpolation::setPow(int pow)
{
    m_pow = pow;
    m_powOutInterpolation.reset(new spine::PowOutInterpolation(m_pow));
    emit powChanged();
}

SpineSwrilVertexEffect::SpineSwrilVertexEffect(QObject *parent):
    SpineVertexEffect(parent)
{
    spine::PowInterpolation pow2(2);
    m_swrilEffect.reset(new spine::SwirlVertexEffect(0.0, pow2));
}

void SpineSwrilVertexEffect::begin(spine::Skeleton &skeleton)
{
    m_swrilEffect->begin(skeleton);
}

void SpineSwrilVertexEffect::transform(float &x, float &y, float &u, float &v, spine::Color &light, spine::Color &dark)
{
    m_swrilEffect->transform(x, y, y, v, light, dark);
}

void SpineSwrilVertexEffect::end()
{
    m_swrilEffect->end();
}

QObject *SpineSwrilVertexEffect::interpolator()
{
    return m_interpolator;
}

void SpineSwrilVertexEffect::setInterpolator(QObject *val)
{
    m_interpolator = (SpineInterpolation*)val;
    m_swrilEffect.reset(new spine::SwirlVertexEffect(m_radius, *m_interpolator->interpolation()));
}

float SpineSwrilVertexEffect::centerX() const
{
    return m_centerX;
}

void SpineSwrilVertexEffect::setCenterX(float centerX)
{
    m_centerX = centerX;
    m_swrilEffect->setCenterX(m_centerX);
    emit centerXChanged();
}

float SpineSwrilVertexEffect::centerY() const
{
    return m_centerY;
}

void SpineSwrilVertexEffect::setCenterY(float centerY)
{
    m_centerY = centerY;
    m_swrilEffect->setCenterY(m_centerY);
    emit centerYChanged();
}

float SpineSwrilVertexEffect::radius() const
{
    return m_radius;
}

void SpineSwrilVertexEffect::setRadius(float radius)
{
    m_radius = radius;
    m_swrilEffect->setRadius(m_radius);
    emit radiusChanged();
}

float SpineSwrilVertexEffect::angle() const
{
    return m_angle;
}

void SpineSwrilVertexEffect::setAngle(float angle)
{
    m_angle = angle;
    m_swrilEffect->setAngle(m_angle);
    emit angleChanged();
}

float SpineSwrilVertexEffect::worldX() const
{
    return m_worldX;
}

void SpineSwrilVertexEffect::setWorldX(float worldX)
{
    m_worldX = worldX;
    m_swrilEffect->setWorldX(m_worldX);
    emit worldXChanged();
}

float SpineSwrilVertexEffect::worldY() const
{
    return m_worldY;
}

void SpineSwrilVertexEffect::setWorldY(float worldY)
{
    m_worldY = worldY;
    m_swrilEffect->setWorldY(m_worldY);
    emit worldYChanged();
}
