/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.1
 *
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to install, execute and perform the Spine Runtimes
 * Software (the "Software") solely for internal use. Without the written
 * permission of Esoteric Software (typically granted by licensing Spine), you
 * may not (a) modify, translate, adapt or otherwise create derivative works,
 * improvements of the Software or develop new applications using the Software
 * or (b) remove, delete, alter or obscure any trademarks or any copyright,
 * trademark, patent or other intellectual property or proprietary rights
 * notices on or in the Software, including any copy thereof. Redistributions
 * in binary or source form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include "rendercmdscache.h"
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QSGTexture>
#include <QOpenGLShaderProgram>

void ICachedGLFunctionCall::release()
{
    delete this;
}

QOpenGLFunctions *ICachedGLFunctionCall::glFuncs()
{
    return QOpenGLContext::currentContext()->functions();
}

class BlendFunction: public ICachedGLFunctionCall
{
public:
    explicit BlendFunction(GLenum sfactor, GLenum dfactor):msfactor(sfactor), mdfactor(dfactor){}
    virtual ~BlendFunction(){}

    virtual void invoke(){
        if (msfactor == GL_ONE && mdfactor == GL_ZERO){
            glFuncs()->glDisable(GL_BLEND);
        }
        else
        {
            glFuncs()->glEnable(GL_BLEND);
            glFuncs()->glBlendFunc(msfactor, mdfactor);
        }
    }

private:
    GLenum msfactor;
    GLenum mdfactor;
};

class BindShader: public ICachedGLFunctionCall
{
public:
    explicit BindShader(QOpenGLShaderProgram* program, const QRectF& rect):mShaderProgram(program), mRect(rect){}
    virtual ~BindShader(){}

    virtual void invoke(){
        QMatrix4x4 matrix;
        matrix.ortho(mRect);

        mShaderProgram->bind();
        mShaderProgram->setUniformValue("u_matrix", matrix);

        if (mShaderProgram->attributeLocation("a_position") != -1)
            mShaderProgram->enableAttributeArray("a_position");

        if (mShaderProgram->attributeLocation("a_color") != -1)
            mShaderProgram->enableAttributeArray("a_color");

        if (mShaderProgram->attributeLocation("a_texCoord") != -1)
            mShaderProgram->enableAttributeArray("a_texCoord");
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    QRectF  mRect;
};

class DrawColor: public ICachedGLFunctionCall
{
public:
    explicit DrawColor(QOpenGLShaderProgram* program, const QColor& color):mShaderProgram(program), mColor(color){}
    virtual ~DrawColor(){}

    virtual void invoke(){
        if (mShaderProgram->uniformLocation("u_color") != -1)
            mShaderProgram->setUniformValue("u_color", mColor);
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    QColor mColor;
};

class LineWidth: public ICachedGLFunctionCall
{
public:
    explicit LineWidth(GLfloat width):mWidth(width){}
    virtual ~LineWidth(){}

    virtual void invoke(){
        glFuncs()->glLineWidth(mWidth);
    }

private:
    GLfloat mWidth;
};

class PointSize: public ICachedGLFunctionCall
{
public:
    explicit PointSize(GLfloat size):m_size(size){}
    virtual ~PointSize(){}

    virtual void invoke(){
#if defined(Q_OS_OSX)
        glPointSize(m_size);
#elif defined(Q_OS_WIN) && defined(Q_CC_MINGW)
        glPointSize(m_size);
#endif
    }

private:
    GLfloat m_size;
};

class DrawTrigngles: public ICachedGLFunctionCall
{
public:
    explicit DrawTrigngles(QOpenGLShaderProgram* program, QSGTexture* texture, spine::Vector<SpineVertex> vertices, spine::Vector<GLushort> triangles)
        :mShaderProgram(program)
        ,mTexture(texture)
    {
        auto numvertices = vertices.size();
        if (triangles.size() <= 0 || numvertices <= 0 || !triangles.buffer())
            return;
        m_vertices.setSize(numvertices, SpineVertex());
        memcpy((float*)m_vertices.buffer(), (float*)vertices.buffer(), sizeof (SpineVertex) * numvertices);
        m_triangles.setSize(triangles.size(), 0);
        memcpy(m_triangles.buffer(), triangles.buffer(), sizeof(GLushort)*triangles.size());
    }

    virtual ~DrawTrigngles()
    {
        if(m_vertices.buffer())
            m_vertices.setSize(0, SpineVertex());
        if(m_triangles.buffer())
            m_triangles.setSize(0, 0);
    }

    virtual void invoke()
    {
        if (mTexture)
            mTexture->bind();

        mShaderProgram->setAttributeArray("a_position", GL_FLOAT, &m_vertices[0].x, 2, sizeof(SpineVertex));
        mShaderProgram->setAttributeArray("a_color", GL_FLOAT, &m_vertices[0].color.r, 4, sizeof(SpineVertex));
        mShaderProgram->setAttributeArray("a_texCoord", GL_FLOAT, &m_vertices[0].u, 2, sizeof(SpineVertex));

        glFuncs()->glDrawElements(GL_TRIANGLES, m_triangles.size(), GL_UNSIGNED_SHORT, m_triangles.buffer());
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    spine::Vector<SpineVertex> m_vertices;
    spine::Vector<GLushort> m_triangles;
    QSGTexture* mTexture;
};

class DrawPolygon: public ICachedGLFunctionCall
{
public:
    explicit DrawPolygon(QOpenGLShaderProgram* program, const Point* points, int pointsCount)
        :mShaderProgram(program)
    {
        if (pointsCount <= 0 || !points)
            return;

        m_points.setSize(pointsCount, Point());
        memcpy(m_points.buffer(), points, sizeof(Point)*pointsCount);
    }

