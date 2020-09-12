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

#ifndef POLYGONBATCH_H
#define POLYGONBATCH_H

#include <QtGlobal>
#include <QColor>
#include <QList>
#include <QRectF>
#include <QOpenGLFunctions>
#include <spine/spine.h>

QT_FORWARD_DECLARE_CLASS(QSGTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

struct Point
{
    Point(float _x, float _y) :x(_x), y(_y) {}
    Point(): x(0.0f), y(0.0f) {}
    GLfloat x;
    GLfloat y;
};

struct SpineVertex{
    float x, y;

    float u, v;

    spine::Color color;
};

class ICachedGLFunctionCall
{
public:
    virtual void invoke() = 0;
    virtual void release();
    virtual ~ICachedGLFunctionCall(){}

    QOpenGLFunctions* glFuncs();
};

class RenderCmdsCache
{
public:
    RenderCmdsCache();
    ~RenderCmdsCache();

    enum ShaderType {
        ShaderTexture,
        ShaderColor
    };

    void clear();

    void blendFunc(GLenum sfactor, GLenum dfactor);
    void bindShader(ShaderType);
    void drawColor(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
    void lineWidth(GLfloat width);
    void pointSize(GLfloat pointSize);

    void drawTriangles(QSGTexture* texture, spine::Vector<SpineVertex> vertices,
                       unsigned short *triangles, size_t trianglesCount);
    void drawPoly(const Point* points, int pointCount);
    void drawLine(const Point& origin, const Point& destination);
    void drawPoint(const Point& point);

    void render();
    void setSkeletonRect(const QRectF& rect);

private:
    QList<ICachedGLFunctionCall*> mglFuncs;
    QRectF mRect;

    QSGTexture* mTexture;
    QOpenGLShaderProgram* mTextureShaderProgram;
    QOpenGLShaderProgram* mColorShaderProgram;
};

#endif // POLYGONBATCH_H