    virtual ~DrawPolygon()
    {
        if(m_points.size() > 0)
            m_points.setSize(0, Point());
    }

    virtual void invoke()
    {
        mShaderProgram->setAttributeArray("a_position", GL_FLOAT, m_points.buffer(), 2, sizeof(Point));
        glFuncs()->glDrawArrays(GL_LINE_LOOP, 0, (GLsizei) m_points.size());
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    spine::Vector<Point> m_points;
};

class DrawLine: public ICachedGLFunctionCall
{
public:
    explicit DrawLine(QOpenGLShaderProgram* program, const Point& origin, const Point& destination)
        :mShaderProgram(program)
    {
        mPoints[0] = origin;
        mPoints[1] = destination;
    }

    virtual ~DrawLine(){}

    virtual void invoke()
    {
        mShaderProgram->setAttributeArray("a_position", GL_FLOAT, mPoints, 2, sizeof(Point));
        glFuncs()->glDrawArrays(GL_LINES, 0, 2);
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    Point mPoints[2];
};

class DrawPoint: public ICachedGLFunctionCall
{
public:
    explicit DrawPoint(QOpenGLShaderProgram* program, const Point& point)
        :mShaderProgram(program)
        ,mPoint(point)
    {}

    virtual ~DrawPoint(){}

    virtual void invoke()
    {
        mShaderProgram->setAttributeArray("a_position", GL_FLOAT, &mPoint, 2, sizeof(Point));
        glFuncs()->glDrawArrays(GL_POINTS, 0, 1);
    }

private:
    QOpenGLShaderProgram* mShaderProgram;
    Point mPoint;
};

RenderCmdsCache::RenderCmdsCache()
    :mTexture(nullptr)
{

    mTextureShaderProgram = new QOpenGLShaderProgram();
    bool res = mTextureShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/texture.vert");
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program addShaderFromSourceCode error:"<<mTextureShaderProgram->log();

    res = mTextureShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/texture.frag");
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program addShaderFromSourceCode error:"<<mTextureShaderProgram->log();

    res = mTextureShaderProgram->link();
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program link error:"<<mTextureShaderProgram->log();

    mColorShaderProgram = new QOpenGLShaderProgram();
    res = mColorShaderProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shader/color.vert");
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program addShaderFromSourceCode error:"<<mColorShaderProgram->log();

    res = mColorShaderProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shader/color.frag");
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program addShaderFromSourceCode error:"<<mColorShaderProgram->log();

    res = mColorShaderProgram->link();
    if (!res)
        qDebug()<<"PolygonBatch::PolygonBatch texture shader program link error:"<<mColorShaderProgram->log();
}

RenderCmdsCache::~RenderCmdsCache()
{
    clear();
    delete mTextureShaderProgram;
    delete mColorShaderProgram;
}

void RenderCmdsCache::clear()
{
    if (mglFuncs.isEmpty())
        return;

    foreach (ICachedGLFunctionCall* func, mglFuncs)
        func->release();

    mglFuncs.clear();
}

void RenderCmdsCache::drawTriangles(QSGTexture* addTexture, spine::Vector<SpineVertex> vertices,
                                    spine::Vector<GLushort> triangles)
{
    mglFuncs.push_back(new DrawTrigngles(mTextureShaderProgram, addTexture, vertices, triangles));
}

void RenderCmdsCache::blendFunc(GLenum sfactor, GLenum dfactor)
{
    mglFuncs.push_back(new BlendFunction(sfactor, dfactor));
}

void RenderCmdsCache::bindShader(RenderCmdsCache::ShaderType type)
{
    if (type == ShaderTexture)
        mglFuncs.push_back(new BindShader(mTextureShaderProgram, mRect));
    else if (type == ShaderColor)
        mglFuncs.push_back(new BindShader(mColorShaderProgram, mRect));
}

void RenderCmdsCache::drawColor(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    mglFuncs.push_back(new DrawColor(mColorShaderProgram, QColor(r,g,b,a)));
}

void RenderCmdsCache::lineWidth(GLfloat width)
{
    mglFuncs.push_back(new LineWidth(width));
}

void RenderCmdsCache::pointSize(GLfloat pointSize)
{
    mglFuncs.push_back(new PointSize(pointSize));
}

void RenderCmdsCache::drawPoly(const Point *points, int pointCount)
{
    mglFuncs.push_back(new DrawPolygon(mColorShaderProgram, points, pointCount));
}

void RenderCmdsCache::drawLine(const Point &origin, const Point &destination)
{
    mglFuncs.push_back(new DrawLine(mColorShaderProgram, origin, destination));
}

void RenderCmdsCache::drawPoint(const Point &point)
{
    mglFuncs.push_back(new DrawPoint(mColorShaderProgram, point));
}

void RenderCmdsCache::render()
{
    QOpenGLFunctions* glFuncs = QOpenGLContext::currentContext()->functions();
    glFuncs->glDisable(GL_DEPTH_TEST);
    glFuncs->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glFuncs->glClear(GL_COLOR_BUFFER_BIT);

    if (mglFuncs.isEmpty())
        return;
    foreach (ICachedGLFunctionCall* func, mglFuncs)
        func->invoke();
}

void RenderCmdsCache::setSkeletonRect(const QRectF &rect)
{
    mRect = rect;
}

